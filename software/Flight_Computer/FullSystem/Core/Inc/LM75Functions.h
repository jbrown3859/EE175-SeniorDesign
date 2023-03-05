/*
 * LM75Functions.h
 *
 *  Created on: Mar 4, 2023
 *      Author: vinhl
 */

#ifndef INC_LM75FUNCTIONS_H_
#define INC_LM75FUNCTIONS_H_

void I2C_Read(I2C_HandleTypeDef i2c, uint16_t address, uint16_t reg, uint8_t *buf);

void I2C_Write(I2C_HandleTypeDef i2c, uint16_t address, uint16_t reg, uint8_t data);

void LM75_init(I2C_HandleTypeDef i2c);

void getTempLM75(I2C_HandleTypeDef i2c, UART_HandleTypeDef uart);

#endif /* INC_LM75FUNCTIONS_H_ */
