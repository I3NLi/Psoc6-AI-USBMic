#!/usr/bin/env bash

set -euo pipefail

RULE_DST="/etc/udev/rules.d/99-usb-bt-bridge.rules"
UNIT_DST="/etc/systemd/system/usb-bt-bridge@.service"
ENV_DST="/etc/default/usb-bt-bridge"

require_root() {
    if [[ "${EUID}" -ne 0 ]]; then
        echo "Please run this script as root."
        exit 1
    fi
}

stop_services() {
    local service_name

    while read -r service_name; do
        [[ -n "${service_name}" ]] || continue
        systemctl stop "${service_name}" || true
    done < <(systemctl list-units --full --all --no-legend 'usb-bt-bridge@*.service' | awk '{print $1}')
}

main() {
    require_root

    stop_services
    rm -f "${RULE_DST}" "${UNIT_DST}" "${ENV_DST}"

    systemctl daemon-reload
    udevadm control --reload-rules

    echo "USB Bluetooth bridge automation removed."
}

main "$@"
