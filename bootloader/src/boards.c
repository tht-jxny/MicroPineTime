/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2018 Ha Thach for Adafruit Industries
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "boards.h"
#include "nrf_spi.h"
#include "nrf_pwm.h"
#include "nrf_wdt.h"
#include "app_scheduler.h"
#include "app_timer.h"
#include "pnvram.h"

//--------------------------------------------------------------------+
// MACRO TYPEDEF CONSTANT ENUM DECLARATION
//--------------------------------------------------------------------+
#define SCHED_MAX_EVENT_DATA_SIZE           sizeof(app_timer_event_t)        /**< Maximum size of scheduler events. */
#define SCHED_QUEUE_SIZE                    30                               /**< Maximum number of events in the scheduler queue. */

#if defined(LED_NEOPIXEL) || defined(LED_RGB_RED_PIN)
  void neopixel_init(void);
  void neopixel_write(uint8_t *pixels);
  void neopixel_teardown(void);
#endif

#ifdef ST7789_SPI_DISPLAY
void st7789_init(void);
void st7789_state(int state);
void st7789_teardown(void);
#endif

//------------- IMPLEMENTATION -------------//
void button_init(uint32_t pin)
{
  nrf_gpio_cfg_sense_input(pin, BUTTON_PULL, NRF_GPIO_PIN_NOSENSE);
#ifdef BUTTON_ENABLE
  /*
   * BUTTON_ENABLE is used when a switch is connected to another GPIO pin
   * rather than the Vcc or ground. In the bootloader we'll just permanently
   * push the enable pin to whatever value the sense pin thinks is the
   * active one.
   */
  nrf_gpio_cfg_output(BUTTON_ENABLE);
  nrf_gpio_pin_write(BUTTON_ENABLE, BUTTON_ACTIVE);
#endif
}

bool button_pressed(uint32_t pin)
{
  return (nrf_gpio_pin_read(pin) == BUTTON_ACTIVE) ? true : false;
}

void board_init(void)
{
  // the watchdog has a long timeout and, when we are running in bootloader
  // mode, will be fed by the systick handler
  wdt_init();

  // stop LF clock just in case we jump from application without reset
  NRF_CLOCK->TASKS_LFCLKSTOP = 1UL;

  // Use Internal OSC to compatible with all boards
  NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_RC;
  NRF_CLOCK->TASKS_LFCLKSTART = 1UL;

  button_init(BUTTON_DFU);
#ifdef BUTTON_FRESET
  button_init(BUTTON_FRESET);
#endif
  NRFX_DELAY_US(100); // wait for the pin state is stable

  // use PMW0 for LED RED
#if LEDS_NUMBER > 0
  led_pwm_init(LED_PRIMARY, LED_PRIMARY_PIN);
#endif
#if LEDS_NUMBER > 1
  led_pwm_init(LED_SECONDARY, LED_SECONDARY_PIN);
#endif

  // use neopixel for use enumeration
#if defined(LED_NEOPIXEL) || defined(LED_RGB_RED_PIN)
  neopixel_init();
#endif

  // Init scheduler
  APP_SCHED_INIT(SCHED_MAX_EVENT_DATA_SIZE, SCHED_QUEUE_SIZE);

  // Init app timer (use RTC1)
  app_timer_init();

  // Configure Systick for led blinky
  NVIC_SetPriority(SysTick_IRQn, 7);
  SysTick_Config(SystemCoreClock/1000);

  // Bring up the display (if there is one)
#ifdef ST7789_SPI_DISPLAY
  st7789_init();
#endif
}

void board_teardown(void)
{
#ifdef ST7789_SPI_DISPLAY
  st7789_teardown();
#endif

  // Disable systick, turn off LEDs
  SysTick->CTRL = 0;

#if LEDS_NUMBER > 0
  // Disable and reset PWM for LEDs
  led_pwm_teardown();
#endif
#if defined(LED_NEOPIXEL) || defined(LED_RGB_RED_PIN)
  neopixel_teardown();
#endif

  // Button

  // Stop RTC1 used by app_timer
  NVIC_DisableIRQ(RTC1_IRQn);
  NRF_RTC1->EVTENCLR    = RTC_EVTEN_COMPARE0_Msk;
  NRF_RTC1->INTENCLR    = RTC_INTENSET_COMPARE0_Msk;
  NRF_RTC1->TASKS_STOP  = 1;
  NRF_RTC1->TASKS_CLEAR = 1;

  // Stop LF clock
  NRF_CLOCK->TASKS_LFCLKSTOP = 1UL;
}

