# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson
"""Ultra-simple settings application.

Currently the settings application contains only one setting: brightness
"""

import wasp
import icons

class SettingsApp():
    NAME = 'Settings'
    ICON = icons.settings_new

    def foreground(self):
        self._draw()
        wasp.system.request_event(wasp.EventMask.TOUCH)

    def touch(self, event):
        brightness = wasp.system.brightness + 1
        if brightness > 3:
            brightness = 1
        wasp.system.brightness = brightness
        self._update()

    def _draw(self):
        """Redraw the display from scratch."""
        wasp.watch.drawable.fill()
        wasp.watch.drawable.string('Settings', 0, 6, width=240)
        wasp.watch.drawable.string('Brightness:', 0, 50, width=240)
        wasp.watch.drawable.string('Sleep-Timer:', 0, 140, width=240)
        self._update()

    def _update(self):
        if wasp.system.brightness == 3:
            icon = icons.switch3_right
        elif wasp.system.brightness == 2:
            icon = icons.switch3_middle
        else:
            icon = icons.switch3_left
        wasp.watch.drawable.blit(icons.switch3_left,x=40,y=90)
