/*
 * Pleasant TWI allows you to work with the device's TWI module, to communicate
 * using the I²C protocol. It exposes an interface that should be relatively
 * easy to use.
 *
 * The TWI module can operate both as a master and a slave, and both operating
 * modes are supported by Pleasant TWI. It does not have support for repeated
 * starts.
 *
 * Note that unlike the rest of Pleasant Uno AVR, Pleasant TWI is released
 * under the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2.1 of the License, or (at your option)
 * any later version.
 */

#ifndef PLEASANT_TWI_H
#define PLEASANT_TWI_H

#include <stdbool.h>
#include <stdint.h>

/* Settings -------------------------------------------------------------------
 * These settings don't normally have to change.
 */

#define TWI_FREQUENCY           100000
#define TWI_BUFFER_SIZE         32

/* State ----------------------------------------------------------------------
 * The state of the TWI module is changed based on the operation currently
 * happening on the bus.
 */
enum twi_state {
  TWI_STATE_READY,
  TWI_STATE_MASTER_TRANSMITTING,
  TWI_STATE_MASTER_RECEIVING,
  TWI_STATE_SLAVE_TRANSMITTING,
  TWI_STATE_SLAVE_RECEIVING
};

/* Error ----------------------------------------------------------------------
 * Errors happening on the bus are returned using a pointer to a twi_error.
 */
enum twi_error {
  TWI_ERROR_NONE,
  TWI_ERROR_MASTER_START_REJECTED,
  TWI_ERROR_MASTER_DATA_REJECTED,
  TWI_ERROR_MASTER_ARBITRATION_LOST,
  TWI_ERROR_BUS
};

/* General operation ------------------------------------------------------- */

/*
 * Initialize the TWI module and set up the SDA and SCL ports. Note that this
 * does not globally enable interrupts using sei(), which you will have to do
 * for the library to function.
 */
void twi_init();

/* Master operation -------------------------------------------------------- */

/*
 * Read data from a slave. Data is written into the data buffer, up to the
 * specified length. The length of data read is returned, in case the slave
 * returns less than requested. If the data requested would not fit into the
 * internal buffer, 0 will be returned and no TWI operations will be performed.
 */
uint8_t twi_read(uint8_t address,
                 uint8_t *data,
                 uint8_t length,
                 enum twi_error *error);

/*
 * Write data to a slave. If the data sent does not fit into the internal
 * buffer, false will be returned. Otherwise the return value will be true, and
 * an error will be indicated through the error pointer.
 */
bool twi_write(uint8_t address,
               uint8_t *data,
               uint8_t length,
               enum twi_error *error);

/* Slave operation --------------------------------------------------------- */

/*
 * Set the 7-bit address of this device. The recognize_general_call boolean
 * specifies whether or not messages on the address 0 should also be received.
 */
void twi_set_address(uint8_t address, bool recognize_general_call);

/*
 * Function called when data is received from a master.
 */
extern void (*twi_slave_receive_callback)(volatile uint8_t *data,
                                          uint8_t size);

/*
 * Function called when data is requested by a master. The response should be
 * returned using twi_transmit_reply.
 */
extern void (*twi_slave_transmit_callback)();

/*
 * Return a response to a request for data, as indicated by
 * twi_slave_transmit_callback. If the response does not fit into the internal
 * buffer, or no data is currently being requested, false will be returned.
 */
bool twi_transmit_reply(uint8_t *data, uint8_t size);

#endif /* PLEASANT_TWI_H */
