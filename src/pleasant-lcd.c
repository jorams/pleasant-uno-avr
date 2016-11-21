#include <util/delay.h>
#include "pleasant-spi.h"
#include "pleasant-timer.h"
#include "pleasant-lcd.h"

/* Implementation details -------------------------------------------------- */

#define LCD_PIN_DDR_RST  DDRB
#define LCD_PIN_PORT_RST PORTB
#define LCD_PIN_RST      PORTB0
#define LCD_PIN_DDR_LED  DDRB
#define LCD_PIN_PORT_LED PORTB
#define LCD_PIN_LED      PORTB1
#define LCD_PIN_DDR_CS   DDRD
#define LCD_PIN_PORT_CS  PORTD
#define LCD_PIN_CS       PORTD7

static void lcd_speed_down_spi() {
  spi_configure(SPI_CLOCK_SPEED_DIV_8, SPI_BIT_ORDER_MSB_FIRST);
}

static void lcd_configure_spi() {
  spi_configure(lcd_spi_clock_speed, SPI_BIT_ORDER_MSB_FIRST);
}

static void lcd_enable_transmission() {
  LCD_PIN_PORT_CS |= (1 << LCD_PIN_CS);
}

static void lcd_disable_transmission() {
  LCD_PIN_PORT_CS &= ~(1 << LCD_PIN_CS);
}

static void lcd_send_command(enum lcd_command command) {
  lcd_enable_transmission();
  PORTB &= ~(1 << PORTB3);
  PORTB &= ~(1 << PORTB5);
  SPCR &= ~(1 << SPE);
  PORTB |= (1 << PORTB5);
  SPCR |= (1 << SPE);

  spi_transfer(command);

  lcd_disable_transmission();
}

static void lcd_send_data(uint8_t data) {
  lcd_enable_transmission();

  PORTB |= (1 << PORTB3);
  PORTB &= ~(1 << PORTB5);
  SPCR &= ~(1 << SPE);
  PORTB |= (1 << PORTB5);
  SPCR |= (1 << SPE);

  spi_transfer(data);

  lcd_disable_transmission();
}

static void lcd_send_data16(uint16_t data) {
  lcd_enable_transmission();

  PORTB |= (1 << PORTB3);
  PORTB &= ~(1 << PORTB5);
  SPCR &= ~(1 << SPE);
  PORTB |= (1 << PORTB5);
  SPCR |= (1 << SPE);

  spi_transfer(data >> 8);

  PORTB |= (1 << PORTB3);
  PORTB &= ~(1 << PORTB5);
  SPCR &= ~(1 << SPE);
  PORTB |= (1 << PORTB5);
  SPCR |= (1 << SPE);

  spi_transfer(data);

  lcd_disable_transmission();
}

static void lcd_prepare_timer(bool pwm_output_enabled) {
  timer1_init(TIMER_WAVE_TYPE_PHASE_CORRECT_PWM,
              TIMER_WRAP_TYPE_8_BITS,
              TIMER_CLOCK_SOURCE_DIV_64,
              TIMER_DEFAULT_INTERRUPT,
              pwm_output_enabled
              ? TIMER_COMPARE_OUTPUT_MODE_CLEAR
              : TIMER_COMPARE_OUTPUT_MODE_OFF,
              TIMER_DEFAULT_COMPARE_OUTPUT_MODE,
              TIMER_DEFAULT_INPUT_CAPTURE_EDGE,
              TIMER_DEFAULT_INPUT_CAPTURE_NOISE_CANCELER);
}

static void lcd_draw(lcd_color color) {
  lcd_send_data16(color);
}

