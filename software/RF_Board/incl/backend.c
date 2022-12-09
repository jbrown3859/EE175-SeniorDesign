#include <msp430.h>
#include <backend.h>

#include <serial.h>

char UART_RXBUF[256];
unsigned char UART_RX_PTR = 0;
unsigned char UART_RX_BASE = 0;

#define RADIOTYPE_SBAND 1

int read_UART_FIFO(void) {
    char c;
    if (UART_RX_PTR == UART_RX_BASE) {
        return -1; //empty
    }
    else {
        c = UART_RXBUF[UART_RX_BASE];
        UART_RX_BASE++;
        return c;
    }
}

unsigned char get_UART_FIFO_size(void) {
    return (UART_RX_PTR - UART_RX_BASE);
}


void main_loop(void) {
    enum State state = INIT;
    char command;

    for (;;) {
        //state actions
        switch(state) {
        case INIT:
            state = WAIT;
            break;
        case WAIT:
            if (get_UART_FIFO_size != 0) { //await command
                command = read_UART_FIFO();

                switch(command) {
                case 0x61: //get info
                    state = SEND_INFO;
                    break;
                }
            }
            break;
        case SEND_INFO:
            #ifdef RADIOTYPE_UHF
            putchar(0x41);
            #endif
            #ifdef RADIOTYPE_SBAND
            putchar(0x42);
            #endif
            state = WAIT;
            break;
        }
    }
}
