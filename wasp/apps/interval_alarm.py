# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson
# Copyright (C) 2020 Joris Warmbier
# Copyright (c) 2020 Johannes Wache
"""Interval Timer Application
~~~~~~~~~~~~~~~~~~~~

An application to set a vibration alarm that repeats after a desired interval.
All settings can be accessed from the Watch UI.

    .. figure:: res/AlarmApp.png
        :width: 179

        Screenshot of the Alarm Application

"""

import wasp
import time

# 2-bit RLE, generated from res/alarm_icon.png, 285 bytes
icon = (
    b'\x02'
    b'`@'
    b'?\xff\xff\x80\xc5\x1f\xc55\xc8\x0b\xc7\x0b\xc91\xca\x06'
    b'\xcf\x07\xc90\xca\x06\xd1\x06\xca.\xca\x04\xd7\x04\xca-'
    b'\xc9\x04\xd9\x04\xc9,\xc9\x03\xde\x02\xc9+\xc8\x03\xe0\x02'
    b'\xc8+\xc7\x03\xcd\x07\xce\x02\xc7+\xc6\x03\xcd\t\xcd\x03'
    b'\xc6+\xc5\x03\xcb\x0f\xcb\x03\xc5+\xc4\x03\xca\x13\xca\x03'
    b'\xc4,\xc2\x04\xc9\x15\xc9\x04\xc22\xc9\x17\xc96\xc9\x19'
    b'\xc94\xca\x1a\xc93\xc9\r\xc1\x0e\xc83\xc7\x0e\xc3\x0e'
    b'\xc72\xc8\x0e\xc3\x0e\xc81\xc7\x0f\xc3\x0f\xc71\xc7\x0f'
    b'\xc3\x0f\xc71\xc7\x0f\xc3\x0f\xc71\xc6\x10\xc3\x10\xc60'
    b'\xc7\x10\xc3\x10\xc7/\xc7\x10\xc3\x10\xc7/\xc7\x10\xc3\x10'
    b'\xc7/\xc7\x0f\xc4\x10\xc7/\xc7\x0e\xc4\x11\xc7/\xc7\r'
    b'\xc4\x12\xc70\xc6\x0c\xc4\x13\xc61\xc7\n\xc4\x13\xc71'
    b'\xc7\n\xc3\x14\xc71\xc7\n\xc2\x15\xc71\xc8\x1f\xc82'
    b'\xc7\x1f\xc73\xc8\x1d\xc83\xc9\x1b\xc94\xc9\x19\xc96'
    b'\xc9\x17\xc98\xc9\x15\xc99\xca\x13\xca:\xcb\x0f\xcb<'
    b'\xce\x08\xcd>\xe1?\x01\xdf?\x03\xdd?\x04\xde?\x01'
    b'\xe1?\x00\xc5\x03\xd1\x03\xc5?\x00\xc4\t\xc7\t\xc4?'
    b'\x01\xc2\x0b\xc5\x0b\xc2?\xff\xff"'
)

