# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson

import icons
import watch

green, red = 0x07e0, 0xf800

class BatteryMeter(object):
    def __init__(self):
        self.level = -2

    def draw(self):
        self.level = -2
        self.update()

    def update(self):
        icon = icons.vbattery_charging
        draw = watch.drawable
    
        if watch.battery.charging():
            if self.level != -1:
                draw.rleblit(icon, pos=(239-icon[0], 0), fg=0xe73c)
                self.level = -1
        else:
            level = watch.battery.level()
            if ((level <= (self.level + 4)) and (level >= (self.level - 5))):
                return

            if level <= 8:
                draw.rleblit(icon, pos=(239-icon[0], 0), fg= red)
                draw.fill(0, 203, 14, 26, 17)
                return

            draw.rleblit(icon, pos=(239-icon[0], 0), fg=0xe73c)
            draw.fill(0, 203, 14, 26, 17)
            width = (level // 7 )*2
            draw.fill(green, 205 + (25-width), 13, width, 17)


            self.level = level

class ScrollIndicator():
    def __init__(self, x=240-18, y=240-24):
        self._pos = (x, y)
        self.up = True
        self.down = True

    def draw(self):
        self.update()

    def update(self):
        draw = watch.drawable
        if self.up:
            draw.rleblit(icons.up_arrow, pos=self._pos, fg=0x7bef)
        if self.down:
            draw.rleblit(icons.down_arrow, pos=(self._pos[0], self._pos[1] + 13), fg=0x7bef)
