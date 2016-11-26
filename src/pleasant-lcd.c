#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "pleasant-spi.h"
#include "pleasant-timer.h"
#include "pleasant-lcd.h"

uint16_t lcd_width = LCD_WIDTH;
uint16_t lcd_height = LCD_HEIGHT;
enum spi_clock_speed lcd_spi_clock_speed;

/* Pins -------------------------------------------------------------------- */

#define LCD_PIN_DDR_RST    DDRB
#define LCD_PIN_PORT_RST   PORTB
#define LCD_PIN_RST        PORTB0

#define LCD_PIN_DDR_CS     DDRD
#define LCD_PIN_PORT_CS    PORTD
#define LCD_PIN_CS         PORTD7

#define LCD_PIN_DDR_LED    DDRB
#define LCD_PIN_PORT_LED   PORTB
#define LCD_PIN_LED        PORTB1

#define LCD_PIN_DDR_ADSCS  DDRD
#define LCD_PIN_PORT_ADSCS PORTD
#define LCD_PIN_ADSCS      PORTD6

#define LCD_PIN_DDR_MOSI   DDRB
#define LCD_PIN_PORT_MOSI  PORTB
#define LCD_PIN_MOSI       PORTB3

#define LCD_PIN_DDR_SCK    DDRB
#define LCD_PIN_PORT_SCK   PORTB
#define LCD_PIN_SCK        PORTB5

static void lcd_disable_rst() { LCD_PIN_PORT_RST |= (1 << LCD_PIN_RST); }
static void lcd_enable_rst() { LCD_PIN_PORT_RST &= ~(1 << LCD_PIN_RST); }
static void lcd_disable_cs() { LCD_PIN_PORT_CS |= (1 << LCD_PIN_CS); }
static void lcd_enable_cs() { LCD_PIN_PORT_CS &= ~(1 << LCD_PIN_CS); }
static void lcd_disable_adscs() { LCD_PIN_PORT_ADSCS |= (1 << LCD_PIN_ADSCS); }
static void lcd_enable_adscs() { LCD_PIN_PORT_ADSCS &= ~(1 << LCD_PIN_ADSCS); }

/* SPI --------------------------------------------------------------------- */

static void lcd_speed_down_spi() {
  spi_configure(SPI_CLOCK_SPEED_DIV_8, SPI_BIT_ORDER_MSB_FIRST);
}

static void lcd_configure_spi() {
  spi_configure(lcd_spi_clock_speed, SPI_BIT_ORDER_MSB_FIRST);
}

static void lcd_start_transmission() { lcd_enable_cs(); }
static void lcd_stop_transmission() { lcd_disable_cs(); }

static void lcd_send_9th_bit(bool enabled) {
  if (enabled) LCD_PIN_PORT_MOSI |= (1 << LCD_PIN_MOSI);
  else LCD_PIN_PORT_MOSI &= ~(1 << LCD_PIN_MOSI);

  LCD_PIN_PORT_SCK &= ~(1 << LCD_PIN_SCK);
  SPCR &= ~(1 << SPE);          /* disable SPI */
  LCD_PIN_PORT_SCK |= (1 << LCD_PIN_SCK);
  SPCR |= (1 << SPE);           /* enable SPI */
}

static void lcd_send_raw(uint8_t data, bool is_command) {
  lcd_send_9th_bit(!is_command);
  spi_transfer(data);
}

static void lcd_send_command(enum lcd_command command) {
  lcd_send_raw(command, true);
}

static void lcd_send_data(uint8_t data) {
  lcd_send_raw(data, false);
}

static void lcd_send_data16(uint16_t data) {
  lcd_send_raw(data >> 8, false);
  lcd_send_raw(data, false);
}

static uint8_t lcd_read_spi() {
  return spi_transfer(0);
}

/* Reset ------------------------------------------------------------------- */

