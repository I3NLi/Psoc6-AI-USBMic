/******************************************************************************
* File Name   : audio.h
*
* Description : This file contains the function prototypes and constants used
*               in audio_in.c
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
#ifndef AUDIO_H
#define AUDIO_H

#include "cyhal.h"
#include "Global.h"
#include "rtos.h"
#include "queue.h"
#include "Microphone_configs.h"

#if defined(__cplusplus)
extern "C" {
#endif

/******************************************************************************
* Macros
******************************************************************************/
#define WRITE_TIMEOUT               (10)

#define AUDIO_IN_NUM_CHANNELS                   (2)
#define AUDIO_IN_SUB_FRAME_SIZE                 (2)
#define AUDIO_IN_BIT_RESOLUTION                 (16)

#define MAX_AUDIO_IN_BUFFER_SIZE                (((MICROPHONE_FREQUENCIES * (AUDIO_IN_BIT_RESOLUTION / 8) * AUDIO_IN_NUM_CHANNELS) / 1000) / AUDIO_IN_SUB_FRAME_SIZE)

/*********************************************************************
*
*      Types
*
**********************************************************************/
typedef enum {
    MSG_SPEAKER_ON,
    MSG_SPEAKER_OFF,
    MSG_SPEAKER_DATA,
    MSG_MIC_ON,
    MSG_MIC_OFF,
    MSG_MIC_DATA,
    MSG_MIC_CONTROL_UPDATE,
    MSG_SPEAKERAUX_ON,
    MSG_SPEAKERAUX_OFF,
    MSG_SPEAKERAUX_DATA,
} MSG_TYPE;

typedef struct {
    MSG_TYPE   Event;
    U32        NumBtyes;
    void* pBuff;
} MESSAGE;

struct AC_Global_t {
    U32   CurrSpeakerFreq;
    U32   CurrSpeakerAuxFreq;
    U32   CurrMicFreq;
    U16   SpeakerVolume;
    U8    SpeakerMute[3];
    U8    SpeakerAltSetting;
    U8    SpeakerCurrBuff;
    U8    MicrophoneMute;
    U16   MicrophoneVolume;
    U8    MicrophoneAltSetting;
    U32   MicSoundSize;
    U32   MicDataRate;
    U32   MicSilentPackets;
    U32   MicDataSend;
    U32   MicSilentCount;
    U32   RemData;
    U16   SideToneVolume;
    U8    SideToneMute;
    const U8* pMicSound;
    const U8* pMicData;
    USBD_AC_STREAM_INTF_INFO MicInfo;
    USBD_AC_STREAM_INTF_INFO SpeakerInfo;
};

/******************************************************************************
* Externs
******************************************************************************/
extern cyhal_pdm_pcm_t pdm_pcm;
extern struct AC_Global_t AC_Global;
extern QueueHandle_t    Mail_Box;

/******************************************************************************
* Functions
******************************************************************************/
void audio_in_init(void);
void audio_in_process(void *arg);
void audio_clock_init(void);
void audio_app_task(void* arg);
void i2s_isr_handler(void *arg, cyhal_i2s_event_t event);
void audio_notify_microphone_control_change_from_isr(void);

#if defined(__cplusplus)
}
#endif

#endif /* AUDIO_H */

/* [] END OF FILE */
