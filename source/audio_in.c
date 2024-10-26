/******************************************************************************
* File Name   : audio_in.c
*
* Description : This file contains the implementation of adding audio interface
*               and main processing the audio app code.
*
* Note        : See README.md
*
*******************************************************************************
* Copyright 2023-2024, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
******************************************************************************/
#include "audio.h"
#include "audio_usb.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "cyhal.h"
#include "rtos.h"
#include "queue.h"
#include "Global.h"
#include "USB.h"
#include "USB_AC.h"
#include "USB_HW_Cypress_PSoC6.h"

/*********************************************************************
*      AUDIO configurations
**********************************************************************/
#include "Microphone_configs.h"

/*****************************************************************************
* Macros
*****************************************************************************/
#define DELAY_TICKS                  (50U)

/* PDM/PCM Pins */
#ifndef CYBSP_PDM_DATA
#define CYBSP_PDM_DATA          CYBSP_A5
#endif
#ifndef CYBSP_PDM_CLK
#define CYBSP_PDM_CLK           CYBSP_A4
#endif

/* Decimation Rate of the PDM/PCM block */
#define DECIMATION_RATE             (64U)

/* Audio Subsystem Clock. Typical values depends on the desired sample rate:
     * 8KHz / 16 KHz / 32 KHz / 48 KHz    : 24.576 MHz
     * 22.05 KHz / 44.1 KHz               : 22.579 MHz
     */
#if ((AUDIO_SAMPLING_RATE_22KHZ == MICROPHONE_FREQUENCIES) || (AUDIO_SAMPLING_RATE_44KHZ == MICROPHONE_FREQUENCIES))
#define AUDIO_SYS_CLOCK_HZ                  (22579200U)
#else
#define AUDIO_SYS_CLOCK_HZ                  (24576000U)
#endif

/*****************************************************************************
* Global Variables
*****************************************************************************/
/* RTOS task handles */
TaskHandle_t rtos_audio_app_task;
TaskHandle_t rtos_audio_in_task;

static volatile bool usb_suspend_flag = false;

/* PCM buffer data (16-bits) */
uint16_t audio_in_pcm_buffer_ping[(MAX_AUDIO_IN_BUFFER_SIZE)];
uint16_t audio_in_pcm_buffer_pong[(MAX_AUDIO_IN_BUFFER_SIZE)];

/* HAL object */
cyhal_pdm_pcm_t pdm_pcm;
static cyhal_clock_t audio_clock;

/* HAL Config for pdm_pcm */
const cyhal_pdm_pcm_cfg_t pdm_pcm_cfg =
{
    .sample_rate = MICROPHONE_FREQUENCIES,
    .decimation_rate = DECIMATION_RATE,
    .mode = CYHAL_PDM_PCM_MODE_STEREO,
    .word_length = AUDIO_IN_BIT_RESOLUTION,  /* bits */
    .left_gain = CYHAL_PDM_PCM_MAX_GAIN,   /* dB */
    .right_gain = CYHAL_PDM_PCM_MAX_GAIN,   /* dB */
};

/********************************************************************************
 * Function Name: vApplicationTickHook
 ********************************************************************************
 * Summary:
 *  Tick hook function called at every tick (1ms). It checks for the activity in
 *  the USB bus.
 *
 * Parameters:
 *  None
 *
 * Return:
 *  None
 *
 *******************************************************************************/
void vApplicationTickHook(void)
{
    /* Supervisor of suspend conditions on the bus */
#if defined (COMPONENT_CAT1A)
    USB_DRIVER_Cypress_PSoC6_SysTick();
#endif /* COMPONENT_CAT1A */

    if (USBD_GetState() & USB_STAT_SUSPENDED)
    {
        /* Suspend condition on USB bus is detected */
        usb_suspend_flag = true;
    }
    else
    {
        /* Clear suspend conditions */
        usb_suspend_flag = false;
    }
}


/*****************************************************************************
* Function Name: audio_in_init
******************************************************************************
* Summary:
*  Initialize the PDM/PCM block and create the "Audio App Task" and "Audio In Task" which will
*  process the Audio IN endpoint transactions.
*
* Parameters:
*  None
*
* Return:
*  None
*
*****************************************************************************/
void audio_in_init(void)
{
    BaseType_t rtos_task_status;

    /* Create the AUDIO APP RTOS task */
    rtos_task_status = xTaskCreate(audio_app_task, "Audio App Task", AUDIO_TASK_STACK_DEPTH, NULL,
        AUDIO_APP_TASK_PRIORITY, &rtos_audio_app_task);
    if (pdPASS != rtos_task_status)
    {
        CY_ASSERT(0);
    }

    /* Initialize the PDM PCM block */
    cyhal_pdm_pcm_init(&pdm_pcm, CYBSP_PDM_DATA, CYBSP_PDM_CLK, &audio_clock, &pdm_pcm_cfg);

    /* Create the AUDIO Write RTOS task */
    rtos_task_status = xTaskCreate(audio_in_process, "Audio In Task", AUDIO_TASK_STACK_DEPTH, NULL, AUDIO_WRITE_TASK_PRIORITY, &rtos_audio_in_task);
    if (pdPASS != rtos_task_status)
    {
        CY_ASSERT(0);
    }
}

