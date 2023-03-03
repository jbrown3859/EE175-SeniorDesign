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
uint8_t resolution = OV2640_160x120;
uint8_t Light_Mode = Auto;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
static const uint8_t CLPD_reg = 0x07;
static const uint8_t test_reg = 0x00;
static const uint8_t Sensor_interface_timing_reg = 0x02;
static const uint16_t Sensor_addr_write = 0x60;
static const uint16_t Sensor_addr_read = 0x61;
static const uint16_t OV2640_CHIPID_HIGH = 0x0A;
static const uint16_t OV2640_CHIPID_LOW = 0x0B;

#define ARDUCHIP_FIFO      		0x04  //FIFO and I2C control
#define FIFO_CLEAR_MASK    		0x01
#define FIFO_START_MASK    		0x02

#define FIFO_SIZE1				0x42  //Camera write FIFO size[7:0] for burst to read
#define FIFO_SIZE2				0x43  //Camera write FIFO size[15:8]
#define FIFO_SIZE3				0x44  //Camera write FIFO size[18:16]
#define MAX_FIFO_SIZE			0x7FFFFF		//8MByte

#define BURST_FIFO_READ			0x3C  //Burst FIFO read operation
#define SINGLE_FIFO_READ		0x3D  //Single FIFO read operation

#define ARDUCHIP_TRIG      		0x41  //Trigger source
#define CAP_DONE_MASK      		0x08

int capture = 0;

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#define pgm_read_word(x)        ( ((*((unsigned char *)x + 1)) << 8) + (*((unsigned char *)x)))
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
uint8_t count = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void newline() {
	uint8_t buf[12];
	strcpy((char*) buf, "	\r\n");
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

void send_buffer(uint8_t *buffer, uint32_t buffer_length) {
	for (int i = 0; i < buffer_length; i++) {
		//HAL_UART_Transmit(&huart2, &buffer[i], 1, HAL_MAX_DELAY);
		print_hex(buffer[i]);
	}
}

void count_init() {
	count++;
	print_decimal(count, 3);
	space();
}

char SPI_Read(uint8_t address) {
	char data;
	address = address & 0x7F;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, (uint8_t*) &(address), 1, 100);
	HAL_SPI_Receive(&hspi2, (uint8_t*) &data, 1, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
	return data;
}

void Read_SPI_Regs(uint8_t start_add, uint8_t end_add) {
	uint8_t i;;
	for(i = start_add; i <= end_add; i++){
		print_hex(i);
		print_string(": ");
		print_hex(SPI_Read(i));
		newline();
	}
}

char SPI_Transfer_FIFO(uint8_t address) {
	char data;
	address = address & 0x7F;
	HAL_SPI_TransmitReceive(&hspi2, (uint8_t*) &(address), (uint8_t*) &data, 1,
			100);
	return data;
}

void SPI_Write(uint8_t address, uint8_t data) {
	address = address | 0x80;
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	HAL_SPI_Transmit(&hspi2, (uint8_t*) &(address), 1, 100);
	HAL_SPI_Transmit(&hspi2, (uint8_t*) &(data), 1, 100);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_SET);
}

char I2C_Read(uint16_t address, uint16_t reg) {
	uint8_t data;
	HAL_I2C_Mem_Read(&hi2c1, address, reg & 0x00FF, 1, &data, 1, 1000);
	return data;
}

void I2C_Write(uint16_t address, uint16_t reg, uint8_t data) {
	data = data & 0x00FF;
	HAL_I2C_Mem_Write(&hi2c1, address, reg & 0x00FF, 1, &data, 1, 1000);
}

void I2C_Write_sensor_reg(const struct sensor_reg reglist[]) {
	uint16_t reg_addr = 0;
	uint16_t reg_val = 0;
	const struct sensor_reg *next = reglist;
	while ((reg_addr != 0xff) | (reg_val != 0xff)) {
		reg_addr = pgm_read_word(&next->reg);
		reg_val = pgm_read_word(&next->val);
		I2C_Write(Sensor_addr_write, reg_addr, reg_val);
//	    print_hex(reg_addr);
//	    print_string(": ");
//	    print_hex(I2C_Read(Sensor_addr_read, reg_addr));
//	    newline();
		next++;
	}
}

void OV2640_set_JPEG_size(uint8_t size) {
	switch (size) {
	case OV2640_160x120:
		I2C_Write_sensor_reg(OV2640_160x120_JPEG);
		break;
	case OV2640_176x144:
		I2C_Write_sensor_reg(OV2640_176x144_JPEG);
		break;
	case OV2640_320x240:
		I2C_Write_sensor_reg(OV2640_320x240_JPEG);
		break;
	case OV2640_352x288:
		I2C_Write_sensor_reg(OV2640_352x288_JPEG);
		break;
	case OV2640_640x480:
		I2C_Write_sensor_reg(OV2640_640x480_JPEG);
		break;
	case OV2640_800x600:
		I2C_Write_sensor_reg(OV2640_800x600_JPEG);
		break;
	case OV2640_1024x768:
		I2C_Write_sensor_reg(OV2640_1024x768_JPEG);
		break;
	case OV2640_1280x1024:
		I2C_Write_sensor_reg(OV2640_1280x1024_JPEG);
		break;
	case OV2640_1600x1200:
		I2C_Write_sensor_reg(OV2640_1600x1200_JPEG);
		break;
	default:
		I2C_Write_sensor_reg(OV2640_320x240_JPEG);
		break;
	}
}

