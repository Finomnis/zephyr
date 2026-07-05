#!/bin/bash

# . ../.venv/bin/activate

# west build -b nrf52840dongle/nrf52840/bare \
#     samples/subsys/mgmt/mcumgr/smp_svr \
#     --sysbuild \
#     -- \
#     -DEXTRA_CONF_FILE="cdc.conf;fs.conf;shell.conf;shell-mgmt.conf" \
#     -DEXTRA_DTC_OVERLAY_FILE="usb.overlay" \
#     -DCONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS=y \
#     -DCONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE=4096 \
#     -DCONFIG_MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE=8192 \
#     -DCONFIG_SHELL_BACKEND_DUMMY_BUF_SIZE=4096 \
#     -DCONFIG_LOG=n

# WORKS:
# west build -b nrf52840dongle/nrf52840/bare \
#     samples/subsys/mgmt/mcumgr/smp_svr \
#     --sysbuild \
#     -- \
#     -DEXTRA_CONF_FILE="cdc.conf;fs.conf;shell.conf;shell-mgmt.conf" \
#     -DEXTRA_DTC_OVERLAY_FILE="usb.overlay" \
#     -DCONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS=y


west build -b nrf52840dongle/nrf52840/bare \
    samples/subsys/mgmt/mcumgr/smp_svr \
    --sysbuild \
    -- \
    -DEXTRA_CONF_FILE="bt.conf;cdc.conf;fs.conf;shell-mgmt.conf;nrf52840-dongle-dev.conf" \
    -DEXTRA_DTC_OVERLAY_FILE="usb.overlay"
