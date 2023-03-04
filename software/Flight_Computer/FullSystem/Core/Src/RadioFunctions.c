/*
 * RadioFunctions.c
 *
 *  Created on: Mar 3, 2023
 *      Author: vinhl
 */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <TerminalOutputs.h>
#include "RadioFunctions.h"

char GetRadioInfo(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x61;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[19];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 19, 1000);

	//send_buffer(uartTerminal, Rx_data, 19);

	print_string(uartTerminal, "[");

	print_ASCII(uartTerminal, Rx_data[0]);
	print_ASCII(uartTerminal, Rx_data[1]);
	space(uartTerminal);

	print_ASCII(uartTerminal, Rx_data[2]);
	space(uartTerminal);

	for (int i = 3; i < 13; i++) {
		print_ASCII(uartTerminal, Rx_data[i]);
	}
	space(uartTerminal);
	for (int i = 13; i < 20; i++) {
		print_ASCII(uartTerminal, Rx_data[i]);
	}
	print_string(uartTerminal, "]");
	space(uartTerminal);

	return Rx_data[2];
}

void WriteTXBufferString(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, const char *a) {
	uint8_t data = 0x67;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t length = strlen(a);
//	print_string(uartTerminal, "Packet length: ");
//	print_hex(uartTerminal, length);
//	newline(uartTerminal);
	HAL_UART_Transmit(&uartRadio, &length, 1, 100);
	HAL_UART_Transmit(&uartRadio, (uint8_t*) a, length, 100);
}

void WriteTXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, uint8_t *a, uint8_t length) {
	uint8_t data = 0x67;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	HAL_UART_Transmit(&uartRadio, &length, 1, 100);
	HAL_UART_Transmit(&uartRadio, (uint8_t*) a, length, 100);
}

void WriteRXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, uint8_t *a, uint8_t length) {
	uint8_t data = 0x70;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
		HAL_UART_Transmit(&uartRadio, &length, 1, 100);
		HAL_UART_Transmit(&uartRadio, (uint8_t*) a, length, 100);
}

//void BurstWrite(const char *a) {
//	uint8_t data = 0x68;
//	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
//	uint8_t length = strlen(a);
//	HAL_UART_Transmit(&uartRadio, &length, 1, 100);
//	HAL_UART_Transmit(&uartRadio, (uint8_t*) a, length, 100);
//}

void GetTXBufferState(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x66;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 5, 1000);
	print_string(uartTerminal, "[");

	print_hex(uartTerminal, Rx_data[0]);
	space(uartTerminal);

	print_hex(uartTerminal, Rx_data[1]);
	space(uartTerminal);

	print_hex(uartTerminal, Rx_data[2]);
	print_hex(uartTerminal, Rx_data[3]);
	space(uartTerminal);

	print_hex(uartTerminal, Rx_data[4]);
	print_string(uartTerminal, "]");
	newline(uartTerminal);

	print_string(uartTerminal, "[Status: ");
	uint8_t radioStatus = Rx_data[0] & 0xC0;
	if (radioStatus == 0x00) {
		print_string(uartTerminal, "IDLE");
	} else if (radioStatus == 0x40) {
		print_string(uartTerminal, "RX");
	} else if (radioStatus == 0x80) {
		print_string(uartTerminal, "TX WAIT");
	} else if (radioStatus == 0xC0) {
		print_string(uartTerminal, "TX ACTIVE");
	}
	space(uartTerminal);

	print_string(uartTerminal, "	Packet: ");
	uint8_t bufferPacketOverflow = Rx_data[0] & 0x02;
	if (bufferPacketOverflow == 0x02) {
		print_string(uartTerminal, "Packet limit being reached.");
	} else if (bufferPacketOverflow == 0x00) {
		print_string(uartTerminal, "No overflow last write.");
	}
	space(uartTerminal);

	print_string(uartTerminal, "	Data: ");
	uint8_t bufferDataOverflow = Rx_data[0] & 0x01;
	if (bufferDataOverflow == 0x01) {
		print_string(uartTerminal, "Data buffer overflow.]");
	} else if (bufferDataOverflow == 0x00) {
		print_string(uartTerminal, "No overflow last write.]");
	}
	newline(uartTerminal);

	print_string(uartTerminal, "[Packets: ");
	print_decimal(uartTerminal, Rx_data[1], 2);
	print_string(uartTerminal, "	Bytes: ");
	uint16_t TXSize = ((uint16_t) Rx_data[2] << 8) + Rx_data[3];
	print_decimal(uartTerminal, TXSize, 5);
	print_string(uartTerminal, "	Packet size: ");
	print_decimal(uartTerminal, Rx_data[4], 3);
	print_string(uartTerminal, "]");
	newline(uartTerminal);

