# SPDX-License-Identifier: LGPL-3.0-or-later
# Copyright (C) 2020 Daniel Thompson
"""simple calculator application.

Currently the calculator application contains only the function for drawing the buttons
"""

import wasp
import parser
import icons

class CalculatorApp():
    NAME = 'Calc'
    ICON = icons.calc
    def foreground(self):
        self.output = ""
        self.fields = [["7","8","9","+","("],
                        ["4","5","6","-",")"],
                        ["1","2","3","*","^"],
                        ["C","0",".",":","="]]
        self.fields_to_button_path = {"0":icons.calc_num_0,
                                      "1":icons.calc_num_1,
                                      "2":icons.calc_num_2,
                                      "3":icons.calc_num_3,
                                      "4":icons.calc_num_4,
                                      "5":icons.calc_num_5,
                                      "6":icons.calc_num_6,
                                      "7":icons.calc_num_7,
                                      "8":icons.calc_num_8,
                                      "9":icons.calc_num_9,
                                      "+":icons.calc_symbol_plus,
                                      "-":icons.calc_symbol_minus,
                                      "*":icons.calc_symbol_multi,
                                      ":":icons.calc_symbol_divide,
                                      ")":icons.calc_symbol_close,
                                      "(":icons.calc_symbol_open,
                                      "^":icons.calc_symbol_hat,
                                      ".":icons.calc_symbol_point,
                                      "=":icons.calc_symbol_equal,
                                      "C":icons.calc_symbol_new}
        self.drawButtons()
        self._draw()
        wasp.system.request_event(wasp.EventMask.TOUCH)

    def drawButtons(self):
        wasp.watch.drawable.fill()
        for y in range(4):
            for x in range(5):
                icon = self.fields_to_button_path[self.fields[y][x]]
                if (x == 0):
                    wasp.watch.drawable._rle2bit(icon, x*47,y*47+46)
                else:
                    wasp.watch.drawable._rle2bit(icon, x*47-1,y*47+46)
        wasp.watch.drawable._rle2bit(icons.calc_symbol_undo, 205,2)

    def touch(self, event): #event[2] is y; event[1] is x
        if (event[2] < 48):
            if (event[1] > 200): # undo button pressed
                if (self.output != ""):
                    self.output = self.output[:-1]
            else:  # Touched on output field
                self.output = "Use the buttons"
        else:
            indices = [(event[2]// 48)-1,event[1]//47]
            # Error handling for touching at the border
            if (indices[0]>3):
                indices[0] = 3
            if (indices[1]>4):
                indices[1] = 4
            button_pressed = self.fields[indices[0]][indices[1]]
            if (button_pressed == "C"):
                self.output = ""
            elif (button_pressed == "="):
                self.output = self.calculate(self.output)
            else:
                self.output +=  button_pressed
        self.displayOutput()

    def calculate(self,s):
        equation = s
        for i in range(len(s)):
            if (s[i] =="^"):
                equation = s[:i] + "**"+s[i+1:]

        try:
            result = eval(parser.expr(equation).compile())
        except ImportError:
            print("Error")
            result = ""
        
        return str(result)

    def _draw(self):
        pass


    def displayOutput(self):
        wasp.watch.drawable.fill(x=2,y=2,w=195,h=40) 
        if (self.output != ""):
            if len(self.output) >= 11:
                wasp.watch.drawable.string(self.output[len(self.output)-11:], 5, 8, width=190)
            else:
                wasp.watch.drawable.string(self.output, 5, 8, width=190)

        
