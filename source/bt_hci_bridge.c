/*******************************************************************************
* File Name   : bt_hci_bridge.c
*
* Description : Composite USB Audio + Bluetooth HCI bridge implementation.
*
*******************************************************************************/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cy_result.h"
#include "cybsp.h"
#include "cycfg_pins.h"
#include "audio_usb.h"
#include "bt_hci_bridge.h"
#include "rtos.h"
#include "USB_CDC.h"
#include "cybt_platform_config.h"
#include "cybt_platform_hci.h"
#include "cybt_platform_interface.h"

#define BT_HCI_BRIDGE_TASK_STACK_DEPTH            (768U)
#define BT_HCI_BRIDGE_MANAGER_TASK_PRIORITY       (AUDIO_APP_TASK_PRIORITY)
#define BT_HCI_BRIDGE_RX_TASK_PRIORITY            (AUDIO_APP_TASK_PRIORITY)
#define BT_HCI_BRIDGE_TX_TASK_PRIORITY            (AUDIO_APP_TASK_PRIORITY)

#define BT_HCI_BRIDGE_MAX_PACKET_SIZE             (2048U)
#define BT_HCI_BRIDGE_DISCARD_BUFFER_SIZE         (64U)
#define BT_HCI_BRIDGE_BOOT_EVENT_BUFFER_SIZE      (64U)

#define BT_HCI_BRIDGE_USB_POLL_DELAY_MS           (10U)
#define BT_HCI_BRIDGE_USB_READ_TIMEOUT_MS         (500U)
#define BT_HCI_BRIDGE_USB_WRITE_TIMEOUT_MS        (500U)
#define BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS      (250U)
#define BT_HCI_BRIDGE_UART_IDLE_TIMEOUT_MS        (20U)

#define BT_HCI_BRIDGE_MINIDRV_DELAY_MS            (50U)
#define BT_HCI_BRIDGE_POST_PATCH_DELAY_MS         (250U)

#define BT_HCI_OPCODE_RESET                       (0x0C03U)
#define BT_HCI_OPCODE_DOWNLOAD_MINIDRV            (0xFC2EU)

#define BT_HCI_EVENT_COMMAND_COMPLETE             (0x0EU)

#define BT_HCI_BRIDGE_RSLT_BASE                   (CY_RSLT_MODULE_MIDDLEWARE_MW)
#define BT_HCI_BRIDGE_RSLT_INIT_FAILED            \
    CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, BT_HCI_BRIDGE_RSLT_BASE, 0x1100)
#define BT_HCI_BRIDGE_RSLT_TASK_CREATE_FAILED     \
    CY_RSLT_CREATE(CY_RSLT_TYPE_ERROR, BT_HCI_BRIDGE_RSLT_BASE, 0x1101)

typedef struct
{
    volatile bool controller_ready;
    volatile bool host_open;
    volatile bool host_rts;
    volatile uint32_t host_line_baud;
    TaskHandle_t manager_task;
    TaskHandle_t usb_to_bt_task;
    TaskHandle_t bt_to_usb_task;
} bt_hci_bridge_state_t;

static bt_hci_bridge_state_t bt_hci_bridge_state =
{
    .host_line_baud = 115200U,
};

static uint8_t bt_hci_bridge_usb_to_bt_packet[BT_HCI_BRIDGE_MAX_PACKET_SIZE];
static uint8_t bt_hci_bridge_bt_to_usb_packet[BT_HCI_BRIDGE_MAX_PACKET_SIZE];
static uint8_t bt_hci_bridge_event_buffer[BT_HCI_BRIDGE_BOOT_EVENT_BUFFER_SIZE];
static uint8_t bt_hci_bridge_discard_buffer[BT_HCI_BRIDGE_DISCARD_BUFFER_SIZE];

extern const uint8_t brcm_patchram_buf[];
extern const int brcm_patch_ram_length;