const uint8_t initdataQT9[] PROGMEM = {
  0x40| 1, LCD_COMMAND_POWER_CTRLB,
  0x80| 3, 0x00, 0x83, 0x30,    /* 0x83 0x81 0xAA */
  0x40| 1, LCD_COMMAND_POWERON_SEQ_CTRL,
  0x80| 4, 0x64, 0x03, 0x12, 0x81, /* 0x64 0x67 */
  0x40| 1, LCD_COMMAND_DRV_TIMING_CTRLA,
  0x80| 3, 0x85, 0x01, 0x79,    /* 0x79 0x78 */
  0x40| 1, LCD_COMMAND_POWER_CTRLA,
  0x80| 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
  0x40| 1, LCD_COMMAND_PUMP_RATIO_CTRL,
  0x80| 1, 0x20,
  0x40| 1, LCD_COMMAND_DRV_TIMING_CTRLB,
  0x80| 2, 0x00, 0x00,
  0x40| 1, LCD_COMMAND_POWER_CTRL1,
  0x80| 1, 0x26,                /* 0x26 0x25 */
  0x40| 1, LCD_COMMAND_POWER_CTRL2,
  0x80| 1, 0x11,
  0x40| 1, LCD_COMMAND_VCOM_CTRL1,
  0x80| 2, 0x35, 0x3E,
  0x40| 1, LCD_COMMAND_VCOM_CTRL2,
  0x80| 1, 0xBE,                /* 0xBE 0x94 */
  0x40| 1, LCD_COMMAND_FRAME_CTRL,
  0x80| 2, 0x00, 0x1B,          /* 0x1B 0x70 */
  0x40| 1, LCD_COMMAND_ENABLE_3G,
  0x80| 1, 0x08,                /* 0x08 0x00 */
  0x40| 1, LCD_COMMAND_GAMMA,
  0x80| 1, 0x01,                /* G2.2 */
  0x40| 1, LCD_COMMAND_POS_GAMMA,
  0x80|15, 0x1F, 0x1A, 0x18, 0x0A, 0x0F, 0x06, 0x45, 0x87, 0x32, 0x0A, 0x07, 0x02, 0x07, 0x05, 0x00,
  0x40| 1, LCD_COMMAND_NEG_GAMMA,
  0x80|15, 0x00, 0x25, 0x27, 0x05, 0x10, 0x09, 0x3A, 0x78, 0x4D, 0x05, 0x18, 0x0D, 0x38, 0x3A, 0x1F,
  0x40| 1, LCD_COMMAND_DISPLAY_CTRL,
  0x80| 4, 0x0A, 0x82, 0x27, 0x00,
  0x40| 1, LCD_COMMAND_ENTRY_MODE,
  0x80| 1, 0x07,
  0x40| 1, LCD_COMMAND_PIXEL_FORMAT,
  0x80| 1, 0x55,                /* 16bit */
  0x40| 1, LCD_COMMAND_MEMACCESS_CTRL,
  0x80| 1, (LCD_MEMORY_ACCESS_CONTROL_BGR
            | LCD_MEMORY_ACCESS_CONTROL_MX
            | LCD_MEMORY_ACCESS_CONTROL_MY
            | LCD_MEMORY_ACCESS_CONTROL_MV),
  0x40| 1, LCD_COMMAND_COLUMN,
  0x80| 2, 0x00, 0x00,
  0x80| 2, ((LCD_WIDTH - 1) >> 8) & 0xFF, (LCD_WIDTH - 1) & 0xFF,
  0x40| 1, LCD_COMMAND_PAGE,
  0x80| 2, 0x00, 0x00,
  0x80| 2, ((LCD_HEIGHT - 1) >> 8) & 0xFF, (LCD_HEIGHT - 1) & 0xFF,
  0x40| 1, LCD_COMMAND_SLEEPOUT,
  0xFF
};