static uint32_t _systick_count = 0;
static uint32_t _long_press_count = 0;
void SysTick_Handler(void)
{
  _systick_count++;
  pnvram_add_ms(pnvram, 1);
#if LEDS_NUMBER > 0
  led_tick();
#endif

  /*
   * Feed the dog... this is a backstop. The *only* reason we are running
   * the watchdog is to help us recover if the bootloader crashes in some
   * way that makes it impossible for us to reboot using the button test
   * code below. This is OK to feed it from a periodic interrupt.
   */
  if (!button_pressed(BUTTON_DFU))
    nrf_wdt_reload_request_set(NRF_WDT, 0);

  /*
   * Detect and (trivially) debounce a press of the DFU button. When
   * found try to launch the application regardless of the DFU button
   * state. We ignore the button press for our first few seconds of life
   * because makes it harder to accidentally start the application when
   * recovering from a flat battery.
   */
  if (button_pressed(BUTTON_DFU)) {
    if (_systick_count > 4750 && _long_press_count++ >  250) {
      NRF_POWER->GPREGRET = BOARD_MAGIC_FORCE_APP_BOOT;
      NVIC_SystemReset();
    }
  } else {
    _long_press_count = 0;
  }
}


uint32_t tusb_hal_millis(void)
{
  return ( ( ((uint64_t)app_timer_cnt_get())*1000*(APP_TIMER_CONFIG_RTC_FREQUENCY+1)) / APP_TIMER_CLOCK_FREQ );
}
#if LEDS_NUMBER > 0

void pwm_teardown(NRF_PWM_Type* pwm )
{
  pwm->TASKS_SEQSTART[0] = 0;
  pwm->ENABLE            = 0;

  pwm->PSEL.OUT[0] = 0xFFFFFFFF;
  pwm->PSEL.OUT[1] = 0xFFFFFFFF;
  pwm->PSEL.OUT[2] = 0xFFFFFFFF;
  pwm->PSEL.OUT[3] = 0xFFFFFFFF;

  pwm->MODE        = 0;
  pwm->COUNTERTOP  = 0x3FF;
  pwm->PRESCALER   = 0;
  pwm->DECODER     = 0;
  pwm->LOOP        = 0;
  pwm->SEQ[0].PTR  = 0;
  pwm->SEQ[0].CNT  = 0;
}

static uint16_t led_duty_cycles[PWM0_CH_NUM] = { 0 };

#if LEDS_NUMBER > PWM0_CH_NUM
#error "Only " PWM0_CH_NUM " concurrent status LEDs are supported."
#endif

void led_pwm_init(uint32_t led_index, uint32_t led_pin)
{
  NRF_PWM_Type* pwm    = NRF_PWM0;

  pwm->ENABLE = 0;

  nrf_gpio_cfg_output(led_pin);
  nrf_gpio_pin_write(led_pin, 1 - LED_STATE_ON);

  pwm->PSEL.OUT[led_index] = led_pin;

  pwm->MODE            = PWM_MODE_UPDOWN_Up;
  pwm->COUNTERTOP      = 0xff;
  pwm->PRESCALER       = PWM_PRESCALER_PRESCALER_DIV_16;
  pwm->DECODER         = PWM_DECODER_LOAD_Individual;
  pwm->LOOP            = 0;

  pwm->SEQ[0].PTR      = (uint32_t) (led_duty_cycles);
  pwm->SEQ[0].CNT      = 4; // default mode is Individual --> count must be 4
  pwm->SEQ[0].REFRESH  = 0;
  pwm->SEQ[0].ENDDELAY = 0;

  pwm->ENABLE = 1;

  pwm->EVENTS_SEQEND[0] = 0;
//  pwm->TASKS_SEQSTART[0] = 1;
}

void led_pwm_teardown(void)
{
  pwm_teardown(NRF_PWM0);
}

void led_pwm_duty_cycle(uint32_t led_index, uint16_t duty_cycle)
{
  led_duty_cycles[led_index] = duty_cycle;
  nrf_pwm_event_clear(NRF_PWM0, NRF_PWM_EVENT_SEQEND0);
  nrf_pwm_task_trigger(NRF_PWM0, NRF_PWM_TASK_SEQSTART0);
}

static uint32_t primary_cycle_length;
#ifdef LED_SECONDARY_PIN
static uint32_t secondary_cycle_length;
#endif
void led_tick() {
    uint32_t millis = _systick_count;

    uint32_t cycle = millis % primary_cycle_length;
    uint32_t half_cycle = primary_cycle_length / 2;
    if (cycle > half_cycle) {
        cycle = primary_cycle_length - cycle;
    }
    uint16_t duty_cycle = 0x4f * cycle / half_cycle;
    #if LED_STATE_ON == 1
    duty_cycle = 0xff - duty_cycle;
    #endif
    led_pwm_duty_cycle(LED_PRIMARY, duty_cycle);

    #ifdef LED_SECONDARY_PIN
    cycle = millis % secondary_cycle_length;
    half_cycle = secondary_cycle_length / 2;
    if (cycle > half_cycle) {
        cycle = secondary_cycle_length - cycle;
    }
    duty_cycle = 0x8f * cycle / half_cycle;
    #if LED_STATE_ON == 1
    duty_cycle = 0xff - duty_cycle;
    #endif
    led_pwm_duty_cycle(LED_SECONDARY, duty_cycle);
    #endif
}

