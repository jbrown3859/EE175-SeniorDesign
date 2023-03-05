/*
 * BMM150Functions.c
 *
 *  Created on: Mar 3, 2023
 *      Author: vinhl
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TerminalOutputs.h>
#include "BMM150Functions.h"

char SPI_ReadBMM150(SPI_HandleTypeDef spi, uint8_t address) {
	char data;
	address = address | 0x80;
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&spi, (uint8_t*) &(address), 1, 100);
	HAL_SPI_Receive(&spi, (uint8_t*) &data, 1, 10);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
	return data;
}

void SPI_WriteBMM150(SPI_HandleTypeDef spi, uint8_t address, uint8_t data) {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&spi, (uint8_t*) &(address), 1, 100);
	HAL_SPI_Transmit(&spi, (uint8_t*) &(data), 1, 10);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
}

void BMM150_Normal(SPI_HandleTypeDef spi) {
	uint8_t temp;
	SPI_WriteBMM150(spi, 0x4B, 0x01); //set to sleep
	temp = SPI_ReadBMM150(spi, 0x4C);
	temp &= 0xF9; //clears last two bits
	SPI_WriteBMM150(spi, 0x4C, temp);	//set to normal
}

void BMM150_Set(SPI_HandleTypeDef spi) {
	uint8_t temp;
	temp = SPI_ReadBMM150(spi, 0x4E);
	temp &= 0x38; //  0011 1000
	SPI_WriteBMM150(spi, 0x4E, temp);	//set to normal
}

void BMM150getData(SPI_HandleTypeDef spi, UART_HandleTypeDef uart, uint8_t *buf) {
	char x_mag = SPI_ReadBMM150(spi, 0x43);
	char y_mag = SPI_ReadBMM150(spi, 0x45);
	char z_mag = SPI_ReadBMM150(spi, 0x47);

	buf[11] = x_mag;
	print_string(uart, "Magnetometer x: ");
	print_hex(uart, x_mag);
	//print_hex(uart, SPI_ReadBMM150(spi, 0x42));

	buf[12] = y_mag;
	print_string(uart, "	y: ");
	print_hex(uart, y_mag);
	//print_hex(uart, SPI_ReadBMM150(spi, 0x44));

	buf[13] = z_mag;
	print_string(uart, "	z: ");
	print_hex(uart, z_mag);
	//print_hex(uart, SPI_ReadBMM150(spi, 0x46));
}

