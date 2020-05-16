# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson

""" Real Time Clock based on the nRF-family low power counter """

import time

#class Stim(object):
#    def __init__(self):
#        self(0)
#
#    def __call__(self, v):
#        self.c = v
#    
#    def counter(self):
#        return self.c

class RTC(object):
    """Real Time Clock based on the nRF-family low power counter."""

    def __init__(self, counter):
        self.counter = counter
        self._uptime = 0
        self.set_localtime((2020, 5, 5, 19, 41, 0, 0, 0))

    def update(self):
        newcount = self.counter.counter()
        split = newcount - self.lastcount
        if split == 0:
            return False
        if split < 0:
            split += (1 << 24)

        self.lastcount += split
        self.lastcount &= (1 << 24) - 1   
        self._uptime += split

        return True

    def set_localtime(self, t):
        self.lastcount = self.counter.counter()

        if len(t) < 8:
            yyyy = t[0]
            mm = t[1]
            dd = t[2]
            HH = t[3]
            MM = t[4]
            SS = t[5]

            t = (yyyy, mm, dd, HH, MM, SS, 0, 0)

        lt = time.mktime(t)
        self.offset = lt - (self._uptime >> 3)

    def get_localtime(self):
        self.update()
        return time.localtime(self.offset + (self._uptime >> 3))

    def get_time(self):
        localtime = self.get_localtime()
        return localtime[3:6]

    @property
    def uptime(self):
        """Provide the current uptime in seconds."""
        return self._uptime // 8

    def get_uptime_ms(self):
        """Return the current uptime in milliseconds."""
        return self._uptime * 125
