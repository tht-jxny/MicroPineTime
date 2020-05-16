# # SPDX-License-Identifier: LGPL-3.0-or-later
# # Copyright (C) 2020 Daniel Thompson
# """Ultra-simple settings application.

# Currently the settings application contains only one setting: brightness
# """

# import wasp
# import icons

# class SettingsApp():
#     NAME = 'Settings'
#     ICON = icons.settings

#     def foreground(self):
#         self._draw()
#         wasp.system.request_event(wasp.EventMask.TOUCH)

#     def touch(self, event):
#         brightness = wasp.system.brightness + 1
#         if brightness > 3:
#             brightness = 1
#         wasp.system.brightness = brightness
#         self._update()

#     def _draw(self):
#         """Redraw the display from scratch."""
#         wasp.watch.drawable.fill()
#         wasp.watch.drawable.string('Settings', 0, 6, width=240)
#         wasp.watch.drawable.string('Brightness', 0, 120 - 3 - 24, width=240)
#         self._update()

#     def _update(self):
#         if wasp.system.brightness == 3:
#             say = "High"
#         elif wasp.system.brightness == 2:
#             say = "Mid"
#         else:
#             say = "Low"
#         wasp.watch.drawable.string(say, 0, 120 + 3, width=240)


# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson
"""Ultra-simple settings application.

Currently the settings application contains only one setting: brightness
"""

import wasp
import icons
import fonts.gotham36

class SettingsApp():
    NAME = 'Settings'
    ICON = icons.settings_new

    def foreground(self):
        self._draw()
        wasp.system.request_event(wasp.EventMask.TOUCH)

    def touch(self, event):
        if (event[2] >= 45) and (event[2] <=85): # Brightness
            if (event[1] >= 40) and (event[1] <= 80):
                wasp.system.brightness = 1
            elif (event[1] >= 100) and (event[1] <= 140):
                wasp.system.brightness = 2
            elif (event[1] >= 160) and (event[1] <= 200):
                wasp.system.brightness = 3
            else:
                print("Error with brightness touch")
        if (event[2] >= 165) and (event[2] <=205): # Sleep timer
            if (event[1] >= 40) and (event[1] <= 80):
                wasp.system.blank_after = 7
            elif (event[1] >= 100) and (event[1] <= 140):
                wasp.system.blank_after = 15
            elif (event[1] >= 160) and (event[1] <= 200):
                wasp.system.blank_after = 30
            else:
                print("Error with sleep timer touch")
        self._update()

    def _draw(self):
        """Redraw the display from scratch."""
        wasp.watch.drawable.fill()
        wasp.watch.drawable.string('Low  Mid  High', 0, 90, width=240)
        wasp.watch.drawable.string(' 7s   15s  30s', 0, 210, width=240)
        wasp.watch.drawable.set_font(fonts.gotham36)
        wasp.watch.drawable.string('Brightness:', 0, 4, width=240)
        wasp.watch.drawable.string('Sleeptimer:', 0, 120, width=240)
        wasp.watch.drawable.reset()
        self._update()

    def _update(self):
        if wasp.system.brightness == 3:
            brightness_icon = icons.switch3_right
        elif wasp.system.brightness == 2:
            brightness_icon = icons.switch3_middle
        else:
            brightness_icon = icons.switch3_left
        wasp.watch.drawable.blit(brightness_icon,x=40,y=45)
        if wasp.system.blank_after == 7 :
            timer_icon = icons.switch3_left
        elif wasp.system.blank_after == 15 :
            timer_icon = icons.switch3_middle
        else:
            timer_icon = icons.switch3_right
        wasp.watch.drawable.blit(timer_icon,x=40,y=165)
