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
    -DEXTRA_CONF_FILE="cdc.conf;fs.conf;shell-mgmt.conf" \
    -DCONFIG_RTC=y \
    -DCONFIG_THREAD_MONITOR=y \
    -DCONFIG_THREAD_RUNTIME_STATS=y \
    -DCONFIG_SCHED_THREAD_USAGE=y \
    -DCONFIG_SHELL_BACKEND_SERIAL=n \
    -DCONFIG_FILE_SYSTEM_SHELL=y \
    -DCONFIG_MCUMGR_TRANSPORT_NETBUF_SIZE=4096 \
    -DCONFIG_MCUMGR_TRANSPORT_WORKQUEUE_STACK_SIZE=8192 \
    -DCONFIG_MCUMGR_GRP_OS_MCUMGR_PARAMS=y \
    -DCONFIG_MCUMGR_GRP_OS_INFO=y \
    -DCONFIG_MCUMGR_GRP_OS_TASKSTAT=y \
    -DCONFIG_MCUMGR_GRP_OS_TASKSTAT_STACK_INFO=y \
    -DCONFIG_MCUMGR_GRP_OS_BOOTLOADER_INFO=y \
    -DCONFIG_MCUMGR_GRP_OS_DATETIME=y \
    -DCONFIG_MCUMGR_GRP_IMG=y \
    -DCONFIG_MCUMGR_GRP_IMG_FRUGAL_LIST=y \
    -DCONFIG_MCUMGR_GRP_IMG_SLOT_INFO=y \
    -DCONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH=y \
    -DCONFIG_MCUMGR_GRP_FS_HASH_SHA256=y \
    -DCONFIG_MCUMGR_GRP_FS_CHECKSUM_HASH_SUPPORTED_CMD=y \
    -DCONFIG_MCUMGR_GRP_ZBASIC=y \
    -DCONFIG_MCUMGR_GRP_ZBASIC_STORAGE_ERASE=y \
    -DCONFIG_LOG=n