void OV2640_set_Light_Mode(uint8_t Light_Mode) {
	switch (Light_Mode) {

	case Auto:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0xc7, 0x00); //AWB on
		break;
	case Sunny:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0xc7, 0x40); //AWB off
		I2C_Write(Sensor_addr_write, 0xcc, 0x5e);
		I2C_Write(Sensor_addr_write, 0xcd, 0x41);
		I2C_Write(Sensor_addr_write, 0xce, 0x54);
		break;
	case Cloudy:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0xc7, 0x40); //AWB off
		I2C_Write(Sensor_addr_write, 0xcc, 0x65);
		I2C_Write(Sensor_addr_write, 0xcd, 0x41);
		I2C_Write(Sensor_addr_write, 0xce, 0x4f);
		break;
	case Office:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0xc7, 0x40); //AWB off
		I2C_Write(Sensor_addr_write, 0xcc, 0x52);
		I2C_Write(Sensor_addr_write, 0xcd, 0x41);
		I2C_Write(Sensor_addr_write, 0xce, 0x66);
		break;
	case Home:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0xc7, 0x40); //AWB off
		I2C_Write(Sensor_addr_write, 0xcc, 0x42);
		I2C_Write(Sensor_addr_write, 0xcd, 0x3f);
		I2C_Write(Sensor_addr_write, 0xce, 0x71);
		break;
	default:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0xc7, 0x00); //AWB on
		break;
	}
}

void OV2640_set_Special_effects(uint8_t Special_effect) {
	switch (Special_effect) {
	case Antique:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x18);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x40);
		I2C_Write(Sensor_addr_write, 0x7d, 0xa6);
		break;
	case Bluish:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x18);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0xa0);
		I2C_Write(Sensor_addr_write, 0x7d, 0x40);
		break;
	case Greenish:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x18);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x40);
		I2C_Write(Sensor_addr_write, 0x7d, 0x40);
		break;
	case Reddish:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x18);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x40);
		I2C_Write(Sensor_addr_write, 0x7d, 0xc0);
		break;
	case BW:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x18);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		break;
	case Negative:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x40);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		break;
	case BWnegative:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x58);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		break;
	case Normal:
		I2C_Write(Sensor_addr_write, 0xff, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x00);
		I2C_Write(Sensor_addr_write, 0x7d, 0x00);
		I2C_Write(Sensor_addr_write, 0x7c, 0x05);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		I2C_Write(Sensor_addr_write, 0x7d, 0x80);
		break;
	}
}

void InitCAM() {
	I2C_Write(Sensor_addr_write, 0xFF, 0x01);
	//	print_string("0xFF: ");
	//	print_hex(I2C_Read(Sensor_addr_read, 0xFF));
	I2C_Write(Sensor_addr_write, 0x12, 0x80);	//reset common control
	HAL_Delay(100);
	if (format == JPEG) {
		//		print_string("OV2640_JPEG_INIT");
		//		newline();
		I2C_Write_sensor_reg(OV2640_JPEG_INIT);
		I2C_Write_sensor_reg(OV2640_YUV422);
		I2C_Write_sensor_reg(OV2640_JPEG);
		I2C_Write(Sensor_addr_write, 0xff, 0x01);
		I2C_Write(Sensor_addr_write, 0x15, 0x00);
		I2C_Write_sensor_reg(OV2640_320x240_JPEG);
	} else {
		I2C_Write_sensor_reg(OV2640_QVGA);
	}
}

int OV2640_init() {
	//Reset CPLD
	SPI_Write(CLPD_reg, 0x80);
	SPI_Write(CLPD_reg, 0x00);
	// Check if the ArduCAM SPI bus is OK
	SPI_Write(test_reg, 0x55);
//	print_string("test reg: ");
//	print_hex(SPI_Read(test_reg));
	if (SPI_Read(test_reg) != 0x55) {
		return 0;
	}
	//change MCU to display LCD
	SPI_Write(Sensor_interface_timing_reg, 0x00);
	//	  print_string("Sensor_interface_timing_reg: ");
	//	  print_hex(SPI_Read(Sensor_interface_timing_reg));
	//check if its the OV2640
	// Check if the camera module type is OV2640
	I2C_Write(Sensor_addr_write, 0xFF, 0x01);
	//	  	print_string("sensor write: ");
	//	  	print_hex(I2C_Read(Sensor_addr_read, 0xFF));
	uint8_t vid, pid;
	vid = I2C_Read(Sensor_addr_read, OV2640_CHIPID_HIGH);
	pid = I2C_Read(Sensor_addr_read, OV2640_CHIPID_LOW);
	if ((vid != 0x26) && ((pid != 0x41) || (pid != 0x42))) {
		return 0;
	}
//	print_string("vid: ");
//	print_hex(vid);
//	print_string("pid: ");
//	print_hex(pid);

	InitCAM();
	OV2640_set_JPEG_size(resolution);
	OV2640_set_Light_Mode(Light_Mode);
	OV2640_set_Special_effects(Normal);
	HAL_Delay(1000);
	return 1;
}