//	print_hex(uartTerminal, radioStatus); space(uartTerminal);
//	print_hex(uartTerminal, bufferPacketOverflow); space(uartTerminal);
//	print_hex(uartTerminal, bufferDataOverflow);
}

char GetTXActiveState(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal){
	uint8_t data = 0x66;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 5, 1000);

	uint8_t TX_ACTIVE_status = 0;
	uint8_t radioStatus = Rx_data[0] & 0xC0;
	if (radioStatus == 0xC0) {
		TX_ACTIVE_status = 1;
	}
	return TX_ACTIVE_status;
}

void GetRXBufferState(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x62;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 5, 1000);
	print_string(uartTerminal, "[");

	print_hex(uartTerminal, Rx_data[0]);
	space(uartTerminal);

	print_hex(uartTerminal, Rx_data[1]);
	space(uartTerminal);

	print_hex(uartTerminal, Rx_data[2]);
	print_hex(uartTerminal, Rx_data[3]);
	space(uartTerminal);

	print_hex(uartTerminal, Rx_data[4]);
	print_string(uartTerminal, "]");
	newline(uartTerminal);

	print_string(uartTerminal, "[Status: ");
	uint8_t radioStatus = Rx_data[0] & 0xC0;
	if (radioStatus == 0x00) {
		print_string(uartTerminal, "IDLE");
	} else if (radioStatus == 0x40) {
		print_string(uartTerminal, "RX");
	} else if (radioStatus == 0x80) {
		print_string(uartTerminal, "TX WAIT");
	} else if (radioStatus == 0xC0) {
		print_string(uartTerminal, "TX ACTIVE");
	}
	space(uartTerminal);

	print_string(uartTerminal, "	Packet: ");
	uint8_t bufferPacketOverflow = Rx_data[0] & 0x02;
	if (bufferPacketOverflow == 0x02) {
		print_string(uartTerminal, "Packet limit being reached.");
	} else if (bufferPacketOverflow == 0x00) {
		print_string(uartTerminal, "No overflow last write.");
	}
	space(uartTerminal);

	print_string(uartTerminal, "	Data: ");
	uint8_t bufferDataOverflow = Rx_data[0] & 0x01;
	if (bufferDataOverflow == 0x01) {
		print_string(uartTerminal, "Data buffer overflow.]");
	} else if (bufferDataOverflow == 0x00) {
		print_string(uartTerminal, "No overflow last write.]");
	}
	newline(uartTerminal);

	print_string(uartTerminal, "[Packets: ");
	print_decimal(uartTerminal, Rx_data[1], 2);
	print_string(uartTerminal, "	Bytes: ");
	uint16_t RXSize = ((uint16_t) Rx_data[2] << 8) + Rx_data[3];
	print_decimal(uartTerminal, RXSize, 5);
	print_string(uartTerminal, "	Packet size: ");
	print_decimal(uartTerminal, Rx_data[4], 3);
	print_string(uartTerminal, "]");
	newline(uartTerminal);

//	print_hex(uartTerminal, radioStatus); space(uartTerminal);
//	print_hex(uartTerminal, bufferPacketOverflow); space(uartTerminal);
//	print_hex(uartTerminal, bufferDataOverflow);
}

char GetRXNumPackets(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x62;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 5, 1000);
	return Rx_data[1];
}