static void lcd_initialize_device() {
  /* lcd_send_command(LCD_COMMAND_RESET); */
  /* _delay_ms(5); */

  lcd_send_command(LCD_COMMAND_DISPLAY_OFF);
  _delay_ms(20);

  lcd_send_command(LCD_COMMAND_POWER_CTRLB);
  lcd_send_data16(0x0000);
  lcd_send_data16(0x0083);
  lcd_send_data16(0x0030);

  lcd_send_command(LCD_COMMAND_POWERON_SEQ_CTRL);
  lcd_send_data16(0x0064);
  lcd_send_data16(0x0003);
  lcd_send_data16(0x0012);
  lcd_send_data16(0x0081);

  lcd_send_command(LCD_COMMAND_DRV_TIMING_CTRLA);
  lcd_send_data16(0x0085);
  lcd_send_data16(0x0001);
  lcd_send_data16(0x0079);

  lcd_send_command(LCD_COMMAND_POWER_CTRLA);
  lcd_send_data16(0x0039);
  lcd_send_data16(0x002c);
  lcd_send_data16(0x0000);
  lcd_send_data16(0x0034);
  lcd_send_data16(0x0002);

  lcd_send_command(LCD_COMMAND_PUMP_RATIO_CTRL);
  lcd_send_data16(0x0020);

  lcd_send_command(LCD_COMMAND_DRV_TIMING_CTRLB);
  lcd_send_data16(0x0000);
  lcd_send_data16(0x0000);

  lcd_send_command(LCD_COMMAND_POWER_CTRL1);
  lcd_send_data16(0x0026);
  lcd_send_command(LCD_COMMAND_POWER_CTRL2);
  lcd_send_data16(0x0011);

  lcd_send_command(LCD_COMMAND_VCOM_CTRL1);
  lcd_send_data16(0x0035);
  lcd_send_data16(0x003e);

  lcd_send_command(LCD_COMMAND_VCOM_CTRL2);
  lcd_send_data16(0x00be);

  lcd_send_command(LCD_COMMAND_FRAME_CTRL);
  lcd_send_data16(0x0000);
  lcd_send_data16(0x001b);

  lcd_send_command(LCD_COMMAND_ENABLE_3G);
  lcd_send_data16(0x0008);

  lcd_send_command(LCD_COMMAND_GAMMA);
  lcd_send_data16(0x0001);

  lcd_send_command(LCD_COMMAND_POS_GAMMA);
  lcd_send_data16(0x001f);
  lcd_send_data16(0x001a);
  lcd_send_data16(0x0018);
  lcd_send_data16(0x000a);
  lcd_send_data16(0x000f);
  lcd_send_data16(0x0006);
  lcd_send_data16(0x0045);
  lcd_send_data16(0x0087);
  lcd_send_data16(0x0032);
  lcd_send_data16(0x000a);
  lcd_send_data16(0x0007);
  lcd_send_data16(0x0002);
  lcd_send_data16(0x0007);
  lcd_send_data16(0x0005);
  lcd_send_data16(0x0000);

  lcd_send_command(LCD_COMMAND_NEG_GAMMA);
  lcd_send_data16(0x0000);
  lcd_send_data16(0x0025);
  lcd_send_data16(0x0027);
  lcd_send_data16(0x0005);
  lcd_send_data16(0x0010);
  lcd_send_data16(0x0009);
  lcd_send_data16(0x003a);
  lcd_send_data16(0x0078);
  lcd_send_data16(0x004d);
  lcd_send_data16(0x0005);
  lcd_send_data16(0x0018);
  lcd_send_data16(0x000d);
  lcd_send_data16(0x0038);
  lcd_send_data16(0x003a);
  lcd_send_data16(0x001f);

  lcd_send_command(LCD_COMMAND_DISPLAY_CTRL);
  lcd_send_data16(0x000a);
  lcd_send_data16(0x0082);
  lcd_send_data16(0x0027);
  lcd_send_data16(0x0000);

  lcd_send_command(LCD_COMMAND_ENTRY_MODE);
  lcd_send_data16(0x0007);

  lcd_send_command(LCD_COMMAND_PIXEL_FORMAT);
  lcd_send_data16(0x0055);

  lcd_set_orientation(LCD_ORIENTATION_0);

  lcd_send_command(LCD_COMMAND_SLEEPOUT);
  _delay_ms(60);
  _delay_ms(60);

  lcd_send_command(LCD_COMMAND_DISPLAY_ON);
  _delay_ms(20);
}

