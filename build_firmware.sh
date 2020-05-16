make -C bootloader/ BOARD=pinetime_nrf52832 all genhex
make -C micropython/mpy-cross
make[1]: Entering directory '/home/johannes/coding/PineTime/SynologyDrive/MicroPineTime/micropython/mpy-cross'
Use make V=1 or set BUILD_VERBOSE in your environment to increase build verbosity.
make[1]: Entering directory '/home/johannes/coding/PineTime/SynologyDrive/MicroPineTime/bootloader'
LD pinetime_nrf52832_bootloader--nosd.out
GEN build/genhdr/mpversion.h
CC main.c

CR pinetime_nrf52832_bootloader--nosd.hex
   text	   data	    bss	    dec	    hex	filename
  21832	    148	  14994	  36974	   906e	_build-pinetime_nrf52832/pinetime_nrf52832_bootloader--nosd.out

make[1]: Leaving directory '/home/johannes/coding/PineTime/SynologyDrive/MicroPineTime/bootloader'
python3 -m nordicsemi dfu genpkg \
	--bootloader bootloader/_build-pinetime_nrf52832/pinetime_nrf52832_bootloader-*-nosd.hex \
	--softdevice bootloader/lib/softdevice/s132_nrf52_6.1.1/s132_nrf52_6.1.1_softdevice.hex \
	bootloader.zip
Failed to import ecdsa, cannot do signing
Makefile:18: recipe for target 'bootloader' failed
LINK mpy-cross
   text	   data	    bss	    dec	    hex	filename
 297599	  17264	    872	 315735	  4d157	mpy-cross
make[1]: Leaving directory '/home/johannes/coding/PineTime/SynologyDrive/MicroPineTime/micropython/mpy-cross'
rm -f micropython/ports/nrf/build-pinetime-s132/frozen_content.c
make -C micropython/ports/nrf \
	BOARD=pinetime SD=s132 \
	MICROPY_VFS_LFS2=1 \
	FROZEN_MANIFEST=/home/johannes/coding/PineTime/SynologyDrive/MicroPineTime/wasp/boards/pinetime/manifest.py
make[1]: Entering directory '/home/johannes/coding/PineTime/SynologyDrive/MicroPineTime/micropython/ports/nrf'
Use make V=1 or set BUILD_VERBOSE in your environment to increase build verbosity.
GEN build-pinetime-s132/genhdr/mpversion.h
CC modules/uos/moduos.c
CC ../../lib/utils/pyexec.c
MPY apps/fibonacci_clock.py
MPY apps/snake.py
MPY apps/stopwatch.py
MPY draw565.py
MPY fonts/__init__.py
MPY wasp.py
GEN build-pinetime-s132/frozen_content.c
CC build-pinetime-s132/frozen_content.c
LINK build-pinetime-s132/firmware.elf
Makefile:428: recipe for target 'build-pinetime-s132/firmware.elf' failed
Makefile:38: recipe for target 'micropython' failed
make -j 8 BOARD=pinetime all
