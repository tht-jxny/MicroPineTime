# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson

import wasp
import icons
import fonts

class StopwatchApp():
    NAME = 'Timer'
    ICON = icons.app

    def __init__(self):
        self._meter = wasp.widgets.BatteryMeter()
        self._reset()
        self._count = 0

    def foreground(self):
        """Activate the application."""
        self._last_clock = ( -1, -1, -1, -1, -1, -1 )

        self._draw()
        wasp.system.request_tick(97)
        wasp.system.request_event(wasp.EventMask.TOUCH |
                                  wasp.EventMask.BUTTON)

    def sleep(self):
        return True

    def wake(self):
        self._update()

    def press(self, button, state):
        if not state:
            return

        if self._started_at:
            self._update()
            self._started_at = 0
        else:
            uptime = wasp.watch.rtc.get_uptime_ms()
            uptime //= 10
            self._started_at = uptime - self._count
            self._update()

    def touch(self, event):
        if self._started_at:
            self._update()
            self._splits.insert(0, self._count)
            del self._splits[4:]
        else:
            self._reset()
            self._update()

        self._draw_splits()

    def tick(self, ticks):
        self._update()

    def _reset(self):
        self._started_at = 0
        self._count = 0
        self._last_count = -1
        self._splits = []

    def _draw_splits(self):
        draw = wasp.watch.drawable
        splits = self._splits
        if 0 == len(splits):
            draw.fill(0, 0, 120, 240, 120)
            return
        y = 240 - 12 - (len(splits) * 24)
        for i, s in enumerate(splits):
            if s:
                centisecs = s
                secs = centisecs // 100
                centisecs %= 100
                minutes = secs // 60
                secs %= 60

                t = '#{}  {:02}:{:02}.{:02}'.format(i+1, minutes, secs, centisecs)
            else:
                t = ''

            draw.set_font(fonts.sans24)
            w = fonts.width(fonts.sans24, t)
            draw.string(t, 0, y + (i*24), 240)

    def _draw(self):
        """Draw the display from scratch."""
        draw = wasp.watch.drawable
        draw.fill()

        self._last_count = -1
        self._update()
        self._meter.draw()
        self._draw_splits()

    def _update(self):
        # Before we do anything else let's make sure _count is
        # up to date
        if self._started_at:
            uptime = wasp.watch.rtc.get_uptime_ms()
            uptime //= 10
            self._count = uptime - self._started_at
            if self._count > 999*60*100:
                self._reset()

        draw = wasp.watch.drawable

        # Lazy update of the clock and battery meter
        now = wasp.watch.rtc.get_localtime()
        if now[4] != self._last_clock[4]:
            t1 = '{:02}:{:02}'.format(now[3], now[4])
            draw.set_font(fonts.sans24)
            draw.string(t1, 48, 16, 240-96)
            self._last_clock = now
            self._meter.update()

        if self._last_count != self._count:
            centisecs = self._count
            secs = centisecs // 100
            centisecs %= 100
            minutes = secs // 60
            secs %= 60

            t1 = '{}:{:02}'.format(minutes, secs)
            t2 = '{:02}'.format(centisecs)

            draw.set_font(fonts.sans36)
            w = fonts.width(fonts.sans36, t1)
            draw.string(t1, 180-w, 120-36)
            draw.fill(0, 0, 120-36, 180-w, 36)

            draw.set_font(fonts.sans24)
            draw.string(t2, 180, 120-36+18, width=46)
            self._last_count = self._count
