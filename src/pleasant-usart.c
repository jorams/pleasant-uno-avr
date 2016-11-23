#include <avr/io.h>
#include "pleasant-usart.h"

void usart_init(uint32_t baud_rate,
                enum usart_asynchronous_mode asynchronous_mode,
                enum usart_parity parity,
                enum usart_stop_bit_count stop_bit_count,
                enum usart_character_size character_size) {
  UCSR0A = 0;
  UCSR0B = 0;
  UCSR0C = 0;

  /* Baud rate */
  if (asynchronous_mode == USART_ASYNCHRONOUS_MODE_DOUBLE_SPEED) {
    UCSR0A |= (1 << U2X0);
  }
  UBRR0 = ((F_CPU / (asynchronous_mode * baud_rate)) - 1);

  /* Frame settings */
  UCSR0C |= (parity << 4);
  UCSR0C |= (stop_bit_count << 3);

  UCSR0B |= (character_size & (1 << 2) ? (1 << UCSZ02) : 0);
  UCSR0C |= (character_size & (1 << 1) ? (1 << UCSZ01) : 0);
  UCSR0C |= (character_size & (1 << 0) ? (1 << UCSZ00) : 0);

  /* Enable */
  UCSR0B |= (1 << TXEN0) | (1 << RXEN0);
}

void usart_write(uint8_t byte) {
  while (!(UCSR0A & (1 << UDRE0)));
  UDR0 = byte;
}

uint8_t usart_read(enum usart_error *error) {
  while (!(UCSR0A & (1 << RXC0)));
  *error =
    (UCSR0A & FE0 ? USART_ERROR_FRAME_ERROR : 0)
    | (UCSR0A & DOR0 ? USART_ERROR_DATA_OVERRUN : 0)
    | (UCSR0A & UPE0 ? USART_ERROR_PARITY_MISMATCH : 0);
  return UDR0;
}

void usart_write_bytes(uint8_t *bytes, size_t count) {
  size_t i;

  for (i = 0; i < count; i++) {
    usart_write(*(bytes + i));
  }
}

void usart_read_bytes(uint8_t *bytes, size_t count, enum usart_error *error) {
  size_t i;

  for (i = 0; i < count; i++) {
    *(bytes + i) = usart_read(error);
    if (*error != USART_ERROR_NO_ERROR) return;
  }
}

void usart_write_string(char *characters) {
  while (*characters != '\0') {
    usart_write(*characters);
    characters++;
  }
}

void usart_read_string(char *characters, size_t max, enum usart_error *error) {
  size_t i;

  if (max < 1) return;

  for (i = 0; i < (max - 1); i++) {
    *(characters + i) = usart_read(error);
    if (*error != USART_ERROR_NO_ERROR) return;

    if (*(characters + i) == '\n') {
      *(characters + i) = '\0';

      if (i > 0 && *(characters + i - 1) == '\r')
        *(characters + i - 1) = '\0';

      return;
    }
  }

  *(characters + i) = '\0';
}

bool usart_byte_available() {
  return (UCSR0A & (1 << RXC0));
}
