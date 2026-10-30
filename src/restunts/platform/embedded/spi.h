// Copyright 2021-2022 XMOS LIMITED.
// This Software is subject to the terms of the XMOS Public Licence: Version 1.
// Modified 2026-10-30: master-only API using architecture-aware SDK operations.
// Original source: https://github.com/xmos/fwk_io (XMOS Public Licence v1).
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <xclib.h>
#include <xcore/port.h>
#include <xcore/clock.h>
#include <xcore/thread.h>

#define SPI_MASTER_MINIMUM_DELAY 10
#define SPI_IO_SETC_PAD_DELAY(n) (0x7007 | ((n) << 3))
#define SPI_IO_RESOURCE_SETC(res, word) __xcore_resource_setc((res), (word))

static inline void spi_io_port_outpw(resource_t port, uint32_t word, uint32_t bits)
{
    port_out_part_word(port, word, bits);
}

typedef enum {
    spi_master_sample_delay_0 = 0,
    spi_master_sample_delay_1 = 1,
    spi_master_sample_delay_2 = 2,
    spi_master_sample_delay_3 = 3,
    spi_master_sample_delay_4 = 4,
} spi_master_sample_delay_t;

typedef enum {
    spi_master_source_clock_ref = 0,
    spi_master_source_clock_xcore
} spi_master_source_clock_t;

typedef struct {
    xclock_t clock_block;
    port_t cs_port;
    port_t sclk_port;
    port_t mosi_port;
    port_t miso_port;
    uint32_t current_device;
    int delay_before_transfer;
} spi_master_t;

typedef struct {
    spi_master_t *spi_master_ctx;
    spi_master_source_clock_t source_clock;
    int clock_divisor;
    spi_master_sample_delay_t miso_sample_delay;
    uint32_t miso_pad_delay;
    uint32_t miso_initial_trigger_delay;
    uint32_t cs_assert_val;
    uint32_t clock_delay;
    uint32_t clock_bits;
    uint32_t cs_to_clk_delay_ticks;
    uint32_t clk_to_cs_delay_ticks;
    uint32_t cs_to_cs_delay_ticks;
    thread_mode_t thread_mode;
} spi_master_device_t;

void spi_master_init(
        spi_master_t *spi,
        xclock_t clock_block,
        port_t cs_port,
        port_t sclk_port,
        port_t mosi_port,
        port_t miso_port);

void spi_master_device_init(
        spi_master_device_t *dev,
        spi_master_t *spi,
        uint32_t cs_pin,
        int cpol,
        int cpha,
        spi_master_source_clock_t source_clock,
        uint32_t clock_divisor,
        spi_master_sample_delay_t miso_sample_delay,
        uint32_t miso_pad_delay,
        uint32_t cs_to_clk_delay_ticks,
        uint32_t clk_to_cs_delay_ticks,
        uint32_t cs_to_cs_delay_ticks);

void spi_master_start_transaction(spi_master_device_t *dev);
void spi_master_transfer(
        spi_master_device_t *dev,
        uint8_t *data_out,
        uint8_t *data_in,
        size_t len);
void spi_master_end_transaction(spi_master_device_t *dev);
void spi_master_deinit(spi_master_t *spi);

static inline void spi_master_delay_before_next_transfer(
        spi_master_device_t *dev,
        uint32_t delay_ticks)
{
    spi_master_t *spi = dev->spi_master_ctx;

    spi->delay_before_transfer = 1;
    port_clear_trigger_time(spi->cs_port);
    port_out(spi->cs_port, dev->cs_assert_val);
    port_sync(spi->cs_port);

    if (delay_ticks >= SPI_MASTER_MINIMUM_DELAY) {
        port_out_at_time(spi->cs_port, port_get_trigger_time(spi->cs_port) + delay_ticks, dev->cs_assert_val);
    }
}