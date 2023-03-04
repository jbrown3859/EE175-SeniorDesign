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
UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void print_ASCII(uint8_t data) {
	HAL_UART_Transmit(&huart2, &data, 1, 100);
}

void newline() {
	uint8_t buf[12];
	strcpy((char*) buf, "\r\n	");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void newlineFinal() {
	newline();
	uint8_t buf[12];
	strcpy((char*) buf, "\r\n");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void print_string(const char *a) {
	uint8_t buf[100];
	strncpy((char*) buf, a, sizeof(buf));
	buf[sizeof(buf) - 1] = '\0';
	HAL_UART_Transmit(&huart2, (uint8_t*) buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

void space() {
	uint8_t buf[12];
	strcpy((char*) buf, "	");
	HAL_UART_Transmit(&huart2, buf, strlen((char*) buf), 100);
	memset(buf, 0, sizeof(buf));
}

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
	//space();
	//memset(nibble, 0, sizeof(nibble));
}

void print_decimal(const long long data, const unsigned char len) {
	int i;
	char nibble;
	for (i = (len - 1); i >= 0; i--) {
		nibble = ((int) (data / pow(10, i)) % 10) + 48;
		HAL_UART_Transmit(&huart2, (uint8_t*) &nibble, sizeof(nibble), 10);
	}
}

uint8_t count = 0;
void count_init() {
	count++;
	print_decimal(count, 3);
	space();
}

void send_buffer(uint8_t *buffer, uint32_t buffer_length) {
	for (int i = 0; i < buffer_length; i++) {
		//HAL_UART_Transmit(&huart2, &buffer[i], 1, HAL_MAX_DELAY);
		print_hex(buffer[i]);
	}
	newline();
//	memset(buffer, 0, buffer_length);
}

void GetRadioInfo() {
	uint8_t data = 0x61;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[19];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 19, 1000);

	//send_buffer(Rx_data, 19);

	print_string("[");

	print_ASCII(Rx_data[0]);
	print_ASCII(Rx_data[1]);
	space();

	print_ASCII(Rx_data[2]);
	space();

	for (int i = 3; i < 13; i++) {
		print_ASCII(Rx_data[i]);
	}
	space();
	for (int i = 13; i < 20; i++) {
		print_ASCII(Rx_data[i]);
	}
	print_string("]");
	space();
}

void WriteTXBufferString(const char *a) {
	uint8_t data = 0x67;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t length = strlen(a);
//	print_string("Packet length: ");
//	print_hex(length);
//	newline();
	HAL_UART_Transmit(&huart1, &length, 1, 100);
	HAL_UART_Transmit(&huart1, (uint8_t*) a, length, 100);
}

void WriteTXBuffer(uint8_t *a, uint8_t length) {
	uint8_t data = 0x67;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	HAL_UART_Transmit(&huart1, &length, 1, 100);
	HAL_UART_Transmit(&huart1, (uint8_t*) a, length, 100);
}

void WriteRXBuffer(const char *a) {
	uint8_t data = 0x70;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t length = strlen(a);
//	print_string("Packet length: ");
//	print_hex(length);
//	newline();
	HAL_UART_Transmit(&huart1, &length, 1, 100);
	HAL_UART_Transmit(&huart1, (uint8_t*) a, length, 100);
}

//void BurstWrite(const char *a) {
//	uint8_t data = 0x68;
//	HAL_UART_Transmit(&huart1, &data, 1, 100);
//	uint8_t length = strlen(a);
//	HAL_UART_Transmit(&huart1, &length, 1, 100);
//	HAL_UART_Transmit(&huart1, (uint8_t*) a, length, 100);
//}

char GetTXBufferState() {
	uint8_t data = 0x66;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 5, 1000);
	print_string("[");

	print_hex(Rx_data[0]);
	space();

	print_hex(Rx_data[1]);
	space();

	print_hex(Rx_data[2]);
	print_hex(Rx_data[3]);
	space();

	print_hex(Rx_data[4]);
	print_string("]");
	newline();

	uint8_t TX_ACTIVE_status = 0;
	print_string("[Status: ");
	uint8_t radioStatus = Rx_data[0] & 0xC0;
	if (radioStatus == 0x00) {
		print_string("IDLE");
	} else if (radioStatus == 0x40) {
		print_string("RX");
	} else if (radioStatus == 0x80) {
		print_string("TX WAIT");
	} else if (radioStatus == 0xC0) {
		print_string("TX ACTIVE");
		TX_ACTIVE_status = 1;
	}
	space();

	print_string("	Packet: ");
	uint8_t bufferPacketOverflow = Rx_data[0] & 0x02;
	if (bufferPacketOverflow == 0x02) {
		print_string("Packet limit being reached.");
	} else if (bufferPacketOverflow == 0x00) {
		print_string("No overflow last write.");
	}
	space();

	print_string("	Data: ");
	uint8_t bufferDataOverflow = Rx_data[0] & 0x01;
	if (bufferDataOverflow == 0x01) {
		print_string("Data buffer overflow.]");
	} else if (bufferDataOverflow == 0x00) {
		print_string("No overflow last write.]");
	}
	newline();

	print_string("[Packets: ");
	print_decimal(Rx_data[1], 2);
	print_string("	Bytes: ");
	uint16_t TXSize = ((uint16_t) Rx_data[2] << 8) + Rx_data[3];
	print_decimal(TXSize, 5);
	print_string("	Packet size: ");
	print_decimal(Rx_data[4], 3);
	print_string("]");
	newline();

//	print_hex(radioStatus); space();
//	print_hex(bufferPacketOverflow); space();
//	print_hex(bufferDataOverflow);
	return TX_ACTIVE_status;
}

char GetRXBufferState() {
	uint8_t data = 0x62;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 5, 1000);
	print_string("[");

	print_hex(Rx_data[0]);
	space();

	print_hex(Rx_data[1]);
	space();

	print_hex(Rx_data[2]);
	print_hex(Rx_data[3]);
	space();

	print_hex(Rx_data[4]);
	print_string("]");
	newline();

	print_string("[Status: ");
	uint8_t radioStatus = Rx_data[0] & 0xC0;
	if (radioStatus == 0x00) {
		print_string("IDLE");
	} else if (radioStatus == 0x40) {
		print_string("RX");
	} else if (radioStatus == 0x80) {
		print_string("TX WAIT");
	} else if (radioStatus == 0xC0) {
		print_string("TX ACTIVE");
	}
	space();

	print_string("	Packet: ");
	uint8_t bufferPacketOverflow = Rx_data[0] & 0x02;
	if (bufferPacketOverflow == 0x02) {
		print_string("Packet limit being reached.");
	} else if (bufferPacketOverflow == 0x00) {
		print_string("No overflow last write.");
	}
	space();

	print_string("	Data: ");
	uint8_t bufferDataOverflow = Rx_data[0] & 0x01;
	if (bufferDataOverflow == 0x01) {
		print_string("Data buffer overflow.]");
	} else if (bufferDataOverflow == 0x00) {
		print_string("No overflow last write.]");
	}
	newline();

	print_string("[Packets: ");
	print_decimal(Rx_data[1], 2);
	print_string("	Bytes: ");
	uint16_t RXSize = ((uint16_t) Rx_data[2] << 8) + Rx_data[3];
	print_decimal(RXSize, 5);
	print_string("	Packet size: ");
	print_decimal(Rx_data[4], 3);
	print_string("]");
	newline();

//	print_hex(radioStatus); space();
//	print_hex(bufferPacketOverflow); space();
//	print_hex(bufferDataOverflow);
	return Rx_data[1];
}

