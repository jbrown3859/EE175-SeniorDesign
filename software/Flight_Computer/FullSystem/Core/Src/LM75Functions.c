/*
 * LM75Functions.c
 *
 *  Created on: Mar 5, 2023
 *      Author: vinhl
 */


#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TerminalOutputs.h>
#include <LM75Functions.h>

const uint16_t LM75_ADDR = 0x96;
const uint16_t LM75_CONFIG_REG = 0x01;
const uint16_t LM75_TEMP_REG = 0x00;

void I2C_Read(I2C_HandleTypeDef i2c, uint16_t address, uint16_t reg, uint8_t *buf) {
	HAL_I2C_Mem_Read(&i2c, address, reg & 0x00FF, 1, buf, 2, 10);
}

void I2C_Write(I2C_HandleTypeDef i2c, uint16_t address, uint16_t reg, uint8_t data) {
	data = data & 0x00FF;
	HAL_I2C_Mem_Write(&i2c, address, reg & 0x00FF, 1, &data, 1, 10);
}

void LM75_init(I2C_HandleTypeDef i2c) {
	I2C_Write(i2c, LM75_ADDR, LM75_CONFIG_REG, 0x00);	//config temp
}

uint8_t getTempLM75(I2C_HandleTypeDef i2c, UART_HandleTypeDef uart) {
	uint8_t temp[2];
	I2C_Write(i2c, LM75_ADDR, LM75_CONFIG_REG, 0x00);
	I2C_Read(i2c, LM75_ADDR, LM75_TEMP_REG, temp);

	int16_t temporary =  ((uint16_t) temp[0] << 8) | (uint16_t)temp[1];
	temporary /= 128;
	temporary = (char) (temporary & 0xFF);

	print_string(uart, "Temperature: ");
	//send_buffer(uart, temp, 2);
	print_decimal(uart, temporary/2, 2);
	print_string(uart, " celsius");
	return temporary;
}
