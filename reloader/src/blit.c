/*
 * SPDX-License-Identifier: LGPL-3.0-or-later
 * Copyright (C) 2020 Daniel Thompson
 */

#include "blit.h"

#include <stdint.h>
#include <nrf_wdt.h>

#include "board.h"
#include "spinor.h"
#include "util.h"

#ifdef CONFIG_HAVE_SPINOR

// 2-bit RLE, generated from res/pine64_small.png, 767 bytes
static const uint8_t logo[] = {
  0x2, 0xf0, 0xf0, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfc, 0x40, 0x5e, 0x42, 0x3f,
  0xae, 0x44, 0x3f, 0xac, 0x46, 0x3f, 0xaa, 0x48, 0x3f, 0xa8, 0x4a, 0x3f,
  0xa6, 0x4c, 0x3f, 0xa4, 0x4e, 0x3f, 0xa2, 0x50, 0x3f, 0xa0, 0x52, 0x3f,
  0x9e, 0x54, 0x3f, 0x9c, 0x56, 0x3f, 0x9a, 0x58, 0x3f, 0x99, 0x58, 0x3f,
  0x9b, 0x54, 0x3f, 0x91, 0x80, 0x3a, 0x82, 0xc, 0x50, 0xc, 0x82, 0x3f,
  0x84, 0x85, 0xc, 0x4c, 0xc, 0x85, 0x3f, 0x83, 0x87, 0xc, 0x48, 0xc,
  0x87, 0x3f, 0x82, 0x8a, 0xc, 0x44, 0xc, 0x8a, 0x3f, 0x81, 0x8c, 0x18,
  0x8c, 0x3f, 0x80, 0x8f, 0x14, 0x8f, 0x3f, 0x7f, 0x91, 0x10, 0x91, 0x3f,
  0x7f, 0x8f, 0x14, 0x8f, 0x3f, 0x7e, 0x8e, 0x18, 0x8e, 0x3f, 0x7d, 0x8c,
  0xc, 0xc0, 0x34, 0xc4, 0xc, 0x8c, 0x3f, 0x7c, 0x8b, 0xc, 0xc8, 0xc,
  0x8b, 0x3f, 0x7b, 0x89, 0xb, 0xce, 0xb, 0x89, 0x3f, 0x7b, 0x87, 0xb,
  0xd2, 0xb, 0x87, 0x3f, 0x7a, 0x86, 0xa, 0xd8, 0xa, 0x86, 0x3f, 0x79,
  0x83, 0xb, 0xdc, 0xa, 0x84, 0x3f, 0x85, 0xe0, 0xa, 0x82, 0x3f, 0x83,
  0xe4, 0x3f, 0x8a, 0xea, 0x3f, 0x85, 0xee, 0x3f, 0x80, 0xf4, 0x3f, 0x7b,
  0xf8, 0x3f, 0x78, 0xfa, 0x3f, 0x6c, 0x40, 0x33, 0x41, 0xb, 0xf8, 0xa,
  0x42, 0x3f, 0x61, 0x44, 0xa, 0xf4, 0xa, 0x44, 0x3f, 0x61, 0x46, 0xb,
  0xee, 0xb, 0x46, 0x3f, 0x61, 0x49, 0xa, 0xea, 0xa, 0x49, 0x3f, 0x61,
  0x4b, 0xb, 0xe4, 0xb, 0x4b, 0x3f, 0x61, 0x4d, 0xb, 0xe0, 0xb, 0x4d,
  0x3f, 0x61, 0x4f, 0xb, 0xdc, 0xb, 0x4f, 0x3f, 0x61, 0x52, 0xa, 0xd8,
  0xa, 0x52, 0x3f, 0x61, 0x54, 0xb, 0xd2, 0xb, 0x54, 0x3f, 0x61, 0x56,
  0xb, 0xce, 0xb, 0x56, 0x3f, 0x61, 0x58, 0xc, 0xc8, 0xc, 0x58, 0x3f,
  0x61, 0x5b, 0xb, 0xc4, 0xb, 0x5b, 0x3f, 0x61, 0x5d, 0x16, 0x5d, 0x3f,
  0x61, 0x60, 0x10, 0x60, 0x3f, 0x61, 0x60, 0x10, 0x60, 0x3f, 0x61, 0x5f,
  0x12, 0x5f, 0x3f, 0x61, 0x5c, 0x17, 0x5d, 0x3f, 0x61, 0x5a, 0xc, 0x44,
  0xb, 0x5b, 0x3f, 0x61, 0x58, 0xc, 0x48, 0xc, 0x58, 0x3f, 0x61, 0x56,
  0xb, 0x4f, 0xa, 0x56, 0x3f, 0x61, 0x53, 0xc, 0x52, 0xc, 0x53, 0x3f,
  0x61, 0x51, 0xc, 0x56, 0xb, 0x52, 0x3f, 0x61, 0x4f, 0xc, 0x5a, 0xc,
  0x4f, 0x3f, 0x61, 0x4d, 0xb, 0x60, 0xb, 0x4d, 0x3f, 0x61, 0x4a, 0xc,
  0x64, 0xc, 0x4a, 0x3f, 0x61, 0x48, 0xb, 0x6a, 0xb, 0x48, 0x3f, 0x61,
  0x46, 0xb, 0x6e, 0xb, 0x46, 0x3f, 0x61, 0x44, 0xb, 0x72, 0xb, 0x44,
  0x3f, 0x61, 0x41, 0xc, 0x76, 0xc, 0x41, 0x3f, 0x6b, 0x7c, 0x3f, 0x73,
  0x7f, 0x1, 0x3f, 0x61, 0x80, 0x32, 0x82, 0xd, 0x7f, 0x3, 0xc, 0x82,
  0x3f, 0x53, 0x83, 0xd, 0x7e, 0xd, 0x83, 0x3f, 0x54, 0x85, 0xc, 0x7a,
  0xc, 0x85, 0x3f, 0x55, 0x87, 0xc, 0x76, 0xc, 0x87, 0x3f, 0x56, 0x89,
  0xb, 0x71, 0xc, 0x89, 0x3f, 0x57, 0x8b, 0xc, 0x6c, 0xc, 0x8b, 0x3f,
  0x57, 0x8e, 0xb, 0x68, 0xb, 0x8d, 0x3f, 0x59, 0x8f, 0xc, 0x62, 0xc,
  0x8f, 0x3f, 0x59, 0x92, 0xb, 0x5e, 0xb, 0x92, 0x3f, 0x5a, 0x93, 0xb,
  0x59, 0xc, 0x93, 0x3f, 0x5b, 0x95, 0xb, 0x55, 0xc, 0x95, 0x3f, 0x5c,
  0x97, 0xb, 0x50, 0xb, 0x97, 0x3f, 0x5d, 0x99, 0xb, 0x4c, 0xb, 0x99,
  0x3f, 0x5d, 0x9b, 0xc, 0x46, 0xc, 0x9a, 0x3f, 0x5f, 0x9d, 0xb, 0x42,
  0xb, 0x9d, 0x3f, 0x5f, 0x9f, 0x14, 0x9f, 0x3f, 0x60, 0xa1, 0xf, 0xa0,
  0x3f, 0x61, 0xa0, 0x10, 0xa0, 0x3f, 0x62, 0x9e, 0x12, 0x9e, 0x3f, 0x63,
  0x9b, 0xb, 0x82, 0xb, 0x9b, 0x3f, 0x64, 0x98, 0xa, 0x87, 0xb, 0x98,
  0x3f, 0x65, 0x95, 0xc, 0x8a, 0xb, 0x96, 0x3f, 0x65, 0x94, 0xa, 0x90,
  0xa, 0x94, 0x3f, 0x66, 0x90, 0xb, 0x94, 0xb, 0x90, 0x3f, 0x67, 0x8e,
  0xa, 0x9a, 0xa, 0x8e, 0x3f, 0x68, 0x8a, 0xb, 0x9e, 0xb, 0x8a, 0x3f,
  0x69, 0x88, 0xb, 0xa2, 0xb, 0x88, 0x3f, 0x6a, 0x85, 0xb, 0xa6, 0xb,
  0x85, 0x3f, 0x6b, 0x83, 0xa, 0xac, 0xa, 0x83, 0x3f, 0x76, 0xb0, 0x3f,
  0x7f, 0xb4, 0x3f, 0x7b, 0xb8, 0x3f, 0x78, 0xba, 0x3f, 0x79, 0xb6, 0x3f,
  0x7d, 0xb2, 0x3f, 0x81, 0xae, 0x3f, 0x85, 0xaa, 0x3f, 0x8a, 0xa4, 0x3f,
  0x8f, 0xa0, 0x3f, 0x93, 0x9c, 0x3f, 0x97, 0x98, 0x3f, 0x9c, 0x92, 0x3f,
  0xa1, 0x8e, 0x3f, 0xa5, 0x8a, 0x3f, 0xa9, 0x86, 0x3f, 0xab, 0x86, 0x3f,
  0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f,
  0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f,
  0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f, 0xab, 0x86, 0x3f,
  0xab, 0x86, 0x3f, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
  0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xbc,
};

