/*
 * TerminalOutputs.c
 *
 *  Created on: Feb 27, 2023
 *      Author: vinhl
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TerminalOutputs.h>

void print_ASCII(UART_HandleTypeDef uart, uint8_t data) {
	HAL_UART_Transmit(&uart, &data, 1, 100);
}

void space(UART_HandleTypeDef uart) {
	uint8_t buf[12];
	strcpy((char*) buf, "	");
	HAL_UART_Transmit(&uart, buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void newline(UART_HandleTypeDef uart) {
	uint8_t buf[12];
	strcpy((char*) buf, "\r\n	");
	HAL_UART_Transmit(&uart, buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void newlineFinal(UART_HandleTypeDef uart) {
	newline(uart);
	uint8_t buf[12];
	strcpy((char*) buf, "\r\n");
	HAL_UART_Transmit(&uart, buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void print_string(UART_HandleTypeDef uart, const char *a) {
	uint8_t buf[100];
	strncpy((char*) buf, a, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';
	HAL_UART_Transmit(&uart, (uint8_t*) buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void print_decimal(UART_HandleTypeDef uart, const long long data, const unsigned char len) {
	int i;
	char nibble;
	for (i = (len - 1); i >= 0; i--) {
		nibble = ((int) (data / pow(10, i)) % 10) + 48;
		HAL_UART_Transmit(&uart, (uint8_t*) &nibble, sizeof(nibble), 10);
	}
}

void print_hex(UART_HandleTypeDef uart, char h) {
	int i;
	char nibble;
	for (i = 1; i >= 0; i--) {
		nibble = ((h >> (4 * i)) & 0x0F);
		if (nibble < 10) { //decimal number
			nibble += 48;
			HAL_UART_Transmit(&uart, (uint8_t*) &nibble, sizeof(nibble), 10);
		} else { //letter
			nibble += 55;
			HAL_UART_Transmit(&uart, (uint8_t*) &nibble, sizeof(nibble), 10);
		}
	}
	//space();
	//memset(nibble, 0, sizeof(nibble));
}

void send_buffer(UART_HandleTypeDef uart, uint8_t *buffer, uint32_t buffer_length) {
	for (int i = 0; i < buffer_length; i++) {
		//HAL_UART_Transmit(&uart, &buffer[i], 1, HAL_MAX_DELAY);
		print_hex(uart, buffer[i]);
	}
	newline(uart);
	memset(buffer, 0, buffer_length);
}

uint8_t count = 0;
void count_init(UART_HandleTypeDef uart) {
	count++;
	print_decimal(uart, count, 3);
	space(uart);
}
