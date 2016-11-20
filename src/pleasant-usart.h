/*
 * Pleasant USART allows you to use the device's USART module in a simple
 * manner, while still supporting much of its functionality. It has the
 * following limitations:
 *
 * - It only supports asynchronous operation
 * - It does not support 9-bit characters
 */

#ifndef PLEASANT_USART_H
#define PLEASANT_USART_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Asynchronous mode ----------------------------------------------------------
 * The USART can either run at normal speed or at double speed. The values of
 * usart_asynchronous_mode are the associated divisor values of the baud rate
 * divider.
 */
enum usart_asynchronous_mode {
  USART_ASYNCHRONOUS_MODE_NORMAL_SPEED = 16,
  USART_ASYNCHRONOUS_MODE_DOUBLE_SPEED = 8
};

/* Parity ---------------------------------------------------------------------
 * The USART can both generate and check a parity bit for each frame.
 */
enum usart_parity {
  USART_PARITY_DISABLED = 0,
  USART_PARITY_EVEN     = 2,
  USART_PARITY_ODD      = 3
};

/* Stop bits ------------------------------------------------------------------
 * The number of stop bits sent by the transmitter can be configured. The
 * receiver ignores this.
 */
enum usart_stop_bit_count {
  USART_STOP_BIT_COUNT_1_BIT  = 0,
  USART_STOP_BIT_COUNT_2_BITS = 1
};

/* Character size -------------------------------------------------------------
 * USART can work with characters of sizes from 5 to 9 bits. Pleasant USART
 * does not support 9-bit characters.
 */
enum usart_character_size {
  USART_CHARACTER_SIZE_5_BITS = 0,
  USART_CHARACTER_SIZE_6_BITS = 1,
  USART_CHARACTER_SIZE_7_BITS = 2,
  USART_CHARACTER_SIZE_8_BITS = 3
  /* 9-bit characters not supported. */
};

/* Errors ---------------------------------------------------------------------
 * Various errors can occur while receiving data. They will be indicated
 * through a bitwise OR of 0 or more of the following values.
 */

enum usart_error {
  USART_ERROR_NO_ERROR        = 0,
  USART_ERROR_FRAME_ERROR     = 1,
  USART_ERROR_DATA_OVERRUN    = 2,
  USART_ERROR_PARITY_MISMATCH = 4
};

/* Defaults ---------------------------------------------------------------- */

#define USART_DEFAULT_ASYNCHRONOUS_MODE USART_ASYNCHRONOUS_MODE_DOUBLE_SPEED
#define USART_DEFAULT_PARITY            USART_PARITY_DISABLED
#define USART_DEFAULT_STOP_BIT_COUNT    USART_STOP_BIT_COUNT_1_BIT
#define USART_DEFAULT_CHARACTER_SIZE    USART_CHARACTER_SIZE_8_BITS

/* API functions ----------------------------------------------------------- */

/*
 * Configure the USART's various settings. For all but the baud rate, suggested
 * settings are USART_DEFAULT_*.
 */
void usart_init(uint32_t baud_rate,
                enum usart_asynchronous_mode asynchronous_mode,
                enum usart_parity parity,
                enum usart_stop_bit_count stop_bit_count,
                enum usart_character_size character_size);

/*
 * Write a single byte to the USART.
 */
void usart_write(uint8_t byte);

/*
 * Read a single byte from the USART.
 */
uint8_t usart_read(enum usart_error *error);

/*
 * Write a number of bytes to the USART.
 */
void usart_write_bytes(uint8_t *bytes, size_t count);

/*
 * Read a number of bytes from the USART. Will return when count bytes have
 * been read, or when an error occurs.
 */
void usart_read_bytes(uint8_t *bytes, size_t count, enum usart_error *error);

/*
 * Write a string to the USART. Writing will end before the first \0
 * encountered.
 */
void usart_write_string(char *characters);

/*
 * Read a string from the USART. Will read up to max-1 characters, or until the
 * first newline encountered, or until an error occurs. Note that max includes
 * an ending \0 byte, which will always be added (unless either max < 1 or an
 * error occurs).
 *
 * A newline is either a single LF (0x0B) or a CR (0x0D) followed by an LF.
 * Both characters will be read, but in the resulting string they will be set
 * to \0.
 */
void usart_read_string(char *characters, size_t max, enum usart_error *error);

#endif /* PLEASANT_USART_H */
