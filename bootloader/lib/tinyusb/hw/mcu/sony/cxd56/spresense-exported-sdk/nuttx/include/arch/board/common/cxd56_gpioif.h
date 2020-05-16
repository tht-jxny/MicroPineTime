/****************************************************************************
 * bsp/board/common/include/cxd56_gpioif.h
 *
 *   Copyright 2018 Sony Semiconductor Solutions Corporation
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Sony Semiconductor Solutions Corporation nor
 *    the names of its contributors may be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/
/**
 * @file cxd56_gpioif.h
 */

#ifndef __BSP_BOARD_COMMON_INCLUDE_CXD56_GPIOIF_H
#define __BSP_BOARD_COMMON_INCLUDE_CXD56_GPIOIF_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <sdk/config.h>

#include <stdint.h>
#include <stdbool.h>

#include <nuttx/irq.h>

#undef EXTERN
#if defined(__cplusplus)
#define EXTERN extern "C"
extern "C"
{
#else
#define EXTERN extern
#endif

/**
 * @defgroup gpioif GPIO Interface driver
 * @{
 */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/**
 * @name Pin Pull Setting
 * Pin floating, pull up, pull down definitions
 * @{
 */

#define PIN_FLOAT               (0) /**< Floating */
#define PIN_PULLUP              (1) /**< Internal Weak Pull Up */
#define PIN_PULLDOWN            (2) /**< Internal Weak Pull Down */
#define PIN_BUSKEEPER           (3) /**< Internal Bus-Keeper */

/* @} */

/**
 * @name GPIO Interrupt Setting
 * GPIO interrupt level and edge trigger types
 * @{
 */

#define INT_HIGH_LEVEL          (2) /**< High Level */
#define INT_LOW_LEVEL           (3) /**< Low Level */
#define INT_RISING_EDGE         (4) /**< Rising Edge */
#define INT_FALLING_EDGE        (5) /**< Falling Edge */
#define INT_BOTH_EDGE           (7) /**< Both Edge */

/* @} */

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Function Prototypes
 ****************************************************************************/

/**
 * GPIO Configure
 *
 * @param [in] pin: Pin number
 * @param [in] mode: Function mode 0=GPIO
 * @param [in] input: Input Enable true=enable, false=disable
 * @param [in] drive: Drive Strength true=High Drive(4mA), false=Normal(2mA)
 * @param [in] pull: 0=float, 1=pullup, 2=pulldown, 3=buskeeper
 *          - #PIN_FLOAT
 *          - #PIN_PULLUP
 *          - #PIN_PULLDOWN
 *          - #PIN_BUSKEEPER
 *
 * @return  OK(0) is success. negative value is failure.
 */

int board_gpio_config(uint32_t pin, int mode, bool input, bool drive,
                      int pull);

/**
 * GPIO Status
 *
 * @param [in] pin: Pin number
 * @param [out] input: Input Enable true=enable, false=disable
 * @param [out] output: Output Enable true=enable, false=disable
 * @param [out] drive: Drive Strength true=HighDrive(4mA), false=Normal(2mA)
 * @param [out] pull: 0=float, 1=pullup, 2=pulldown, 3=buskeeper
 *          - #PIN_FLOAT
 *          - #PIN_PULLUP
 *          - #PIN_PULLDOWN
 *          - #PIN_BUSKEEPER
 *
 * @return  Pin function mode. negative value is failure.
 */

int board_gpio_status(uint32_t pin, bool *input, bool *output, bool *drive,
                      int *pull);

/**
 * GPIO Write
 *
 * @param [in] pin: Pin number
 * @param [in] value: Write Value 0<high, 0=low, 0>hiz
 */

void board_gpio_write(uint32_t pin, int value);

/**
 * GPIO Read
 *
 * @param [in] pin: Pin number
 *
 * @return  read value 1=high, 0=low
 */

int board_gpio_read(uint32_t pin);

/**
 * GPIO Interrupt Configure
 *
 * @param [in] pin: Pin number
 * @param [in] mode: Interrupt polarity
 *          - #INT_HIGH_LEVEL
 *          - #INT_LOW_LEVEL
 *          - #INT_RISING_EDGE
 *          - #INT_FALLING_EDGE
 *          - #INT_BOTH_EDGE
 * @param [in] filter: Noise Filter true=enable, false=disable
 * @param [in] isr: Interrupt Service Routine
 *
 * @return  IRQ number. negative value is failure.
 */

int board_gpio_intconfig(uint32_t pin, int mode, bool filter, xcpt_t isr);

/**
 * GPIO Interrupt Configure
 *
 * @param [in] pin: Pin number
 * @param [out] mode: Interrupt polarity
 *          - #INT_HIGH_LEVEL
 *          - #INT_LOW_LEVEL
 *          - #INT_RISING_EDGE
 *          - #INT_FALLING_EDGE
 *          - #INT_BOTH_EDGE
 * @param [out] filter: Noise Filter true=enable, false=disable
 * @param [out] enabled: Interrupt true=enable, false=disable
 *
 * @return  IRQ number. negative value is failure.
 */

int board_gpio_intstatus(uint32_t pin, int *mode, bool *filter, bool *enabled);

/**
 * GPIO Interrupt Enable/Disable
 *
 * @param [in] pin: Pin number
 * @param [in] enable: Interrupt true=enable, false=disable
 *
 * @return  IRQ number. negative value is failure.
 */

int board_gpio_int(uint32_t pin, bool enable);

/* @} gpioif */

#undef EXTERN
#if defined(__cplusplus)
}
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BSP_BOARD_COMMON_INCLUDE_CXD56_GPIOIF_H */
