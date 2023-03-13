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
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TerminalOutputs.h>
#include <RadioFunctions.h>
#include <MPU6050Functions.h>
#include <BMM150Functions.h>
#include <CameraFunctions.h>
#include <LM75Functions.h>

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define	IDLE	0
#define	RX_MODE	1
#define	TX_MODE	2
#define	START	3

#define BMP 	0
#define JPEG	1
#define RAW	  2

#define Auto                 0
#define Sunny                1
#define Cloudy               2
#define Office               3
#define Home                 4

//Special effects

#define Antique                      0
#define Bluish                       1
#define Greenish                     2
#define Reddish                      3
#define BW                           4
#define Negative                     5
#define BWnegative                   6
#define Normal                       7
#define Sepia                        8
#define Overexposure                 9
#define Solarize                     10
#define  Blueish                     11
#define Yellowish                    12

#define OV2640_160x120 		0	//160x120
#define OV2640_176x144 		1	//176x144
#define OV2640_320x240 		2	//320x240
#define OV2640_352x288 		3	//352x288
#define OV2640_640x480		4	//640x480
#define OV2640_800x600 		5	//800x600
#define OV2640_1024x768		6	//1024x768
#define OV2640_1280x1024	7	//1280x1024
#define OV2640_1600x1200	8	//1600x1200

