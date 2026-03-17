/*******************************************************************************
* File Name:   main.c
*
* Description: This is the source code for the USB audio recorder
*              example for ModusToolbox.
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
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "audio.h"
#include "audio_usb.h"
#include "bt_hci_bridge.h"
#include "rtos.h"


/*****************************************************************************
* Function Name: main
******************************************************************************
* Summary:
*  This is the main function for CM4 CPU. It does...
*    1. Initializes the target BSP.
*    2. Initializes the I2C.
*    3. Initializes retarget-io to use the debug UART port.
*    4. Initialize the OLED display.
*    3. Initializes the User LED.
*    4. Initializes the audio app and starts the FreeRTOS scheduler.
*
* Parameters:
*  None
*
* Return:
*  int
*
*****************************************************************************/
// #define CYBSP_OUT_I2C_SCL (P0_2) already added in cycfg_pins.h
// #define CYBSP_OUT_I2C_SDA (P0_3)
int main(void)
{
    cy_rslt_t result;
    // cyhal_i2c_t i2c_obj;

    /* Initialize the device and board peripherals */
    result = cybsp_init();
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Initialize the I2C to use with the OLED display */
    // result = cyhal_i2c_init(&i2c_obj, CYBSP_I2C_SDA, CYBSP_I2C_SCL, NULL);

    /* I2C init failed. Stop program execution */
    // if (CY_RSLT_SUCCESS != result)
    // {
    //     CY_ASSERT(0);
    // }

    /* Initialize retarget-io to use the debug UART port */
    result = cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* retarget-io init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Initialize the OLED display */
    // result = mtb_ssd1306_init_i2c(&i2c_obj);

    /* OLED init failed. Stop program execution */
    // if (CY_RSLT_SUCCESS != result)
    // {
    //     CY_ASSERT(0);
    // }

    /* Initialize the User LED */
    result = cyhal_gpio_init(CYBSP_USER_LED, CYHAL_GPIO_DIR_OUTPUT, CYHAL_GPIO_DRIVE_STRONG, CYBSP_LED_STATE_OFF);

    /* GPIO init failed. Stop program execution */
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    // GUI_Init();
    // GUI_DispString("Audio recorder");

    /* \x1b[2J\x1b[;H - ANSI ESC sequence for clear screen */
    printf("\x1b[2J\x1b[;H");

    printf("******************"
        " emUSB-Device: Audio + BT bridge "
        "******************\r\n\n");

    /* Initialize the audio clock based on audio sample rate */
    audio_clock_init();

    /* Init the audio IN application */
    audio_in_init();

    /* Initialize the USB and Audio application */
    audio_usb_init();

    /* Initialize the Bluetooth HCI bridge */
    result = bt_hci_bridge_init();
    if (CY_RSLT_SUCCESS != result)
    {
        CY_ASSERT(0);
    }

    /* Start the RTOS Scheduler */
    vTaskStartScheduler();

    /* Should never get there */
    printf("APP_LOG: Error: FreeRTOS doesn't start\r\n");

    return 0;
}

/* [] END OF FILE */