void start_capture() {
	print_string("Starting image capture...");
	newline();
	SPI_Write(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);	//flush_fifo
	//print_hex(SPI_Read(ARDUCHIP_FIFO));
	SPI_Write(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);	//clear fifo_flag
	//print_hex(SPI_Read(ARDUCHIP_FIFO));
	SPI_Write(ARDUCHIP_FIFO, FIFO_START_MASK);	//start capture
	//print_hex(SPI_Read(ARDUCHIP_FIFO));
	print_string("Image capture started!");
	newline();
}

int capture_ready() {
	uint8_t temp;
	temp = SPI_Read(ARDUCHIP_TRIG);
	temp = temp & CAP_DONE_MASK;
	return temp;
}

uint32_t read_fifo_length() {
	uint32_t len1, len2, len3, length = 0;
	len1 = SPI_Read(FIFO_SIZE1);
	len2 = SPI_Read(FIFO_SIZE2);
	len3 = SPI_Read(FIFO_SIZE3) & 0x7f;
	length = ((len3 << 16) | (len2 << 8) | len1) & 0x07fffff;
	return length;
}

int read_image_to_buffer(uint8_t **buffer, uint32_t *buffer_length) {
	if (!capture_ready()) {
		print_string("Image capture not ready\r\n");
		return 0;
	} else {
		print_string("Reading image to buffer...\r\n");
	}

	// Get the image file length
	uint32_t length = read_fifo_length();
	*buffer_length = length;

	if (length >= MAX_FIFO_SIZE) {
		print_string("length larger than MAX_FIFO_SIZE, buffer length: ");
		print_hex(length);
		return 0;
	}
	if (length == 0) {
		print_string("length is 0, buffer length: ");
		print_hex(length);
		return 0;
	}

	// create the buffer
	uint8_t *buf = (uint8_t*) malloc(length * sizeof(uint8_t));

	uint8_t temp = 0;

	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);
	SPI_Transfer_FIFO(BURST_FIFO_READ);

	while (length--) {
		temp = SPI_Transfer_FIFO(0x00);
		//Read JPEG data from FIFO
		print_hex(temp);
	}

	SPI_Write(ARDUCHIP_FIFO, FIFO_CLEAR_MASK);

	InitCAM();
	//OV2640_set_JPEG_size(resolution);

	// return the buffer
	*buffer = buf;

	return 1;
}

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
	MX_SPI2_Init();
	/* USER CODE BEGIN 2 */

	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	OV2640_init();
	while (!OV2640_init()) {
		print_string("OV2640 not initialized");
		newline();
		HAL_Delay(1000);
	}
	capture = 1;
	print_string("OV2640 initialized");
	newline();
	Read_SPI_Regs(0x00, 0x48);
	while (1) {
		/* USER CODE END WHILE */
		count_init();
		newline();
		start_capture();
		while (!capture_ready()) {
			HAL_Delay(100);
			print_string("Capture is not ready...delaying...");
			newline();
		}
		print_string("Capture ready");
		newline();

		uint8_t *buffer;
		uint32_t length;

		print_string("Preparing to capture... ");
		if (read_image_to_buffer(&buffer, &length)) {
			print_string("\r\nlength: ");
			print_decimal(length, 8);
			print_string("\r\nImage read to buffer! ");
			print_string("Writing image to file...\r\n");
			//send_buffer(buffer, length);
			print_string("\r\nImage read to file");
		} else {
			print_string("Failed to read image");
		}

		newline();
		HAL_Delay(1000);

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
  * @brief SPI2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI2_Init(void)
{

  /* USER CODE BEGIN SPI2_Init 0 */

  /* USER CODE END SPI2_Init 0 */

  /* USER CODE BEGIN SPI2_Init 1 */

  /* USER CODE END SPI2_Init 1 */
  /* SPI2 parameter configuration*/
  hspi2.Instance = SPI2;
  hspi2.Init.Mode = SPI_MODE_MASTER;
  hspi2.Init.Direction = SPI_DIRECTION_2LINES;
  hspi2.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi2.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi2.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi2.Init.NSS = SPI_NSS_SOFT;
  hspi2.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi2.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi2.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi2.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi2.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI2_Init 2 */

  /* USER CODE END SPI2_Init 2 */

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, GPIO_PIN_RESET);

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

  /*Configure GPIO pin : PB1 */
  GPIO_InitStruct.Pin = GPIO_PIN_1;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

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