static uint32_t rgb_color;
static bool temp_color_active = false;
void led_state(uint32_t state)
{
    uint32_t new_rgb_color = rgb_color;
    uint32_t temp_color = 0;
    switch (state) {
        case STATE_USB_MOUNTED:
          new_rgb_color = 0x00ff00;
          primary_cycle_length = 3000;
          break;

        case STATE_BOOTLOADER_STARTED:
        case STATE_USB_UNMOUNTED:
          new_rgb_color = 0xff0000;
          primary_cycle_length = 300;
          break;

        case STATE_WRITING_STARTED:
          temp_color = 0xff0000;
          primary_cycle_length = 100;
          break;

        case STATE_WRITING_FINISHED:
          // Empty means to unset any temp colors.
          primary_cycle_length = 3000;
          break;

        case STATE_BLE_CONNECTED:
          new_rgb_color = 0x0000ff;
          #ifdef LED_SECONDARY_PIN
          secondary_cycle_length = 300;
          #else
          primary_cycle_length = 300;
          #endif
          break;

        case STATE_BLE_DISCONNECTED:
          new_rgb_color = 0xff00ff;
          #ifdef LED_SECONDARY_PIN
          secondary_cycle_length = 3000;
          #else
          primary_cycle_length = 3000;
          #endif
          break;

        default:
        break;
    }
    uint8_t* final_color = NULL;
    new_rgb_color &= BOARD_RGB_BRIGHTNESS;
    if (temp_color != 0){
        temp_color &= BOARD_RGB_BRIGHTNESS;
        final_color = (uint8_t*)&temp_color;
        temp_color_active = true;
    } else if (new_rgb_color != rgb_color) {
        final_color = (uint8_t*)&new_rgb_color;
        rgb_color = new_rgb_color;
    } else if (temp_color_active) {
        final_color = (uint8_t*)&rgb_color;
    }
    #if LED_NEOPIXEL || defined(LED_RGB_RED_PIN)
    if (final_color != NULL) {
        neopixel_write(final_color);
    }
    #else
    (void) final_color;
    #endif
}
#else //  LEDS_NUMBER > 0
void led_state(uint32_t state)
{
#ifdef ST7789_SPI_DISPLAY
  st7789_state(state);
#endif
}
#endif

#ifdef LED_NEOPIXEL

// WS2812B (rev B) timing is 0.4 and 0.8 us
#define MAGIC_T0H               6UL | (0x8000) // 0.375us
#define MAGIC_T1H              13UL | (0x8000) // 0.8125us
#define CTOPVAL                20UL            // 1.25us

#define BYTE_PER_PIXEL  3

static uint16_t pixels_pattern[NEOPIXELS_NUMBER*BYTE_PER_PIXEL * 8 + 2];

// use PWM1 for neopixel
void neopixel_init(void)
{
  // To support both the SoftDevice + Neopixels we use the EasyDMA
  // feature from the NRF25. However this technique implies to
  // generate a pattern and store it on the memory. The actual
  // memory used in bytes corresponds to the following formula:
  //              totalMem = numBytes*8*2+(2*2)
  // The two additional bytes at the end are needed to reset the
  // sequence.
  NRF_PWM_Type* pwm = NRF_PWM1;

  // Set the wave mode to count UP
  // Set the PWM to use the 16MHz clock
  // Setting of the maximum count
  // but keeping it on 16Mhz allows for more granularity just
  // in case someone wants to do more fine-tuning of the timing.
  nrf_pwm_configure(pwm, NRF_PWM_CLK_16MHz, NRF_PWM_MODE_UP, CTOPVAL);

  // Disable loops, we want the sequence to repeat only once
  nrf_pwm_loop_set(pwm, 0);

  // On the "Common" setting the PWM uses the same pattern for the
  // for supported sequences. The pattern is stored on half-word of 16bits
  nrf_pwm_decoder_set(pwm, PWM_DECODER_LOAD_Common, PWM_DECODER_MODE_RefreshCount);

  // The following settings are ignored with the current config.
  nrf_pwm_seq_refresh_set(pwm, 0, 0);
  nrf_pwm_seq_end_delay_set(pwm, 0, 0);

  // The Neopixel implementation is a blocking algorithm. DMA
  // allows for non-blocking operation. To "simulate" a blocking
  // operation we enable the interruption for the end of sequence
  // and block the execution thread until the event flag is set by
  // the peripheral.
  //    pwm->INTEN |= (PWM_INTEN_SEQEND0_Enabled<<PWM_INTEN_SEQEND0_Pos);

  // PSEL must be configured before enabling PWM
  nrf_pwm_pins_set(pwm, (uint32_t[] ) { LED_NEOPIXEL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL });

  // Enable the PWM
  nrf_pwm_enable(pwm);
}