static uint16_t clut8_rgb565(uint8_t i)
{
    uint16_t rgb565;
    uint16_t rg, gr6, gr5;

    if (i < 216) {
        rgb565  = (( i  % 6) * 0x33) >> 3;
        rg = i / 6;
        rgb565 += ((rg  % 6) * (0x33 << 3)) & 0x07e0;
        rgb565 += ((rg / 6) * (0x33 << 8)) & 0xf800;
    } else if (i < 252) {
        i -= 216;
        rgb565  = (0x7f + (( i  % 3) * 0x33)) >> 3;
        rg = i / 3;
        rgb565 += ((0x4c << 3) + ((rg  % 4) * (0x33 << 3))) & 0x07e0;
        rgb565 += ((0x7f << 8) + ((rg / 4) * (0x33 << 8))) & 0xf800;
    } else {
        i -= 252;
        gr6 = (0x2c + (0x10 * i)) >> 2;
        gr5 = gr6 >> 1;
        rgb565 = (gr5 << 11) + (gr6 << 5) + gr5;
    }

    return rgb565;
}

static void blit_rle2bit(const uint8_t *image, unsigned int imagelen,
	                 uint16_t *buf, unsigned int buflen,
	                 void (*write)(void *ctx, uint16_t *buf,
				       unsigned int buflen),
	                 void *ctx)
{
    uint16_t palette[4] = { 0, 0x4a69, 0x7bef, 0xffff };
    int next_color = 1;
    int rl = 0;
    int bp = 0;
    int px;

    for (int i=3; i<imagelen; i++) {
	int op = image[i];

	if (rl == 0) {
	    px = op >> 6;
	    rl = op & 0x3f;
	    if (0 == rl) {
		// queue palette change
		rl = -1;
		continue;
	    }
	    if (rl >= 63)
		continue;
	} else if (rl > 0) {
            rl += op;
            if (op >= 255)
                continue;
	} else {
	    // perform the palette change
            palette[next_color] = clut8_rgb565(op);
	    next_color = (next_color < 3 ? next_color + 1 : 1);
            rl = 0;
            continue;
	}

        while (rl) {
            int count = rl < (buflen - bp) ? rl : buflen - bp;
	    uint16_t color = (palette[px] >> 8) | (palette[px] << 8);
	    for (int j=0; j<count; j++)
		buf[bp+j] = color;

            bp += count;
            rl -= count;

	    if (bp >= buflen) {
		write(ctx, buf, bp);
                bp = 0;
	    }
	}
    }

    if (bp)
	write(ctx, buf, bp);
}

static void blit_write(void *ctx, uint16_t *buf, unsigned int buflen)
{
    int *addr = ctx;

    nrf_wdt_reload_request_set(NRF_WDT, 0);

    /* launch a sector erase every time we cross a sector boundary */
    if ((*addr & 4095) == 0)
	spinor_sector_erase(*addr);

    /* write this page */
    spinor_burst_write(*addr, (uint8_t *) buf, buflen*2);

    /* update the offset */
    *addr += buflen*2;

    /* update the display */
    report_progress(100 * *addr / 115200);
}

void blit_write_logo(void)
{
    int addr = 0;
    uint16_t buf[128];

    blit_rle2bit(logo, sizeof(logo), buf, sizeof(buf)/2, blit_write, &addr);
}

#endif // CONFIG_HAVE_SPINOR
