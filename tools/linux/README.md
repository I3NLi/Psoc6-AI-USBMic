# Linux Auto-Attach

This folder installs Linux automation for the USB Audio + Bluetooth bridge firmware.

What it does:

- keeps the microphone on the standard `snd-usb-audio` path
- detects the board's `ttyACM` CDC interface with `udev`
- starts `btattach` through `systemd`
- creates `hci0` automatically when the board is plugged in

The rule matches the board's USB vendor ID `0x0669` and product IDs `0x0225` or `0x0226`.

## Requirements

- Linux with `systemd`
- `BlueZ` installed, including `btattach`
- kernel support for `cdc_acm`, `bluetooth`, and `hci_uart`

## Install

From the project root:

```bash
sudo bash tools/linux/install_usb_bt_bridge.sh
```

If the board is already connected, the script restarts the matching `usb-bt-bridge@ttyACM*.service`.
If the board is not connected yet, plug it in after installation and `udev` will launch the service automatically.

## Verify

```bash
systemctl status 'usb-bt-bridge@*'
hciconfig -a
arecord -l
```

You should see:

- a running `usb-bt-bridge@ttyACM*.service`
- a Bluetooth controller such as `hci0`
- the USB microphone still listed by ALSA

## Configuration

Default `btattach` settings live in:

```bash
/etc/default/usb-bt-bridge
```

Current defaults:

```bash
BT_ATTACH_PROTOCOL=h4
BT_ATTACH_BAUD=115200
```

After editing that file, reload the service:

```bash
sudo systemctl restart 'usb-bt-bridge@*'
```

## Uninstall

```bash
sudo bash tools/linux/uninstall_usb_bt_bridge.sh
```