void neopixel_teardown(void)
{
  uint8_t rgb[3] = { 0, 0, 0 };

  NRFX_DELAY_US(50);  // wait for previous write is complete

  neopixel_write(rgb);
  NRFX_DELAY_US(50);  // wait for this write

  pwm_teardown(NRF_PWM1);
}

// write 3 bytes color RGB to built-in neopixel
void neopixel_write (uint8_t *pixels)
{
  // convert RGB to GRB
  uint8_t grb[BYTE_PER_PIXEL] = {pixels[1], pixels[2], pixels[0]};
  uint16_t pos = 0;    // bit position

  // Set all neopixel to same value
  for (uint16_t n = 0; n < NEOPIXELS_NUMBER; n++ )
  {
    for(uint8_t c = 0; c < BYTE_PER_PIXEL; c++)
    {
      uint8_t const pix = grb[c];

      for ( uint8_t mask = 0x80; mask > 0; mask >>= 1 )
      {
        pixels_pattern[pos] = (pix & mask) ? MAGIC_T1H : MAGIC_T0H;
        pos++;
      }
    }
  }

  // Zero padding to indicate the end of sequence
  pixels_pattern[pos++] = 0 | (0x8000);    // Seq end
  pixels_pattern[pos++] = 0 | (0x8000);    // Seq end

  NRF_PWM_Type* pwm = NRF_PWM1;

  nrf_pwm_seq_ptr_set(pwm, 0, pixels_pattern);
  nrf_pwm_seq_cnt_set(pwm, 0, sizeof(pixels_pattern)/2);
  nrf_pwm_event_clear(pwm, NRF_PWM_EVENT_SEQEND0);
  nrf_pwm_task_trigger(pwm, NRF_PWM_TASK_SEQSTART0);

  // blocking wait for sequence complete
  while( !nrf_pwm_event_check(pwm, NRF_PWM_EVENT_SEQEND0) ) {}
  nrf_pwm_event_clear(pwm, NRF_PWM_EVENT_SEQEND0);
}
#endif

#if defined(LED_RGB_RED_PIN) && defined(LED_RGB_GREEN_PIN) && defined(LED_RGB_BLUE_PIN)

#ifdef LED_SECONDARY_PIN
#error "Cannot use secondary LED at the same time as an RGB status LED."
#endif

#define LED_RGB_RED   1
#define LED_RGB_BLUE  2
#define LED_RGB_GREEN 3

void neopixel_init(void)
{
  led_pwm_init(LED_RGB_RED, LED_RGB_RED_PIN);
  led_pwm_init(LED_RGB_GREEN, LED_RGB_GREEN_PIN);
  led_pwm_init(LED_RGB_BLUE, LED_RGB_BLUE_PIN);
}

void neopixel_teardown(void)
{
  uint8_t rgb[3] = { 0, 0, 0 };
  neopixel_write(rgb);
  nrf_gpio_cfg_default(LED_RGB_RED_PIN);
  nrf_gpio_cfg_default(LED_RGB_GREEN_PIN);
  nrf_gpio_cfg_default(LED_RGB_BLUE_PIN);
}

// write 3 bytes color to a built-in neopixel
void neopixel_write (uint8_t *pixels)
{
  led_pwm_duty_cycle(LED_RGB_RED, pixels[2]);
  led_pwm_duty_cycle(LED_RGB_GREEN, pixels[1]);
  led_pwm_duty_cycle(LED_RGB_BLUE, pixels[0]);
}
#endif

#ifdef ST7789_SPI_DISPLAY

#define SPIx NRF_SPI0
#define SPI_MODE NRF_SPI_MODE_3
#define SPI_SCK 2
#define SPI_MOSI 3
#define DISP_SS 25
#define DISP_DC 18
#define DISP_RESET 26
#define BACKLIGHT 14 /* lowest level */

void spi_init(void)
{
  nrf_gpio_pin_write(SPI_SCK, SPI_MODE >= 2);
  nrf_gpio_cfg(SPI_SCK, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_CONNECT,
               NRF_GPIO_PIN_NOPULL, NRF_GPIO_PIN_S0S1, NRF_GPIO_PIN_NOSENSE);
  nrf_gpio_pin_clear(SPI_MOSI);
  nrf_gpio_cfg_output(SPI_MOSI);


  nrf_spi_pins_set(SPIx, SPI_SCK, SPI_MOSI, NRF_SPI_PIN_NOT_CONNECTED);
  nrf_spi_frequency_set(SPIx, NRF_SPI_FREQ_8M);
  nrf_spi_configure(SPIx, SPI_MODE, NRF_SPI_BIT_ORDER_MSB_FIRST);

  nrf_spi_enable(SPIx);
}

void spi_teardown(void)
{
  /* no need to tear down SCK and MOSI - output pins can be left alone */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
  nrf_spi_disable(SPIx);
  nrf_gpio_cfg_default(SPI_MOSI);
  nrf_gpio_cfg_default(SPI_SCK);
}

