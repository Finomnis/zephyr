#!/bin/bash

# . ../.venv/bin/activate

west build -b nrf52840dongle/nrf52840/bare \
    samples/subsys/mgmt/mcumgr/smp_svr \
    --sysbuild \
    -- \
    -DEXTRA_CONF_FILE="bt.conf;cdc.conf;fs.conf;shell-mgmt.conf;nrf52840-dongle-dev.conf" \
    -DEXTRA_DTC_OVERLAY_FILE="usb.overlay"