static const cybt_platform_config_t bt_hci_bridge_platform_cfg =
{
    .hci_config =
    {
        .hci_transport = CYBT_HCI_UART,
        .hci =
        {
            .hci_uart =
            {
                .uart_tx_pin = CYBSP_BT_UART_TX,
                .uart_rx_pin = CYBSP_BT_UART_RX,
                .uart_rts_pin = CYBSP_BT_UART_RTS,
                .uart_cts_pin = CYBSP_BT_UART_CTS,
                .baud_rate_for_fw_download = 115200U,
                .baud_rate_for_feature = 115200U,
                .data_bits = 8U,
                .stop_bits = 1U,
                .parity = CYHAL_UART_PARITY_NONE,
                .flow_control = true
            }
        }
    },
    .controller_config =
    {
        .bt_power_pin = CYBSP_BT_POWER,
        .sleep_mode =
        {
            .sleep_mode_enabled = CYBT_SLEEP_MODE_DISABLED,
            .device_wakeup_pin = NC,
            .host_wakeup_pin = NC,
            .device_wake_polarity = CYBT_WAKE_ACTIVE_LOW,
            .host_wake_polarity = CYBT_WAKE_ACTIVE_LOW
        }
    },
    .task_mem_pool_size = 2048U
};

static void bt_hci_bridge_update_serial_state(void)
{
    USB_CDC_HANDLE cdc_handle = audio_usb_get_cdc_handle();
    USB_CDC_SERIAL_STATE serial_state;

    if (cdc_handle < 0 || !audio_usb_configured())
    {
        return;
    }

    memset(&serial_state, 0, sizeof(serial_state));
    serial_state.DCD = bt_hci_bridge_state.controller_ready ? 1U : 0U;
    serial_state.DSR = bt_hci_bridge_state.controller_ready ? 1U : 0U;
    serial_state.CTS = 1U;

    USBD_CDC_UpdateSerialState(cdc_handle, &serial_state);
}

static void bt_hci_bridge_on_control_line_state(USB_CDC_CONTROL_LINE_STATE *p_line_state)
{
    bt_hci_bridge_state.host_open = (p_line_state->DTR != 0U);
    bt_hci_bridge_state.host_rts = (p_line_state->RTS != 0U);
}

static void bt_hci_bridge_on_line_coding(USB_CDC_LINE_CODING *p_line_coding)
{
    bt_hci_bridge_state.host_line_baud = p_line_coding->DTERate;
}

static void bt_hci_bridge_register_usb_callbacks(void)
{
    USB_CDC_HANDLE cdc_handle = audio_usb_get_cdc_handle();

    if (cdc_handle < 0)
    {
        return;
    }

    USBD_CDC_SetOnControlLineState(cdc_handle, bt_hci_bridge_on_control_line_state);
    USBD_CDC_SetOnLineCoding(cdc_handle, bt_hci_bridge_on_line_coding);
}

static cybt_result_t bt_hci_bridge_hci_read_exact(uint8_t *p_buffer,
                                                  uint32_t length,
                                                  uint32_t timeout_ms)
{
    while (length > 0U)
    {
        uint32_t chunk_length = length;
        cybt_result_t result = cybt_platform_hci_read(HCI_PACKET_TYPE_IGNORE,
                                                      p_buffer,
                                                      &chunk_length,
                                                      timeout_ms);
        if (result != CYBT_SUCCESS)
        {
            return result;
        }

        if (chunk_length == 0U)
        {
            return CYBT_ERR_TIMEOUT;
        }

        p_buffer += chunk_length;
        length -= chunk_length;
    }

    return CYBT_SUCCESS;
}

static bool bt_hci_bridge_is_command_complete_ok(const uint8_t *p_event,
                                                 size_t event_length,
                                                 uint16_t expected_opcode)
{
    uint16_t event_opcode;

    if (event_length < 7U)
    {
        return false;
    }

    if (p_event[0] != HCI_PACKET_TYPE_EVENT || p_event[1] != BT_HCI_EVENT_COMMAND_COMPLETE)
    {
        return false;
    }

    event_opcode = (uint16_t)p_event[4] | ((uint16_t)p_event[5] << 8);
    if (event_opcode != expected_opcode)
    {
        return false;
    }

    return (p_event[6] == 0U);
}