char GetRXNextPacketSize(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal){
	uint8_t data = 0x62;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[5];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 5, 1000);
	return Rx_data[4];
}

void FlushTXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x69;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[1024];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);

	newline(uartTerminal);
	print_string(uartTerminal, "Flushing...TX buffer contents: ");
	newline(uartTerminal);
	send_buffer(uartTerminal, Rx_data, 1024);
}

void FlushRXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x65;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[1024];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);

	newline(uartTerminal);
	print_string(uartTerminal, "Flushing...RX buffer contents: ");
	newline(uartTerminal);
	send_buffer(uartTerminal, Rx_data, 1024);
}

void ReadTXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x71;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);

	const uint32_t MAX_RX_SIZE = 100;
	uint8_t Rx_data[MAX_RX_SIZE + 1]; // buffer to hold received data plus null terminator
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, MAX_RX_SIZE, 1000);
	Rx_data[MAX_RX_SIZE] = '\0'; // add null terminator to received data

	// Determine actual size of received data
	uint32_t size = 0;
	while (size < MAX_RX_SIZE && Rx_data[size] != '\0') {
		size++;
	}

//	print_string(uartTerminal, "TX buffer size: ");
//	print_decimal(uartTerminal, size, 3);
//	newline(uartTerminal);

	print_string(uartTerminal, "TX First Packet: ");
	newline(uartTerminal);
	send_buffer(uartTerminal, Rx_data, size);
}

void ReadRXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, uint8_t *buffer) {
	uint8_t data = 0x63;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);

	uint8_t Rx_data[100];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 100, 1000);
//	Rx_data[MAX_RX_SIZE] = '\0'; // add null terminator to received data
//
//	// Determine actual size of received data
//	uint32_t size = 0;
//	while (size < MAX_RX_SIZE && Rx_data[size] != '\0') {
//		size++;
//	}

//	print_string(uartTerminal, "TX buffer size: ");
//	print_decimal(uartTerminal, size, 3);
//	newline(uartTerminal);

	print_string(uartTerminal, "RX First Packet: ");
	newline(uartTerminal);
	send_buffer(uartTerminal, Rx_data, GetRXNextPacketSize(uartRadio, uartTerminal));
	buffer = Rx_data;
}

void IDLEMode(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x82;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[1];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);
	while (Rx_data[0] != 0x00) {
		HAL_UART_Transmit(&uartRadio, &data, 1, 100);
		memset(Rx_data, 0, sizeof(Rx_data));
		HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);
		print_string(uartTerminal, "IDLE, chip state:	");
		print_hex(uartTerminal, Rx_data[0]);
		newline(uartTerminal);
	}
	print_string(uartTerminal, "IDLE mode activated");
		newline(uartTerminal);
}

void RXMode(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x83;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[1];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);
	while (Rx_data[0] != 0x40) {
		HAL_UART_Transmit(&uartRadio, &data, 1, 100);
		memset(Rx_data, 0, sizeof(Rx_data));
		HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);
		print_string(uartTerminal, "RX, chip state:	");
		print_hex(uartTerminal, Rx_data[0]);
		newline(uartTerminal);
	}
	print_string(uartTerminal, "RX mode activated");
		newline(uartTerminal);
}

void TXMode(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal) {
	uint8_t data = 0x84;
	HAL_UART_Transmit(&uartRadio, &data, 1, 100);
	uint8_t Rx_data[1];
	memset(Rx_data, 0, sizeof(Rx_data));
	HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);
	while (Rx_data[0] != 0x80) {
		HAL_UART_Transmit(&uartRadio, &data, 1, 100);
		memset(Rx_data, 0, sizeof(Rx_data));
		HAL_UART_Receive(&uartRadio, Rx_data, 1024, 1000);
		print_string(uartTerminal, "TX, chip state:	");
		print_hex(uartTerminal, Rx_data[0]);
		newline(uartTerminal);
	}
	print_string(uartTerminal, "TX mode activated");
	newline(uartTerminal);
}
