#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include "pleasant-spi.h"
#include "pleasant-timer.h"
#include "pleasant-lcd.h"

uint16_t lcd_width = LCD_WIDTH;
uint16_t lcd_height = LCD_HEIGHT;
enum lcd_orientation lcd_current_orientation = LCD_ORIENTATION_0;
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

  lcd_current_orientation = orientation;
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

void lcd_batch_start(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
  lcd_set_area(x, y, x + w - 1, y + h - 1);

  lcd_start_transmission();
  lcd_send_command(LCD_COMMAND_WRITE);
}

void lcd_batch_draw(lcd_color color) {
  lcd_send_data16(color);
}

void lcd_batch_stop() {
  lcd_stop_transmission();
}

void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color color) {
  if (x >= lcd_width || y >= lcd_height) return;

  lcd_batch_start(x, y, 1, 1);
  lcd_batch_draw(color);
  lcd_batch_stop();
}

void lcd_fill_screen(uint16_t color) {
  uint32_t i;

  lcd_batch_start(0, 0, lcd_width, lcd_height);
  for (i = ((uint32_t)lcd_width * lcd_height); i > 0; i--) {
    lcd_batch_draw(color);
  }
  lcd_batch_stop();
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

  lcd_batch_start(x, y, w, h);
  for (i = (uint32_t)w * h; i > 0; i--) lcd_batch_draw(color);
  lcd_batch_stop();
}

/* Touch ------------------------------------------------------------------- */

struct calibration_point {
  uint32_t x;
  uint32_t y;
};

struct calibration_matrix {
  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t e;
  uint32_t f;
  uint32_t div;
};

struct calibration_matrix tp_matrix;

uint16_t lcd_x, lcd_y, lcd_z;
uint16_t tp_x, tp_y;
uint16_t tp_last_x, tp_last_y;

#define CAL_POINT_X1   20
#define CAL_POINT_Y1   20
#define CAL_POINT1     {CAL_POINT_X1, CAL_POINT_Y1}

#define CAL_POINT_X2   LCD_WIDTH - 20 /* 300 */
#define CAL_POINT_Y2   LCD_HEIGHT / 2 /* 120 */
#define CAL_POINT2     {CAL_POINT_X2, CAL_POINT_Y2}

#define CAL_POINT_X3   LCD_WIDTH / 2   /* 160 */
#define CAL_POINT_Y3   LCD_HEIGHT - 20 /* 220 */
#define CAL_POINT3     {CAL_POINT_X3, CAL_POINT_Y3}

static void lcd_touch_render_target_square(uint16_t x,
                                           uint16_t y,
                                           uint16_t size,
                                           lcd_color color,
                                           lcd_color background_color) {
  lcd_fill_rect(x - (size / 2) - 1,
                y - (size / 2) - 1,
                size,
                size,
                color);
  size -= 2;
  lcd_fill_rect(x - (size / 2) - 1,
                y - (size / 2) - 1,
                size,
                size,
                background_color);
};

static void lcd_touch_set_calibration(struct calibration_point *lcd,
                                      struct calibration_point *tp) {
  tp_matrix.div = ((tp[0].x - tp[2].x) * (tp[1].y - tp[2].y)) -
                  ((tp[1].x - tp[2].x) * (tp[0].y - tp[2].y));

  tp_matrix.a = ((lcd[0].x - lcd[2].x) * (tp[1].y - tp[2].y)) -
                ((lcd[1].x - lcd[2].x) * (tp[0].y - tp[2].y));

  tp_matrix.b = ((tp[0].x - tp[2].x) * (lcd[1].x - lcd[2].x)) -
                ((lcd[0].x - lcd[2].x) * (tp[1].x - tp[2].x));

  tp_matrix.c = (tp[2].x * lcd[1].x - tp[1].x * lcd[2].x) * tp[0].y +
                (tp[0].x * lcd[2].x - tp[2].x * lcd[0].x) * tp[1].y +
                (tp[1].x * lcd[0].x - tp[0].x * lcd[1].x) * tp[2].y;

  tp_matrix.d = ((lcd[0].y - lcd[2].y) * (tp[1].y - tp[2].y)) -
                ((lcd[1].y - lcd[2].y) * (tp[0].y - tp[2].y));

  tp_matrix.e = ((tp[0].x - tp[2].x) * (lcd[1].y - lcd[2].y)) -
                ((lcd[0].y - lcd[2].y) * (tp[1].x - tp[2].x));

  tp_matrix.f = (tp[2].x * lcd[1].y - tp[1].x * lcd[2].y) * tp[0].y +
                (tp[0].x * lcd[2].y - tp[2].x * lcd[0].y) * tp[1].y +
                (tp[1].x * lcd[0].y - tp[0].x * lcd[1].y) * tp[2].y;
}