static void lcd_reset() {
  uint8_t i;
  uint8_t instruction;
  const uint8_t *ptr;

  lcd_disable_cs();
  lcd_enable_rst();
  _delay_ms(50);
  lcd_disable_rst();
  _delay_ms(120);

  lcd_start_transmission();
  lcd_send_command(LCD_COMMAND_DISPLAY_OFF);

  ptr = &initdataQT9[0];
  while (1) {
    instruction = pgm_read_byte(ptr++);

    if (instruction == 0xFF) break;       /* End of data */

    for (i = instruction & 0x3F; i != 0; i--) {
      lcd_send_raw(pgm_read_byte(ptr++), !(instruction & 0x80));
    }
  }

  lcd_send_command(LCD_COMMAND_DISPLAY_ON);
  lcd_stop_transmission();

  lcd_fill_screen(0);
}

/* Control commands -------------------------------------------------------- */

void lcd_init(enum spi_clock_speed clock_speed) {
  /* Timer 1 is used to control the display brightness. */
  timer1_init(TIMER_WAVE_TYPE_PHASE_CORRECT_PWM,
              TIMER_WRAP_TYPE_8_BITS,
              TIMER_CLOCK_SOURCE_DIV_64,
              TIMER_DEFAULT_INTERRUPT,
              TIMER_COMPARE_OUTPUT_MODE_CLEAR,
              TIMER_DEFAULT_COMPARE_OUTPUT_MODE,
              TIMER_DEFAULT_INPUT_CAPTURE_EDGE,
              TIMER_DEFAULT_INPUT_CAPTURE_NOISE_CANCELER);

  /* Initialize LCD */
  LCD_PIN_DDR_RST |= (1 << LCD_PIN_RST);
  LCD_PIN_DDR_LED |= (1 << LCD_PIN_LED);
  LCD_PIN_DDR_CS |= (1 << LCD_PIN_CS);
  lcd_disable_cs();

  /* Initialize ADS */
  LCD_PIN_DDR_ADSCS |= (1 << LCD_PIN_ADSCS);
  lcd_disable_adscs();

  /* Initialize SPI */
  lcd_spi_clock_speed = clock_speed;
  lcd_configure_spi();

  lcd_reset();
  lcd_set_brightness(50);
}

static void lcd_set_area(uint16_t x0,
                         uint16_t y0,
                         uint16_t x1,
                         uint16_t y1) {
  lcd_start_transmission();

  lcd_send_command(LCD_COMMAND_COLUMN);
  lcd_send_data16(x0);
  lcd_send_data16(x1);

  lcd_send_command(LCD_COMMAND_PAGE);
  lcd_send_data16(y0);
  lcd_send_data16(y1);

  lcd_stop_transmission();
}

void lcd_set_orientation(enum lcd_orientation orientation) {
  enum lcd_base_orientation base_orientation
    = (orientation & LCD_ORIENTATION_BASE_ORIENTATION_MASK);

  lcd_width = base_orientation == LCD_BASE_ORIENTATION_LANDSCAPE
    ? LCD_WIDTH : LCD_HEIGHT;
  lcd_height = base_orientation == LCD_BASE_ORIENTATION_LANDSCAPE
    ? LCD_HEIGHT : LCD_WIDTH;

  lcd_start_transmission();
  lcd_send_command(LCD_COMMAND_MEMACCESS_CTRL);
  lcd_send_data(orientation & LCD_ORIENTATION_MEMORY_ACCESS_MASK);
  lcd_stop_transmission();

  lcd_set_area(0, 0, lcd_width - 1, lcd_height - 1);
}

void lcd_set_inverted(bool inverted) {
  lcd_start_transmission();
  lcd_send_command(inverted ? LCD_COMMAND_INV_ON : LCD_COMMAND_INV_OFF);
  lcd_stop_transmission();
}

void lcd_set_brightness(uint8_t brightness) {
  TIMER1_COMPARE_A = (uint16_t)brightness * 255 / 100;
}

/* Drawing ----------------------------------------------------------------- */

