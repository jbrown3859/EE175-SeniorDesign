/*
 * RadioFunctions.h
 *
 *  Created on: Mar 3, 2023
 *      Author: vinhl
 */

#ifndef INC_RADIOFUNCTIONS_H_
#define INC_RADIOFUNCTIONS_H_

char GetRadioInfo(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void WriteTXBufferString(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, const char *a);
void WriteTXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, uint8_t *a, uint8_t length);
void WriteRXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, uint8_t *a, uint8_t length);
void GetTXBufferState(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
char GetTXActiveState(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void GetRXBufferState(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
char GetRXNumPackets(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
char GetRXNextPacketSize(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void FlushTXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void FlushRXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void ReadTXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void ReadRXBuffer(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal, uint8_t *buffer);
void IDLEMode(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void RXMode(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);
void TXMode(UART_HandleTypeDef uartRadio, UART_HandleTypeDef uartTerminal);





#endif /* INC_RADIOFUNCTIONS_H_ */
