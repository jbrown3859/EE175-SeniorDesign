/* serial.h
 *
 * This file defines UART configuration and communication functions for the MSP430FR2355
 *
 *
 */

#ifndef SERIAL_H_
#define SERIAL_H_

void Software_Trim(void);
void init_clock(void);

/* UART Functions */
void init_UART(void);

int getchar(void);
void putchar(char c);
void putchars(char* msg);

/* SPI functions */
void init_SPI_master(void);
void init_SPI_slave(void);

int SPI_RX(void);
void SPI_TX(char c);

#endif /* SERIAL_H_ */
