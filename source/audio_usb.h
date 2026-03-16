/******************************************************************************
* File Name   : audio_usb.h
*
* Description : This file contains the function prototypes and constants used
*               in audio_usb.c
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
#ifndef AUDIO_USB_H
#define AUDIO_USB_H

#include "cyhal.h"
#include "Global.h"
#include "Microphone_configs.h"

#if defined(__cplusplus)
extern "C" {
#endif

#define AUDIO_USB_MIC_VOLUME_MIN_DB_256         ((int16_t)-3072)
#define AUDIO_USB_MIC_VOLUME_MAX_DB_256         ((int16_t)2688)
#define AUDIO_USB_MIC_VOLUME_RES_DB_256         ((int16_t)128)
#define AUDIO_USB_MIC_VOLUME_DEFAULT_DB_256     (AUDIO_USB_MIC_VOLUME_MAX_DB_256)

#define AUDIO_DEVICE_VENDOR_ID               (0x0669)
#if (AUDIO_SAMPLING_RATE_22KHZ == MICROPHONE_FREQUENCIES)
#define AUDIO_DEVICE_PRODUCT_ID              (0x0225)
#else
#define AUDIO_DEVICE_PRODUCT_ID              (0x0226)
#endif /* (AUDIO_SAMPLING_RATE_22KHZ == MICROPHONE_FREQUENCIES) */

/* Information that is used during enumeration. */
static const USB_DEVICE_INFO usb_device_info = {
  AUDIO_DEVICE_VENDOR_ID,        // VendorId
  AUDIO_DEVICE_PRODUCT_ID,      // ProductId
  "Infineon Technologies",      // VendorName
  "USB Sound Recorder",     // ProductName
  "13245678"      // SerialNumber
};

/******************************************************************************
* Externs
******************************************************************************/
extern MESSAGE          Msg_Buff[5];

/******************************************************************************
* Functions
******************************************************************************/
void audio_in_callback(USBD_AC_EVENT Event, const void* pData, void* pUserContext);
void audio_class_init_data(void);
int audio_usb_suspended();
int audio_usb_configured();
void audio_usb_init(void);
void audio_usb_reset_state(void);

#if defined(__cplusplus)
}
#endif

#endif /* AUDIO_USB_H */

/* [] END OF FILE */