void lcd_touch_start_calibration() {
  struct calibration_point lcd_points[3] = {CAL_POINT1,
                                            CAL_POINT2,
                                            CAL_POINT3};
  struct calibration_point tp_points[3];

  lcd_color color =            RGB(0,   255, 0);
  lcd_color background_color = RGB(0,   0,   0);
  lcd_color touch_color =      RGB(255, 0,   255);

  /* Save current orientation and clear screen */
  enum lcd_orientation original_orientation = lcd_current_orientation;
  lcd_set_orientation(LCD_ORIENTATION_0);
  lcd_fill_screen(background_color);

  /* Show calibration points */
  for (uint8_t i = 0; i < 3; ) {
    lcd_touch_render_target_square(lcd_points[i].x,
                                   lcd_points[i].y,
                                   20,
                                   color,
                                   background_color);
    lcd_touch_render_target_square(lcd_points[i].x,
                                   lcd_points[i].y,
                                   8,
                                   color,
                                   background_color);

     /* Save point when a press is detected */
    uint16_t x, y;
    while (!lcd_touch_read_raw(NULL, &x, &y)) { }

    lcd_fill_rect(lcd_points[i].x - 4,
                  lcd_points[i].y - 4,
                  6,
                  6,
                  touch_color);
    tp_points[i].x = x;
    tp_points[i].y = y;

    /*
     * Clear the screen. This also acts as a delay so we don't register one
     * touch as the calibration for all three points
     */
    lcd_fill_screen(background_color);

    i++;
  }

  lcd_touch_set_calibration(lcd_points, tp_points);

  /* Restore original orientation */
  lcd_set_orientation(original_orientation);
}

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

bool lcd_touch_read_raw(uint16_t *touch_pressure,
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

static void lcd_touch_calculate_points() {
  uint32_t x, y;

  if(tp_x != tp_last_x) {
    tp_last_x = tp_x;
    x = tp_x;
    y = tp_y;
    x = ((tp_matrix.a * x) + (tp_matrix.b * y) + tp_matrix.c) / tp_matrix.div;

    switch (lcd_current_orientation) {
    case LCD_ORIENTATION_0:
    case LCD_ORIENTATION_180:
           if (x >= (uint16_t)(lcd_width * 2)) { x = 0; }
      else if (x >= (uint16_t)(lcd_width * 1)) { x = lcd_width - 1; }
      break;
    case LCD_ORIENTATION_90:
    case LCD_ORIENTATION_270:
           if (x >= (uint16_t)(lcd_height * 2)) { x = 0; }
      else if (x >= (uint16_t)(lcd_height * 1)) { x = lcd_height - 1; }
      break;
    }

    lcd_x = x;
  }

  if(tp_y != tp_last_y) {
    tp_last_y = tp_y;
    x = tp_x;
    y = tp_y;
    y = ((tp_matrix.d * x) + (tp_matrix.e * y) + tp_matrix.f) / tp_matrix.div;

    switch (lcd_current_orientation) {
    case LCD_ORIENTATION_0:
    case LCD_ORIENTATION_180:
           if (y >= (uint16_t)(lcd_height * 2)) { y = 0; }
      else if (y >= (uint16_t)(lcd_height * 1)) { y = lcd_height - 1; }
      break;
    case LCD_ORIENTATION_90:
    case LCD_ORIENTATION_270:
           if (y >= (uint16_t)(lcd_width * 2)) { y = 0; }
      else if (y >= (uint16_t)(lcd_width * 1)) { y = lcd_width - 1; }
      break;
    }

    lcd_y = y;
  }
}

/*
 * The next two functions should only be called after calling
 * lcd_touch_calculate_points()!
 */
static uint16_t lcd_touch_x() {
  if (lcd_current_orientation == LCD_ORIENTATION_0) {
    return lcd_x;
  } else if (lcd_current_orientation ==  LCD_ORIENTATION_90) {
    return lcd_y;
  } else if (lcd_current_orientation == LCD_ORIENTATION_180) {
    return lcd_width - lcd_x;
  } else { /* lcd_current_orientation == LCD_ORIENTATION_270 */
    return lcd_width - lcd_y;
  }
}

static uint16_t lcd_touch_y() {
  if (lcd_current_orientation == LCD_ORIENTATION_0) {
    return lcd_y;
  } else if (lcd_current_orientation == LCD_ORIENTATION_90) {
    return lcd_height - lcd_x;
  } else if (lcd_current_orientation == LCD_ORIENTATION_180) {
    return lcd_height - lcd_y;
  } else { /* lcd_current_orientation == LCD_ORIENTATION_270 */
    return lcd_x;
  }
}

bool lcd_touch_read(uint16_t *touch_pressure,
                    uint16_t *touch_x,
                    uint16_t *touch_y) {
  bool consistent = lcd_touch_read_raw(&lcd_z, &tp_x, &tp_y);
  lcd_touch_calculate_points();

  if (consistent) {
    if (touch_pressure) *touch_pressure = lcd_z;
    if (touch_x)        *touch_x = lcd_touch_x();
    if (touch_y)        *touch_y = lcd_touch_y();
    return true;
  }

  return false;
}
