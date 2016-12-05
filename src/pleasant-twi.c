#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/twi.h>
#include "pleasant-twi.h"

/* State ------------------------------------------------------------------- */

static volatile enum twi_state twi_state;
static volatile enum twi_error twi_error;
static volatile uint8_t twi_slave_addressing_byte;

/* Callbacks --------------------------------------------------------------- */

void (*twi_slave_receive_callback)(volatile uint8_t *data,
                                   uint8_t size);
void (*twi_slave_transmit_callback)();

/* Buffers --------------------------------------------------------------------
 * Many operations have to write or read data, but because of the asynchronous
 * nature of the TWI module we need a place to store that data. There are three
 * such buffers, which are used in different situations.
 */

/* The master buffer is used when we are the master, and we are sending or
   receiving data. */

static volatile uint8_t twi_master_buffer[TWI_BUFFER_SIZE];
static volatile uint8_t twi_master_buffer_next_index;
static volatile uint8_t twi_master_buffer_data_size;

static void twi_master_buffer_start_reading() {
  twi_master_buffer_next_index = 0;
}

static bool twi_master_buffer_data_left() {
  return twi_master_buffer_next_index < twi_master_buffer_data_size;
}

static uint8_t twi_master_buffer_read() {
  return twi_master_buffer[twi_master_buffer_next_index++];
}

static void twi_master_buffer_start_writing(uint8_t size) {
  twi_master_buffer_next_index = 0;
  twi_master_buffer_data_size = size;
}

static void twi_master_buffer_write(uint8_t byte) {
  twi_master_buffer[twi_master_buffer_next_index++] = byte;
}

static uint8_t twi_master_buffer_data_written() {
  return twi_master_buffer_next_index;
}

/* The slave transmit buffer is used when we are a slave, and we have received
   a request for data. The buffer will be filled by the
   twi_slave_transmit_callback, using the function twi_transmit_reply. */

static volatile uint8_t twi_slave_transmit_buffer[TWI_BUFFER_SIZE];
static volatile uint8_t twi_slave_transmit_buffer_next_index;
static volatile uint8_t twi_slave_transmit_buffer_data_size;

static void twi_slave_transmit_buffer_start_reading() {
  twi_slave_transmit_buffer_next_index = 0;
}

static bool twi_slave_transmit_buffer_data_left() {
  return twi_slave_transmit_buffer_next_index
    < twi_slave_transmit_buffer_data_size;
}

static uint8_t twi_slave_transmit_buffer_read() {
  return twi_slave_transmit_buffer[twi_slave_transmit_buffer_next_index++];
}

static void twi_slave_transmit_buffer_start_writing(uint8_t size) {
  twi_slave_transmit_buffer_next_index = 0;
  twi_slave_transmit_buffer_data_size = size;
}

static void twi_slave_transmit_buffer_write(uint8_t byte) {
  twi_slave_transmit_buffer[twi_slave_transmit_buffer_next_index++] = byte;
}

static uint8_t twi_slave_transmit_buffer_data_written() {
  return twi_slave_transmit_buffer_next_index;
}

/* The slave receive buffer is used when we are a slave, and receive data. Its
   contents are to be processed by the twi_slave_receive_callback. */

static volatile uint8_t twi_slave_receive_buffer[TWI_BUFFER_SIZE];
static volatile uint8_t twi_slave_receive_buffer_next_index;

static void twi_slave_receive_buffer_start_writing() {
  twi_slave_receive_buffer_next_index = 0;
}

static void twi_slave_receive_buffer_write(uint8_t byte) {
  twi_slave_receive_buffer[twi_slave_receive_buffer_next_index++] = byte;
}

static uint8_t twi_slave_receive_buffer_data_written() {
  return twi_slave_receive_buffer_next_index;
}

static bool twi_slave_receive_buffer_space_left() {
  return twi_slave_receive_buffer_next_index < TWI_BUFFER_SIZE;
}

/* Transmission ------------------------------------------------------------ */

static void twi_send_start() {
  TWCR =
    (1 << TWSTA)
    | (1 << TWEN)
    | (1 << TWIE)
    | (1 << TWEA)
    | (1 << TWINT);
}

static void twi_continue(bool ack) {
  TWCR =
    (1 << TWINT)
    | (1 << TWEN)
    | (1 << TWIE)
    | (ack ? (1 << TWEA) : 0);
}

static void twi_stop() {
  TWCR =
    (1 << TWSTO)
    | (1 << TWEN)
    | (1 << TWIE)
    | (1 << TWEA)
    | (1 << TWINT);

  /* After the stop condition is sent, the interrupt is not signalled. */
  while (TWCR & (1 << TWSTO));

  twi_state = TWI_STATE_READY;
}

static void twi_release_bus() {
  TWCR = (1 << TWEN) | (1 << TWIE) | (1 << TWEA) | (1 << TWINT);
  twi_state = TWI_STATE_READY;
}