class IntervalAlarmApp():
    """Allows the user to set a vibration alarm.
    """
    NAME = 'Intervals'
    ICON = icon

    def __init__(self):
        """Initialize the application."""

        self.active = False
        self.ringing = False
        self.interval = [0,0,0]
        self.display_time = ""

        self._set_current_alarm()

    def foreground(self):
        """Activate the application."""
        self._draw()
        wasp.system.request_event(wasp.EventMask.TOUCH)
        wasp.system.request_tick(1000)
        wasp.system.cancel_alarm(self.current_alarm, self._alert)

    def background(self):
        """De-activate the application."""
        # TODO:
        if self.active and self.ringing:
            self.ringing = False

    def tick(self, ticks):
        """Notify the application that its periodic tick is due."""
        if self.ringing:
            wasp.watch.vibrator.pulse(duty=50, ms=500)
            wasp.system.keep_awake()

    def touch(self, event):
        """Notify the application of a touchscreen touch event."""
        draw = wasp.watch.drawable
        if self.ringing:
            mute = wasp.watch.display.mute
            self.ringing = False
            mute(True)
            self._draw()
            mute(False)
            self._set_current_alarm()

        elif event[1] in range(150, 210) and event[2] in range(200,240):
            self.active = not self.active
            # TODO:
            if self.active:
                self._set_current_alarm()
                wasp.system.set_alarm(self.current_alarm, self._alert)


        elif event[1] in range(15,70):
            if event[2] in range(20,75):
                self.interval[0] += 1
                if self.interval[0] > 23:
                    self.interval[0] = 0
                self.active = False

            elif event[2] in range(85,140):
                self.interval[0] -= 1
                if self.interval[0] < 0:
                    self.interval[0] = 23
                self.active = False

        elif event[1] in range(75,155):
            if event[2] in range(20,75):
                self.interval[1] += 1
                if self.interval[1] > 59:
                    self.interval[1] = 0
                self.active = False

            elif event[2] in range(85,140):
                self.interval[1] -= 1
                if self.interval[1] < 0:
                    self.interval[1] = 59
                self.active = False

        elif event[1] in range(165,225):
            if event[2] in range(20,75):
                self.interval[2] += 1
                if self.interval[2] > 59:
                    self.interval[2] = 0
                self.active = False

            elif event[2] in range(85,140):
                self.interval[2] -= 1
                if self.interval[2] < 0:
                    self.interval[2] = 59
                self.active = False

        if not self.active:
            wasp.system.cancel_alarm(self.current_alarm, self._alert)
        self._update()

    def _draw(self):
        """Draw the display from scratch."""
        draw = wasp.watch.drawable
        if not self.ringing:
            draw.fill()
            draw.string('INTERVAL TIMER', 0, 6, width=240)

            draw.string(":", 155, 78, width=10)
            draw.string(":", 75, 78, width=10)

            for posx in [30, 110, 190]:
                draw.string("+", posx, 45, width=20)
                draw.string("-", posx, 110, width=20)

            draw.fill(0xffff, 160, 190, 40, 40)

            self._update()
        else:
            draw.fill()
            draw.string("Alarm", 0, 150, width=240)
            draw.blit(icon, 73, 50)

    def _update(self):
        """Update the dynamic parts of the application display."""
        draw = wasp.watch.drawable
        if self.active:
            draw.string("Repeating",0,200,width=160)
            draw.fill(0xf001, 162, 192, 36, 36)
            draw.string("Due: "+self.display_time,20,150,width=200)
        else:
            draw.string("Disabled",0,200,width=160)
            draw.fill(0x0000, 162, 192, 36, 36)
            draw.fill(0x0000, 20, 140, 200, 40)

        if self.interval[0] < 10:
            draw.string("0"+str(self.interval[0]), 25, 80, width=30)
        else:
            draw.string(str(self.interval[0]), 25, 80, width=30)

        if self.interval[1] < 10:
            draw.string("0"+str(self.interval[1]), 105, 80, width=30)
        else:
            draw.string(str(self.interval[1]), 105, 80, width=30)

        if self.interval[2] < 10:
            draw.string("0"+str(self.interval[2]), 185, 80, width=30)
        else:
            draw.string(str(self.interval[2]), 185, 80, width=30)

    def _alert(self):
        #if self.active:
            self.ringing = True
            wasp.system.wake()
            wasp.system.switch(self)

    def _set_current_alarm(self):
        now = wasp.watch.rtc.get_localtime()
        yyyy, mm, dd, HH, MM, SS = now[0], now[1], now[2], now[3], now[4], now[5]
        #if self.interval[1] < MM or (self.interval[1] == MM and self.interval[2] <= SS):
        #    HH += 1
        #if self.interval[0] < HH or (self.interval[0] == HH and self.interval[1] <= MM):
        #    dd += 1
        HH += self.interval[0]
        MM += self.interval[1]
        SS += self.interval[2]
        if SS > 59:
            MM += 1
            SS -= 60
        if MM > 59:
            HH += 1
            MM -= 60
        if HH > 23:
            dd += 1
            HH -= 24
        self.display_time = str(HH)+":"+str(MM)+":"+str(SS)
        self.current_alarm = (time.mktime((yyyy, mm, dd, HH, MM, SS, 0, 0, 0)))
