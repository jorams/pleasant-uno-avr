/*
 * Pleasant LCD implements support for the MI0283QT-9 TFT display with touch
 * panel, as can be bought from Watterott electronic. It is based on the
 * MI0283QT9 library by Watterott electronic, with many changes made to port it
 * to pure C and to improve clarity.
 *
 * It makes use of SPI for communication with the device(s), and timer 1 is
 * used to control the brightness of the display.
 *
 * The pins B0, D7, B1, and D6 are used, in addition to the ports used for SPI.
 */

#ifndef SIMPLE_LCD_H
#define SIMPLE_LCD_H

#include <stdbool.h>
#include <stdint.h>
#include "pleasant-spi.h"

/* Commands ------------------------------------------------------------------- */

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

/* ADS control ----------------------------------------------------------------
 * The ADS is controlled through 8-bit commands. These commands contain various
 * bits of information, each of which has been given a name. They can be ORed
 * together to get a full command.
 */

#define LCD_ADS_START (1 << 7)

enum lcd_ads_channel {
  LCD_ADS_CHANNEL_X_POSITION  = (0b101 << 4),
  LCD_ADS_CHANNEL_Y_POSITION  = (0b001 << 4),
  LCD_ADS_CHANNEL_Z1_POSITION = (0b011 << 4),
  LCD_ADS_CHANNEL_Z2_POSITION = (0b100 << 4)
};

enum lcd_ads_conversion {
  LCD_ADS_CONVERSION_12_BITS = (0 << 3),
  LCD_ADS_CONVERSION_8_BITS  = (1 << 3)
};

enum lcd_ads_reference {
  LCD_ADS_REFERENCE_DIFFERENTIAL = (0 << 2),
  LCD_ADS_REFERENCE_SINGLE_ENDED = (1 << 2)
};

enum lcd_ads_power_down_mode {
  LCD_ADS_POWER_DOWN_MODE_ENABLED  = 0b00,
  /* Omitted are the partial on/off states. */
  LCD_ADS_POWER_DOWN_MODE_DISABLED = 0b11
};

/* Memory access control ------------------------------------------------------
 * The LCD's memory can be accessed in different patterns, controlled using
 * LCD_COMMAND_MEMACCESS_CTRL.
 */

enum lcd_memory_access_control {
  LCD_MEMORY_ACCESS_CONTROL_MY  = 0b10000000,
  LCD_MEMORY_ACCESS_CONTROL_MX  = 0b01000000,
  LCD_MEMORY_ACCESS_CONTROL_MV  = 0b00100000,
  LCD_MEMORY_ACCESS_CONTROL_ML  = 0b00010000,
  LCD_MEMORY_ACCESS_CONTROL_BGR = 0b00001000,
  LCD_MEMORY_ACCESS_CONTROL_MH  = 0b00000100
};

/* Orientation ----------------------------------------------------------------
 * The effective orientation of the device can be changed by altering the
 * memory access pattern. Four orientations are supported.
 */

#define LCD_ORIENTATION_MEMORY_ACCESS_MASK    0b11111100
#define LCD_ORIENTATION_BASE_ORIENTATION_MASK 0b00000011

enum lcd_base_orientation {
  LCD_BASE_ORIENTATION_LANDSCAPE = 0,
  LCD_BASE_ORIENTATION_PORTRAIT  = 1
};

enum lcd_orientation {
  LCD_ORIENTATION_0   = (LCD_BASE_ORIENTATION_LANDSCAPE
                         | LCD_MEMORY_ACCESS_CONTROL_BGR
                         | LCD_MEMORY_ACCESS_CONTROL_MX
                         | LCD_MEMORY_ACCESS_CONTROL_MY
                         | LCD_MEMORY_ACCESS_CONTROL_MV),
  LCD_ORIENTATION_90  = (LCD_BASE_ORIENTATION_PORTRAIT
                         | LCD_MEMORY_ACCESS_CONTROL_BGR
                         | LCD_MEMORY_ACCESS_CONTROL_MX),
  LCD_ORIENTATION_180 = (LCD_BASE_ORIENTATION_LANDSCAPE
                         | LCD_MEMORY_ACCESS_CONTROL_BGR
                         | LCD_MEMORY_ACCESS_CONTROL_ML
                         | LCD_MEMORY_ACCESS_CONTROL_MV),
  LCD_ORIENTATION_270 = (LCD_BASE_ORIENTATION_PORTRAIT
                         | LCD_MEMORY_ACCESS_CONTROL_BGR
                         | LCD_MEMORY_ACCESS_CONTROL_MY),
};


