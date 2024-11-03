/*******************************************************************************
 * File Name: cycfg_dmas.h
 *
 * Description:
 * DMA configuration
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

#if !defined(CYCFG_DMAS_H)
#define CYCFG_DMAS_H

#include "cycfg_notices.h"
#include "cy_dma.h"

#if defined (CY_USING_HAL)
#include "cyhal_hwmgr.h"
#endif /* defined (CY_USING_HAL) */

#if defined (CY_USING_HAL) || defined (CY_USING_HAL_LITE)
#include "cyhal_dma.h"
#endif /* defined (CY_USING_HAL) || defined (CY_USING_HAL_LITE) */

#if defined(__cplusplus)
extern "C" {
#endif /* defined(__cplusplus) */

#define cpuss_0_dw0_0_chan_8_ENABLED 1U
#define cpuss_0_dw0_0_chan_8_HW DW0
#define cpuss_0_dw0_0_chan_8_CHANNEL 8U
#define cpuss_0_dw0_0_chan_8_IRQ cpuss_interrupts_dw0_8_IRQn

extern const cy_stc_dma_descriptor_config_t cpuss_0_dw0_0_chan_8_Descriptor_0_config;
extern const cy_stc_dma_descriptor_config_t cpuss_0_dw0_0_chan_8_Descriptor_1_config;
extern const cy_stc_dma_descriptor_config_t cpuss_0_dw0_0_chan_8_Descriptor_2_config;
extern const cy_stc_dma_descriptor_config_t cpuss_0_dw0_0_chan_8_Descriptor_3_config;
extern cy_stc_dma_descriptor_t cpuss_0_dw0_0_chan_8_Descriptor_0;
extern cy_stc_dma_descriptor_t cpuss_0_dw0_0_chan_8_Descriptor_1;
extern cy_stc_dma_descriptor_t cpuss_0_dw0_0_chan_8_Descriptor_2;
extern cy_stc_dma_descriptor_t cpuss_0_dw0_0_chan_8_Descriptor_3;
extern const cy_stc_dma_channel_config_t cpuss_0_dw0_0_chan_8_channelConfig;
extern const cy_stc_dma_crc_config_t cpuss_0_dw0_0_chan_8_crcConfig;

#if defined (CY_USING_HAL) || defined (CY_USING_HAL_LITE)
extern const cyhal_resource_inst_t cpuss_0_dw0_0_chan_8_obj;
extern const cyhal_dma_configurator_t cpuss_0_dw0_0_chan_8_hal_config;
#endif /* defined (CY_USING_HAL) || defined (CY_USING_HAL_LITE) */

void reserve_cycfg_dmas(void);

#if defined(__cplusplus)
}
#endif /* defined(__cplusplus) */

#endif /* CYCFG_DMAS_H */
