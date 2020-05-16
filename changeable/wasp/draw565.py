# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson

import array
import fonts.sans24
import fonts.gotham24
import micropython

@micropython.viper
def _bitblit(bitbuf, pixels, bgfg: int, count: int):
    mv = ptr16(bitbuf)
    px = ptr8(pixels)

    # Extract and byte-swap
    bg = ((bgfg >> 24) & 0xff) + ((bgfg >> 8) & 0xff00)
    fg = ((bgfg >>  8) & 0xff) + ((bgfg & 0xff) << 8)

    bitselect = 0x80
    pxp = 0
    mvp = 0

    for bit in range(count):
        # Draw the pixel
        active = px[pxp] & bitselect
        mv[mvp] = fg if active else bg
        mvp += 1

        # Advance to the next bit
        bitselect >>= 1
        if not bitselect:
            bitselect = 0x80
            pxp += 1

@micropython.viper
def _expand_rgb(eightbit: int) -> int:
    r = eightbit >> 5
    r = (r << 2) | (r >> 1)
    g = (eightbit >> 2) & 7
    g *= 9
    b = eightbit & 3
    b *= 10

    return (r << 11) | (g << 5) | b

@micropython.viper
def _fill(mv, color: int, count: int, offset: int):
    p = ptr16(mv)
    color = (color >> 8) + ((color & 0xff) << 8)

    for x in range(offset, offset+count):
        p[x] = color

def _bounding_box(s, font):
    w = 0
    for ch in s:
        (_, h, wc) = font.get_ch(ch)
        w += wc + 1

    return (w, h)

@micropython.native
def _draw_glyph(display, glyph, x, y, bgfg):
    (px, h, w) = glyph

    buf = memoryview(display.linebuffer)[0:2*(w+1)]
    buf[2*w] = 0
    buf[2*w + 1] = 0
    bytes_per_row = (w + 7) // 8

    display.set_window(x, y, w+1, h)
    for row in range(h):
        _bitblit(buf, px[row*bytes_per_row:], bgfg, w)
        display.write_data(buf)

class Draw565(object):
    """Drawing library for RGB565 displays.

    A full framebufer is not required although the library will
    'borrow' a line buffer from the underlying display driver.
    """

    def __init__(self, display):
        """Initialise the library.

        Defaults to white-on-black for monochrome drawing operations
        and 24 pt Gotham text.
        """
        self._display = display
        self.reset()

    def reset(self):
        """Restore the default colour and font."""
        self.set_color(0xffff)
        self.set_font(fonts.gotham24)

    def fill(self, bg=None, x=0, y=0, w=None, h=None):
        """Draw a solid color rectangle.

        If no arguments a provided the whole display will be filled with
        the background color.
        """
        if bg is None:
            bg = self._bgfg >> 16
        self._display.fill(bg, x, y, w, h)

    @micropython.native
    def blit(self, image, x, y):
        if len(image) == 3:
            # Legacy 1-bit image
            self.rleblit(image, (x, y))
        else: #elif image[0] == 2:
            # 2-bit RLE image, (255x255, v1)
            self._rle2bit(image, x, y)

    @micropython.native
    def rleblit(self, image, pos=(0, 0), fg=0xffff, bg=0):
        """Decode and draw a 1-bit RLE image."""
        display = self._display
        write_data = display.write_data
        (sx, sy, rle) = image

        display.set_window(pos[0], pos[1], sx, sy)

        buf = memoryview(display.linebuffer)[0:2*sx]
        bp = 0
        color = bg

        for rl in rle:
            while rl:
                count = min(sx - bp, rl)
                _fill(buf, color, count, bp)
                bp += count
                rl -= count

                if bp >= sx:
                    write_data(buf)
                    bp = 0

            if color == bg:
                color = fg
            else:
                color = bg

    @micropython.native
    def _rle2bit(self, image, x, y):
        """Decode and draw a 2-bit RLE image."""
        display = self._display
        quick_write = display.quick_write
        sx = image[1]
        sy = image[2]
        rle = memoryview(image)[3:]

        display.set_window(x, y, sx, sy)

        if sx <= (len(display.linebuffer) / 2) and not bool(sy & 1):
            sx *= 2
            sy /= 2

        palette = array.array('H', (0, 0xfffe, 0x7bef, 0xffff))
        next_color = 1
        rl = 0
        buf = memoryview(display.linebuffer)[0:2*sx]
        bp = 0

        display.quick_start()
        for op in rle:
            if rl == 0:
                px = op >> 6
                rl = op & 0x3f
                if 0 == rl:
                    rl = -1
                    continue
                if rl >= 63:
                    continue
            elif rl > 0:
                rl += op
                if op >= 255:
                    continue
            else:
                palette[next_color] = _expand_rgb(op)
                if next_color < 3:
                    next_color += 1
                else:
                    next_color = 1
                rl = 0
                continue

            while rl:
                count = min(sx - bp, rl)
                _fill(buf, palette[px], count, bp)
                bp += count
                rl -= count

                if bp >= sx:
                    quick_write(buf)
                    bp = 0
        display.quick_end()

    def set_color(self, color, bg=0):
        """Set the foreground (color) and background (bg) color.

        The supplied color will be used for all monochrome drawing operations.
        If no background color is provided then the background will be set
        to black.
        """
        self._bgfg = (bg << 16) + color

    def set_font(self, font):
        """Set the font used for rendering text."""
        self._font = font

    def string(self, s, x, y, width=None):
        """Draw a string at the supplied position.

        If no width is provided then the text will be left justified,
        otherwise the text will be centered within the provided width and,
        importantly, the remaining width will be filled with the background
        color (to ensure that if we update one string with a narrower one
        there is no need to "undraw" it)
        """
        display = self._display
        bgfg = self._bgfg
        font = self._font

        if width:
            (w, h) = _bounding_box(s, font)
            leftpad = (width - w) // 2
            rightpad = width - w - leftpad
            display.fill(0, x, y, leftpad, h)
            x += leftpad

        for ch in s:
            glyph = font.get_ch(ch)
            _draw_glyph(display, glyph, x, y, bgfg)
            x += glyph[2] + 1

        if width:
            display.fill(0, x, y, rightpad, h)

    def wrap(self, s, width):
        font = self._font
        max = len(s)
        chunks = [ 0, ]
        end = 0

        while end < max:
            start = end
            l = 0

            for i in range(start, max+1):
                if i >= len(s):
                    break
                ch = s[i]
                if ch == '\n':
                    end = i+1
                    break
                if ch == ' ':
                    end = i+1
                (_, h, w) = font.get_ch(ch)
                l += w + 1
                if l > width:
                    break
            if end <= start:
                end = i
            chunks.append(end)

        return chunks
