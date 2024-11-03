/*******************************************************************************
 * File Name: cycfg_routing.h
 *
 * Description:
 * Establishes all necessary connections between hardware elements.
 * This file was automatically generated and should not be modified.
 * Configurator Backend 3.30.0
 * device-db 4.18.0.7028
 * mtb-pdl-cat1 3.12.0.36524
 *
 *******************************************************************************
 * Copyright 2024 Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ******************************************************************************/

#if !defined(CYCFG_ROUTING_H)
#define CYCFG_ROUTING_H

#include "cycfg_notices.h"

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

#define ioss_0_port_0_pin_0_ANALOG P0_0_SRSS_WCO_IN
#define ioss_0_port_0_pin_1_ANALOG P0_1_SRSS_WCO_OUT
#define ioss_0_port_6_pin_4_HSIOM P6_4_CPUSS_SWJ_SWO_TDO
#define ioss_0_port_6_pin_6_HSIOM P6_6_CPUSS_SWJ_SWDIO_TMS
#define ioss_0_port_6_pin_7_HSIOM P6_7_CPUSS_SWJ_SWCLK_TCLK
#define ioss_0_port_12_pin_6_ANALOG P12_6_SRSS_ECO_IN
#define ioss_0_port_12_pin_7_ANALOG P12_7_SRSS_ECO_OUT
#define ioss_0_port_14_pin_0_AUX USBDP_USB_USB_DP_PAD
#define ioss_0_port_14_pin_1_AUX USBDM_USB_USB_DM_PAD
#define cpuss_0_dw0_0_chan_8_tr_in_0_TRIGGER_OUT TRIG_OUT_1TO1_5_USB_DMA0_TO_PDMA0_TR_IN8
#define cpuss_0_dw0_0_chan_8_tr_out_0_TRIGGER_IN TRIG_IN_1TO1_6_PDMA0_TR_OUT8_TO_USB_ACK0
#define usb_0_dma_burstend_0_TRIGGER_OUT TRIG_OUT_1TO1_6_PDMA0_TR_OUT8_TO_USB_ACK0
#define usb_0_dma_req_0_TRIGGER_IN TRIG_IN_1TO1_5_USB_DMA0_TO_PDMA0_TR_IN8

void init_cycfg_routing(void);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* CYCFG_ROUTING_H */
