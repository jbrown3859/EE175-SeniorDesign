/*
 * MPU6050Functions.h
 *
 *  Created on: Feb 24, 2023
 *      Author: vinhl
 */

#ifndef INC_MPU6050FUNCTIONS_H_
#define INC_MPU6050FUNCTIONS_H_

extern const uint16_t MPU6050_ADDR;
extern const uint16_t WHO_AM_I_REG;
extern const uint16_t PWR_MGMT_1_REG;
extern const uint16_t SAMPLE_RATE_REG;
extern const uint16_t ACCEL_CONFIG_REG;
extern const uint16_t GYRO_CONFIG_REG;
extern const uint16_t ACCEL_XOUT_H_REG;
extern const uint16_t GYRO_XOUT_H_REG;

void MPU6050_Init(I2C_HandleTypeDef i2c);

void MPU6050_Read_Accel(I2C_HandleTypeDef i2c, UART_HandleTypeDef uart, uint8_t *accel);

void MPU6050_Read_Gyro(I2C_HandleTypeDef i2c, UART_HandleTypeDef uart, uint8_t *gyro);

#endif /* INC_MPU6050FUNCTIONS_H_ */
