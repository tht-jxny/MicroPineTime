# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson
"""The complete set of wasp-os application entry points are documented
below as part of a template application. Note that the template does
not rely on any specific parent class. This is because applications in
wasp-os can rely on *duck typing* making a class hierarchy pointless.
"""

import wasp, time, icons
from random import randint

class SnakeGameApp():
    NAME = 'Snake'
    ICON = icons.snake_game

    def __init__(self):
        self.running = True
        self.snake = Snake()
        self.foodLocation()
        

    def foreground(self):
        """Activate the application."""
        wasp.watch.drawable.fill()
        self._draw()
        wasp.system.request_event(wasp.EventMask.TOUCH |
                                  wasp.EventMask.SWIPE_UPDOWN |
                                  wasp.EventMask.SWIPE_LEFTRIGHT)
        wasp.system.request_tick(250)

    def background(self):
        """De-activate the application."""
        pass

    def sleep(self):
        """Notify the application the device is about to sleep."""
        return False

    def wake(self):
        """Notify the application the device is waking up."""
        pass
    
    def touch(self,event):
        if not self.running:
            self.running = True
            wasp.watch.drawable.fill()

    def swipe(self, event):
      if self.running:
        """Notify the application of a touchscreen swipe event."""
        if event[0] == wasp.EventType.UP:
            self.snake.setDir(0,15)
        elif event[0] == wasp.EventType.DOWN:
            self.snake.setDir(0,-15)
        elif event[0] == wasp.EventType.LEFT:
            self.snake.setDir(15,0)
        elif event[0] == wasp.EventType.RIGHT:
            self.snake.setDir(-15,0)
        else:
            print("Error!")

    def tick(self, ticks):
        """Notify the application that its periodic tick is due."""
        self.update()
        
    
    def foodLocation(self):
        x = randint(0,15) * 15
        y = randint(0,15) * 15
        self.food = [x,y]

    def _draw(self):
        if self.running:
            self.update()
    
    def update(self):
        draw = wasp.watch.drawable
        """Draw the display from scratch."""
                       
        if (self.snake.eat(self.food)):
            self.foodLocation()
        draw.fill(x=self.food[0],y=self.food[1],w=15,h=15,bg=0x00ff)
        self.snake.update()
        if (self.snake.endGame()):
                self.running = False
                wasp.watch.vibrator.pulse()
                self.snake = Snake()
                draw.fill()
                draw.set_color(0xf000)
                draw.string('GAME', 0, 60, width=240)
                draw.string('OVER', 0, 98, width=240)
                draw.reset()
                return True
        self.snake.show()
        return True


# Based on https://www.youtube.com/watch?v=OMoVcohRgZA
class Snake():
    def __init__(self):
        self.body = []
        self.body.append([120,120])
        self.xdir = 0
        self.ydir = 0
        self.len = 1
    
    def setDir(self,x,y):
        self.xdir = x
        self.ydir = y
    
    def update(self):
        head = self.body[-1].copy()
        self.body = self.body[1:]
        head[0] += self.xdir
        head[1] += self.ydir
        self.body.append(head)

    def grow(self):
        head = self.body[-1]
        self.len += 1
        self.body.append(head)
        #self.update()
    
    def eat(self,pos):
        x = self.body[-1][0]
        y = self.body[-1][1]
        if (x == pos[0] and y == pos[1]):
            self.grow()
            return True
        return False

    def endGame(self):
        x = self.body[-1][0]
        y = self.body[-1][1]
        if (x >= 240 or x < 0) or (y >= 240 or y < 0):
            print("Inside 1")
            return True
        for i in range(len(self.body)-1):
            part = self.body[i]
            if (part[0] == x and part[1] == y):
                print("Inside 2")
                return True
        return False
    
    def show(self):
        draw = wasp.watch.drawable
        if self.len == 1: #vanish old and show new
            draw.fill(x=(self.body[0][0]-self.xdir),y=(self.body[0][1]-self.ydir),w=15,h=15,bg=0x0000)
            draw.fill(x=self.body[0][0]+1,y=self.body[0][1]+1,w=13,h=13,bg=0xffff)
        else: # vanish last and show first
           draw.fill(x=self.body[0][0],y=self.body[0][1],w=15,h=15,bg=0x0000)
           draw.fill(x=self.body[-1][0]+1,y=self.body[-1][1]+1,w=13,h=13,bg=0xffff)
        #for i in range(self.len):
        #   draw.fill(x=self.body[i][0]+1,y=self.body[i][1]+1,w=13,h=13,bg=0xffff)

