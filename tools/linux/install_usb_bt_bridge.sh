#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
RULE_SRC="${SCRIPT_DIR}/99-usb-bt-bridge.rules"
UNIT_SRC="${SCRIPT_DIR}/usb-bt-bridge@.service"
ENV_SRC="${SCRIPT_DIR}/usb-bt-bridge.env"

RULE_DST="/etc/udev/rules.d/99-usb-bt-bridge.rules"
UNIT_DST="/etc/systemd/system/usb-bt-bridge@.service"
ENV_DST="/etc/default/usb-bt-bridge"

require_root() {
    if [[ "${EUID}" -ne 0 ]]; then
        echo "Please run this script as root."
        exit 1
    fi
}

require_command() {
    if ! command -v "$1" >/dev/null 2>&1; then
        echo "Missing required command: $1"
        exit 1
    fi
}

is_matching_bridge_tty() {
    local tty_path="$1"
    local props

    props="$(udevadm info --query=property --name "${tty_path}" 2>/dev/null || true)"
    [[ -n "${props}" ]] || return 1

    grep -q '^ID_VENDOR_ID=0669$' <<< "${props}" || return 1
    grep -Eq '^ID_MODEL_ID=0225$|^ID_MODEL_ID=0226$' <<< "${props}" || return 1
}

restart_present_bridge_services() {
    local tty_path
    local service_name
    local matched=0

    for tty_path in /dev/ttyACM*; do
        [[ -e "${tty_path}" ]] || continue

        if ! is_matching_bridge_tty "${tty_path}"; then
            continue
        fi

        matched=1
        service_name="usb-bt-bridge@${tty_path#/dev/}.service"
        echo "Restarting ${service_name}"
        systemctl restart "${service_name}"
    done

    if [[ "${matched}" -eq 0 ]]; then
        echo "No matching bridge device is connected right now."
        echo "Plug the board into Linux later and udev will start btattach automatically."
    fi
}

main() {
    require_root
    require_command install
    require_command systemctl
    require_command udevadm
    require_command btattach

    install -d /etc/udev/rules.d /etc/systemd/system /etc/default
    install -m 0644 "${RULE_SRC}" "${RULE_DST}"
    install -m 0644 "${UNIT_SRC}" "${UNIT_DST}"

    if [[ -e "${ENV_DST}" ]]; then
        echo "Keeping existing ${ENV_DST}"
    else
        install -m 0644 "${ENV_SRC}" "${ENV_DST}"
    fi

    systemctl daemon-reload
    udevadm control --reload-rules

    restart_present_bridge_services

    echo
    echo "Install complete."
    echo "Check status with:"
    echo "  systemctl status 'usb-bt-bridge@*'"
    echo "Check Bluetooth devices with:"
    echo "  hciconfig -a"
}

main "$@"
