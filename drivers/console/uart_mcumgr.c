/*
 * Copyright Runtime.io 2018. All rights reserved.
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief A driver for sending and receiving mcumgr packets over UART.
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/mgmt/mcumgr/transport/serial.h>
#include <zephyr/drivers/console/uart_mcumgr.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(uart_mcumgr, CONFIG_MCUMGR_TRANSPORT_LOG_LEVEL);

static const struct device *const uart_mcumgr_dev =
	DEVICE_DT_GET(DT_CHOSEN(zephyr_uart_mcumgr));

/** Callback to execute when a valid fragment has been received. */
static uart_mcumgr_recv_fn *uart_mcumgr_recv_cb;

/** Contains the fragment currently being received. */
static struct uart_mcumgr_rx_buf *uart_mcumgr_cur_buf;

#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
/**
 * Whether the line currently being read should be ignored.  This is true if
 * the line is too long or if there is no buffer available to hold it.
 */
static bool uart_mcumgr_ignoring;
#endif

/** Contains buffers to hold incoming request fragments. */
K_MEM_SLAB_DEFINE_TYPE(uart_mcumgr_slab, struct uart_mcumgr_rx_buf,
		       CONFIG_UART_MCUMGR_RX_BUF_COUNT);

#if !defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
/**
 * Whether reception is paused because no receive buffer is available.  It is
 * resumed when a buffer is freed.
 */
static atomic_t uart_mcumgr_rx_paused;
#endif

#if defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
uint8_t async_buffer[CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUFS]
		    [CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUF_SIZE];
static int async_current;
#endif

static struct uart_mcumgr_rx_buf *uart_mcumgr_alloc_rx_buf(void)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	void *block;
	int rc;

	rc = k_mem_slab_alloc(&uart_mcumgr_slab, &block, K_NO_WAIT);
	if (rc != 0) {
		return NULL;
	}

	rx_buf = block;
	rx_buf->length = 0;
	return rx_buf;
}

void uart_mcumgr_free_rx_buf(struct uart_mcumgr_rx_buf *rx_buf)
{
	void *block;

	block = rx_buf;
	k_mem_slab_free(&uart_mcumgr_slab, block);

#if !defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
	if (atomic_cas(&uart_mcumgr_rx_paused, 1, 0)) {
		uart_irq_rx_enable(uart_mcumgr_dev);
	}
#endif
}

/**
 * Processes a single incoming byte.
 */
static struct uart_mcumgr_rx_buf *uart_mcumgr_rx_byte(uint8_t byte)
{
	struct uart_mcumgr_rx_buf *rx_buf;

#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	if (!uart_mcumgr_ignoring) {
#endif
		if (uart_mcumgr_cur_buf == NULL) {
			uart_mcumgr_cur_buf = uart_mcumgr_alloc_rx_buf();
			if (uart_mcumgr_cur_buf == NULL) {
				LOG_WRN("Insufficient buffers, fragment dropped");
#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
				uart_mcumgr_ignoring = true;
#endif
			}
		}
#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	}
#endif

	rx_buf = uart_mcumgr_cur_buf;
#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	if (!uart_mcumgr_ignoring) {
#endif
		if (rx_buf->length >= sizeof(rx_buf->data)) {
			LOG_WRN("Line too long, fragment dropped");
			uart_mcumgr_free_rx_buf(uart_mcumgr_cur_buf);
			uart_mcumgr_cur_buf = NULL;
#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
			uart_mcumgr_ignoring = true;
#endif
		} else {
			rx_buf->data[rx_buf->length++] = byte;
		}
#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	}
#endif

#if defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	uart_mcumgr_cur_buf = NULL;
	return rx_buf;
#else
	if (byte == '\n') {
		/* Fragment complete. */
		if (uart_mcumgr_ignoring) {
			uart_mcumgr_ignoring = false;
		} else {
			uart_mcumgr_cur_buf = NULL;
			return rx_buf;
		}
	}

	return NULL;
#endif
}

#if defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
static void uart_mcumgr_async(const struct device *dev, struct uart_event *evt, void *user_data)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	uint8_t *p;
	int len;

	ARG_UNUSED(dev);

	switch (evt->type) {
	case UART_TX_DONE:
	case UART_TX_ABORTED:
		break;
	case UART_RX_RDY:
		len = evt->data.rx.len;
		p = &evt->data.rx.buf[evt->data.rx.offset];

		for (int i = 0; i < len; i++) {
			rx_buf = uart_mcumgr_rx_byte(p[i]);
			if (rx_buf != NULL) {
				uart_mcumgr_recv_cb(rx_buf);
			}
		}
		break;
	case UART_RX_DISABLED:
		async_current = 0;
		break;
	case UART_RX_BUF_REQUEST:
		/*
		 * Note that when buffer gets filled, the UART_RX_BUF_RELEASED will be reported,
		 * aside to UART_RX_RDY.  The UART_RX_BUF_RELEASED is not processed because
		 * it has been assumed that the mcumgr will be able to consume bytes faster
		 * than UART will receive them and, since there is nothing to release, only
		 * UART_RX_BUF_REQUEST is processed.
		 */
		++async_current;
		async_current %= CONFIG_MCUMGR_TRANSPORT_UART_ASYNC_BUFS;
		uart_rx_buf_rsp(dev, async_buffer[async_current],
				sizeof(async_buffer[async_current]));
		break;
	case UART_RX_BUF_RELEASED:
	case UART_RX_STOPPED:
		break;
	}
}
#else
/**
 * Makes sure that a receive buffer is available for the next incoming byte.
 */
