/*
 * Pleasant SPI allows you to easily configure and use the device's SPI module.
 * It only supports master operation.
 */

#ifndef PLEASANT_SPI_H
#define PLEASANT_SPI_H

#include <avr/io.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Bit order --------------------------------------------------------------- */

#define SPI_BIT_ORDER_MASK (1 << DORD)

enum spi_bit_order {
  SPI_BIT_ORDER_MSB_FIRST = (0 << DORD),
  SPI_BIT_ORDER_LSB_FIRST = (1 << DORD)
};

/* Clock speed ------------------------------------------------------------- */

#define SPI_CLOCK_SPEED_SPCR_MASK (1 << SPR1) | (1 << SPR0)
#define SPI_CLOCK_SPEED_SPSR_MASK (1 << SPI2X)

/*
 * The spi_clock_speed values are 3-bit integers, whose MSB represents the
 * value of SPI2X, followed by SPR1, and SPR0.
 */
enum spi_clock_speed {
  SPI_CLOCK_SPEED_DIV_2    = 0b100,
  SPI_CLOCK_SPEED_DIV_4    = 0b000,
  SPI_CLOCK_SPEED_DIV_8    = 0b101,
  SPI_CLOCK_SPEED_DIV_16   = 0b001,
  SPI_CLOCK_SPEED_DIV_32   = 0b110,
  /* There are two ways to get a divisor of 64. Which is used does not
     matter. */
  SPI_CLOCK_SPEED_DIV_64   = 0b010,
  SPI_CLOCK_SPEED_DIV_64_2 = 0b111,
  SPI_CLOCK_SPEED_DIV_128  = 0b011
};

/* State ------------------------------------------------------------------- */

#define SPI_DEFAULT_CLOCK_SPEED SPI_CLOCK_SPEED_DIV_64
#define SPI_DEFAULT_BIT_ORDER   SPI_BIT_ORDER_MSB_FIRST

extern bool spi_prepared;
extern bool spi_configured;

/* API functions ----------------------------------------------------------- */

/*
 * Prepare the ports required for SPI operation.
 *
 * Configures PORTB5 (SCK) as an output port, PORTB4 (MISO) as an input port,
 * PORTB3 (MOSI) as an output port and PORTB2 (SS) as an output port.
 *
 * You should normally not need to call this function directly, because
 * spi_configure does it for you.
 */
void spi_prepare();

/*
 * Configure some SPI settings required for operation.
 *
 * Note that the actual clock speed depends both on the configured speed
 * setting and on the speed of the CPU.
 *
 * If you don't call this function before transferring data, it will
 * automatically be called with SPI_DEFAULT_CLOCK_SPEED and
 * SPI_DEFAULT_BIT_ORDER as arguments.
 */
void spi_configure(enum spi_clock_speed clock_speed,
                   enum spi_bit_order bit_order);

/*
 * Send a single byte, returning the received byte.
 */
uint8_t spi_transfer(uint8_t data);

/*
 * Send and receive a number of bytes. Each byte sent will be replaced with the
 * corresponding received byte.
 */
void spi_transfer_bytes(uint8_t *bytes, size_t count);

#endif /* PLEASANT_SPI_H */
