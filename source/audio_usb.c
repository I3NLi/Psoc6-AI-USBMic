/******************************************************************************
* File Name   : audio_usb.c
*
* Description : This file contains the implementation of initializing the USB communication.
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
#include "USB_CDC.h"

/*********************************************************************
*      AUDIO configurations
**********************************************************************/
#include "Microphone_configs.h"

/*********************************************************************
*
*      Global Variables
*
**********************************************************************/

static const uint32_t mic_frequencies[] = { MICROPHONE_FREQUENCIES };

struct AC_Global_t AC_Global =
{
    .CurrMicFreq = MICROPHONE_FREQUENCIES,
    .MicrophoneVolume = (U16)(int16_t)AUDIO_USB_MIC_VOLUME_DEFAULT_DB_256,
    .MicrophoneMute = 0U,
};
MESSAGE          Msg_Buff[5];
QueueHandle_t    Mail_Box;
static StaticQueue_t    Static_Queue;
static USB_CDC_HANDLE   audio_usb_cdc_handle = -1;
static U8               audio_usb_cdc_out_buffer[USB_FS_BULK_MAX_PACKET_SIZE];

static U16 audio_clamp_microphone_volume(int32_t volume_db_256)
{
    if (volume_db_256 < AUDIO_USB_MIC_VOLUME_MIN_DB_256)
    {
        volume_db_256 = AUDIO_USB_MIC_VOLUME_MIN_DB_256;
    }
    else if (volume_db_256 > AUDIO_USB_MIC_VOLUME_MAX_DB_256)
    {
        volume_db_256 = AUDIO_USB_MIC_VOLUME_MAX_DB_256;
    }

    return (U16)(int16_t)volume_db_256;
}

void audio_usb_request_microphone_control_update(void)
{
    MESSAGE Msg;

    if (Mail_Box == NULL)
    {
        return;
    }

    Msg.Event = MSG_MIC_CONTROL_UPDATE;
    (void)xQueueSend(Mail_Box, &Msg, 0);
}

void audio_usb_set_microphone_volume_db_256(int16_t volume_db_256)
{
    AC_Global.MicrophoneVolume = audio_clamp_microphone_volume(volume_db_256);
    audio_usb_request_microphone_control_update();
}

void audio_usb_set_microphone_mute(U8 mute_enabled)
{
    AC_Global.MicrophoneMute = (mute_enabled != 0U) ? 1U : 0U;
    audio_usb_request_microphone_control_update();
}

int16_t audio_usb_get_microphone_volume_db_256(void)
{
    return (int16_t)AC_Global.MicrophoneVolume;
}

U8 audio_usb_get_microphone_mute(void)
{
    return AC_Global.MicrophoneMute;
}

