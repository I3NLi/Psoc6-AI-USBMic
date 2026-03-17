# emUSB-Device: Audio recorder

This example demonstrates how to set up the USB block of an Infineon MCU using the audio class of Segger's [emUSB-Device](https://github.com/Infineon/emusb-device) middleware to implement an audio recorder. It is currently supported on PSoC&trade; 6 MCU. 
This code example uses digital microphones with the pulse density modulation (PDM) to pulse code modulation (PCM) converter hardware block. Audio data captured by the microphones is streamed over USB to a PC using the audio device class. An audio recorder software tool, such as [Audacity](https://www.audacityteam.org/), running on a computer initiates the recording and streaming of audio data.


**Figure 1. Block diagram**

![Block Diagram](images/usbd-audio-recorder-block-diagram.png)

The PDM/PCM interface requires two wires - PDM Clock and PDM Data. Up to two digital PDM microphones can share the same PDM data line. One microphone samples in the falling edge and the other on the rising edge.

[View this README on GitHub.](https://github.com/Infineon/mtb-example-usb-device-audio-recorder-freertos)

[Provide feedback on this code example.](https://cypress.co1.qualtrics.com/jfe/form/SV_1NTns53sK2yiljn?Q_EED=eyJVbmlxdWUgRG9jIElkIjoiQ0UyMzY3ODUiLCJTcGVjIE51bWJlciI6IjAwMi0zNjc4NSIsIkRvYyBUaXRsZSI6ImVtVVNCLURldmljZTogQXVkaW8gcmVjb3JkZXIiLCJyaWQiOiJtYWp1bWRhciIsIkRvYyB2ZXJzaW9uIjoiMi4wLjAiLCJEb2MgTGFuZ3VhZ2UiOiJFbmdsaXNoIiwiRG9jIERpdmlzaW9uIjoiTUNEIiwiRG9jIEJVIjoiSUNXIiwiRG9jIEZhbWlseSI6IlBTT0MifQ==)


## Requirements


- [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) v3.2 or later (tested with v3.2)
- Board support package (BSP) minimum required version: 4.0.0
- Programming language: C
- Associated parts: All [PSoC&trade; 6 MCU](https://www.infineon.com/cms/en/product/microcontroller/32-bit-psoc-arm-cortex-microcontroller/psoc-6-32-bit-arm-cortex-m4-mcu) parts


## Supported toolchains (make variable 'TOOLCHAIN')

- GNU Arm&reg; Embedded Compiler v11.3.1 (`GCC_ARM`) – Default value of `TOOLCHAIN`
- Arm&reg; Compiler v6.16 (`ARM`)
- IAR C/C++ Compiler v9.40.2 (`IAR`)



## Supported kits (make variable 'TARGET')

- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; Prototyping Kit](https://www.infineon.com/CY8CPROTO-062-4343W) (`CY8CPROTO-062-4343W`) – Default value of `TARGET`
- [PSoC&trade; 6 Wi-Fi Bluetooth&reg; Pioneer Kit](https://www.infineon.com/CY8CKIT-062-WIFI-BT) (`CY8CKIT-062-WIFI-BT`)
- [PSoC&trade; 62S2 Wi-Fi Bluetooth&reg; Pioneer Kit](https://www.infineon.com/CY8CKIT-062S2-43012) (`CY8CKIT-062S2-43012`)
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; Pioneer Kit](https://www.infineon.com/CYW9P62S1-43438EVB-01) (`CYW9P62S1-43438EVB-01`)
- [PSoC&trade; 62S1 Wi-Fi Bluetooth&reg; Pioneer Kit](https://www.infineon.com/CYW9P62S1-43012EVB-01) (`CYW9P62S1-43012EVB-01`)
- [PSoC&trade; 62S2 Evaluation Kit](https://www.infineon.com/CY8CEVAL-062S2) (`CY8CEVAL-062S2`, `CY8CEVAL-062S2-LAI-4373M2`, `CY8CEVAL-062S2-MUR-43439M2`, `CY8CEVAL-062S2-LAI-43439M2`, `CY8CEVAL-062S2-MUR-4373EM2`, `CY8CEVAL-062S2-MUR-4373M2`, `CY8CEVAL-062S2-CYW43022CUB`)




## Hardware setup

This code example requires a PDM microphone. If using an Arduino-compatible kit, you can use the [TFT display shield board CY8CKIT-028-TFT](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-028-tft/), which has a PDM microphone. This shield comes with CY8CKIT-062-WIFI-BT kit. It can also be purchased standalone and be used with other supported kits listed.

> **Note:** The PSoC&trade; 6 Bluetooth&reg; LE Pioneer Kit (CY8CKIT-062-BLE) and the PSoC&trade; 6 Wi-Fi Bluetooth&reg; Pioneer Kit (CY8CKIT-062-WIFI-BT) ship with KitProg2 installed. ModusToolbox&trade; requires KitProg3. Before using this code example, make sure that the board is upgraded to KitProg3. The tool and instructions are available in the [Firmware Loader](https://github.com/Infineon/Firmware-loader) GitHub repository. If you do not upgrade, you will see an error like "unable to find CMSIS-DAP device" or "KitProg firmware is out of date".


## Software setup

See the [ModusToolbox&trade; tools package installation guide](https://www.infineon.com/ModusToolboxInstallguide) for information about installing and configuring the tools package.

Install a terminal emulator if you don't have one. Instructions in this document use [Tera Term](https://teratermproject.github.io/index-en.html).

This example uses the [Audacity](https://www.audacityteam.org/) tool to stream the audio recording from Infineon MCU detected as microphone. You can also use any other software tool that is able to stream audio data.

For Linux hosts that need this board to provide Bluetooth in addition to the USB microphone, see [tools/linux/README.md](tools/linux/README.md) for the `udev + systemd + btattach` auto-attach setup.

For Windows hosts, see [tools/windows/README.md](tools/windows/README.md). The current firmware works as USB audio plus a CDC ACM port on Windows, but using the board as a Windows Bluetooth adapter needs a separate Windows transport-driver path or a native USB Bluetooth device mode.



## Using the code example


### Create the project

The ModusToolbox&trade; tools package provides the Project Creator as both a GUI tool and a command line tool.

<details><summary><b>Use Project Creator GUI</b></summary>

1. Open the Project Creator GUI tool.

   There are several ways to do this, including launching it from the dashboard or from inside the Eclipse IDE. For more details, see the [Project Creator user guide](https://www.infineon.com/ModusToolboxProjectCreator) (locally available at *{ModusToolbox&trade; install directory}/tools_{version}/project-creator/docs/project-creator.pdf*).

2. On the **Choose Board Support Package (BSP)** page, select a kit supported by this code example. See [Supported kits](#supported-kits-make-variable-target).

   > **Note:** To use this code example for a kit not listed here, you may need to update the source files. If the kit does not have the required resources, the application may not work.

3. On the **Select Application** page:

   a. Select the **Applications(s) Root Path** and the **Target IDE**.

   > **Note:** Depending on how you open the Project Creator tool, these fields may be pre-selected for you.

   b. Select this code example from the list by enabling its check box.

   > **Note:** You can narrow the list of displayed examples by typing in the filter box.

   c. (Optional) Change the suggested **New Application Name** and **New BSP Name**.

   d. Click **Create** to complete the application creation process.

</details>


<details><summary><b>Use Project Creator CLI</b></summary>

The 'project-creator-cli' tool can be used to create applications from a CLI terminal or from within batch files or shell scripts. This tool is available in the *{ModusToolbox&trade; install directory}/tools_{version}/project-creator/* directory.

Use a CLI terminal to invoke the 'project-creator-cli' tool. On Windows, use the command-line 'modus-shell' program provided in the ModusToolbox&trade; installation instead of a standard Windows command-line application. This shell provides access to all ModusToolbox&trade; tools. You can access it by typing "modus-shell" in the search box in the Windows menu. In Linux and macOS, you can use any terminal application.

The following example clones the "[emUSB-Device: Audio device](https://github.com/Infineon/mtb-example-usb-device-audio-freertos)" application with the desired name "USBAudioDevice" configured for the *CY8CPROTO-062-4343W* BSP into the specified working directory, *C:/mtb_projects*:

   ```
   project-creator-cli --board-id CY8CPROTO-062-4343W --app-id mtb-example-usb-device-audio-freertos --user-app-name USBAudioDevice --target-dir "C:/mtb_projects"
   ```

The 'project-creator-cli' tool has the following arguments:

Argument | Description | Required/optional
---------|-------------|-----------
`--board-id` | Defined in the <id> field of the [BSP](https://github.com/Infineon?q=bsp-manifest&type=&language=&sort=) manifest | Required
`--app-id`   | Defined in the <id> field of the [CE](https://github.com/Infineon?q=ce-manifest&type=&language=&sort=) manifest | Required
`--target-dir`| Specify the directory in which the application is to be created if you prefer not to use the default current working directory | Optional
`--user-app-name`| Specify the name of the application if you prefer to have a name other than the example's default name | Optional

<br>

> **Note:** The project-creator-cli tool uses the `git clone` and `make getlibs` commands to fetch the repository and import the required libraries. For details, see the "Project creator tools" section of the [ModusToolbox&trade; tools package user guide](https://www.infineon.com/ModusToolboxUserGuide) (locally available at {ModusToolbox&trade; install directory}/docs_{version}/mtb_user_guide.pdf).

</details>


### Open the project

After the project has been created, you can open it in your preferred development environment.

<details><summary><b>Eclipse IDE</b></summary>

If you opened the Project Creator tool from the included Eclipse IDE, the project will open in Eclipse automatically.

For more details, see the [Eclipse IDE for ModusToolbox&trade; user guide](https://www.infineon.com/MTBEclipseIDEUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_ide_user_guide.pdf*).

</details>


<details><summary><b>Visual Studio (VS) Code</b></summary>

Launch VS Code manually, and then open the generated *{project-name}.code-workspace* file located in the project directory.

For more details, see the [Visual Studio Code for ModusToolbox&trade; user guide](https://www.infineon.com/MTBVSCodeUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_vscode_user_guide.pdf*).

</details>


<details><summary><b>Keil µVision</b></summary>

Double-click the generated *{project-name}.cprj* file to launch the Keil µVision IDE.

For more details, see the [Keil µVision for ModusToolbox&trade; user guide](https://www.infineon.com/MTBuVisionUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_uvision_user_guide.pdf*).

</details>

<details><summary><b>IAR Embedded Workbench</b></summary>

Open IAR Embedded Workbench manually, and create a new project. Then select the generated *{project-name}.ipcf* file located in the project directory.

For more details, see the [IAR Embedded Workbench for ModusToolbox&trade; user guide](https://www.infineon.com/MTBIARUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mt_iar_user_guide.pdf*).

</details>

<details><summary><b>Command line</b></summary>

If you prefer to use the CLI, open the appropriate terminal, and navigate to the project directory. On Windows, use the command-line 'modus-shell' program; on Linux and macOS, you can use any terminal application. From there, you can run various `make` commands.

For more details, see the [ModusToolbox&trade; tools package user guide](https://www.infineon.com/ModusToolboxUserGuide) (locally available at *{ModusToolbox&trade; install directory}/docs_{version}/mtb_user_guide.pdf*).

</details>


## Operation


1. Connect the board to your Windows computer using the provided USB cable through the KitProg3 USB connector.

2. Open a terminal program and select the KitProg3 COM port. Set the serial port parameters to 8N1 and 115200 baud.

3. Program the board using one of the following:

   <details><summary><b>Using Eclipse IDE</b></summary>

      1. Select the application project in the Project Explorer.

      2. In the **Quick Panel**, scroll down, and click **\<Application Name> Program (KitProg3_MiniProg4)**.
   </details>


   <details><summary><b>In other IDEs</b></summary>

   Follow the instructions in your preferred IDE.
   </details>


   <details><summary><b>Using CLI</b></summary>

     From the terminal, execute the `make program` command to build and program the application using the default toolchain to the default target. The default toolchain is specified in the application's Makefile but you can override this value manually:
      ```
      make program TOOLCHAIN=<toolchain>
      ```

      Example:
      ```
      make program TOOLCHAIN=GCC_ARM
      ```
   </details>

4. After programming, the application starts automatically. Confirm that the title of the code example is displayed on the serial terminal and the kit user LED keeps blinking until the USB cable is plugged to the USB device connector.

   **Figure 2. USB audio device startup**

   ![](images/usbd-audio-recorder-startup.png)

5. Connect another USB cable to the USB device connector (see the kit user guide for its location). If using the CY8CKIT-062-WIFI-BT or any other Arduino-compatible kit, ensure the TFT shield is connected to the main board.

6. On the Windows computer, verify that a new USB device is enumerated as a microphone and named as "USB audio recorder".

   **Figure 3. USB audio device enumeration**

   ![](images/usbd-audio-recorder-enumeration.png)

7. Note that the kit user LED is turned OFF and a USB audio device connected status is displayed on the serial terminal as shown in **Figure 4**.

   **Figure 4. USB audio device connected**

   ![](images/usbd-audio-recorder-connected.png)

8. Open an audio recorder software tool in the computer. If you do not have one, download the open-source software [Audacity](https://www.audacityteam.org/). Make sure to select the correct microphone in the audio recorder tool as shown in **Figure 5**.

   **Figure 5. Microphone on Audacity**

   ![](images/usbd-audio-recorder-mic.png)
   
9. Start a recording session in the audio recorder software and play a sound, or speak over the microphone in the kit. Subsequently the kit user LED is turned ON indicating that the device has started recording.

10. To exercise the microphone's mute control, open the **Control Panel** from Windows Start menu and double-click on the **Sound** menu in it. In the **Sound** window, navigate to the **Recording** tab and select the detected **Microphone (Audio Control)**.

11. Double-click the selected **Microphone (Audio Control)** to open its properties. In the **Microphone Properties** window, toggle the **Mute** ![](images/mute-mic.png) button under the **Levels** tab to mute/unmute the microphone.

    **Figure 6. USB audio device mute**

    ![](images/usbd-audio-recorder-mute-control.png)

12. Stop the recording session and play it to confirm that the audio was recorded correctly. Also observe that the kit user LED is turned OFF indicating the end of recording session.

13. Once the USB cable gets disconnected from the USB device, USB audio device disconnected status is displayed on the serial terminal as shown in **Figure 7**.

    **Figure 7. USB audio device disconnected**

    ![](images/usbd-audio-recorder-disconnected.png)
      
      
## Debugging

You can debug the example to step through the code.


<details><summary><b>In Eclipse IDE</b></summary>

Use the **\<Application Name> Debug (KitProg3_MiniProg4)** configuration in the **Quick Panel**. For details, see the "Program and debug" section in the [Eclipse IDE for ModusToolbox&trade; user guide](https://www.infineon.com/MTBEclipseIDEUserGuide).

> **Note:** **(Only while debugging)** On the CM4 CPU, some code in `main()` may execute before the debugger halts at the beginning of `main()`. This means that some code executes twice – once before the debugger stops execution, and again after the debugger resets the program counter to the beginning of `main()`. See [KBA231071](https://community.infineon.com/docs/DOC-21143) to learn about this and for the workaround.

</details>


<details><summary><b>In other IDEs</b></summary>

Follow the instructions in your preferred IDE.

</details>


## Design and implementation

The PDM/PCM hardware block can sample one or two PDM digital microphones. In this application, the hardware block is configured to sample stereo audio at 4 ksps (default) with 16-bits resolution. The sample audio data is eventually transferred to the USB data endpoint buffer. 

1. This example can support audio sampling rate up to 44.1 ksps on PSoC&trade; 6. Because when the USB host and PSoC&trade; 6 audio subsystem are out of sync, the PDM/PCM block can generate variable number of bytes (196/188) at every 1 ms to be sent to the USB host. If all the bytes are not sent, the PDM/PCM FIFO will eventually overflow or underflow, causing disruption in the audio streaming to the USB host. As the ISO transfer is limited to 192-bytes in the  PSoC&trade; 6 driver, variable number of bytes generated by the audio subsystem cannot be transferred at audio sample rate greater than or equal to 48 ksps with 16-bits resolution.
    > **Note:** This limitation is not applicable for DMA driver.
2. You can change the sample frequencies by generating the “USB audio design” files by useing the example (.uad) files provided in 'configs' folder and include it in application. For more information see, [USB Audio Device Generator tool](#usb-audio-device-generator-tool) section.
3. The USB descriptor implements the audio device class with one endpoint:
      - **Audio IN Endpoint:** Sends the data to the USB host

The firmware consists of a main() function which creates an "Audio App Task". This task invokes audio_class_init_data() function to add the audio interface to USB stack. It configures the device descriptor for enumeration using USBD_SetDeviceInfo() API. Once the configuration is done, "Audio App Task" calls audio_in_init() function to initialize the PDM PCM block. At the end it creates the "Audio In Task" and calls the target API USBD_Start() to start the USB stack. This task keeps track of the USB connection/disconnection events by monitoring the suspend and resume conditions on the bus.


The application has DMA enabled by default. Follow these steps to disable the DMA feature and ensure that the application functions without DMA:

1. Remove the `USBD_ENABLE_DMA` from the DEFINE variable in the Makefile.
2. Disable all DMA DataWire in the DMA tab from the device configurator.
3. Turn off all DMA channel for endpoints in the USB personality from the device configurator.
4. Disable the 'Enable DMA mode' in the USB personality from the device configurator.


### USB Audio Device Generator tool

A microphone configuration must be defined by creating an “USB audio design” file (extension .uad). This file specifies all characteristics of the audio device and is converted by the [USBAudioDeviceGenerator.exe](https://github.com/Infineon/emusb-device/blob/master/tool/USBAudioDeviceGenerator.exe) tool into a C source file and a C header file, that should be used to build the audio application. The tool provided in [emUSB-Device](https://github.com/Infineon/emusb-device) middleware library. For more details, see Section 17.2 from [emUSB-Device User Guide & Reference Manual](https://www.segger.com/downloads/emusb-device/UM09001).

The */configs/Microphone_configs_48kHz* files contains the default audio configuration for the application and also provides the *USB audio design* file (Microphone_configs_48kHz.uad).

The USB Audio Device Generator tool is a command-line tool that can be invoked from a command shell:

```
USBAudioDeviceGenerator.exe [-s] [-o=<output-file>] <USB-audio-design-file>

   -s = Silent execution (except for errors).
   -o = Base name for the generated .c and .h files.
   If not specified, the name of the USB audio design file is used.
```


### Resources and settings

**Table 1. Application resources**

 Resource  |  Alias/object     |    Purpose
 :------- | :------------    | :------------ 
 USBDEV (HAL) | CYBSP_EMUSB_DEV  | USB device block configured with audio descriptor
 UART (HAL)   | CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX | UART Tx and Rx pins used by Retarget-IO for printing on the console
 GPIO (HAL)    | CYBSP_USER_LED | User LED is turned ON when audio is recorded
 PDM/PCM (HAL) | pdm_pcm | Interfaces with the microphone

<br>


## Related resources

Resources  | Links
-----------|----------------------------------
Application notes  | [AN228571](https://www.infineon.com/AN228571) – Getting started with PSoC&trade; 6 MCU on ModusToolbox&trade; <br>  [AN215656](https://www.infineon.com/AN215656) – PSoC&trade; 6 MCU: Dual-CPU system design
Code examples  | [Using ModusToolbox&trade;](https://github.com/Infineon/Code-Examples-for-ModusToolbox-Software) on GitHub
Device documentation | [PSoC&trade; 6 MCU datasheets](https://documentation.infineon.com/html/psoc6/bnm1651211483724.html) <br> [PSoC&trade; 6 technical reference manuals](https://documentation.infineon.com/html/psoc6/zrs1651212645947.html)
Development kits | Select your kits from the [Evaluation board finder](https://www.infineon.com/cms/en/design-support/finder-selection-tools/product-finder/evaluation-board)
Libraries on GitHub  | [mtb-pdl-cat1](https://github.com/Infineon/mtb-pdl-cat1) – Peripheral Driver Library (PDL)  <br> [mtb-hal-cat1](https://github.com/Infineon/mtb-hal-cat1) – Hardware Abstraction Layer (HAL) library <br> [retarget-io](https://github.com/Infineon/retarget-io) – Utility library to retarget STDIO messages to a UART port
Middleware on GitHub  | [emUSB-Device](https://github.com/Infineon/emusb-device) – emUSB-Device <br> [emUSB-Device API reference](https://infineon.github.io/emusb-device/html/index.html) – emUSB-Device API Reference <br> [psoc6-middleware](https://github.com/Infineon/modustoolbox-software#psoc-6-middleware-libraries) – Links to all PSoC&trade; 6 MCU middleware
Tools  | [ModusToolbox&trade;](https://www.infineon.com/modustoolbox) – ModusToolbox&trade; software is a collection of easy-to-use libraries and tools enabling rapid development with Infineon MCUs for applications ranging from wireless and cloud-connected systems, edge AI/ML, embedded sense and control, to wired USB connectivity using PSoC&trade; Industrial/IoT MCUs, AIROC&trade; Wi-Fi and Bluetooth&reg; connectivity devices, XMC&trade; Industrial MCUs, and EZ-USB&trade;/EZ-PD&trade; wired connectivity controllers. ModusToolbox&trade; incorporates a comprehensive set of BSPs, HAL, libraries, configuration tools, and provides support for industry-standard IDEs to fast-track your embedded application development.

<br>


## Other resources

Infineon provides a wealth of data at [www.infineon.com](https://www.infineon.com) to help you select the right device, and quickly and effectively integrate it into your design.


## Document history

Document title: *CE236785* - *emUSB-Device: Audio recorder*

 Version | Description of change
 ------- | ---------------------
 1.0.0   | New code example
 2.0.0   | Updated to support ModusToolbox&trade; v3.2 <br> Updated to support new audio class and DMA <br> Added support for CY8CEVAL-062S2-MUR-4373EM2, CY8CEVAL-062S2-MUR-4373M2, and CY8CEVAL-062S2-CYW43022CUB kits

<br>


All referenced product or service names and trademarks are the property of their respective owners.

The Bluetooth&reg; word mark and logos are registered trademarks owned by Bluetooth SIG, Inc., and any use of such marks by Infineon is under license.


---------------------------------------------------------

© Cypress Semiconductor Corporation, 2023-2024. This document is the property of Cypress Semiconductor Corporation, an Infineon Technologies company, and its affiliates ("Cypress").  This document, including any software or firmware included or referenced in this document ("Software"), is owned by Cypress under the intellectual property laws and treaties of the United States and other countries worldwide.  Cypress reserves all rights under such laws and treaties and does not, except as specifically stated in this paragraph, grant any license under its patents, copyrights, trademarks, or other intellectual property rights.  If the Software is not accompanied by a license agreement and you do not otherwise have a written agreement with Cypress governing the use of the Software, then Cypress hereby grants you a personal, non-exclusive, nontransferable license (without the right to sublicense) (1) under its copyright rights in the Software (a) for Software provided in source code form, to modify and reproduce the Software solely for use with Cypress hardware products, only internally within your organization, and (b) to distribute the Software in binary code form externally to end users (either directly or indirectly through resellers and distributors), solely for use on Cypress hardware product units, and (2) under those claims of Cypress's patents that are infringed by the Software (as provided by Cypress, unmodified) to make, use, distribute, and import the Software solely for use with Cypress hardware products.  Any other use, reproduction, modification, translation, or compilation of the Software is prohibited.
<br>
TO THE EXTENT PERMITTED BY APPLICABLE LAW, CYPRESS MAKES NO WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, WITH REGARD TO THIS DOCUMENT OR ANY SOFTWARE OR ACCOMPANYING HARDWARE, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  No computing device can be absolutely secure.  Therefore, despite security measures implemented in Cypress hardware or software products, Cypress shall have no liability arising out of any security breach, such as unauthorized access to or use of a Cypress product. CYPRESS DOES NOT REPRESENT, WARRANT, OR GUARANTEE THAT CYPRESS PRODUCTS, OR SYSTEMS CREATED USING CYPRESS PRODUCTS, WILL BE FREE FROM CORRUPTION, ATTACK, VIRUSES, INTERFERENCE, HACKING, DATA LOSS OR THEFT, OR OTHER SECURITY INTRUSION (collectively, "Security Breach").  Cypress disclaims any liability relating to any Security Breach, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any Security Breach.  In addition, the products described in these materials may contain design defects or errors known as errata which may cause the product to deviate from published specifications. To the extent permitted by applicable law, Cypress reserves the right to make changes to this document without further notice. Cypress does not assume any liability arising out of the application or use of any product or circuit described in this document. Any information provided in this document, including any sample design information or programming code, is provided only for reference purposes.  It is the responsibility of the user of this document to properly design, program, and test the functionality and safety of any application made of this information and any resulting product.  "High-Risk Device" means any device or system whose failure could cause personal injury, death, or property damage.  Examples of High-Risk Devices are weapons, nuclear installations, surgical implants, and other medical devices.  "Critical Component" means any component of a High-Risk Device whose failure to perform can be reasonably expected to cause, directly or indirectly, the failure of the High-Risk Device, or to affect its safety or effectiveness.  Cypress is not liable, in whole or in part, and you shall and hereby do release Cypress from any claim, damage, or other liability arising from any use of a Cypress product as a Critical Component in a High-Risk Device. You shall indemnify and hold Cypress, including its affiliates, and its directors, officers, employees, agents, distributors, and assigns harmless from and against all claims, costs, damages, and expenses, arising out of any claim, including claims for product liability, personal injury or death, or property damage arising from any use of a Cypress product as a Critical Component in a High-Risk Device. Cypress products are not intended or authorized for use as a Critical Component in any High-Risk Device except to the limited extent that (i) Cypress's published data sheet for the product explicitly states Cypress has qualified the product for use in a specific High-Risk Device, or (ii) Cypress has given you advance written authorization to use the product as a Critical Component in the specific High-Risk Device and you have signed a separate indemnification agreement.
<br>
Cypress, the Cypress logo, and combinations thereof, ModusToolbox, PSoC, CAPSENSE, EZ-USB, F-RAM, and TRAVEO are trademarks or registered trademarks of Cypress or a subsidiary of Cypress in the United States or in other countries. For a more complete list of Cypress trademarks, visit www.infineon.com. Other names and brands may be claimed as property of their respective owners.
