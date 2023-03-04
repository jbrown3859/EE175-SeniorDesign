/*
 * MPU6050Functions.c
 *
 *  Created on: Feb 27, 2023
 *      Author: vinhl
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TerminalOutputs.h>
#include <MPU6050Functions.h>

const uint16_t MPU6050_ADDR = 0xD0;
const uint16_t WHO_AM_I_REG = 0x75;
const uint16_t PWR_MGMT_1_REG = 0x6B;
const uint16_t SAMPLE_RATE_REG = 0x19;
const uint16_t ACCEL_CONFIG_REG = 0x1C;
const uint16_t GYRO_CONFIG_REG = 0x43;
const uint16_t ACCEL_XOUT_H_REG = 0x3B;
const uint16_t GYRO_XOUT_H_REG = 0x43;

void MPU6050_Init(I2C_HandleTypeDef i2c) {
	uint8_t check, data;
	HAL_I2C_Mem_Read(&i2c, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, 1000); //check if its there
	//print_hex(check);	space(uart);

	data = 0;
	//HAL_I2C_Mem_Read (&i2c, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &check, 1, 1000);		print_hex(check);	space(uart);	//check reg
	HAL_I2C_Mem_Write(&i2c, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &data, 1, 1000);//wakes up sensor by sending 0x00 to PWR_MGMT_1_REG
	//HAL_I2C_Mem_Read (&i2c, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &check, 1, 1000);		print_hex(check);	space(uart);	//check reg

	data = 0x07;
	HAL_I2C_Mem_Write(&i2c, MPU6050_ADDR, SAMPLE_RATE_REG, 1, &data, 1, 1000);// Set DATA RATE of 1KHz by writing SAMPLE_RATE_REG register

	data = 0x00;
	HAL_I2C_Mem_Write(&i2c, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &data, 1, 1000);	//set acceleration config

	data = 0x00;
	HAL_I2C_Mem_Write(&i2c, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &data, 1, 1000);//set gyro config
}

void MPU6050_Read_Accel(I2C_HandleTypeDef i2c, UART_HandleTypeDef uart, uint8_t *accel) {

	uint8_t data[6];
	HAL_I2C_Mem_Read(&i2c, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, data, 6, 1000);
//	float x_accel = (int16_t) (data[0] << 8 | data[1]) / 16384.0;
//	float y_accel = (int16_t) (data[2] << 8 | data[3]) / 16384.0;
//	float z_accel = (int16_t) (data[4] << 8 | data[5]) / 16384.0;
	uint8_t x_accel = data[0];
	uint8_t y_accel = data[2];
	uint8_t z_accel = data[4];

	accel[5] = x_accel;
	print_string(uart, "Acceleration x: ");
	print_hex(uart, x_accel);
	space(uart);

	accel[6] = y_accel;
	print_string(uart, "y: ");
	print_hex(uart, y_accel);
	space(uart);

	accel[7] = z_accel;
	print_string(uart, "z: ");
	print_hex(uart, z_accel);
	space(uart);
	space(uart);
	space(uart);
}

void MPU6050_Read_Gyro(I2C_HandleTypeDef i2c, UART_HandleTypeDef uart, uint8_t *gyro) {

	uint8_t data[6];
	HAL_I2C_Mem_Read(&i2c, MPU6050_ADDR, GYRO_XOUT_H_REG, 1, data, 6, 10);
//	float x_gyro = (int16_t) (data[0] << 8 | data[1]) / 131.0;
//	float y_gyro = (int16_t) (data[2] << 8 | data[3]) / 131.0;
//	float z_gyro = (int16_t) (data[4] << 8 | data[5]) / 131.0;
	uint8_t x_gyro = data[0];
	uint8_t y_gyro = data[2];
	uint8_t z_gyro = data[4];

	gyro[8] = x_gyro;
	print_string(uart, "Gyroscope: x: ");
	print_hex(uart, x_gyro);
	space(uart);

	gyro[9] = y_gyro;
	print_string(uart, "y: ");
	print_hex(uart, y_gyro);
	space(uart);

	gyro[10] = z_gyro;
	print_string(uart, "z: ");
	print_hex(uart, z_gyro);
	space(uart);
}