void spi_write(const uint8_t *data, unsigned len)
{
  const uint8_t *endp = data + len;

  /* paranoid... but worthwhile due to the havoc this could cause */
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);

  /* send first character */
  nrf_spi_txd_set(SPIx, *data++);

  /* TXD is double buffers so we can xmit and then poll for the event */
  while (data < endp) {
    nrf_spi_txd_set(SPIx, *data++);

    while (!nrf_spi_event_check(SPIx, NRF_SPI_EVENT_READY)) {}
    nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
    (void) nrf_spi_rxd_get(SPIx);
  }

  /* wait for the final character */
  while (!nrf_spi_event_check(SPIx, NRF_SPI_EVENT_READY)) {}
  nrf_spi_event_clear(SPIx, NRF_SPI_EVENT_READY);
  (void) nrf_spi_rxd_get(SPIx);
}

#define NOP      0x00
#define SWRESET  0x01
#define SLPOUT   0x11
#define NORON    0x13
#define INVOFF   0x20
#define INVON    0x21
#define DISPON   0x29
#define CASET    0x2a
#define RASET    0x2b
#define RAMWR    0x2c
#define COLMOD   0x3a
#define MADCTL   0x36

struct st7789_cmd {
  uint8_t cmd;
  const uint8_t *data;
  uint8_t len;
};

const static struct st7789_cmd st7789_init_data[] = {
  { COLMOD,   (uint8_t *) "\x05", 1 }, // MCU will send 16-bit RGB565
  { MADCTL,   (uint8_t *) "\x00", 1 }, // Left to right, top to bottom
  //{ INVOFF,   NULL }, // Results in odd palette
  { INVON,    NULL },
  { NORON,    NULL },
  { NOP,      NULL },
};


static void st7789_send(uint8_t cmd, const uint8_t *data, unsigned len)
{
  nrf_gpio_pin_clear(DISP_SS);

  // this means nop cannot be sent... but helps with big RAMWR dumps
  if (cmd) {
    nrf_gpio_pin_clear(DISP_DC);
    spi_write(&cmd, 1);
  }

  if (data) {
    nrf_gpio_pin_set(DISP_DC);
    spi_write(data, len);
  }

  nrf_gpio_pin_set(DISP_SS);
}