static cybt_result_t bt_hci_bridge_read_boot_event(uint8_t *p_buffer,
                                                   size_t buffer_size,
                                                   size_t *p_event_length)
{
    cybt_result_t result;
    uint8_t event_length;

    if (buffer_size < 3U)
    {
        return CYBT_ERR_BADARG;
    }

    result = bt_hci_bridge_hci_read_exact(p_buffer, 1U, BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    result = bt_hci_bridge_hci_read_exact(&p_buffer[1], 2U, BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    event_length = p_buffer[2];
    if ((size_t)(event_length + 3U) > buffer_size)
    {
        return CYBT_ERR_GENERIC;
    }

    result = bt_hci_bridge_hci_read_exact(&p_buffer[3], event_length, BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    *p_event_length = (size_t)event_length + 3U;
    return CYBT_SUCCESS;
}

static cybt_result_t bt_hci_bridge_send_boot_command(uint16_t opcode,
                                                     const uint8_t *p_payload,
                                                     uint8_t payload_length)
{
    uint8_t command_packet[260];
    size_t event_length = 0U;
    cybt_result_t result;

    command_packet[0] = HCI_PACKET_TYPE_COMMAND;
    command_packet[1] = (uint8_t)(opcode & 0xFFU);
    command_packet[2] = (uint8_t)((opcode >> 8) & 0xFFU);
    command_packet[3] = payload_length;
    if (payload_length > 0U)
    {
        memcpy(&command_packet[4], p_payload, payload_length);
    }

    result = cybt_platform_hci_write(HCI_PACKET_TYPE_COMMAND,
                                     command_packet,
                                     (uint32_t)payload_length + 4U);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    result = bt_hci_bridge_read_boot_event(bt_hci_bridge_event_buffer,
                                           sizeof(bt_hci_bridge_event_buffer),
                                           &event_length);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    if (!bt_hci_bridge_is_command_complete_ok(bt_hci_bridge_event_buffer, event_length, opcode))
    {
        return CYBT_ERR_GENERIC;
    }

    return CYBT_SUCCESS;
}

static cybt_result_t bt_hci_bridge_download_patch(void)
{
    size_t offset = 0U;

    while ((offset + 3U) <= (size_t)brcm_patch_ram_length)
    {
        uint16_t opcode;
        uint8_t payload_length;
        cybt_result_t result;

        opcode = (uint16_t)brcm_patchram_buf[offset]
               | ((uint16_t)brcm_patchram_buf[offset + 1U] << 8);
        payload_length = brcm_patchram_buf[offset + 2U];

        if ((offset + 3U + payload_length) > (size_t)brcm_patch_ram_length)
        {
            return CYBT_ERR_GENERIC;
        }

        result = bt_hci_bridge_send_boot_command(opcode,
                                                 &brcm_patchram_buf[offset + 3U],
                                                 payload_length);
        if (result != CYBT_SUCCESS)
        {
            return result;
        }

        offset += (size_t)payload_length + 3U;
    }

    vTaskDelay(pdMS_TO_TICKS(BT_HCI_BRIDGE_POST_PATCH_DELAY_MS));
    return CYBT_SUCCESS;
}

static cybt_result_t bt_hci_bridge_boot_controller(void)
{
    cybt_result_t result;

    result = cybt_platform_hci_open(NULL);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    result = bt_hci_bridge_send_boot_command(BT_HCI_OPCODE_RESET, NULL, 0U);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    result = bt_hci_bridge_send_boot_command(BT_HCI_OPCODE_DOWNLOAD_MINIDRV, NULL, 0U);
    if (result != CYBT_SUCCESS)
    {
        return result;
    }

    vTaskDelay(pdMS_TO_TICKS(BT_HCI_BRIDGE_MINIDRV_DELAY_MS));

    return bt_hci_bridge_download_patch();
}

static size_t bt_hci_bridge_get_header_size(uint8_t packet_type, bool from_host)
{
    switch (packet_type)
    {
        case HCI_PACKET_TYPE_COMMAND:
            return from_host ? 3U : 0U;
        case HCI_PACKET_TYPE_ACL:
            return 4U;
        case HCI_PACKET_TYPE_SCO:
            return 3U;
        case HCI_PACKET_TYPE_EVENT:
            return from_host ? 0U : 2U;
        case HCI_PACKET_TYPE_ISO:
            return 4U;
        case HCI_PACKET_TYPE_DIAG:
            return from_host ? 0U : 63U;
        default:
            return 0U;
    }
}

static size_t bt_hci_bridge_get_payload_length(uint8_t packet_type,
                                               const uint8_t *p_header,
                                               bool from_host)
{
    switch (packet_type)
    {
        case HCI_PACKET_TYPE_COMMAND:
            return from_host ? p_header[2] : 0U;
        case HCI_PACKET_TYPE_ACL:
            return (size_t)p_header[2] | ((size_t)p_header[3] << 8);
        case HCI_PACKET_TYPE_SCO:
            return p_header[2];
        case HCI_PACKET_TYPE_EVENT:
            return from_host ? 0U : p_header[1];
        case HCI_PACKET_TYPE_ISO:
            return ((size_t)p_header[2] | ((size_t)p_header[3] << 8)) & 0x3FFFU;
        case HCI_PACKET_TYPE_DIAG:
            return 0U;
        default:
            return 0U;
    }
}

static bool bt_hci_bridge_wait_for_host(void)
{
    return audio_usb_cdc_ready()
        && bt_hci_bridge_state.controller_ready;
}

static bool bt_hci_bridge_usb_read_exact(uint8_t *p_buffer, size_t length)
{
    USB_CDC_HANDLE cdc_handle = audio_usb_get_cdc_handle();
    TickType_t start_ticks = xTaskGetTickCount();

    if (cdc_handle < 0)
    {
        return false;
    }

    USBD_CDC_ReadOverlapped(cdc_handle, p_buffer, (unsigned)length);

    while (USBD_CDC_GetNumBytesRemToRead(cdc_handle) > 0)
    {
        TickType_t elapsed_ticks = xTaskGetTickCount() - start_ticks;

        if (!bt_hci_bridge_wait_for_host())
        {
            USBD_CDC_CancelRead(cdc_handle);
            return false;
        }

        if (elapsed_ticks >= pdMS_TO_TICKS(BT_HCI_BRIDGE_USB_READ_TIMEOUT_MS))
        {
            USBD_CDC_CancelRead(cdc_handle);
            return false;
        }

        vTaskDelay(pdMS_TO_TICKS(1U));
    }

    return true;
}

static void bt_hci_bridge_usb_discard(size_t length)
{
    while (length > 0U)
    {
        size_t chunk_length = (length > sizeof(bt_hci_bridge_discard_buffer))
                            ? sizeof(bt_hci_bridge_discard_buffer)
                            : length;

        if (!bt_hci_bridge_usb_read_exact(bt_hci_bridge_discard_buffer, chunk_length))
        {
            return;
        }

        length -= chunk_length;
    }
}

static void bt_hci_bridge_uart_discard(size_t length)
{
    while (length > 0U)
    {
        uint32_t chunk_length = (uint32_t)((length > sizeof(bt_hci_bridge_discard_buffer))
                            ? sizeof(bt_hci_bridge_discard_buffer)
                            : length);
        if (bt_hci_bridge_hci_read_exact(bt_hci_bridge_discard_buffer,
                                         chunk_length,
                                         BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS) != CYBT_SUCCESS)
        {
            return;
        }

        length -= chunk_length;
    }
}

static bool bt_hci_bridge_read_usb_packet(uint8_t *p_packet, size_t *p_packet_length)
{
    size_t header_length;
    size_t payload_length;
    size_t total_length;
    uint8_t packet_type;

    if (!bt_hci_bridge_wait_for_host())
    {
        return false;
    }

    if (!bt_hci_bridge_usb_read_exact(&p_packet[0], 1U))
    {
        return false;
    }

    packet_type = p_packet[0];
    header_length = bt_hci_bridge_get_header_size(packet_type, true);
    if (header_length == 0U)
    {
        return false;
    }

    if (!bt_hci_bridge_usb_read_exact(&p_packet[1], header_length))
    {
        return false;
    }

    payload_length = bt_hci_bridge_get_payload_length(packet_type, &p_packet[1], true);
    total_length = 1U + header_length + payload_length;
    if (total_length > BT_HCI_BRIDGE_MAX_PACKET_SIZE)
    {
        bt_hci_bridge_usb_discard(payload_length);
        return false;
    }

    if (payload_length > 0U && !bt_hci_bridge_usb_read_exact(&p_packet[1U + header_length], payload_length))
    {
        return false;
    }

    *p_packet_length = total_length;
    return true;
}

static bool bt_hci_bridge_read_controller_packet(uint8_t *p_packet, size_t *p_packet_length)
{
    size_t header_length;
    size_t payload_length;
    size_t total_length;
    cybt_result_t result;

    result = bt_hci_bridge_hci_read_exact(&p_packet[0], 1U, BT_HCI_BRIDGE_UART_IDLE_TIMEOUT_MS);
    if (result == CYBT_ERR_TIMEOUT)
    {
        return false;
    }

    if (result != CYBT_SUCCESS)
    {
        return false;
    }

    if (p_packet[0] == HCI_PACKET_TYPE_DIAG)
    {
        bt_hci_bridge_uart_discard(63U);
        return false;
    }

    header_length = bt_hci_bridge_get_header_size(p_packet[0], false);
    if (header_length == 0U)
    {
        return false;
    }

    if (bt_hci_bridge_hci_read_exact(&p_packet[1], (uint32_t)header_length, BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS) != CYBT_SUCCESS)
    {
        return false;
    }

    payload_length = bt_hci_bridge_get_payload_length(p_packet[0], &p_packet[1], false);
    total_length = 1U + header_length + payload_length;
    if (total_length > BT_HCI_BRIDGE_MAX_PACKET_SIZE)
    {
        bt_hci_bridge_uart_discard(payload_length);
        return false;
    }

    if (payload_length > 0U
        && bt_hci_bridge_hci_read_exact(&p_packet[1U + header_length],
                                        (uint32_t)payload_length,
                                        BT_HCI_BRIDGE_UART_PACKET_TIMEOUT_MS) != CYBT_SUCCESS)
    {
        return false;
    }

    *p_packet_length = total_length;
    return true;
}

static bool bt_hci_bridge_write_usb_packet(const uint8_t *p_packet, size_t packet_length)
{
    USB_CDC_HANDLE cdc_handle = audio_usb_get_cdc_handle();

    if (!bt_hci_bridge_wait_for_host() || cdc_handle < 0)
    {
        return false;
    }

    if (USBD_CDC_Write(cdc_handle,
                       p_packet,
                       (unsigned)packet_length,
                       BT_HCI_BRIDGE_USB_WRITE_TIMEOUT_MS) < 0)
    {
        return false;
    }

    return (USBD_CDC_WaitForTX(cdc_handle, BT_HCI_BRIDGE_USB_WRITE_TIMEOUT_MS) >= 0);
}

static void bt_hci_bridge_manager_task(void *arg)
{
    cybt_result_t result;

    (void)arg;

    cybt_platform_config_init(&bt_hci_bridge_platform_cfg);
    cybt_platform_init();

    result = bt_hci_bridge_boot_controller();
    if (result != CYBT_SUCCESS)
    {
        printf("APP_LOG: BT bridge bootstrap failed (0x%02X)\r\n", (unsigned)result);
        vTaskDelete(NULL);
        return;
    }

    bt_hci_bridge_state.controller_ready = true;
    bt_hci_bridge_update_serial_state();

    printf("APP_LOG: BT controller ready over USB CDC ACM\r\n");
    printf("APP_LOG: Linux can attach with btattach after /dev/ttyACM* appears\r\n");

    for (;;)
    {
        bt_hci_bridge_update_serial_state();
        vTaskDelay(pdMS_TO_TICKS(1000U));
    }
}

static void bt_hci_bridge_usb_to_bt_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        size_t packet_length = 0U;

        if (!bt_hci_bridge_read_usb_packet(bt_hci_bridge_usb_to_bt_packet, &packet_length))
        {
            vTaskDelay(pdMS_TO_TICKS(BT_HCI_BRIDGE_USB_POLL_DELAY_MS));
            continue;
        }

        if (cybt_platform_hci_write((hci_packet_type_t)bt_hci_bridge_usb_to_bt_packet[0],
                                    bt_hci_bridge_usb_to_bt_packet,
                                    (uint32_t)packet_length) != CYBT_SUCCESS)
        {
            vTaskDelay(pdMS_TO_TICKS(BT_HCI_BRIDGE_USB_POLL_DELAY_MS));
        }
    }
}

static void bt_hci_bridge_bt_to_usb_task(void *arg)
{
    (void)arg;

    for (;;)
    {
        size_t packet_length = 0U;

        if (!bt_hci_bridge_state.controller_ready)
        {
            vTaskDelay(pdMS_TO_TICKS(BT_HCI_BRIDGE_USB_POLL_DELAY_MS));
            continue;
        }

        if (!bt_hci_bridge_read_controller_packet(bt_hci_bridge_bt_to_usb_packet, &packet_length))
        {
            vTaskDelay(pdMS_TO_TICKS(1U));
            continue;
        }

        (void)bt_hci_bridge_write_usb_packet(bt_hci_bridge_bt_to_usb_packet, packet_length);
    }
}

cy_rslt_t bt_hci_bridge_init(void)
{
    BaseType_t task_status;

    if (audio_usb_get_cdc_handle() < 0)
    {
        return BT_HCI_BRIDGE_RSLT_INIT_FAILED;
    }

    bt_hci_bridge_register_usb_callbacks();

    task_status = xTaskCreate(bt_hci_bridge_manager_task,
                              "BT Bridge",
                              BT_HCI_BRIDGE_TASK_STACK_DEPTH,
                              NULL,
                              BT_HCI_BRIDGE_MANAGER_TASK_PRIORITY,
                              &bt_hci_bridge_state.manager_task);
    if (task_status != pdPASS)
    {
        return BT_HCI_BRIDGE_RSLT_TASK_CREATE_FAILED;
    }

    task_status = xTaskCreate(bt_hci_bridge_usb_to_bt_task,
                              "BT USB->HCI",
                              BT_HCI_BRIDGE_TASK_STACK_DEPTH,
                              NULL,
                              BT_HCI_BRIDGE_TX_TASK_PRIORITY,
                              &bt_hci_bridge_state.usb_to_bt_task);
    if (task_status != pdPASS)
    {
        return BT_HCI_BRIDGE_RSLT_TASK_CREATE_FAILED;
    }

    task_status = xTaskCreate(bt_hci_bridge_bt_to_usb_task,
                              "BT HCI->USB",
                              BT_HCI_BRIDGE_TASK_STACK_DEPTH,
                              NULL,
                              BT_HCI_BRIDGE_RX_TASK_PRIORITY,
                              &bt_hci_bridge_state.bt_to_usb_task);
    if (task_status != pdPASS)
    {
        return BT_HCI_BRIDGE_RSLT_TASK_CREATE_FAILED;
    }

    return CY_RSLT_SUCCESS;
}