static void lcd_start_drawing() {
  lcd_start_transmission();
  lcd_send_command(LCD_COMMAND_WRITE);
}

static void lcd_stop_drawing() {
  lcd_stop_transmission();
}

void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color color) {
  if (x >= lcd_width || y >= lcd_height) return;

  lcd_set_area(x, y, x, y);

  lcd_start_drawing();
  lcd_send_data16(color);
  lcd_stop_drawing();
}

void lcd_fill_screen(uint16_t color) {
  uint32_t i;

  lcd_start_drawing();
  for (i = ((uint32_t)lcd_width * lcd_height); i > 0; i--) {
    lcd_send_data16(color);
  }
  lcd_stop_drawing();
}

void lcd_fill_rect(uint16_t x,
                   uint16_t y,
                   uint16_t w,
                   uint16_t h,
                   lcd_color color) {
  uint32_t i;

  if (x >= lcd_width)      x = lcd_width - 1;
  if (y >= lcd_height)     y = lcd_height - 1;
  if (x + w >= lcd_width)  w = lcd_width - x;
  if (y + h >= lcd_height) h = lcd_height - y;

  lcd_set_area(x, y, x + w - 1, x + h - 1);

  lcd_start_drawing();
  for (i = w * h; i > 0; i--) lcd_send_data16(color);
  lcd_stop_drawing();
}

/* Touch ------------------------------------------------------------------- */

static uint8_t lcd_touch_get_pressure() {
  uint8_t z1, z2;

  spi_transfer(LCD_ADS_START
               | LCD_ADS_REFERENCE_DIFFERENTIAL
               | LCD_ADS_CONVERSION_8_BITS
               | LCD_ADS_CHANNEL_Z1_POSITION);
  z1 = lcd_read_spi();

  spi_transfer(LCD_ADS_START
               | LCD_ADS_REFERENCE_DIFFERENTIAL
               | LCD_ADS_CONVERSION_8_BITS
               | LCD_ADS_CHANNEL_Z2_POSITION);
  z2 = lcd_read_spi();

  return (z1 & 0x7F) + ((255 - z2) & 0x7F);
}

static uint16_t lcd_touch_get_position(enum lcd_ads_channel channel) {
  uint8_t a1, a2, /* b1, */ b2;

  spi_transfer(LCD_ADS_START
               | LCD_ADS_REFERENCE_DIFFERENTIAL
               | LCD_ADS_CONVERSION_12_BITS
               | channel);
  a1 = lcd_read_spi();
  /* b1 = */ lcd_read_spi();
  spi_transfer(LCD_ADS_START
               | LCD_ADS_REFERENCE_DIFFERENTIAL
               | LCD_ADS_CONVERSION_12_BITS
               | channel);
  a2 = lcd_read_spi();
  b2 = lcd_read_spi();

  if (a1 == a2) {
    return ((a2 << 2) | (b2 >> 6));
  }
  return 0;
}

bool lcd_touch_read(uint16_t *touch_pressure,
                    uint16_t *touch_x,
                    uint16_t *touch_y) {
  uint16_t pressure;
  uint16_t x = 0, y = 0;
  bool x_consistent = false, y_consistent = false;

  lcd_speed_down_spi();
  lcd_enable_adscs();

  pressure = lcd_touch_get_pressure();

  if (pressure > LCD_TOUCH_REQUIRED_PRESSURE) {
    x = lcd_touch_get_position(LCD_ADS_CHANNEL_X_POSITION);
    x_consistent = x != 0;

    if (x_consistent) {
      y = lcd_touch_get_position(LCD_ADS_CHANNEL_Y_POSITION);
      y_consistent = y != 0;
    }
  }

  lcd_disable_adscs();
  lcd_configure_spi();

  if (x_consistent && y_consistent) {
    if (touch_pressure) *touch_pressure = pressure;
    if (touch_x)        *touch_x = x;
    if (touch_y)        *touch_y = y;
    return true;
  }

  return false;
}
