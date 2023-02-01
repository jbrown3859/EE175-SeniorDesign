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

extern char SPI_TIMEOUT;

/* UART Functions and Variables */
extern char UART_RXBUF[256];
extern unsigned char UART_RX_PTR;
extern unsigned char UART_RX_BASE;

void init_UART(unsigned long);

int getchar(void);
void putchar(char c);
void putchars(char* msg);
void print_binary(char b);
void print_hex(char h);
void print_dec(const long long data, const unsigned char len);

/* SPI functions */
void init_SPI_master(void);
void set_SPI_mode(char phase, char polarity);
void set_SPI_timer(char mode);

//int SPI_RX(void);
void SPI_TX(char addr, char c);
char SPI_RX(char addr);

#endif /* SERIAL_H_ */
