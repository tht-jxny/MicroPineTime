/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (C) 2020 Daniel Thompson
 */

#include "st7789.h"

#include <string.h>

#include <nrf_gpio.h>
#include <nrf_spi.h>

#define SPIx NRF_SPI0
#define SPI_MODE NRF_SPI_MODE_3
#define SPI_SCK 2
#define SPI_MOSI 3
#define DISP_SS 25
#define DISP_DC 18
#define DISP_RESET 26
#define BACKLIGHT 14 /* lowest level */

static void spi_init(void)
{
  nrf_gpio_pin_write(SPI_SCK, SPI_MODE >= 2);
  nrf_gpio_cfg(SPI_SCK, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_CONNECT,
               NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_pin_clear(SPI_MOSI);
  nrf_gpio_cfg_output(SPI_MOSI);


  nrf_spi_pins_set(SPIx, SPI_SCK, SPI_MOSI, NRF_SPI_PIN_NOT_CONNECTED);
  nrf_spi_frequency_set(SPIx, NRF_SPI_FREQ_8M);
  nrf_spi_configure(SPIx, SPI_MODE, NRF_SPI_BIT_ORDER_MSB_FIRST);

  nrf_spi_enable(SPIx);
}

static void spi_teardown(void)
{
  /* no need to tear down SCK and MOSI - output pins can be left alone */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
  nrf_spi_disable(SPIx);
  nrf_gpio_cfg_default(SPI_MOSI);
  nrf_gpio_cfg_default(SPI_SCK);
}

static void spi_write(const uint8_t *data, unsigned len)
{
  const uint8_t *endp = data + len;

  /* paranoid... but worthwhile due to the havoc this could cause */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);

  /* send first character */
  nrf_spi_txd_set(SPIx, *data++);

  /* TXD is double buffers so we can xmit and then poll for the event */
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

#define NOP      0x00
#define SWRESET  0x01
#define SLPOUT   0x11
#define NORON    0x13
#define INVOFF   0x20
#define INVON    0x21
#define DISPON   0x29
#define CASET    0x2a
#define RASET    0x2b
#define RAMWR    0x2c
#define COLMOD   0x3a
#define MADCTL   0x36

struct st7789_cmd {
  uint8_t cmd;
  const uint8_t *data;
  uint8_t len;
};

const static struct st7789_cmd st7789_init_data[] = {
  { COLMOD,   (uint8_t *) "\x05", 1 }, // MCU will send 16-bit RGB565
  { MADCTL,   (uint8_t *) "\x00", 1 }, // Left to right, top to bottom
  //{ INVOFF,   NULL }, // Results in odd palette
  { INVON,    NULL },
  { NORON,    NULL },
  { NOP,      NULL },
};

static void st7789_send(uint8_t cmd, const uint8_t *data, unsigned len)
{
  nrf_gpio_pin_clear(DISP_SS);

  // this means nop cannot be sent... but helps with big RAMWR dumps
  if (cmd) {
    nrf_gpio_pin_clear(DISP_DC);
    spi_write(&cmd, 1);
  }

  if (data) {
    nrf_gpio_pin_set(DISP_DC);
    spi_write(data, len);
  }

  nrf_gpio_pin_set(DISP_SS);
}

void st7789_state(int percent_complete)
{
  bool topclip = true;
  bool bottomclip = true;

  uint8_t linebuffer[2*240];

  // 1-bit RLE, generated from res/pinedfu.png, 1085 bytes
  const uint8_t rle[] = {
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0x14, 0x8, 0x8, 0x3,
    0x2b, 0x3, 0xac, 0xd, 0x6, 0x3, 0x2b, 0x3, 0xab, 0xe, 0x6, 0x3,
    0x2b, 0x3, 0xab, 0x4, 0x8, 0x2, 0x6, 0x3, 0x2b, 0x3, 0xaa, 0x4,
    0x11, 0x3, 0x2b, 0x3, 0xaa, 0x3, 0x10, 0xb, 0x6, 0x7, 0xb, 0x3,
    0x3, 0x4, 0x1, 0xb, 0xa4, 0x3, 0x10, 0xb, 0x4, 0xb, 0x9, 0x3,
    0x1, 0x6, 0x1, 0xb, 0xa4, 0x3, 0x10, 0xb, 0x4, 0xc, 0x8, 0xa,
    0x1, 0xb, 0xa4, 0x4, 0x11, 0x3, 0xa, 0x2, 0x7, 0x4, 0x7, 0x6,
    0x7, 0x3, 0xab, 0x5, 0xf, 0x3, 0x14, 0x3, 0x7, 0x4, 0x9, 0x3,
    0xac, 0x9, 0xa, 0x3, 0x15, 0x3, 0x6, 0x4, 0x9, 0x3, 0xad, 0xa,
    0x8, 0x3, 0x15, 0x3, 0x6, 0x3, 0xa, 0x3, 0xb0, 0x8, 0x7, 0x3,
    0xd, 0xb, 0x6, 0x3, 0xa, 0x3, 0xb4, 0x5, 0x6, 0x3, 0xb, 0xd,
    0x6, 0x3, 0xa, 0x3, 0xb6, 0x4, 0x5, 0x3, 0xa, 0xe, 0x6, 0x3,
    0xa, 0x3, 0xb7, 0x3, 0x5, 0x3, 0x9, 0x5, 0x7, 0x3, 0x6, 0x3,
    0xa, 0x3, 0xb7, 0x3, 0x5, 0x3, 0x9, 0x3, 0x9, 0x3, 0x6, 0x3,
    0xa, 0x3, 0xb7, 0x3, 0x5, 0x3, 0x9, 0x3, 0x9, 0x3, 0x6, 0x3,
    0xa, 0x3, 0xb6, 0x4, 0x5, 0x3, 0x9, 0x3, 0x8, 0x4, 0x6, 0x3,
    0xa, 0x3, 0xaa, 0x2, 0x9, 0x4, 0x6, 0x4, 0x8, 0x4, 0x5, 0x6,
    0x6, 0x3, 0xa, 0x4, 0xa9, 0xf, 0x7, 0x8, 0x4, 0xe, 0x6, 0x3,
    0xb, 0x8, 0xa4, 0xe, 0x8, 0x8, 0x5, 0x9, 0x1, 0x3, 0x6, 0x3,
    0xb, 0x8, 0xa6, 0x9, 0xd, 0x6, 0x6, 0x6, 0x3, 0x3, 0x6, 0x3,
    0xd, 0x6, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0x2c, 0x2, 0xed, 0x4,
    0xa6, 0x1, 0x44, 0x6, 0xa4, 0x3, 0x42, 0x8, 0xa2, 0x5, 0x40, 0xa,
    0xa0, 0x7, 0x3e, 0xc, 0x9e, 0x7, 0x3e, 0xe, 0x93, 0x1, 0x8, 0x7,
    0x3e, 0x10, 0x92, 0x2, 0x6, 0x7, 0x3e, 0x12, 0x91, 0x3, 0x4, 0x7,
    0x3e, 0x14, 0x90, 0x4, 0x2, 0x7, 0x3e, 0x16, 0x8f, 0xc, 0x3e, 0x18,
    0x8e, 0xb, 0x3f, 0x18, 0x8e, 0xa, 0x42, 0x14, 0x90, 0x9, 0x37, 0x2,
    0xc, 0x10, 0xc, 0x2, 0x84, 0x9, 0x36, 0x5, 0xc, 0xc, 0xc, 0x5,
    0x83, 0xa, 0x35, 0x7, 0xc, 0x8, 0xc, 0x7, 0x83, 0xb, 0x33, 0xa,
    0xc, 0x4, 0xc, 0xa, 0x82, 0xc, 0x32, 0xc, 0x18, 0xc, 0x82, 0xd,
    0x30, 0xf, 0x14, 0xf, 0xbe, 0x11, 0x10, 0x11, 0xbe, 0xf, 0x14, 0xf,
    0xbd, 0xe, 0x18, 0xe, 0xbc, 0xc, 0xc, 0x4, 0xc, 0xc, 0xbb, 0xb,
    0xc, 0x8, 0xc, 0xb, 0xba, 0x9, 0xb, 0xe, 0xb, 0x9, 0xba, 0x7,
    0xb, 0x12, 0xb, 0x7, 0xb9, 0x6, 0xa, 0x18, 0xa, 0x6, 0xb8, 0x3,
    0xb, 0x1c, 0xa, 0x4, 0xc4, 0x20, 0xa, 0x2, 0xc2, 0x24, 0xc9, 0x2a,
    0xc4, 0x2e, 0xbf, 0x34, 0xba, 0x38, 0xb7, 0x3a, 0xab, 0x1, 0xb, 0x38,
    0xa, 0x2, 0xa0, 0x4, 0xa, 0x34, 0xa, 0x4, 0xa0, 0x6, 0xb, 0x2e,
    0xb, 0x6, 0xa0, 0x9, 0xa, 0x2a, 0xa, 0x9, 0xa0, 0xb, 0xb, 0x24,
    0xb, 0xb, 0xa0, 0xd, 0xb, 0x20, 0xb, 0xd, 0xa0, 0xf, 0xb, 0x1c,
    0xb, 0xf, 0xa0, 0x12, 0xa, 0x18, 0xa, 0x12, 0xa0, 0x14, 0xb, 0x12,
    0xb, 0x14, 0xa0, 0x16, 0xb, 0xe, 0xb, 0x16, 0xa0, 0x18, 0xc, 0x8,
    0xc, 0x18, 0xa0, 0x1b, 0xb, 0x4, 0xb, 0x1b, 0xa0, 0x1d, 0x16, 0x1d,
    0xa0, 0x20, 0x10, 0x20, 0xa0, 0x20, 0x10, 0x20, 0xa0, 0x1f, 0x12, 0x1f,
    0xa0, 0x1c, 0x17, 0x1d, 0xa0, 0x1a, 0xc, 0x4, 0xb, 0x1b, 0xa0, 0x18,
    0xc, 0x8, 0xc, 0x18, 0xa0, 0x16, 0xb, 0xf, 0xa, 0x16, 0xa0, 0x13,
    0xc, 0x12, 0xc, 0x13, 0xa0, 0x11, 0xc, 0x16, 0xb, 0x12, 0xa0, 0xf,
    0xc, 0x1a, 0xc, 0xf, 0xa0, 0xd, 0xb, 0x20, 0xb, 0xd, 0xa0, 0xa,
    0xc, 0x24, 0xc, 0xa, 0xa0, 0x8, 0xb, 0x2a, 0xb, 0x8, 0xa0, 0x6,
    0xb, 0x2e, 0xb, 0x6, 0xa0, 0x4, 0xb, 0x32, 0xb, 0x4, 0xa0, 0x1,
    0xc, 0x36, 0xc, 0x1, 0xaa, 0x3c, 0xb2, 0x40, 0xa0, 0x2, 0xd, 0x42,
    0xc, 0x2, 0x92, 0x3, 0xd, 0x3e, 0xd, 0x3, 0x93, 0x5, 0xc, 0x3a,
    0xc, 0x5, 0x94, 0x7, 0xc, 0x36, 0xc, 0x7, 0x95, 0x9, 0xb, 0x31,
    0xc, 0x9, 0x96, 0xb, 0xc, 0x2c, 0xc, 0xb, 0x96, 0xe, 0xb, 0x28,
    0xb, 0xd, 0x98, 0xf, 0xc, 0x22, 0xc, 0xf, 0x98, 0x12, 0xb, 0x1e,
    0xb, 0x12, 0x99, 0x13, 0xb, 0x19, 0xc, 0x13, 0x9a, 0x15, 0xb, 0x15,
    0xc, 0x15, 0x9b, 0x17, 0xb, 0x10, 0xb, 0x17, 0x9c, 0x19, 0xb, 0xc,
    0xb, 0x19, 0x9c, 0x1b, 0xc, 0x6, 0xc, 0x1a, 0x9e, 0x1d, 0xb, 0x2,
    0xb, 0x1d, 0x9e, 0x1f, 0x14, 0x1f, 0x9f, 0x21, 0xf, 0x20, 0xa0, 0x20,
    0x10, 0x20, 0xa1, 0x1e, 0x12, 0x1e, 0xa2, 0x1b, 0xb, 0x2, 0xb, 0x1b,
    0xa3, 0x18, 0xa, 0x7, 0xb, 0x18, 0xa4, 0x15, 0xc, 0xa, 0xb, 0x16,
    0xa4, 0x14, 0xa, 0x10, 0xa, 0x14, 0xa5, 0x10, 0xb, 0x14, 0xb, 0x10,
    0xa6, 0xe, 0xa, 0x1a, 0xa, 0xe, 0xa7, 0xa, 0xb, 0x1e, 0xb, 0xa,
    0xa8, 0x8, 0xb, 0x22, 0xb, 0x8, 0xa9, 0x5, 0xb, 0x26, 0xb, 0x5,
    0xaa, 0x3, 0xa, 0x2c, 0xa, 0x3, 0xb5, 0x30, 0xbe, 0x34, 0xba, 0x38,
    0xb7, 0x3a, 0xb8, 0x36, 0xbc, 0x32, 0xc0, 0x2e, 0xc4, 0x2a, 0xc9, 0x24,
    0xce, 0x20, 0xd2, 0x1c, 0xd6, 0x18, 0xdb, 0x12, 0xe0, 0xe, 0xe4, 0xa,
    0xe8, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6,
    0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6,
    0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0x52, 0x6, 0xe7, 0xc, 0xe2, 0x10,
    0xde, 0x14, 0xdb, 0x16, 0xd9, 0x18, 0xd7, 0x1a, 0xd5, 0x1c, 0xd4, 0xb,
    0x3, 0xe, 0xd3, 0xc, 0x4, 0xe, 0xd2, 0xc, 0x6, 0xc, 0xd1, 0xd,
    0x7, 0xc, 0xd0, 0xd, 0x8, 0xb, 0xd0, 0xd, 0x9, 0xa, 0xd0, 0xd,
    0x4, 0x1, 0x6, 0x8, 0xd0, 0xd, 0x4, 0x2, 0x6, 0x7, 0xd0, 0xd,
    0x4, 0x3, 0x6, 0x6, 0xd0, 0x6, 0x3, 0x4, 0x4, 0x4, 0x5, 0x6,
    0xd0, 0x6, 0x4, 0x3, 0x4, 0x3, 0x6, 0x6, 0xd0, 0x6, 0x6, 0x1,
    0x4, 0x2, 0x6, 0x7, 0xd0, 0x7, 0xa, 0x1, 0x6, 0x8, 0xd0, 0x8,
    0xf, 0x9, 0xd0, 0xa, 0xb, 0xb, 0xd0, 0xb, 0x9, 0xc, 0xd0, 0xc,
    0x7, 0xd, 0xd0, 0xc, 0x7, 0xd, 0xd0, 0xb, 0x9, 0xc, 0xd0, 0xa,
    0xb, 0xb, 0xd0, 0x8, 0xf, 0x9, 0xd0, 0x7, 0xa, 0x1, 0x6, 0x8,
    0xd0, 0x6, 0x6, 0x1, 0x4, 0x2, 0x6, 0x7, 0xd0, 0x6, 0x4, 0x3,
    0x4, 0x3, 0x6, 0x6, 0xd0, 0x6, 0x3, 0x4, 0x4, 0x4, 0x5, 0x6,
    0xd0, 0xd, 0x4, 0x3, 0x6, 0x6, 0xd0, 0xd, 0x4, 0x2, 0x6, 0x7,
    0xd0, 0xd, 0x4, 0x1, 0x6, 0x8, 0xd0, 0xd, 0x9, 0xa, 0xd0, 0xd,
    0x8, 0xb, 0xd0, 0xd, 0x7, 0xc, 0xd1, 0xc, 0x6, 0xc, 0xd2, 0xc,
    0x4, 0xe, 0xd3, 0xb, 0x3, 0xe, 0xd4, 0x1c, 0xd5, 0x1a, 0xd7, 0x18,
    0xd9, 0x16, 0xdb, 0x14, 0xde, 0x10, 0xe2, 0xc, 0xe7, 0x6, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0x85,
  };

  // draw 240 x 240
  st7789_send(CASET, (uint8_t *) "\x00\x00\x00\xef", 4);
  st7789_send(RASET, (uint8_t *) "\x00\x00\x00\xef", 4);
  st7789_send(RAMWR, NULL, 0);

  int y = 0;
  uint16_t bp = 0;
  uint16_t fg = percent_complete < 100 ? 0x033f : 0xffff;
  const uint16_t bg = 0;
  uint16_t color = bg;

  for (int i=0; i<sizeof(rle); i++) {
    uint8_t rl = rle[i];
    while (rl) {
      linebuffer[bp] = color >> 8;
      linebuffer[bp+1] = color & 0xff;
      bp += 2;
      rl -= 1;

      if (bp >= sizeof(linebuffer)) {
	/* currently we have a single image which we clip to show different modes */
	if (topclip) {
	  if (y < 48)
	    memset(linebuffer, 0, sizeof(linebuffer));
	  else
	    memset(linebuffer, 0, 64*2);
	}
	if (bottomclip) {
	  memset(linebuffer+2*(240-64), 0, 64*2);
	}
	if (y >= (165-percent_complete)) {
	  fg = 0xffff;
	}

        st7789_send(NOP, linebuffer, sizeof(linebuffer));
        bp = 0;
	y += 1;
      }
    }

    if (color == bg)
      color = fg;
    else
      color = bg;
  }
}

void st7789_init(void)
{
  nrf_gpio_pin_set(DISP_SS);
  nrf_gpio_cfg_output(DISP_SS);
  spi_init();

  nrf_gpio_cfg_output(DISP_DC);

  /* deliver a reset */
  nrf_gpio_pin_clear(DISP_RESET);
  nrf_gpio_cfg_output(DISP_RESET);
  NRFX_DELAY_US(10*1000);
  nrf_gpio_pin_set(DISP_RESET);
  NRFX_DELAY_US(125*1000);

  /* initialize the display */
  st7789_send(SLPOUT, NULL, 0);
  NRFX_DELAY_US(10*1000);
  for (const struct st7789_cmd *i = st7789_init_data; i->cmd != NOP; i++)
    st7789_send(i->cmd, i->data, i->len);

  /* draw the initial screen content. this will take ~116ms so, with the
   * delay after the SLPOUT we will have complete the SLPOUT/SLPIN lock
   * out period during the update.
   */
  st7789_state(STATE_BOOTLOADER_STARTED);


  /* enable the display and light the backlight */
  st7789_send(DISPON, NULL, 0);
  nrf_gpio_pin_clear(BACKLIGHT);
  nrf_gpio_cfg_output(BACKLIGHT);
}

void st7789_teardown(void)
{
  nrf_gpio_cfg_default(DISP_DC);
  nrf_gpio_cfg_default(DISP_RESET);
  nrf_gpio_cfg_default(BACKLIGHT);

  spi_teardown();
  nrf_gpio_cfg_default(DISP_SS);
}
