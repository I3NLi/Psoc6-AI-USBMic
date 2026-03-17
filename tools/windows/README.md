# Windows Support

This project now supports Windows in two different ways, and they are not equivalent.

## What works right now

With the current firmware, Windows should enumerate the board as:

- a standard USB microphone
- a standard USB CDC ACM serial port

That means:

- audio recording should work with the in-box USB audio driver
- the CDC interface should show up as a COM port and can use the in-box `Usbser.sys` path on modern Windows

What does **not** happen automatically with the current firmware:

- Windows will **not** turn that COM port into a system Bluetooth radio by itself

This is the main difference from Linux, where `btattach` can bind the CDC ACM port to `hci0`.

## Why Windows is different

According to Microsoft's Bluetooth transport guidance:

- Bluetooth over `USB` has in-box support
- Bluetooth over `non-USB HCI transport` requires a vendor transport driver

Our current board-to-host Bluetooth path is `USB CDC ACM -> H4 stream`, which is convenient for Linux, but on Windows it still needs a transport-driver story if you want the OS Bluetooth stack to own the radio.

## Windows paths

### Path 1: Keep current firmware and add a Windows driver

Keep the current `USB Audio + CDC ACM + H4 bridge` firmware and build a Windows Bluetooth transport driver around it.

Microsoft's starting point for that route is:

- Bluetooth Serial HCI Bus Driver sample
- Bluetooth transport bus driver documentation

This path is possible, but it is a real WDK driver project, not a small helper script.

### Path 2: Change firmware to a native USB Bluetooth device

Change the firmware so the board exposes:

- USB Audio
- USB Bluetooth HCI transport

This is the better path if you want Windows to treat the board like a normal USB Bluetooth adapter with in-box transport support.

It also means the current CDC ACM bridge mode would no longer be the Windows Bluetooth path.

## What is included in this folder

- [Get-UsbBtBridgeStatus.ps1](Get-UsbBtBridgeStatus.ps1)
- [Test-UsbBtBridgeHci.ps1](Test-UsbBtBridgeHci.ps1)

This script checks whether Windows can see the board's:

- USB microphone interface
- CDC ACM COM port
- Bluetooth-class interface

Run it in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\Get-UsbBtBridgeStatus.ps1
```

If you only see audio and COM, that is expected for the current firmware.

`Test-UsbBtBridgeHci.ps1` opens the bridge COM port, asserts `DTR/RTS`, sends a raw `HCI Reset`, and checks for a valid controller response.

Run it in PowerShell:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\Test-UsbBtBridgeHci.ps1
```

Or specify the port explicitly:

```powershell
powershell -ExecutionPolicy Bypass -File .\tools\windows\Test-UsbBtBridgeHci.ps1 -ComPort COM12
```

## Recommendation

If your goal is only:

- microphone on Windows
- debug/control COM port on Windows

the current firmware is already the right shape.

If your goal is:

- let Windows use this board as its actual Bluetooth adapter

then the recommended next step is to move from `CDC ACM + H4 bridge` to a native `USB Bluetooth` interface for Windows, or to start a WDK transport-driver project.

## Official references

- Microsoft Bluetooth transport guidance:
  `https://learn.microsoft.com/en-us/windows-hardware/design/component-guidelines/bluetooth`
- Microsoft Bluetooth Serial HCI Bus Driver sample:
  `https://learn.microsoft.com/en-us/samples/microsoft/windows-driver-samples/bluetooth-serial-hci-bus-driver/`
- Microsoft USB CDC `Usbser.sys` guidance:
  `https://learn.microsoft.com/en-us/windows-hardware/drivers/usbcon/usb-driver-installation-based-on-compatible-ids`