uint8_t format = RAW;
uint8_t light_mode = Auto;
uint8_t effects = Normal;
uint8_t resolution = OV2640_160x120;
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART3_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {
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
	MX_GPIO_Init();

	MX_USART2_UART_Init();
	MX_I2C1_Init();
	MX_SPI1_Init();
	MX_USART1_UART_Init();
	MX_USART3_UART_Init();
	/* USER CODE BEGIN 2 */
///////////////////////////////////////////////////////////////////////////////	Radio Init	//////////////////////////////////////////////////
	while (GetRadioInfo(huart1, huart2) != 0x53) {
		count_init(huart2);
		newlineFinal(huart2);
	}
	newline(huart2);
	while (GetRadioInfo(huart3, huart2) != 0x55) {
		count_init(huart2);
		newlineFinal(huart2);
	}
///////////////////////////////////////////////////////////////////////////////	Tel Init	//////////////////////////////////////////////////
	MPU6050_Init(hi2c1);

//	BMM150_Normal(hspi1);
//	BMM150_Set(hspi1);			//init in SM

	LM75_init(hi2c1);

	uint8_t commands[16];
	const uint8_t img_cmd160x120[4] = { 0x20, 0x01, 0x01, 0x02 };
	const uint8_t img_cmd320x240[4] = { 0x20, 0x02, 0x01, 0x02 };
	const uint8_t img_cmd640x480[4] = { 0x20, 0x03, 0x01, 0x02 };
	uint8_t TXPacket[32];
	int cmd_recieved = 0;
	uint8_t RadioStateSBand = IDLE;
	uint8_t RadioStateUHF = IDLE;
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1) {
		/* USER CODE END WHILE */
		count_init(huart2);
///////////////////////////////////////////////////////////////////////////////	SBand Trans		/////////////////////////////////////////////
		switch (RadioStateSBand) {
		case START:
			RadioStateSBand = IDLE;
			IDLEMode(huart1, huart2);
		case IDLE:
			RadioStateSBand = RX_MODE;
			RXMode(huart1, huart2);
			break;
		case RX_MODE:
			if (cmd_recieved == 1) {
				RadioStateSBand = TX_MODE;
				TXMode(huart1, huart2);
				FlushRXBuffer(huart1, huart2);
				cmd_recieved = 0;
			} else {
				RadioStateSBand = RX_MODE;
			}
			break;
		case TX_MODE:
			RadioStateSBand = RX_MODE;
			RXMode(huart1, huart2);
			break;
		}

///////////////////////////////////////////////////////////////////////////////	SBand SM	//////////////////////////////////////////////////
		switch (RadioStateSBand) {
		case IDLE:
			break;
		case RX_MODE:
			while (GetRXNumPackets(huart1, huart2) != 0) {
				GetRXBufferState(huart1, huart2);
				ReadRXBuffer(huart1, huart2, commands, 16);
				if (compare_buffer(huart2, commands, img_cmd160x120, 4)
						|| compare_buffer(huart2, commands, img_cmd320x240, 4)
						|| compare_buffer(huart2, commands, img_cmd640x480,
								4)) {
					print_string(huart2, "Image command recieved.");
					newline(huart2);
					if (commands[1] == 2) {
						resolution = OV2640_320x240;
					} else if (commands[1] == 3) {
						resolution = OV2640_640x480;
					} else {
						resolution = OV2640_160x120;
					}
					cmd_recieved = 1;
					break;
				}
				print_string(huart2, "Recieved Packet: ");
				send_buffer(huart2, commands, 16);
			}
			break;
		case TX_MODE:
			OV2640_init(hi2c1, hspi1, format, resolution, light_mode, effects);
			while (!OV2640_init(hi2c1, hspi1, format, resolution, light_mode,
					effects)) {
				print_string(huart2, "OV2640 not initialized");
				newline(huart2);
				HAL_Delay(1000);
			}
			print_string(huart2, "OV2640 initialized");
			newline(huart2);

			start_capture(hspi1, huart2);
			while (!capture_ready(hspi1)) {
				HAL_Delay(100);
				print_string(huart2, "Capture is not ready...delaying...");
				newline(huart2);
			}
			print_string(huart2, "Capture ready");
			newline(huart2);
			print_string(huart2, "Transmitting image...");
			uint8_t res_packet[18];
			uint16_t max_index;
			memset(res_packet, 0, 18);
			res_packet[0] = 0x52;
			if (resolution == OV2640_320x240) {
				res_packet[1] = 0x02;
				max_index = 4800;
			} else if (resolution == OV2640_640x480) {
				res_packet[1] = 0x03;
				max_index = 19200;
			} else {
				res_packet[1] = 0x01;
				max_index = 1200;
			}
			for (int i = 0; i < 10; i++) {
				WriteTXBuffer(huart1, huart2, res_packet, 18);
			}
			uint32_t length = read_fifo_length(hspi1);
			uint8_t img_buf[32];
			uint8_t img_packet[18];
			uint16_t img_index = 0;
			while (length >= 32 && img_index <= max_index) {
				get_FIFO_bytes(hspi1, img_buf, 32);
				//send_buffer(huart2, img_buf, 32);
				make_img_packet(img_buf, img_packet, img_index);
				while (GetTXActiveState(huart1, huart2) == 1)
					;
				WriteTXBuffer(huart1, huart2, img_packet, 18);
				WriteTXBuffer(huart1, huart2, img_packet, 18); //second send for better quality
				if (img_index == 0) {
					for (int i = 0; i < 10; i++) {
						WriteTXBuffer(huart1, huart2, img_packet, 18);
					}
				}
				img_index++;
				length -= 32;
			}
			reset_camera(hi2c1, hspi1, format);
			break;
		default:
			RadioStateSBand = IDLE;
		}

///////////////////////////////////////////////////////////////////////////////	UHF Trans	//////////////////////////////////////////////////
		switch (RadioStateUHF) {
		case START:
			RadioStateUHF = IDLE;
			IDLEMode(huart3, huart2);
		case IDLE:
			RadioStateUHF = TX_MODE;
			//FlushRXBuffer(huart3, huart2);
			TXMode(huart3, huart2);
			break;
		case RX_MODE:
//			RadioStateUHF = TX_MODE;
//			TXMode(huart3, huart2);
			break;
		case TX_MODE:
			RadioStateUHF = TX_MODE;
			//FlushRXBuffer(huart3, huart2);
			//RXMode(huart3, huart2);
			break;
		default:
			RadioStateSBand = IDLE;
		}

///////////////////////////////////////////////////////////////////////////////	UHF SM	//////////////////////////////////////////////////////
		switch (RadioStateUHF) {
		case START:
		case IDLE:
			break;
		case RX_MODE:
			break;
		case TX_MODE:
			BMM150_Normal(hspi1);
			BMM150_Set(hspi1);

			MPU6050_Read_Accel(hi2c1, huart2, TXPacket);
			MPU6050_Read_Gyro(hi2c1, huart2, TXPacket);
			print_string(huart2, "	");
			BMM150getData(hspi1, huart2, TXPacket);
			print_string(huart2, "	");
			TXPacket[0] = 0x54;
			TXPacket[1] = (uint8_t) (count >> 24) & 0xFF;
			TXPacket[2] = (uint8_t) (count >> 16) & 0xFF;
			TXPacket[3] = (uint8_t) (count >> 8) & 0xFF;
			TXPacket[4] = (uint8_t) count & 0xFF;
			TXPacket[14] = getTempLM75(hi2c1, huart2);
			TXPacket[15] = sum_bits(count, 32);
			TXPacket[16] = 0;
			for (int i = 0; i < 6; i++) {
				TXPacket[16] |= (sum_bits(TXPacket[i + 5], 8) & 0x01)
						<< (7 - i);
			}
			TXPacket[17] = 0;
			for (int i = 0; i < 4; i++) {
				TXPacket[17] |= (sum_bits(TXPacket[i + 11], 8) & 0x01)
						<< (7 - i);
			}
			TXPacket[31] = '\n';
			//newline(huart2);
			//print_string(huart2, "Sending Packet: ");
			//send_buffer(huart2, TXPacket, 32);
			while (GetTXActiveState(huart3, huart2) == 1) {
				//				print_string(huart2, "TX_ACTIVE");
				//				newline(huart2);
				//				HAL_Delay(10);
			}
			WriteTXBuffer(huart3, huart2, TXPacket, 32);
			break;
		default:
			RadioStateSBand = IDLE;
		}
		newlineFinal(huart2);

		/* USER CODE BEGIN 3 */

	}
	/* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
	RCC_OscInitTypeDef RCC_OscInitStruct = { 0 };
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { 0 };

	/** Initializes the RCC Oscillators according to the specified parameters
	 * in the RCC_OscInitTypeDef structure.
	 */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
	RCC_OscInitStruct.HSIState = RCC_HSI_ON;
	RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI_DIV2;
	RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL16;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		Error_Handler();
	}

	/** Initializes the CPU, AHB and APB buses clocks
	 */
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK
			| RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
		Error_Handler();
	}
}