/* Color ------------------------------------------------------------------- */

typedef uint16_t lcd_color;

#define RGB(r,g,b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

/* Specifications ---------------------------------------------------------- */

#define LCD_WIDTH 320
#define LCD_HEIGHT 240

#define LCD_TOUCH_REQUIRED_PRESSURE 5

#define LCD_DEFAULT_SPI_CLOCK_SPEED SPI_CLOCK_SPEED_DIV_2

/* State ------------------------------------------------------------------- */

extern uint16_t lcd_width;
extern uint16_t lcd_height;
extern enum spi_clock_speed lcd_spi_clock_speed;
extern enum lcd_orientation lcd_current_orientation;

/* API functions ----------------------------------------------------------- */

/*
 * Initialize the LCD screen and the associated touch controller. It
 * initializes SPI, timer 1 and various ports.
 *
 * An SPI clock speed can be chosen if so desired, but it is suggested you use
 * LCD_DEFAULT_SPI_CLOCK_SPEED. Note that lcd_touch_read will temporarily
 * change the clock speed to SPI_CLOCK_SPEED_DIV_8 every time it is called.
 */
void lcd_init(enum spi_clock_speed clock_speed);

/*
 * Set the brightness of the display. A value of 0 means the display is
 * effectively off, and a value of 100 means full brightness.
 */
void lcd_set_brightness(uint8_t brightness);

/*
 * Set the orientation of the display. This changes the direction of the
 * coordinate system.
 */
void lcd_set_orientation(enum lcd_orientation orientation);

/*
 * Set whether or not all colors on the screen should be inverted. This applies
 * to both newly drawn colors and existing colors.
 */
void lcd_set_inverted(bool inverted);

/*
 * Start a draw operation in the specified area. Individual pixels can then be
 * filled using lcd_batch_draw.
 */
void lcd_batch_start(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

/*
 * Draw a single pixel. Calls to this function will fill pixels in the area
 * specified in the call to lcd_batch_start, starting from (x, y), incrementing
 * the x coordinate first, then the y coordinate. The order this represents on
 * the screen depends on the orientation.
 */
void lcd_batch_draw(lcd_color color);

/*
 * Stop the draw operation.
 */
void lcd_batch_stop();

/*
 * Draw a single pixel of the specified color on the screen.
 */
void lcd_draw_pixel(uint16_t x, uint16_t y, lcd_color color);

/*
 * Fill the entire screen with the specified color.
 */
void lcd_fill_screen(lcd_color color);

/*
 * Fill a rectangle on the screen with the specified color.
 */
void lcd_fill_rect(uint16_t x,
                   uint16_t y,
                   uint16_t w,
                   uint16_t h,
                   lcd_color color);

/* Touch ------------------------------------------------------------------- */

/*
 * Calibrate the touch screen in order to accurately read touch position.
 */
void lcd_touch_start_calibration();

/*
 * Read the current state of the touchscreen. The return value specifies
 * whether or not a touch was registered.
 *
 * If a touch was registered, the value the three pointer arguments point to is
 * updated to reflect the current state of the touch panel. If you are not
 * interested in some value, you can pass in NULL.
 */
bool lcd_touch_read_raw(uint16_t *touch_pressure,
                        uint16_t *touch_x,
                        uint16_t *touch_y);

/*
 * Read current state of touchscreen using calibration data. Passed arguments
 * will contain the data upon success. Returns true when a touch was
 * registered.
 */
bool lcd_touch_read(uint16_t *touch_pressure,
                    uint16_t *touch_x,
                    uint16_t *touch_y);

#endif /* SIMPLE_LCD_H */
