#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <avr/io.h>
#include "pleasant-spi.h"

void spi_prepare() {
  DDRB |= (1 << PORTB5);        /* SCK, Serial ClocK */
  DDRB ^= ~(1 << PORTB4);       /* MISO: Master In Slave Out */
  DDRB |= (1 << PORTB3);        /* MOSI: Master Out Slave In */
  DDRB |= (1 << PORTB2);        /* SS: Slave Select */

  spi_prepared = true;
}

void spi_configure(enum spi_clock_speed clock_speed,
                   enum spi_bit_order bit_order) {
  if (!spi_prepared) spi_prepare();

  SPCR |= (1 << MSTR) | (1 << SPE);
  SPCR |= (1 << CPOL) | (1 << CPHA);

  SPSR ^= ~SPI_CLOCK_SPEED_SPSR_MASK;
  SPSR |= (clock_speed & (1 << 2)) ? (1 << SPI2X) : 0;

  SPCR ^= ~SPI_CLOCK_SPEED_SPCR_MASK;
  SPCR |= (clock_speed & (1 << 1)) ? (1 << SPR1) : 0;
  SPCR |= (clock_speed & (1 << 0)) ? (1 << SPR0) : 0;

  SPCR ^= ~SPI_BIT_ORDER_MASK;
  SPCR |= bit_order;

  spi_configured = true;
}

uint8_t spi_transfer(uint8_t data) {
  if (!spi_configured) spi_configure(SPI_DEFAULT_CLOCK_SPEED,
                                     SPI_DEFAULT_BIT_ORDER);

  SPDR = data;
  while (!(SPSR & (1 << SPIF)));
  return SPDR;
}

void spi_transfer_bytes(uint8_t *bytes, size_t count) {
  size_t i;

  for (i = 0; i < count; i++) {
    *(bytes + i) = spi_transfer(*(bytes + i));
  }
}
