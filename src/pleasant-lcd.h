#ifndef SIMPLE_LCD_H
#define SIMPLE_LCD_H

#include <stdbool.h>
#include <stdint.h>
#include "pleasant-spi.h"

/* Commands ---------------------------------------------------------------- */

enum lcd_command {
  LCD_COMMAND_NOP              = 0x00,
  LCD_COMMAND_RESET            = 0x01,
  LCD_COMMAND_SLEEPIN          = 0x10,
  LCD_COMMAND_SLEEPOUT         = 0x11,
  LCD_COMMAND_PARTIAL_MODE     = 0x12,
  LCD_COMMAND_NORMAL_MODE      = 0x13,
  LCD_COMMAND_INV_OFF          = 0x20,
  LCD_COMMAND_INV_ON           = 0x21,
  LCD_COMMAND_GAMMA            = 0x26,
  LCD_COMMAND_DISPLAY_OFF      = 0x28,
  LCD_COMMAND_DISPLAY_ON       = 0x29,
  LCD_COMMAND_COLUMN           = 0x2A,
  LCD_COMMAND_PAGE             = 0x2B,
  LCD_COMMAND_WRITE            = 0x2C,
  LCD_COMMAND_READ             = 0x2E,
  LCD_COMMAND_PARTIAL_AREA     = 0x30,
  LCD_COMMAND_TEARING_OFF      = 0x34,
  LCD_COMMAND_TEARING_ON       = 0x35,
  LCD_COMMAND_MEMACCESS_CTRL   = 0x36,
  LCD_COMMAND_IDLE_OFF         = 0x38,
  LCD_COMMAND_IDLE_ON          = 0x39,
  LCD_COMMAND_PIXEL_FORMAT     = 0x3A,
  LCD_COMMAND_WRITE_CNT        = 0x3C,
  LCD_COMMAND_READ_CNT         = 0x3E,
  LCD_COMMAND_BRIGHTNESS       = 0x51,
  LCD_COMMAND_BRIGHTNESS_CTRL  = 0x53,
  LCD_COMMAND_RGB_CTRL         = 0xB0,
  LCD_COMMAND_FRAME_CTRL       = 0xB1, /* normal mode */
  LCD_COMMAND_FRAME_CTRL_IDLE  = 0xB2, /* idle mode */
  LCD_COMMAND_FRAME_CTRL_PART  = 0xB3, /* partial mode */
  LCD_COMMAND_INV_CTRL         = 0xB4,
  LCD_COMMAND_DISPLAY_CTRL     = 0xB6,
  LCD_COMMAND_ENTRY_MODE       = 0xB7,
  LCD_COMMAND_POWER_CTRL1      = 0xC0,
  LCD_COMMAND_POWER_CTRL2      = 0xC1,
  LCD_COMMAND_VCOM_CTRL1       = 0xC5,
  LCD_COMMAND_VCOM_CTRL2       = 0xC7,
  LCD_COMMAND_POWER_CTRLA      = 0xCB,
  LCD_COMMAND_POWER_CTRLB      = 0xCF,
  LCD_COMMAND_POS_GAMMA        = 0xE0,
  LCD_COMMAND_NEG_GAMMA        = 0xE1,
  LCD_COMMAND_DRV_TIMING_CTRLA = 0xE8,
  LCD_COMMAND_DRV_TIMING_CTRLB = 0xEA,
  LCD_COMMAND_POWERON_SEQ_CTRL = 0xED,
  LCD_COMMAND_ENABLE_3G        = 0xF2,
  LCD_COMMAND_INTERF_CTRL      = 0xF6,
  LCD_COMMAND_PUMP_RATIO_CTRL  = 0xF7
};

/* Memory access control --------------------------------------------------- */

enum lcd_memory_access_control {
  LCD_MEMORY_ACCESS_CONTROL_MY  = 7,
  LCD_MEMORY_ACCESS_CONTROL_MX  = 6,
  LCD_MEMORY_ACCESS_CONTROL_MV  = 5,
  LCD_MEMORY_ACCESS_CONTROL_ML  = 4,
  LCD_MEMORY_ACCESS_CONTROL_BGR = 3,
  LCD_MEMORY_ACCESS_CONTROL_MH  = 2,
};

/* Orientation ----------------------------------------------------------------
 * TODO
 */

#define LCD_ORIENTATION_MEMORY_ACCESS_MASK    0b11111100
#define LCD_ORIENTATION_BASE_ORIENTATION_MASK 0b00000011

enum lcd_base_orientation {
  LCD_BASE_ORIENTATION_LANDSCAPE = 0,
  LCD_BASE_ORIENTATION_PORTRAIT  = 1
};

enum lcd_orientation {
  LCD_ORIENTATION_0   = (LCD_BASE_ORIENTATION_LANDSCAPE
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_BGR)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_MX)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_MY)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_MV)),
  LCD_ORIENTATION_90  = (LCD_BASE_ORIENTATION_PORTRAIT
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_BGR)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_MX)),
  LCD_ORIENTATION_180 = (LCD_BASE_ORIENTATION_LANDSCAPE
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_BGR)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_ML)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_MV)),
  LCD_ORIENTATION_270 = (LCD_BASE_ORIENTATION_PORTRAIT
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_BGR)
                         | (1 << LCD_MEMORY_ACCESS_CONTROL_MY)),
};

/* Color ------------------------------------------------------------------- */

typedef uint16_t lcd_color;

#define RGB(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

/* Defaults ---------------------------------------------------------------- */

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define LCD_DEFAULT_SPI_CLOCK_SPEED SPI_CLOCK_SPEED_DIV_4

/* State ------------------------------------------------------------------- */

extern uint16_t lcd_width;
extern uint16_t lcd_height;
extern enum spi_clock_speed lcd_spi_clock_speed;

/* API functions ----------------------------------------------------------- */

void lcd_init(enum spi_clock_speed clock_speed);
void lcd_reset();

void lcd_set_brightness(uint8_t brightness);
void lcd_set_orientation(enum lcd_orientation orientation);
void lcd_set_area(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1);
void lcd_set_inverted(bool inverted);

void lcd_start_drawing();
void lcd_stop_drawing();

void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color color);
void lcd_fill_screen(lcd_color color);
void lcd_fill_rect(uint16_t x,
                   uint16_t y,
                   uint16_t w,
                   uint16_t h,
                   lcd_color color);

#endif /* SIMPLE_LCD_H */
