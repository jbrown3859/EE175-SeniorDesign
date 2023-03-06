/*
 * CameraFunctions.h
 *
 *  Created on: Mar 4, 2023
 *      Author: vinhl
 */

#ifndef INC_CAMERAFUNCTIONS_H_
#define INC_CAMERAFUNCTIONS_H_

struct sensor_reg {
	uint16_t reg;
	uint16_t val;
};

extern const struct sensor_reg OV2640_QVGA[];
extern const struct sensor_reg OV2640_JPEG_INIT[];
extern const struct sensor_reg OV2640_YUV422[];
extern const struct sensor_reg OV2640_JPEG[];
/* JPG 160x120 */
extern const struct sensor_reg OV2640_160x120_JPEG[];
/* JPG, 0x176x144 */
extern const struct sensor_reg OV2640_176x144_JPEG[];
/* JPG 320x240 */
extern const struct sensor_reg OV2640_320x240_JPEG[];
/* JPG 352x288 */
extern const struct sensor_reg OV2640_352x288_JPEG[];
/* JPG 640x480 */
extern const struct sensor_reg OV2640_640x480_JPEG[];
/* JPG 800x600 */
extern const struct sensor_reg OV2640_800x600_JPEG[];
/* JPG 1024x768 */
extern const struct sensor_reg OV2640_1024x768_JPEG[];
   /* JPG 1280x1024 */
extern const struct sensor_reg OV2640_1280x1024_JPEG[];
   /* JPG 1600x1200 */
extern const struct sensor_reg OV2640_1600x1200_JPEG[];
extern const struct sensor_reg OV2640_SVGA[];
extern const struct sensor_reg OV2640_640x480_JPEG2[];

char SPI_Read_Camera(SPI_HandleTypeDef spi, uint8_t address);
void Read_SPI_Regs(SPI_HandleTypeDef spi, UART_HandleTypeDef uart, uint8_t start_add, uint8_t end_add);
char SPI_Transfer_FIFO(SPI_HandleTypeDef spi, uint8_t address);
void SPI_Write_Camera(SPI_HandleTypeDef spi, uint8_t address, uint8_t data);
char I2C_Read_Camera(I2C_HandleTypeDef i2c, uint16_t address, uint16_t reg);
void I2C_Write_Camera(I2C_HandleTypeDef i2c, uint16_t address, uint16_t reg, uint8_t data);
void I2C_Write_sensor_reg(I2C_HandleTypeDef i2c, const struct sensor_reg reglist[]);
void OV2640_set_JPEG_size(I2C_HandleTypeDef i2c, uint8_t size);
void OV2640_set_Light_Mode(I2C_HandleTypeDef i2c, uint8_t Light_Mode);
void OV2640_set_Special_effects(I2C_HandleTypeDef i2c, uint8_t Special_effect);
void InitCAM(I2C_HandleTypeDef i2c, SPI_HandleTypeDef spi, uint8_t format);
int OV2640_init(I2C_HandleTypeDef i2c, SPI_HandleTypeDef spi, uint8_t format, uint8_t resolution, uint8_t light_mode, uint8_t effects);
void start_capture(SPI_HandleTypeDef spi, UART_HandleTypeDef uart);
int capture_ready(SPI_HandleTypeDef spi);
uint32_t read_fifo_length(SPI_HandleTypeDef spi);
int read_image_to_buffer(I2C_HandleTypeDef i2c, SPI_HandleTypeDef spi, UART_HandleTypeDef uart, uint8_t format, uint8_t **buffer, uint32_t *buffer_length);
#endif /* INC_CAMERAFUNCTIONS_H_ */
