# This file is based on wasp-os's (default) clock.py, by Daniel Thompson:
# https://github.com/daniel-thompson/wasp-os/blob/master/wasp/clock.py

# ANALOGUE CLOCK


import watch
import icons
import math

red, yellow, green, cyan, blue, magenta, white, black = 0xf800, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f, 0xffff, 0x0000
hourhand, minutehand, secondhand, bezel  = {}, {}, {}, {}

# using custom colors:
# algorithm based on https://stackoverflow.com/questions/13720937/c-defined-16bit-high-color
#
# def fromrgb(red, green, blue): # 255, 255, 255  --> "0xffff
#     red = 31*(red+4)
#     green = 63*(green+2)
#     blue = 31*(blue+4)
#     return hex(( int(red/255) << 11 ) | ( int(green/255) <<5) | int(blue/255))
#
# def fromhexrgb(rgb): # "FFFFFF" --> "0xffff
#     red = 31*(int(rgb[:2], 16)+4)
#     green = 63*(int(rgb[2:4], 16)+2)
#     blue = 31*(int(rgb[4:6], 16)+4)
#     return hex(( int(red/255) << 11 ) | ( int(green/255) <<5) | int(blue/255))


# Options
bgcolor = black
bezel = True
hourhand["color"] = white
hourhand["length"] = 75
hourhand["width"] = 1
hourhand["drawres"] = 1
minutehand["color"] = red
minutehand["length"] = 100
minutehand["width"] = 1
minutehand["drawres"] = 1
secondhand["color"] = white
secondhand["length"] = 110
secondhand["width"] = 1
secondhand["drawres"] = 5


# # 3-tuple key: (central/lower end, outer end, thickening factor)
# shapelist = { "chickenwing" : [ (0.00, 1.00, 2),
#                                 (0.15, 0.85, 3),
#                                 (0.18, 0.82, 4),
#                                 (0.21, 0.79, 5) ],
#               "fleecenter" : [ (0.00, 0.10, 0),
#                                (0.10, 1.00, 1) ]
# }

shapelist = [ (0.00, 1.00, 2),
              (0.15, 0.85, 3),
              (0.18, 0.82, 4),
              (0.21, 0.79, 5) ]


# initialize 1-px wide, straight hands
hourhand["shape"] = [1]*hourhand["length"]
minutehand["shape"] = [1]*minutehand["length"]
secondhand["shape"] = [1]*secondhand["length"]
    
# thicken hands at given radiuses
for s in shapelist:#["chickenwing"]:
    start = int(hourhand["length"]*s[0])
    stop = int(hourhand["length"]*s[1])
    hourhand["shape"][start:stop] = [s[2]]*(stop-start)
    
    start = int(minutehand["length"]*s[0])
    stop = int(minutehand["length"]*s[1])
    minutehand["shape"][start:stop] = [s[2]]*(stop-start)
    
# for s in shapelist["fleecenter"]:    
#     start = int(secondhand["length"]*s[0])
#     stop = int(secondhand["length"]*s[1])
#     secondhand["shape"][start:stop] = [s[2]]*(stop-start)


class AnalogueClockApp(object):
    def __init__(self):
        self.meter = BatteryMeter()        
    def handle_event(self, event_view):
        """Process events that the app is subscribed to."""
        if event_view[0] == manager.EVENT_TICK:
            self.update()
        else:
            # TODO: Raise an unexpected event exception
            pass
    def foreground(self, manager, effect=None):
        """Activate the application."""
        self.on_screen = ( -1, -1, -1, -1, -1, -1 )
        self.draw(effect)
        manager.request_tick(1000)
    def tick(self, ticks):
        self.update()

    def background(self):
        """De-activate the application (without losing state)."""
        pass

    def sleep(self):
        return True

    def wake(self):
        self.update()    

    def draw(self, effect=None):
        draw = watch.drawable
        draw.fill(bgcolor)

        if bezel:
            for minute in range(0,60):
                for pix in range(112,118): # 1-minute-step bezel
                    mid = 120
                    shift = -90
                    thecos = math.cos(math.radians(6*minute+shift))
                    thesin = math.sin(math.radians(6*minute+shift))
                    x = mid + thecos*pix
                    y = mid + thesin*pix
                    width = 2
                    draw.fill(white, int(x-width//2), int(y-width//2), int(width), int(width))
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
            for minute in range(0,60, 15):
                for pix in range(107,120): # 15-minunte-step bezel
                    mid = 120
                    shift = -90
                    thecos = math.cos(math.radians(6*minute+shift))
                    thesin = math.sin(math.radians(6*minute+shift))
                    x = mid + thecos*pix
                    y = mid + thesin*pix
                    width = 6
                    draw.fill(red, int(x-width//2), int(y-width//2), int(width), int(width))
                    
        self.on_screen = ( -1, -1, -1, -1, -1, -1 )
        self.update()
        self.meter.draw()

    def update(self):
        now = watch.rtc.get_localtime()
        if now[3] == self.on_screen[3] and now[4] == self.on_screen[4] and now[5] == self.on_screen[5]:
            if now[5] != self.on_screen[5]:
                self.meter.update()                
                self.on_screen = now
                return False        
                
        draw = watch.drawable
        
        # todo: drawhand(rank=rank, length, resolution, shape)
        #  eg:  drawhand(hour     , hourhand["length"], hourhand["drawres"], hourhand["shape"])
        #  eg:  drawhand(minute...
        #  eg:  drawhand(second...        
        
        for pix in range(0,hourhand["length"],hourhand["drawres"]): #hour hand
            mid = 120
            shift = -90
            width = hourhand["shape"][pix]*hourhand["width"]            
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
            
        for pix in range(0,minutehand["length"],minutehand["drawres"]): #minute hand
            mid = 120
            shift = -90
            width = minutehand["shape"][pix]*minutehand["width"]
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

        for pix in range(0,secondhand["length"],secondhand["drawres"]): #second hand
            mid = 120
            shift = -90
            width = secondhand["width"] #secondhand["shape"][pix]*secondhand["width"]            
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
            draw.fill(secondhand["color"], int(x-width//2), int(y-width//2), int(width), int(width))
            
            
        self.on_screen = now
        self.meter.update()        
        return True
