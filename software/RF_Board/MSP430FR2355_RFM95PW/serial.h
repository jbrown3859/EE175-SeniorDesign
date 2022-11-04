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
void print_binary(char b);
void print_hex(char h);

/* SPI functions */
void init_SPI_master(void);
void set_SPI_timer(char mode);

//int SPI_RX(void);
char SPI_TX(char c);
char SPI_RX(char addr);

#endif /* SERIAL_H_ */