ISR(TWI_vect) {
  switch (TW_STATUS) {
  case TW_START:
  case TW_REP_START:
    TWDR = twi_slave_addressing_byte;
    twi_continue(true);
    break;

  case TW_MT_SLA_ACK:
  case TW_MT_DATA_ACK:
    if (twi_master_buffer_data_left()) {
      TWDR = twi_master_buffer_read();
      twi_continue(true);
    } else {
      twi_stop();
    }
    break;

  case TW_MT_SLA_NACK:
    twi_error = TWI_ERROR_MASTER_START_REJECTED;
    twi_stop();
    break;

  case TW_MT_DATA_NACK:
    twi_error = TWI_ERROR_MASTER_DATA_REJECTED;
    twi_stop();
    break;

  case TW_MT_ARB_LOST:
    twi_error = TWI_ERROR_MASTER_ARBITRATION_LOST;
    twi_release_bus();
    break;

  case TW_MR_DATA_ACK:
    twi_master_buffer_write(TWDR);
    /* Fall through */
  case TW_MR_SLA_ACK:
    twi_continue(twi_master_buffer_data_left());
    break;

  case TW_MR_DATA_NACK:
    twi_master_buffer_write(TWDR);
    twi_stop();
    break;

  case TW_MR_SLA_NACK:
    twi_stop();
    break;

  case TW_SR_SLA_ACK:
  case TW_SR_GCALL_ACK:
  case TW_SR_ARB_LOST_SLA_ACK:
  case TW_SR_ARB_LOST_GCALL_ACK:
    twi_state = TWI_STATE_SLAVE_RECEIVING;
    twi_slave_receive_buffer_start_writing();
    twi_continue(true);
    break;

  case TW_SR_DATA_ACK:
  case TW_SR_GCALL_DATA_ACK:
    if (twi_slave_receive_buffer_space_left()) {
      twi_slave_receive_buffer_write(TWDR);
      twi_continue(true);
    } else {
      twi_continue(false);
    }
    break;
  case TW_SR_STOP:
    twi_release_bus();
    if (twi_slave_receive_callback) {
      twi_slave_receive_callback(twi_slave_receive_buffer,
                                 twi_slave_receive_buffer_data_written());
    }
    break;
  case TW_SR_DATA_NACK:
  case TW_SR_GCALL_DATA_NACK:
    twi_continue(0);
    break;

  case TW_ST_SLA_ACK:
  case TW_ST_ARB_LOST_SLA_ACK:
    twi_state = TWI_STATE_SLAVE_TRANSMITTING;

    if (twi_slave_transmit_callback) twi_slave_transmit_callback();

    if (twi_slave_transmit_buffer_data_written() == 0) {
      twi_slave_transmit_buffer_start_writing(1);
      twi_slave_transmit_buffer_write(0);
    }

    twi_slave_transmit_buffer_start_reading();
    /* Fall through */
  case TW_ST_DATA_ACK:
    TWDR = twi_slave_transmit_buffer_read();
    twi_continue(twi_slave_transmit_buffer_data_left());
    break;
  case TW_ST_DATA_NACK:
  case TW_ST_LAST_DATA:
    twi_continue(true);
    twi_state = TWI_STATE_READY;
    break;
  case TW_NO_INFO:
    break;
  case TW_BUS_ERROR:
    twi_error = TWI_ERROR_BUS;
    twi_stop();
    break;
  }
}

/* API functions ----------------------------------------------------------- */

void twi_init() {
  twi_state = TWI_STATE_READY;

  /* SDA */
  DDRC &= ~(1 << PORTC4);
  PORTC |= (1 << PORTC4);
  /* SCL */
  DDRC &= ~(1 << PORTC5);
  PORTC |= (1 << PORTC5);

  /* We always use a prescaler value of 1. */
  TWSR &= ~((1 << TWPS0) | (1 << TWPS1));
  TWBR = ((F_CPU / TWI_FREQUENCY) - 16) / 2;

  TWCR = (1 << TWEA) | (1 << TWEN) | (1 << TWIE);
}

void twi_set_address(uint8_t address, bool recognize_general_call) {
  TWAR = (address << 1) | recognize_general_call;
}

bool twi_write(uint8_t address,
               uint8_t *data,
               uint8_t size,
               enum twi_error *error) {
  uint8_t i;

  if (size > TWI_BUFFER_SIZE) return false;
  while (twi_state != TWI_STATE_READY);

  twi_state = TWI_STATE_MASTER_TRANSMITTING;
  twi_error = TWI_ERROR_NONE;

  twi_slave_addressing_byte = (address << 1) | TW_WRITE;

  twi_master_buffer_start_writing(size);
  for (i = 0; i < size; i++) {
    twi_master_buffer_write(data[i]);
  }

  twi_master_buffer_start_reading();

  twi_send_start();

  while (twi_state == TWI_STATE_MASTER_TRANSMITTING);

  *error = twi_error;
  return true;
}

uint8_t twi_read(uint8_t address,
                 uint8_t *data,
                 uint8_t size,
                 enum twi_error *error) {
  uint8_t i;

  if (size > TWI_BUFFER_SIZE) return 0;
  while (twi_state != TWI_STATE_READY);

  twi_state = TWI_STATE_MASTER_RECEIVING;
  twi_error = TWI_ERROR_NONE;

  twi_slave_addressing_byte = (address << 1) | TW_READ;

  /* When a byte is received, before the interrupt is signalled, the setting of
     TWEA is transmitted in response. Because of that, it needs to be set to
     the correct value *before* the last byte is received. That's why we
     subtract 1 from the expected size. */
  twi_master_buffer_start_writing(size - 1);

  twi_send_start();

  while (twi_state == TWI_STATE_MASTER_RECEIVING);

  if (twi_master_buffer_data_written() < size) {
    /* Less data was received than was requested */
    size = twi_master_buffer_data_written();
  }

  twi_master_buffer_start_reading();
  for (i = 0; i < size; i++) {
    data[i] = twi_master_buffer_read();
  }

  *error = twi_error;
  return size;
}

bool twi_transmit_reply(uint8_t *data, uint8_t size) {
  uint8_t i;
  if (size > TWI_BUFFER_SIZE) return false;
  if (twi_state != TWI_STATE_SLAVE_TRANSMITTING) return false;

  twi_slave_transmit_buffer_start_writing(size);
  for (i = 0; i < size; i++) {
    twi_slave_transmit_buffer_write(data[i]);
  }

  return true;
}