/********************************************************************************
* Function Name: audio_set_interface_control_callback
********************************************************************************
* Summary:
*  Definition of the callback which is called when the hosts sets an alternate
*  setting on an audio interface.
*
* Parameters:
*  InterfaceNo - Number of the audio streaming interface.
*  NewAltSetting - Alternate setting selected by the host.
*
* Return:
*  None
*
*******************************************************************************/
static void audio_set_interface_control_callback(unsigned InterfaceNo, unsigned NewAltSetting) {
    MESSAGE Msg;
    BaseType_t xHigherPriorityTaskWoken;

    switch (InterfaceNo) {
    case USBD_AC_INTERFACE_Microphone:
#if USBD_AC_AUDIO_VERSION == 1
        /* Setting AltInterface also sets the sample frequency */
        if (NewAltSetting > 0) {
            AC_Global.CurrMicFreq = mic_frequencies[NewAltSetting - 1];
        }
#endif
        AC_Global.MicInfo = *USBD_AC_GetStreamInfo(USBD_AC_INTERFACE_Microphone, NewAltSetting);
        AC_Global.MicrophoneAltSetting = NewAltSetting;
        Msg.Event = (NewAltSetting == 0) ? MSG_MIC_OFF : MSG_MIC_ON;

        xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(Mail_Box, &Msg, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            /* Actual macro used here is port specific. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }

        break;
    }
}

/********************************************************************************
* Function Name: audio_control_get_callback
********************************************************************************
* Summary:
*  For audio 1.0 control requests!
*  Definition of the callback which is called when an audio get requests is received.
*  This callback is called in interrupt context and must not block.
*
* Parameters:
*  pReqInfo  : Contains information about the type of the control request.
*  pBuffer    : Pointer to a buffer into which the callback should write the reply (max. 64 bytes).
*
* Return:
*  >= 0:          Audio control request was handled by the callback and response data was put into pBuffer.
*                The callback function must return the length of the response data which will be send to the host.
*  < 0 :          Audio control request was not handled by the callback (i.e. illegal request or parameters).
*                The stack will STALL the request.
*
*******************************************************************************/
#if USBD_AC_AUDIO_VERSION == 1
static int audio_control_get_callback(const USBD_AC_CONTROL_INFO* pReqInfo, U8* pBuffer) {
    U32 Value;

    switch (pReqInfo->ID) {
    case USBD_AC_ID_EP_Microphone + USB_AC_SAMPLING_FREQ_CONTROL:
        switch (pReqInfo->bRequest) {
        case USB_AC_REQ_MIN:
        case USB_AC_REQ_MAX:
        case USB_AC_REQ_CUR:
            Value = AC_Global.CurrMicFreq;
            break;
        case USB_AC_REQ_RES:
            Value = 1;
            break;
        default:
            return -1;
        }
        USBD_StoreU24LE(pBuffer, Value);
        return 3;

    case USBD_AC_ID_UNIT_MicControl + USB_AC_FU_VOLUME_CONTROL:
        switch (pReqInfo->bRequest) {
        case USB_AC_REQ_CUR:
            Value = AC_Global.MicrophoneVolume;
            break;
        case USB_AC_REQ_MIN:
            Value = (U16)(int16_t)AUDIO_USB_MIC_VOLUME_MIN_DB_256;
            break;
        case USB_AC_REQ_MAX:
            Value = (U16)(int16_t)AUDIO_USB_MIC_VOLUME_MAX_DB_256;
            break;
        case USB_AC_REQ_RES:
            Value = (U16)(int16_t)AUDIO_USB_MIC_VOLUME_RES_DB_256;
            break;
        default:
            return -1;
        }
        USBD_StoreU16LE(pBuffer, Value);
        return 2;

    case USBD_AC_ID_UNIT_MicControl + USB_AC_FU_MUTE_CONTROL:
        pBuffer[0] = AC_Global.MicrophoneMute;
        return 1;

    }
    return -1;
}
#endif

/********************************************************************************
* Function Name: audio_control_set_callback
********************************************************************************
* Summary:
*  For audio 1.0 control requests!
*  Definition of the callback which is called when an audio set requests is received.
*  This callback is called in interrupt context and must not block.
*
* Parameters:
*  pReqInfo  : Contains information about the type of the control request.
*  NumBytes  : Number of bytes in pBuffer.
*  pBuffer    : Pointer to a buffer containing the request data.
*
* Return:
*  == 0:          Audio control request was handled by the callback.
*  != 0:          Audio control request was not handled by the callback (i.e. illegal request or parameters).
*                The stack will STALL the request.
*
*******************************************************************************/
#if USBD_AC_AUDIO_VERSION == 1
static int audio_control_set_callback(const USBD_AC_CONTROL_INFO* pReqInfo, U32 NumBytes, const U8* pBuffer) {

    switch (pReqInfo->ID) {
    case USBD_AC_ID_EP_Microphone + USB_AC_SAMPLING_FREQ_CONTROL:
        return 0;

    case USBD_AC_ID_UNIT_MicControl + USB_AC_FU_VOLUME_CONTROL:
        if (pReqInfo->bRequest == USB_AC_REQ_CUR && NumBytes == 2) {
            AC_Global.MicrophoneVolume = audio_clamp_microphone_volume((int16_t)USBD_GetU16LE(pBuffer));
            audio_notify_microphone_control_change_from_isr();
        }
        return 0;

    case USBD_AC_ID_UNIT_MicControl + USB_AC_FU_MUTE_CONTROL:
        AC_Global.MicrophoneMute = (*pBuffer != 0U) ? 1U : 0U;
        audio_notify_microphone_control_change_from_isr();
        return 0;
    }
    return -1;
}
#endif

/********************************************************************************
* Function Name: audio_in_callback
********************************************************************************
* Summary:
*  Definition of the callback which is called when audio data was sent to the host.
*  The function should initiate to send more data.
*
* Parameters:
*  Event - Event occurred on the audio channel.
*  pData - Pointer to the data send, that was provided to the USBD_AC_Send() function.
*  pContext - Pointer from the USBD_AC_RX_CTX structure.
*
* Return:
*  None
*
*******************************************************************************/
void audio_in_callback(USBD_AC_EVENT Event, const void* pData, void* pUserContext) {
    MESSAGE Msg;

    USB_USE_PARA(pData);
    USB_USE_PARA(pUserContext);
    switch (Event) {
    case USBD_AC_EVENT_DATA_SEND:
        Msg.Event = MSG_MIC_DATA;

        BaseType_t xHigherPriorityTaskWoken;
        xHigherPriorityTaskWoken = pdFALSE;
        xQueueSendFromISR(Mail_Box, &Msg, &xHigherPriorityTaskWoken);
        if (xHigherPriorityTaskWoken) {
            /* Actual macro used here is port specific. */
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        }
        break;

    default:
        break;
    }
}

/********************************************************************************
* Function Name: audio_class_init_data
********************************************************************************
* Summary:
*  Initialization data for the Audio class instance.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void audio_class_init_data(void) {
    USBD_AC_INIT_DATA  InitData;

    USB_MEMSET(&InitData, 0, sizeof(InitData));
    InitData.pACConfig = USB_AC_CONFIGURATION;
    InitData.pfControlGet = audio_control_get_callback;
    InitData.pfControlSet = audio_control_set_callback;
    InitData.pfSetAlternate = audio_set_interface_control_callback;

    USBD_AC_Add(&InitData);

    Mail_Box = xQueueCreateStatic(SEGGER_COUNTOF(Msg_Buff), sizeof(MESSAGE), (uint8_t*)Msg_Buff, &Static_Queue);

}

static void audio_usb_cdc_init_data(void)
{
    USB_CDC_INIT_DATA InitData;
    USB_ADD_EP_INFO EPBulkIn;
    USB_ADD_EP_INFO EPBulkOut;
    USB_ADD_EP_INFO EPIntIn;

    USB_MEMSET(&InitData, 0, sizeof(InitData));
    USB_MEMSET(&EPBulkIn, 0, sizeof(EPBulkIn));
    USB_MEMSET(&EPBulkOut, 0, sizeof(EPBulkOut));
    USB_MEMSET(&EPIntIn, 0, sizeof(EPIntIn));

    EPBulkIn.InDir = USB_DIR_IN;
    EPBulkIn.TransferType = USB_TRANSFER_TYPE_BULK;
    EPBulkIn.MaxPacketSize = USB_FS_BULK_MAX_PACKET_SIZE;
    InitData.EPIn = USBD_AddEPEx(&EPBulkIn, NULL, 0);

    EPBulkOut.InDir = USB_DIR_OUT;
    EPBulkOut.TransferType = USB_TRANSFER_TYPE_BULK;
    EPBulkOut.MaxPacketSize = USB_FS_BULK_MAX_PACKET_SIZE;
    InitData.EPOut = USBD_AddEPEx(&EPBulkOut,
                                  audio_usb_cdc_out_buffer,
                                  sizeof(audio_usb_cdc_out_buffer));

    EPIntIn.InDir = USB_DIR_IN;
    EPIntIn.TransferType = USB_TRANSFER_TYPE_INT;
    EPIntIn.Interval = 64U;
    EPIntIn.MaxPacketSize = USB_FS_INT_MAX_PACKET_SIZE;
    InitData.EPInt = USBD_AddEPEx(&EPIntIn, NULL, 0);

    audio_usb_cdc_handle = USBD_CDC_Add(&InitData);
}

/********************************************************************************
* Function Name: audio_usb_configured
********************************************************************************
* Summary:
*  Wrapper of USB configure
*
* Parameters:
*  None
*
* Return:
*  Return the status of USB_SUSPEND flag
*
*******************************************************************************/
int audio_usb_configured()
{
    return((USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)) == USB_STAT_CONFIGURED);
}

/********************************************************************************
* Function Name: audio_usb_suspended
********************************************************************************
* Summary:
*  Wrapper of USB suspend
*
* Parameters:
*  None
*
* Return:
*  Return the status of USB_SUSPEND flag
*
*******************************************************************************/
int audio_usb_suspended()
{
    return(USB_STAT_CONFIGURED != (USBD_GetState() & (USB_STAT_CONFIGURED | USB_STAT_SUSPENDED)));
}

/*******************************************************************************
* Function Name: audio_usb_init
********************************************************************************
* Summary:
*  Initializes the USB communication.
*
* Parameters:
*  None
*
* Return:
*  None
*
*******************************************************************************/
void audio_usb_init(void)
{
    USBD_Init();
    USBD_EnableIAD();

    /* Initialization data for the Audio class instance. */
    audio_class_init_data();
    audio_usb_cdc_init_data();

    /* Set device information*/
    USBD_SetDeviceInfo(&usb_device_info);
}

USB_CDC_HANDLE audio_usb_get_cdc_handle(void)
{
    return audio_usb_cdc_handle;
}

int audio_usb_cdc_ready(void)
{
    return (audio_usb_cdc_handle >= 0) && audio_usb_configured();
}

void audio_usb_reset_state(void)
{
    USB_MEMSET(&AC_Global, 0, sizeof(AC_Global));
    AC_Global.CurrMicFreq = MICROPHONE_FREQUENCIES;
    AC_Global.MicrophoneVolume = (U16)(int16_t)AUDIO_USB_MIC_VOLUME_DEFAULT_DB_256;
    AC_Global.MicrophoneMute = 0U;
}

/* [] END OF FILE */