void st7789_state(int state)
{
  bool topclip = true;
  bool bottomclip = true;

  switch(state) {
  case STATE_BOOTLOADER_STARTED:
    break;
  case STATE_BLE_DISCONNECTED:
    topclip = false;
    /* fallthrough */
  case STATE_BLE_CONNECTED:
    bottomclip = false;
    break;
  default:
    return;
  }

  uint8_t linebuffer[2*240];

  // 1-bit RLE, generated from res/pinedfu.png, 1085 bytes
  const uint8_t rle[] = {
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0x14, 0x8, 0x8, 0x3,
    0x2b, 0x3, 0xac, 0xd, 0x6, 0x3, 0x2b, 0x3, 0xab, 0xe, 0x6, 0x3,
    0x2b, 0x3, 0xab, 0x4, 0x8, 0x2, 0x6, 0x3, 0x2b, 0x3, 0xaa, 0x4,
    0x11, 0x3, 0x2b, 0x3, 0xaa, 0x3, 0x10, 0xb, 0x6, 0x7, 0xb, 0x3,
    0x3, 0x4, 0x1, 0xb, 0xa4, 0x3, 0x10, 0xb, 0x4, 0xb, 0x9, 0x3,
    0x1, 0x6, 0x1, 0xb, 0xa4, 0x3, 0x10, 0xb, 0x4, 0xc, 0x8, 0xa,
    0x1, 0xb, 0xa4, 0x4, 0x11, 0x3, 0xa, 0x2, 0x7, 0x4, 0x7, 0x6,
    0x7, 0x3, 0xab, 0x5, 0xf, 0x3, 0x14, 0x3, 0x7, 0x4, 0x9, 0x3,
    0xac, 0x9, 0xa, 0x3, 0x15, 0x3, 0x6, 0x4, 0x9, 0x3, 0xad, 0xa,
    0x8, 0x3, 0x15, 0x3, 0x6, 0x3, 0xa, 0x3, 0xb0, 0x8, 0x7, 0x3,
    0xd, 0xb, 0x6, 0x3, 0xa, 0x3, 0xb4, 0x5, 0x6, 0x3, 0xb, 0xd,
    0x6, 0x3, 0xa, 0x3, 0xb6, 0x4, 0x5, 0x3, 0xa, 0xe, 0x6, 0x3,
    0xa, 0x3, 0xb7, 0x3, 0x5, 0x3, 0x9, 0x5, 0x7, 0x3, 0x6, 0x3,
    0xa, 0x3, 0xb7, 0x3, 0x5, 0x3, 0x9, 0x3, 0x9, 0x3, 0x6, 0x3,
    0xa, 0x3, 0xb7, 0x3, 0x5, 0x3, 0x9, 0x3, 0x9, 0x3, 0x6, 0x3,
    0xa, 0x3, 0xb6, 0x4, 0x5, 0x3, 0x9, 0x3, 0x8, 0x4, 0x6, 0x3,
    0xa, 0x3, 0xaa, 0x2, 0x9, 0x4, 0x6, 0x4, 0x8, 0x4, 0x5, 0x6,
    0x6, 0x3, 0xa, 0x4, 0xa9, 0xf, 0x7, 0x8, 0x4, 0xe, 0x6, 0x3,
    0xb, 0x8, 0xa4, 0xe, 0x8, 0x8, 0x5, 0x9, 0x1, 0x3, 0x6, 0x3,
    0xb, 0x8, 0xa6, 0x9, 0xd, 0x6, 0x6, 0x6, 0x3, 0x3, 0x6, 0x3,
    0xd, 0x6, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0x2c, 0x2, 0xed, 0x4,
    0xa6, 0x1, 0x44, 0x6, 0xa4, 0x3, 0x42, 0x8, 0xa2, 0x5, 0x40, 0xa,
    0xa0, 0x7, 0x3e, 0xc, 0x9e, 0x7, 0x3e, 0xe, 0x93, 0x1, 0x8, 0x7,
    0x3e, 0x10, 0x92, 0x2, 0x6, 0x7, 0x3e, 0x12, 0x91, 0x3, 0x4, 0x7,
    0x3e, 0x14, 0x90, 0x4, 0x2, 0x7, 0x3e, 0x16, 0x8f, 0xc, 0x3e, 0x18,
    0x8e, 0xb, 0x3f, 0x18, 0x8e, 0xa, 0x42, 0x14, 0x90, 0x9, 0x37, 0x2,
    0xc, 0x10, 0xc, 0x2, 0x84, 0x9, 0x36, 0x5, 0xc, 0xc, 0xc, 0x5,
    0x83, 0xa, 0x35, 0x7, 0xc, 0x8, 0xc, 0x7, 0x83, 0xb, 0x33, 0xa,
    0xc, 0x4, 0xc, 0xa, 0x82, 0xc, 0x32, 0xc, 0x18, 0xc, 0x82, 0xd,
    0x30, 0xf, 0x14, 0xf, 0xbe, 0x11, 0x10, 0x11, 0xbe, 0xf, 0x14, 0xf,
    0xbd, 0xe, 0x18, 0xe, 0xbc, 0xc, 0xc, 0x4, 0xc, 0xc, 0xbb, 0xb,
    0xc, 0x8, 0xc, 0xb, 0xba, 0x9, 0xb, 0xe, 0xb, 0x9, 0xba, 0x7,
    0xb, 0x12, 0xb, 0x7, 0xb9, 0x6, 0xa, 0x18, 0xa, 0x6, 0xb8, 0x3,
    0xb, 0x1c, 0xa, 0x4, 0xc4, 0x20, 0xa, 0x2, 0xc2, 0x24, 0xc9, 0x2a,
    0xc4, 0x2e, 0xbf, 0x34, 0xba, 0x38, 0xb7, 0x3a, 0xab, 0x1, 0xb, 0x38,
    0xa, 0x2, 0xa0, 0x4, 0xa, 0x34, 0xa, 0x4, 0xa0, 0x6, 0xb, 0x2e,
    0xb, 0x6, 0xa0, 0x9, 0xa, 0x2a, 0xa, 0x9, 0xa0, 0xb, 0xb, 0x24,
    0xb, 0xb, 0xa0, 0xd, 0xb, 0x20, 0xb, 0xd, 0xa0, 0xf, 0xb, 0x1c,
    0xb, 0xf, 0xa0, 0x12, 0xa, 0x18, 0xa, 0x12, 0xa0, 0x14, 0xb, 0x12,
    0xb, 0x14, 0xa0, 0x16, 0xb, 0xe, 0xb, 0x16, 0xa0, 0x18, 0xc, 0x8,
    0xc, 0x18, 0xa0, 0x1b, 0xb, 0x4, 0xb, 0x1b, 0xa0, 0x1d, 0x16, 0x1d,
    0xa0, 0x20, 0x10, 0x20, 0xa0, 0x20, 0x10, 0x20, 0xa0, 0x1f, 0x12, 0x1f,
    0xa0, 0x1c, 0x17, 0x1d, 0xa0, 0x1a, 0xc, 0x4, 0xb, 0x1b, 0xa0, 0x18,
    0xc, 0x8, 0xc, 0x18, 0xa0, 0x16, 0xb, 0xf, 0xa, 0x16, 0xa0, 0x13,
    0xc, 0x12, 0xc, 0x13, 0xa0, 0x11, 0xc, 0x16, 0xb, 0x12, 0xa0, 0xf,
    0xc, 0x1a, 0xc, 0xf, 0xa0, 0xd, 0xb, 0x20, 0xb, 0xd, 0xa0, 0xa,
    0xc, 0x24, 0xc, 0xa, 0xa0, 0x8, 0xb, 0x2a, 0xb, 0x8, 0xa0, 0x6,
    0xb, 0x2e, 0xb, 0x6, 0xa0, 0x4, 0xb, 0x32, 0xb, 0x4, 0xa0, 0x1,
    0xc, 0x36, 0xc, 0x1, 0xaa, 0x3c, 0xb2, 0x40, 0xa0, 0x2, 0xd, 0x42,
    0xc, 0x2, 0x92, 0x3, 0xd, 0x3e, 0xd, 0x3, 0x93, 0x5, 0xc, 0x3a,
    0xc, 0x5, 0x94, 0x7, 0xc, 0x36, 0xc, 0x7, 0x95, 0x9, 0xb, 0x31,
    0xc, 0x9, 0x96, 0xb, 0xc, 0x2c, 0xc, 0xb, 0x96, 0xe, 0xb, 0x28,
    0xb, 0xd, 0x98, 0xf, 0xc, 0x22, 0xc, 0xf, 0x98, 0x12, 0xb, 0x1e,
    0xb, 0x12, 0x99, 0x13, 0xb, 0x19, 0xc, 0x13, 0x9a, 0x15, 0xb, 0x15,
    0xc, 0x15, 0x9b, 0x17, 0xb, 0x10, 0xb, 0x17, 0x9c, 0x19, 0xb, 0xc,
    0xb, 0x19, 0x9c, 0x1b, 0xc, 0x6, 0xc, 0x1a, 0x9e, 0x1d, 0xb, 0x2,
    0xb, 0x1d, 0x9e, 0x1f, 0x14, 0x1f, 0x9f, 0x21, 0xf, 0x20, 0xa0, 0x20,
    0x10, 0x20, 0xa1, 0x1e, 0x12, 0x1e, 0xa2, 0x1b, 0xb, 0x2, 0xb, 0x1b,
    0xa3, 0x18, 0xa, 0x7, 0xb, 0x18, 0xa4, 0x15, 0xc, 0xa, 0xb, 0x16,
    0xa4, 0x14, 0xa, 0x10, 0xa, 0x14, 0xa5, 0x10, 0xb, 0x14, 0xb, 0x10,
    0xa6, 0xe, 0xa, 0x1a, 0xa, 0xe, 0xa7, 0xa, 0xb, 0x1e, 0xb, 0xa,
    0xa8, 0x8, 0xb, 0x22, 0xb, 0x8, 0xa9, 0x5, 0xb, 0x26, 0xb, 0x5,
    0xaa, 0x3, 0xa, 0x2c, 0xa, 0x3, 0xb5, 0x30, 0xbe, 0x34, 0xba, 0x38,
    0xb7, 0x3a, 0xb8, 0x36, 0xbc, 0x32, 0xc0, 0x2e, 0xc4, 0x2a, 0xc9, 0x24,
    0xce, 0x20, 0xd2, 0x1c, 0xd6, 0x18, 0xdb, 0x12, 0xe0, 0xe, 0xe4, 0xa,
    0xe8, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6,
    0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0xea, 0x6,
    0xea, 0x6, 0xea, 0x6, 0xea, 0x6, 0x52, 0x6, 0xe7, 0xc, 0xe2, 0x10,
    0xde, 0x14, 0xdb, 0x16, 0xd9, 0x18, 0xd7, 0x1a, 0xd5, 0x1c, 0xd4, 0xb,
    0x3, 0xe, 0xd3, 0xc, 0x4, 0xe, 0xd2, 0xc, 0x6, 0xc, 0xd1, 0xd,
    0x7, 0xc, 0xd0, 0xd, 0x8, 0xb, 0xd0, 0xd, 0x9, 0xa, 0xd0, 0xd,
    0x4, 0x1, 0x6, 0x8, 0xd0, 0xd, 0x4, 0x2, 0x6, 0x7, 0xd0, 0xd,
    0x4, 0x3, 0x6, 0x6, 0xd0, 0x6, 0x3, 0x4, 0x4, 0x4, 0x5, 0x6,
    0xd0, 0x6, 0x4, 0x3, 0x4, 0x3, 0x6, 0x6, 0xd0, 0x6, 0x6, 0x1,
    0x4, 0x2, 0x6, 0x7, 0xd0, 0x7, 0xa, 0x1, 0x6, 0x8, 0xd0, 0x8,
    0xf, 0x9, 0xd0, 0xa, 0xb, 0xb, 0xd0, 0xb, 0x9, 0xc, 0xd0, 0xc,
    0x7, 0xd, 0xd0, 0xc, 0x7, 0xd, 0xd0, 0xb, 0x9, 0xc, 0xd0, 0xa,
    0xb, 0xb, 0xd0, 0x8, 0xf, 0x9, 0xd0, 0x7, 0xa, 0x1, 0x6, 0x8,
    0xd0, 0x6, 0x6, 0x1, 0x4, 0x2, 0x6, 0x7, 0xd0, 0x6, 0x4, 0x3,
    0x4, 0x3, 0x6, 0x6, 0xd0, 0x6, 0x3, 0x4, 0x4, 0x4, 0x5, 0x6,
    0xd0, 0xd, 0x4, 0x3, 0x6, 0x6, 0xd0, 0xd, 0x4, 0x2, 0x6, 0x7,
    0xd0, 0xd, 0x4, 0x1, 0x6, 0x8, 0xd0, 0xd, 0x9, 0xa, 0xd0, 0xd,
    0x8, 0xb, 0xd0, 0xd, 0x7, 0xc, 0xd1, 0xc, 0x6, 0xc, 0xd2, 0xc,
    0x4, 0xe, 0xd3, 0xb, 0x3, 0xe, 0xd4, 0x1c, 0xd5, 0x1a, 0xd7, 0x18,
    0xd9, 0x16, 0xdb, 0x14, 0xde, 0x10, 0xe2, 0xc, 0xe7, 0x6, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0,
    0xff, 0x0, 0xff, 0x0, 0x85,
  };

  // draw 240 x 240
  st7789_send(CASET, (uint8_t *) "\x00\x00\x00\xef", 4);
  st7789_send(RASET, (uint8_t *) "\x00\x00\x00\xef", 4);
  st7789_send(RAMWR, NULL, 0);

  int y = 0;
  uint16_t bp = 0;
  uint16_t fg = 0xffff;
  const uint16_t bg = 0;
  uint16_t color = bg;

  for (int i=0; i<sizeof(rle); i++) {
    uint8_t rl = rle[i];
    while (rl) {
      linebuffer[bp] = color >> 8;
      linebuffer[bp+1] = color & 0xff;
      bp += 2;
      rl -= 1;

      if (bp >= sizeof(linebuffer)) {
	/* currently we have a single image which we clip to show different modes */
	if (topclip) {
	  if (y < 48)
	    memset(linebuffer, 0, sizeof(linebuffer));
	  else
	    memset(linebuffer, 0, 64*2);
	}
	if (bottomclip) {
	  memset(linebuffer+2*(240-64), 0, 64*2);
	} else if (y == 179 && state == STATE_BLE_CONNECTED) {
	  fg = 0x18ff;
	}

        st7789_send(NOP, linebuffer, sizeof(linebuffer));
        bp = 0;
	y += 1;
      }
    }

    if (color == bg)
      color = fg;
    else
      color = bg;
  }
}


