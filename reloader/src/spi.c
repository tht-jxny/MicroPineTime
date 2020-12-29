/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (C) 2020 Daniel Thompson
 */

#include "spi.h"

#include <nrf_gpio.h>
#include <nrf_spi.h>

#define SPIx NRF_SPI0
#define SPI_MODE NRF_SPI_MODE_3
#define SPI_SCK 2
#define SPI_COPI 3
#define SPI_CIPO 4

void spi_init(void)
{
  nrf_gpio_pin_write(SPI_SCK, SPI_MODE >= 2);
  nrf_gpio_cfg(SPI_SCK, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_CONNECT,
               NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_pin_clear(SPI_COPI);
  nrf_gpio_cfg_output(SPI_COPI);
  nrf_gpio_cfg_input(SPI_CIPO, NRF_GPIO_PIN_NOPULL);

  nrf_spi_pins_set(SPIx, SPI_SCK, SPI_COPI, SPI_CIPO);
  nrf_spi_frequency_set(SPIx, NRF_SPI_FREQ_8M);
  nrf_spi_configure(SPIx, SPI_MODE, NRF_SPI_BIT_ORDER_MSB_FIRST);

  nrf_spi_enable(SPIx);
}

void spi_teardown(void)
{
  /* no need to tear down SCK and COPI - output pins can be left alone */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
  nrf_spi_disable(SPIx);
  nrf_gpio_cfg_default(SPI_CIPO);
  nrf_gpio_cfg_default(SPI_COPI);
  nrf_gpio_cfg_default(SPI_SCK);
}

void spi_read(uint8_t *data, unsigned len)
{
  uint8_t *endp = data + len - 1;

  /* paranoid... but worthwhile due to the havoc this could cause */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);

  /* send first character */
  nrf_spi_txd_set(SPIx, 0);

  /* TXD is double buffered so we can xmit and then poll for the event */
  while (data < endp) {
    nrf_spi_txd_set(SPIx, 0);

    while (!nrf_spi_event_check(SPIx, NRF_SPI_EVENT_READY)) {}
    nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
    *data++ = nrf_spi_rxd_get(SPIx);
  }

  /* wait for the final character */
  while (!nrf_spi_event_check(SPIx, NRF_SPI_EVENT_READY)) {}
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
  *data++ = nrf_spi_rxd_get(SPIx);
}

void spi_write(const uint8_t *data, unsigned len)
{
  const uint8_t *endp = data + len;

  /* paranoid... but worthwhile due to the havoc this could cause */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);

  /* send first character */
  nrf_spi_txd_set(SPIx, *data++);

  /* TXD is double buffered so we can xmit and then poll for the event */
  while (data < endp) {
    nrf_spi_txd_set(SPIx, *data++);

    while (!nrf_spi_event_check(SPIx, NRF_SPI_EVENT_READY)) {}
    nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
    (void) nrf_spi_rxd_get(SPIx);
  }

  /* wait for the final character */
  while (!nrf_spi_event_check(SPIx, NRF_SPI_EVENT_READY)) {}
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
  (void) nrf_spi_rxd_get(SPIx);
}