char GetRXBufferLength(){
	uint8_t data = 0x62;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 5, 1000);
	return Rx_data[4];
}

void FlushTXBuffer() {
	uint8_t data = 0x69;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[1024];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);

	newline();
	print_string("TX buffer contents: ");
	newline();
	send_buffer(Rx_data, 1024);
}

void FlushRXBuffer() {
	uint8_t data = 0x65;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[1024];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);

	newline();
	print_string("RX buffer contents: ");
	newline();
	send_buffer(Rx_data, 1024);
}

void ReadTXBuffer() {
	uint8_t data = 0x71;
	HAL_UART_Transmit(&huart1, &data, 1, 100);

	const uint32_t MAX_RX_SIZE = 100;
	uint8_t Rx_data[MAX_RX_SIZE + 1]; // buffer to hold received data plus null terminator
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, MAX_RX_SIZE, 1000);
	Rx_data[MAX_RX_SIZE] = '\0'; // add null terminator to received data

	// Determine actual size of received data
	uint32_t size = 0;
	while (size < MAX_RX_SIZE && Rx_data[size] != '\0') {
		size++;
	}

//	print_string("TX buffer size: ");
//	print_decimal(size, 3);
//	newline();

	print_string("TX First Packet: ");
	newline();
	send_buffer(Rx_data, size);
}

char ReadRXBuffer(uint8_t *buffer) {
	uint8_t data = 0x63;
	HAL_UART_Transmit(&huart1, &data, 1, 100);

	const uint32_t MAX_RX_SIZE = 100;
	uint8_t Rx_data[MAX_RX_SIZE + 1]; // buffer to hold received data plus null terminator
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, MAX_RX_SIZE, 1000);
//	Rx_data[MAX_RX_SIZE] = '\0'; // add null terminator to received data
//
//	// Determine actual size of received data
//	uint32_t size = 0;
//	while (size < MAX_RX_SIZE && Rx_data[size] != '\0') {
//		size++;
//	}

//	print_string("TX buffer size: ");
//	print_decimal(size, 3);
//	newline();

	print_string("RX First Packet: ");
	newline();
	send_buffer(Rx_data, GetRXBufferLength());
	buffer = Rx_data;
	return GetRXBufferLength();
}

void IDLEMode() {
	uint8_t data = 0x82;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[1];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);
	while (Rx_data[0] != 0x00) {
		HAL_UART_Transmit(&huart1, &data, 1, 100);
		memset(Rx_data, 0, sizeof(Rx_data));
		HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);
		print_string("IDLE, chip state:	");
		print_hex(Rx_data[0]);
		newline();
	}
	print_string("IDLE mode activated");
		newline();
}

void RXMode() {
	uint8_t data = 0x83;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[1];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);
	while (Rx_data[0] != 0x40) {
		HAL_UART_Transmit(&huart1, &data, 1, 100);
		memset(Rx_data, 0, sizeof(Rx_data));
		HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);
		print_string("RX, chip state:	");
		print_hex(Rx_data[0]);
		newline();
	}
	print_string("RX mode activated");
		newline();
}

void TXMode() {
	uint8_t data = 0x84;
	HAL_UART_Transmit(&huart1, &data, 1, 100);
	uint8_t Rx_data[1];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);
	while (Rx_data[0] != 0x80) {
		HAL_UART_Transmit(&huart1, &data, 1, 100);
		memset(Rx_data, 0, sizeof(Rx_data));
		HAL_UART_Receive(&huart1, Rx_data, 1024, 1000);
		print_string("TX, chip state:	");
		print_hex(Rx_data[0]);
		newline();
	}
	print_string("TX mode activated");
	newline();
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
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1) {
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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

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
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
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