/* API implementation ------------------------------------------------------ */

uint16_t lcd_width;
uint16_t lcd_height;
enum spi_clock_speed lcd_spi_clock_speed;

void lcd_init(enum spi_clock_speed clock_speed) {
  lcd_spi_clock_speed = clock_speed;

  LCD_PIN_DDR_RST  |= (1 << LCD_PIN_RST);
  LCD_PIN_PORT_RST |= (1 << LCD_PIN_RST);

  LCD_PIN_DDR_LED  |= (1 << LCD_PIN_LED);
  LCD_PIN_DDR_CS   |= (1 << LCD_PIN_CS);

  LCD_PIN_PORT_LED &= ~(1 << LCD_PIN_LED);
  LCD_PIN_PORT_CS  &= ~(1 << LCD_PIN_CS);

  lcd_speed_down_spi();
  lcd_reset();
  lcd_configure_spi();

  lcd_set_brightness(50);
}

void lcd_reset() {
  /* Reset */
  lcd_disable_transmission();
  LCD_PIN_PORT_RST |= (1 << LCD_PIN_RST);
  _delay_ms(50);
  LCD_PIN_PORT_RST &= ~(1 << LCD_PIN_RST);
  _delay_ms(120);

  /* Re-initialize */
  lcd_initialize_device();
  lcd_fill_screen(RGB(0, 0, 0));
}

void lcd_set_brightness(uint8_t brightness) {
  if (brightness == 0) {
    lcd_prepare_timer(false);
    LCD_PIN_PORT_LED &= ~(1 << LCD_PIN_LED);
  } else if (brightness >= 100) {
    lcd_prepare_timer(false);
    LCD_PIN_PORT_LED |= (1 << LCD_PIN_LED);
  } else {
    lcd_prepare_timer(true);
    TIMER1_COMPARE_A = (uint16_t)brightness * 255 / 100;
  }
}

void lcd_set_orientation(enum lcd_orientation orientation) {
  enum lcd_base_orientation base_orientation
    = (orientation & LCD_ORIENTATION_BASE_ORIENTATION_MASK);

  lcd_width = base_orientation == LCD_BASE_ORIENTATION_LANDSCAPE
    ? LCD_WIDTH : LCD_HEIGHT;
  lcd_height = base_orientation == LCD_BASE_ORIENTATION_LANDSCAPE
    ? LCD_HEIGHT : LCD_WIDTH;

  lcd_send_command(LCD_COMMAND_MEMACCESS_CTRL);
  lcd_send_data16(orientation & LCD_ORIENTATION_MEMORY_ACCESS_MASK);

  lcd_set_area(0, 0, lcd_width, lcd_height);
}

void lcd_set_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
  lcd_send_command(LCD_COMMAND_COLUMN);
  lcd_send_data16(x0);
  lcd_send_data16(x1);

  lcd_send_command(LCD_COMMAND_PAGE);
  lcd_send_data16(y0);
  lcd_send_data16(y1);
}

void lcd_set_inverted(bool inverted) {
  lcd_send_command(inverted ? LCD_COMMAND_INV_ON : LCD_COMMAND_INV_ON);
}

void lcd_start_drawing() {
  lcd_send_command(LCD_COMMAND_WRITE);
  lcd_enable_transmission();
}

void lcd_stop_drawing() {
  lcd_disable_transmission();
}

void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color color) {
  if (x >= lcd_width || y >= lcd_height) return;

  lcd_set_area(x, y, x, y);

  lcd_start_drawing();
  lcd_draw(color);
  lcd_stop_drawing();
}

void lcd_fill_screen(lcd_color color) {
  uint32_t i;

  lcd_start_drawing();
  for (i = lcd_width * lcd_height; i > 0; i--) lcd_draw(color);
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
  for (i = w * h; i > 0; i--) lcd_draw(color);
  lcd_stop_drawing();
}
