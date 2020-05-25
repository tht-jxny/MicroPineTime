# MicroPineTime

## Overview
This is an attempt of creating a firware for the Pine64 [PineTime](https://wiki.pine64.org/index.php/PineTime). This project is based on [https://github.com/daniel_thompson/wasp-os](https://github.com/daniel_thompson/wasp-os) fully in python.
I'm working on Ubuntu 18.04 Bionic Beaver and sometimes on a RaspberryPi 4.
For a detailed documentation, troubleshooting and other information, please head over to the original repo.



## Setup
Either follow the guide in the original repo or proceed in this guide.
You need some dependencies:
``` Shellsession
sudo apt install \
  git build-essential libsdl2-2.0.0 \
  python3-click python3-numpy python3-pexpect \
  python3-pil python3-pip python3-serial
pip3 install --user pysdl2
```

Now run these commands to build from source code:
``` Shellsession
git clone https://github.com/tht-jxny/MicroPineTime
cd MicroPineTime
make submodules
make softdevice
make -j `nproc` BOARD=pinetime all
```

Guide will be completed soon

## Current Functionality

The firmware currently consists of the following features:
- Fibonacci Clock (Detailed Explanation on [this site](http://www.fibonacciclock.co.uk/))
- Digital Clock 
- Stopwatch with lap times
- Settings for brightness and a sleep timer
- Calculator App
- The Snake Game
- Flashlight

## Coming soon

I'll try to implement the following hopefully in the near future:
- Other watchfaces (f.ex. analogue)
- Step counter
- Alarm
- Heart Rate Monitor
- Maybe other games

Proposals are gratefully accepted

## License

Currently, the majority of code in this repo is the result of Daniel Thompson's work so if you have questions concerning licensing please go and ask him.