/********************************************************************************
* Function Name: audio_in_process
********************************************************************************
* Summary:
*  Run audio application.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void audio_in_process(void* arg) {
    USBD_AC_TX_CTX TX;
    I8 MicActive = 0;
    MESSAGE Msg;
    size_t audio_in_count;
    static uint16_t *audio_in_pcm_buffer = NULL;

    (void)arg;

    for (;;) {
        USB_MEMSET(&AC_Global, 0, sizeof(AC_Global));
        while (audio_usb_suspended())
        {
            cyhal_gpio_toggle(CYBSP_USER_LED);
            USB_OS_Delay(50);
        }
        printf("APP_LOG: USB Audio Device Connected \r\n");
        cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);

        while (audio_usb_configured()) {

            if (xQueueReceive(Mail_Box, &Msg, 100) == pdFALSE) {
                continue;
            }

            switch (Msg.Event) {
            case MSG_MIC_ON:
                if (MicActive != 0) {
                    USBD_AC_CloseTXStream(&TX);
                    MicActive = 0;
                }

                /* Clear Audio In buffer */
                memset(audio_in_pcm_buffer_ping, 0, sizeof(audio_in_pcm_buffer_ping));
                memset(audio_in_pcm_buffer_pong, 0, sizeof(audio_in_pcm_buffer_pong));

                audio_in_pcm_buffer = audio_in_pcm_buffer_ping;

                /* Clear PDM/PCM RX FIFO */
                cyhal_pdm_pcm_clear(&pdm_pcm);

                /* Start PDM/PCM */
                cyhal_pdm_pcm_start(&pdm_pcm);

                memset(&TX, 0, sizeof(TX));
                TX.Interface = USBD_AC_INTERFACE_Microphone;
                TX.Timeout = 5000;
                TX.pfCallback = audio_in_callback;

                if (USBD_AC_OpenTXStream(&TX) == 0) {
                    MicActive = 1;
                    USBD_AC_Send(&TX, 1, 192, audio_in_pcm_buffer);
                }
                break;

            case MSG_MIC_OFF:
                if (MicActive != 0) {
                    USBD_AC_CloseTXStream(&TX);
                    MicActive = 0;
                    cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_OFF);
                }
                break;

            case MSG_MIC_DATA:
                cyhal_gpio_write(CYBSP_USER_LED, CYBSP_LED_STATE_ON);
                audio_in_count = 96;

                if (audio_in_pcm_buffer == audio_in_pcm_buffer_ping)
                {
                    audio_in_pcm_buffer = audio_in_pcm_buffer_pong;
                }
                else
                {
                    audio_in_pcm_buffer = audio_in_pcm_buffer_ping;
                }

                /* Read all the data in the PDM/PCM buffer */
                cyhal_pdm_pcm_read(&pdm_pcm, (void*)audio_in_pcm_buffer, &audio_in_count);

                USBD_AC_Send(&TX, 1, 192, audio_in_pcm_buffer);
                break;

            default:
                break;
            }
        }
    }
}

/*******************************************************************************
* Function Name: audio_clock_init
********************************************************************************
* Summary:
*  Initializes clock for audio subsystem.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void audio_clock_init(void)
{
    cy_rslt_t result;
    cyhal_clock_t clock_pll;

    /* Initialize, take ownership of PLL0/PLL */
    result = cyhal_clock_reserve(&clock_pll, &CYHAL_CLOCK_PLL[0]);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Set the PLL0/PLL frequency to AUDIO_SYS_CLOCK_HZ based on MICROPHONE_FREQUENCIES */
    result = cyhal_clock_set_frequency(&clock_pll, AUDIO_SYS_CLOCK_HZ, NULL);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* If the PLL0/PLL clock is not already enabled, enable it */
    if (!cyhal_clock_is_enabled(&clock_pll))
    {
        result = cyhal_clock_set_enabled(&clock_pll, true, true);
        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }
    }

    /* Initialize, take ownership of CLK_HF1 */
    result = cyhal_clock_reserve(&audio_clock, &CYHAL_CLOCK_HF[1]);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Source the audio subsystem clock (CLK_HF1) from PLL0/PLL */
    result = cyhal_clock_set_source(&audio_clock, &clock_pll);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Set the divider for audio subsystem clock (CLK_HF1) */
    result = cyhal_clock_set_divider(&audio_clock, 1);
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* If the audio subsystem clock (CLK_HF1) is not already enabled, enable it */
    if (!cyhal_clock_is_enabled(&audio_clock))
    {
        result = cyhal_clock_set_enabled(&audio_clock, true, true);
        if (CY_RSLT_SUCCESS != result)
        {
            CY_ASSERT(0);
        }
    }
}

/*******************************************************************************
* Function Name: audio_app_task
********************************************************************************
* Summary:
*  Start the USB device stack
*  In the main loop, checks USB device connectivity.
*
* Parameters:
*  arg
*
* Return:
*  None
*
*******************************************************************************/
void audio_app_task(void* arg)
{
    volatile bool usb_suspended = false;
    volatile bool usb_connected = false;

    CY_UNUSED_PARAMETER(arg);

    /* Start the USB device stack */
    USBD_Start();

    for (;;)
    {
        /* Check if suspend condition is detected on the bus */
        if (usb_suspend_flag)
        {
            if (!usb_suspended)
            {
                usb_suspended = true;
                usb_connected = false;

                printf("APP_LOG: USB Audio Device Disconnected\r\n");
            }
        }
        else /* USB device connected */
        {
            if (!usb_connected)
            {
                usb_connected = true;
                usb_suspended = false;
            }
        }

        vTaskDelay(pdMS_TO_TICKS(DELAY_TICKS));
    }
}