/**
 * @brief I2C1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_I2C1_Init(void) {

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
	if (HAL_I2C_Init(&hi2c1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN I2C1_Init 2 */

	/* USER CODE END I2C1_Init 2 */

}

/**
 * @brief SPI1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_SPI1_Init(void) {

	/* USER CODE BEGIN SPI1_Init 0 */

	/* USER CODE END SPI1_Init 0 */

	/* USER CODE BEGIN SPI1_Init 1 */

	/* USER CODE END SPI1_Init 1 */
	/* SPI1 parameter configuration*/
	hspi1.Instance = SPI1;
	hspi1.Init.Mode = SPI_MODE_MASTER;
	hspi1.Init.Direction = SPI_DIRECTION_2LINES;
	hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
	hspi1.Init.NSS = SPI_NSS_SOFT;
	hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_4;
	hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
	hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	hspi1.Init.CRCPolynomial = 10;
	if (HAL_SPI_Init(&hspi1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN SPI1_Init 2 */

	/* USER CODE END SPI1_Init 2 */

}

/**
 * @brief USART1 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART1_UART_Init(void) {

	/* USER CODE BEGIN USART1_Init 0 */

	/* USER CODE END USART1_Init 0 */

	/* USER CODE BEGIN USART1_Init 1 */

	/* USER CODE END USART1_Init 1 */
	huart1.Instance = USART1;
	huart1.Init.BaudRate = 115200;
	huart1.Init.WordLength = UART_WORDLENGTH_8B;
	huart1.Init.StopBits = UART_STOPBITS_1;
	huart1.Init.Parity = UART_PARITY_NONE;
	huart1.Init.Mode = UART_MODE_TX_RX;
	huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart1.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart1) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART1_Init 2 */

	/* USER CODE END USART1_Init 2 */

}

/**
 * @brief USART2 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART2_UART_Init(void) {

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
	if (HAL_UART_Init(&huart2) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART2_Init 2 */

	/* USER CODE END USART2_Init 2 */

}

/**
 * @brief USART3 Initialization Function
 * @param None
 * @retval None
 */
static void MX_USART3_UART_Init(void) {

	/* USER CODE BEGIN USART3_Init 0 */

	/* USER CODE END USART3_Init 0 */

	/* USER CODE BEGIN USART3_Init 1 */

	/* USER CODE END USART3_Init 1 */
	huart3.Instance = USART3;
	huart3.Init.BaudRate = 115200;
	huart3.Init.WordLength = UART_WORDLENGTH_8B;
	huart3.Init.StopBits = UART_STOPBITS_1;
	huart3.Init.Parity = UART_PARITY_NONE;
	huart3.Init.Mode = UART_MODE_TX_RX;
	huart3.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&huart3) != HAL_OK) {
		Error_Handler();
	}
	/* USER CODE BEGIN USART3_Init 2 */

	/* USER CODE END USART3_Init 2 */

}

/**
 * @brief GPIO Initialization Function
 * @param None
 * @retval None
 */
static void MX_GPIO_Init(void) {
	GPIO_InitTypeDef GPIO_InitStruct = { 0 };

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);

	/*Configure GPIO pin Output Level */
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

	/*Configure GPIO pin : B1_Pin */
	GPIO_InitStruct.Pin = B1_Pin;
	GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

	/*Configure GPIO pin : PB1 */
	GPIO_InitStruct.Pin = GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	/*Configure GPIO pin : PC7 */
	GPIO_InitStruct.Pin = GPIO_PIN_7;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

	/*Configure GPIO pin : PA9 */
	GPIO_InitStruct.Pin = GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
void Error_Handler(void) {
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
