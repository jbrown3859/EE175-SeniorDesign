/*
 * TerminalOutputs.h
 *
 *  Created on: Feb 24, 2023
 *      Author: vinhl
 */

#ifndef INC_TERMINALOUTPUTS_H_
#define INC_TERMINALOUTPUTS_H_

void print_ASCII(UART_HandleTypeDef uart, uint8_t data);

void newline(UART_HandleTypeDef uart);

void newlineFinal(UART_HandleTypeDef uart);

void print_string(UART_HandleTypeDef uart, const char *a);

void space(UART_HandleTypeDef uart);

void print_hex(UART_HandleTypeDef uart, char h);

void print_decimal(UART_HandleTypeDef uart, const long long data, const unsigned char len);

extern uint32_t count;
void count_init(UART_HandleTypeDef uart);

void send_buffer(UART_HandleTypeDef uart, uint8_t *buffer, uint32_t buffer_length);

#endif /* INC_TERMINALOUTPUTS_H_ */
