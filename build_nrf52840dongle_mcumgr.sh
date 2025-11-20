#!/bin/bash

west build -b nrf52840dongle/nrf52840/bare \
    samples/subsys/mgmt/mcumgr/smp_svr \
    --sysbuild \
    -- \
    -DEXTRA_CONF_FILE="cdc.conf;fs.conf" \
    -DEXTRA_DTC_OVERLAY_FILE="usb.overlay" \
    -DCONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE=4096 \
    -DCONFIG_MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE=8192 \
    -DCONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS=y \
    -DCONFIG_MCUMGR_GRP_ZBASIC=y \
    -DCONFIG_MCUMGR_GRP_ZBASIC_STORAGE_ERASE=y
    #  \
    # -DCONFIG_USB_DEVICE_STACK_NEXT=n \
    # -DCONFIG_USB_DEVICE_STACK=y \
    # -DCONFIG_USB_DEVICE_INITIALIZE_AT_BOOT=y \
    # -DCONFIG_USB_CDC_ACM=y