void st7789_init(void)
{
  nrf_gpio_pin_set(DISP_SS);
  nrf_gpio_cfg_output(DISP_SS);
  spi_init();

  nrf_gpio_cfg_output(DISP_DC);

  /* deliver a reset */
  nrf_gpio_pin_clear(DISP_RESET);
  nrf_gpio_cfg_output(DISP_RESET);
  NRFX_DELAY_MS(10);
  nrf_gpio_pin_set(DISP_RESET);
  NRFX_DELAY_MS(125);

  /* initialize the display */
  st7789_send(SLPOUT, NULL, 0);
  NRFX_DELAY_MS(10);
  for (const struct st7789_cmd *i = st7789_init_data; i->cmd != NOP; i++)
    st7789_send(i->cmd, i->data, i->len);

  /* draw the initial screen content. this will take ~116ms so, with the
   * delay after the SLPOUT we will have complete the SLPOUT/SLPIN lock
   * out period during the update.
   */
  st7789_state(STATE_BOOTLOADER_STARTED);


  /* enable the display and light the backlight */
  st7789_send(DISPON, NULL, 0);
  nrf_gpio_pin_clear(BACKLIGHT);
  nrf_gpio_cfg_output(BACKLIGHT);
}

void st7789_teardown(void)
{
  nrf_gpio_cfg_default(DISP_DC);
  nrf_gpio_cfg_default(DISP_RESET);
  nrf_gpio_cfg_default(BACKLIGHT);

  spi_teardown();
  nrf_gpio_cfg_default(DISP_SS);
}
#endif

void wdt_init(void)
{
  // 1 => keep running during a sleep, stop during SWD debug
  // 9 => run during sleep and SWD debug (e.g. most robust but
  //      makes debugger attachment difficult)
  nrf_wdt_behaviour_set(NRF_WDT, 9);

  // timeout after 5 seconds
  nrf_wdt_reload_value_set(NRF_WDT, 5 * 32768);

  // enable the 0th channel
  nrf_wdt_reload_request_enable(NRF_WDT, NRF_WDT_RR0);

  // set it running
  nrf_wdt_task_trigger(NRF_WDT, NRF_WDT_TASK_START);
}
