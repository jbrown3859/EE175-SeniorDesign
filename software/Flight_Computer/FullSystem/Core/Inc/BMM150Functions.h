/*
 * BMM150Functions.h
 *
 *  Created on: Mar 3, 2023
 *      Author: vinhl
 */

#ifndef INC_BMM150FUNCTIONS_H_
#define INC_BMM150FUNCTIONS_H_

char SPI_ReadBMM150(SPI_HandleTypeDef spi, uint8_t address);
void SPI_WriteBMM150(SPI_HandleTypeDef spi, uint8_t address, uint8_t data);
void BMM150_Normal(SPI_HandleTypeDef spi);
void BMM150_Set(SPI_HandleTypeDef spi);
void BMM150getData(SPI_HandleTypeDef spi, UART_HandleTypeDef uart, uint8_t *buf);

#endif /* INC_BMM150FUNCTIONS_H_ */
