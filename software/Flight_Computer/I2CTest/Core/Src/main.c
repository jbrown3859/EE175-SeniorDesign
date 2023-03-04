/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2023 STMicroelectronics.
 * All rights reserved.
 *
 * This software is licensed under terms that can be found in the LICENSE file
 * in the root directory of this software component.
 * If no LICENSE file comes with this software, it is provided AS-IS.
 *
 ******************************************************************************
 */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

//D10 (PB6)	--> SDL
//Leftmost Pin 9 (PB7) --> SDA
//3v3
static const uint16_t MPU6050_ADDR = 0xD0;
static const uint16_t WHO_AM_I_REG = 0x75;
static const uint16_t PWR_MGMT_1_REG = 0x6B;
static const uint16_t SAMPLE_RATE_REG = 0x19;
static const uint16_t ACCEL_CONFIG_REG = 0x1C;
static const uint16_t GYRO_CONFIG_REG = 0x43;
static const uint16_t ACCEL_XOUT_H_REG = 0x3B;
static const uint16_t GYRO_XOUT_H_REG = 0x43;
uint8_t count = 0;

//void print_float(float floatvalue) {
//	unsigned char *chptr;
//	 chptr = (unsigned char *) &floatvalue;
//	 Tx(*chptr++);Tx(*chptr++);Tx(*chptr++);Tx(*chptr);
//}


void print_hex(char h) {
	int i;
	char nibble;
	for (i = 1; i >= 0; i--) {
		nibble = ((h >> (4 * i)) & 0x0F);
		if (nibble < 10) { //decimal number
			nibble += 48;
			HAL_UART_Transmit(&huart2, (uint8_t*) &nibble, sizeof(nibble), 10);
		} else { //letter
			nibble += 55;
			HAL_UART_Transmit(&huart2, (uint8_t*) &nibble, sizeof(nibble), 10);
		}
	}
}

void print_decimal(const long long data, const unsigned char len) {
    int i;
    char nibble;
    for (i = (len-1); i>=0; i--) {
        nibble = ((int)(data / pow(10, i)) % 10) + 48;
        HAL_UART_Transmit(&huart2, (uint8_t*) &nibble, sizeof(nibble), 10);
    }
}

void newline() {
	uint8_t buf[12];
	strcpy((char*) buf, "	\r\n");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
}

void space() {
	uint8_t buf[12];
	strcpy((char*) buf, "	");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
}

void count_init() {
	count++;
	print_decimal(count, 3);
	space();
}

void MPU6050_Init() {
	uint8_t check, data;
	HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, WHO_AM_I_REG, 1, &check, 1, 1000);//check if its there
	//print_hex(check);	space();

	data = 0;
	//HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &check, 1, 1000);		print_hex(check);	space();	//check reg
	HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &data, 1, 1000);	//wakes up sensor by sending 0x00 to PWR_MGMT_1_REG
	//HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, PWR_MGMT_1_REG, 1, &check, 1, 1000);		print_hex(check);	space();	//check reg

	data = 0x07;
	HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, SAMPLE_RATE_REG, 1, &data, 1, 1000);// Set DATA RATE of 1KHz by writing SAMPLE_RATE_REG register

	data = 0x00;
	HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, ACCEL_CONFIG_REG, 1, &data, 1, 1000);	//set acceleration config

	data = 0x00;
	HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, GYRO_CONFIG_REG, 1, &data, 1, 1000);//set gyro config
}

void MPU6050_Read_Accel() {

	uint8_t data[6];
	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, ACCEL_XOUT_H_REG, 1, data, 6, 1000);
	float x_accel = (int16_t)(data[0] << 8 | data [1])/16384.0;
	float y_accel = (int16_t)(data[2] << 8 | data [3])/16384.0;
	float z_accel = (int16_t)(data[4] << 8 | data [5])/16384.0;

	uint8_t buf[12];
	strcpy((char*) buf, "Acceleration: x: ");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	print_decimal(x_accel * 10000, 5);	space();

	strcpy((char*) buf, "y: ");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	print_decimal(y_accel * 10000, 5);	space();

	strcpy((char*) buf, "z: ");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	print_decimal(z_accel *10000, 5);	space();
}

void MPU6050_Read_Gyro() {

	uint8_t data[6];
	HAL_I2C_Mem_Read (&hi2c1, MPU6050_ADDR, GYRO_XOUT_H_REG, 1, data, 6, 1000);
	float x_gyro = (int16_t)(data[0] << 8 | data [1])/131.0;
	float y_gyro = (int16_t)(data[2] << 8 | data [3])/131.0;
	float z_gyro = (int16_t)(data[4] << 8 | data [5])/131.0;

	uint8_t buf[12];
	strcpy((char*) buf, "Gyroscope: x: ");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	print_decimal(x_gyro*10000, 5);	space();

	strcpy((char*) buf, "y: ");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	print_decimal(y_gyro*10000, 5);	space();

	strcpy((char*) buf, "z: ");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	print_decimal(z_gyro*10000, 5);	space();
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */

  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LD2_Pin */
  GPIO_InitStruct.Pin = LD2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LD2_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1) {
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
