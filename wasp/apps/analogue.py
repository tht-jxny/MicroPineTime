# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson

import wasp
import icons
import math


red, yellow, green, cyan, blue, magenta, white, black = 0xf800, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f, 0xffff, 0x0000
hourhand, minutehand, bezel  = {}, {}, {}

bgcolor = black
bezel = True
hourhand["color"] = white
hourhand["length"] = 55
hourhand["width"] = 1
minutehand["color"] = red
minutehand["length"] = 100
minutehand["width"] = 1





# initialize 1-px wide, straight hands
hourhand["shape"] = [1]*hourhand["length"]
minutehand["shape"] = [1]*minutehand["length"]
    

class AnalogueApp():
    """Displays the time as a Fibonacci Clock
    """
    NAME = 'Analog'
    ICON = icons.app

    def __init__(self):
        pass

    def foreground(self):
        """Activate the application."""
        self.on_screen = ( -1, -1, -1, -1, -1, -1 )
        self.draw()
        wasp.system.request_tick(1000)

    def sleep(self):
        return True

    def wake(self):
        self.update()

    def tick(self, ticks):
        self.update()
        

    def draw(self):
        """Redraw the display from scratch."""
        draw = wasp.watch.drawable
        draw.fill(bgcolor)

        if bezel:
            for minute in range(0,60, 5):
                for pix in range(107,118): # 5-minunte-step bezel
                    mid = 120
                    shift = -90
                    thecos = math.cos(math.radians(6*minute+shift))
                    thesin = math.sin(math.radians(6*minute+shift))
                    x = mid + thecos*pix
                    y = mid + thesin*pix
                    width = 4
                    draw.fill(white, int(x-width//2), int(y-width//2), int(width), int(width))
        self.on_screen = ( -1, -1, -1, -1, -1, -1 )
        self.update()

    def update(self):
        """Update the display (if needed).

        The updates are a lazy as possible and rely on an prior call to
        draw() to ensure the screen is suitably prepared.
        """
        now = wasp.watch.rtc.get_localtime()
        if now[3] == self.on_screen[3] and now[4] == self.on_screen[4] and now[5] == self.on_screen[5]:
            if now[5] != self.on_screen[5]:
                self.on_screen = now
            return False
        draw = wasp.watch.drawable

        for pix in range(0,hourhand["length"]): #hour hand
            mid = 120
            shift = -90
            width = hourhand["width"] *4           
            if (self.on_screen[3] > -1): # clear previous position
                thecos = math.cos(math.radians(30*(self.on_screen[3]+self.on_screen[4]/60)+shift))
                thesin = math.sin(math.radians(30*(self.on_screen[3]+self.on_screen[4]/60)+shift))
                x = mid-int(thecos) + thecos*pix
                y = mid-int(thesin) + thesin*pix
                draw.fill(bgcolor, int(x-width//2), int(y-width//2), int(width), int(width))
            # draw new position
            thecos = math.cos(math.radians(30*(now[3]+now[4]/60)+shift))
            thesin = math.sin(math.radians(30*(now[3]+now[4]/60)+shift))
            x = mid-int(thecos) + thecos*pix
            y = mid-int(thesin) + thesin*pix
            draw.fill(hourhand["color"], int(x-width//2), int(y-width//2), int(width), int(width))
            
        for pix in range(0,minutehand["length"]): #minute hand
            mid = 120
            shift = -90
            width = minutehand["width"]*4
            if (self.on_screen[4] > -1): # clear previous position
                thecos = math.cos(math.radians(6*self.on_screen[4]+shift))
                thesin = math.sin(math.radians(6*self.on_screen[4]+shift))
                x = mid + thecos*pix
                y = mid + thesin*pix
                draw.fill(bgcolor, int(x-width//2), int(y-width//2), int(width), int(width))
            thecos = math.cos(math.radians(6*now[4]+shift))
            thesin = math.sin(math.radians(6*now[4]+shift))
            x = mid + thecos*pix
            y = mid + thesin*pix
            draw.fill(minutehand["color"], int(x-width//2), int(y-width//2), int(width), int(width))

        for pix in range(0,90): #minute hand
            mid = 120
            shift = -90
            width = 4
            if (self.on_screen[5] > -1): # clear previous position
                thecos = math.cos(math.radians(6*self.on_screen[5]+shift))
                thesin = math.sin(math.radians(6*self.on_screen[5]+shift))
                x = mid + thecos*pix
                y = mid + thesin*pix
                draw.fill(bgcolor, int(x-width//2), int(y-width//2), int(width), int(width))
            thecos = math.cos(math.radians(6*now[5]+shift))
            thesin = math.sin(math.radians(6*now[5]+shift))
            x = mid + thecos*pix
            y = mid + thesin*pix
            draw.fill(yellow, int(x-width//2), int(y-width//2), int(width), int(width))

            
        self.on_screen = now      
        return True