static bool uart_mcumgr_rx_buf_ready(void)
{
#if !defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	if (uart_mcumgr_ignoring) {
		return true;
	}
#endif

	if (uart_mcumgr_cur_buf == NULL) {
		uart_mcumgr_cur_buf = uart_mcumgr_alloc_rx_buf();
	}

	return uart_mcumgr_cur_buf != NULL;
}

/**
 * Whether the UART throttles the sender while no data is read from it.
 */
static bool uart_mcumgr_flow_ctrl_enabled(void)
{
	struct uart_config cfg;

	if (uart_config_get(uart_mcumgr_dev, &cfg) == 0) {
		return cfg.flow_ctrl != UART_CFG_FLOW_CTRL_NONE;
	}

	/* Runtime configuration is not available, use the devicetree setting */
	return DT_PROP_OR(DT_CHOSEN(zephyr_uart_mcumgr), hw_flow_control, false);
}

/**
 * ISR that is called when UART bytes are received.
 */
static void uart_mcumgr_isr(const struct device *unused, void *user_data)
{
	struct uart_mcumgr_rx_buf *rx_buf;
	uint8_t byte;

	ARG_UNUSED(unused);
	ARG_UNUSED(user_data);

	uart_irq_update(uart_mcumgr_dev);

	if (uart_irq_rx_ready(uart_mcumgr_dev) <= 0) {
		return;
	}

	while (true) {
		if (!uart_mcumgr_rx_buf_ready() && uart_mcumgr_flow_ctrl_enabled()) {
			/* Leave the data in the UART so that the sender gets
			 * throttled instead of losing fragments. Reception is
			 * resumed by uart_mcumgr_free_rx_buf().
			 *
			 * Without flow control, the data would be lost in the
			 * UART anyway, so keep reading and drop the fragment.
			 */
			atomic_set(&uart_mcumgr_rx_paused, 1);
			uart_irq_rx_disable(uart_mcumgr_dev);

			/* A buffer may have been freed before reception was paused */
			if (!uart_mcumgr_rx_buf_ready()) {
				break;
			}

			atomic_clear(&uart_mcumgr_rx_paused);
			uart_irq_rx_enable(uart_mcumgr_dev);
		}

		/* Read byte by byte, as a single read could otherwise span into
		 * a fragment for which no buffer is available.
		 */
		if (uart_fifo_read(uart_mcumgr_dev, &byte, 1) <= 0) {
			break;
		}

		rx_buf = uart_mcumgr_rx_byte(byte);
		if (rx_buf != NULL) {
			uart_mcumgr_recv_cb(rx_buf);
		}
	}
}
#endif

/**
 * Sends raw data over the UART.
 */
static int uart_mcumgr_send_raw(const void *data, int len)
{
	const uint8_t *u8p;

	u8p = data;
	while (len--) {
		uart_poll_out(uart_mcumgr_dev, *u8p++);
	}

	return 0;
}

int uart_mcumgr_send(const uint8_t *data, int len)
{
#if defined(CONFIG_UART_MCUMGR_RAW_PROTOCOL)
	return uart_mcumgr_send_raw(data, len);
#else
	return mcumgr_serial_tx_pkt(data, len, uart_mcumgr_send_raw);
#endif
}

#if defined(CONFIG_MCUMGR_TRANSPORT_UART_ASYNC)
static void uart_mcumgr_setup(const struct device *uart)
{
	uart_callback_set(uart, uart_mcumgr_async, NULL);

	uart_rx_enable(uart, async_buffer[0], sizeof(async_buffer[0]),
		       CONFIG_UART_CONSOLE_MCUMGR_ASYNC_RX_TIMEOUT_US);
}
#else
static void uart_mcumgr_setup(const struct device *uart)
{
	uart_irq_rx_disable(uart);
	uart_irq_tx_disable(uart);

	uart_irq_callback_set(uart, uart_mcumgr_isr);

	uart_irq_rx_enable(uart);
}
#endif

void uart_mcumgr_register(uart_mcumgr_recv_fn *cb)
{
	uart_mcumgr_recv_cb = cb;

	if (device_is_ready(uart_mcumgr_dev)) {
		uart_mcumgr_setup(uart_mcumgr_dev);
	}
}
