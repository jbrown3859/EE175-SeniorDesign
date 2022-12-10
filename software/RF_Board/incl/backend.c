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

    struct RadioInfo info;

    for (;;) {
        //state actions
        switch(state) {
        case INIT:
            info.frequency = 433500000;
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
            putchar(0xAA); //send preamble to reduce the chance of the groundstation application getting a false positive on device detection
            putchar(0xAA);
            #ifdef RADIOTYPE_UHF
            putchar('U');
            #endif
            #ifdef RADIOTYPE_SBAND
            putchar('S');
            #endif
            print_dec(info.frequency, 10);

            //putchars("\n\r");
            state = WAIT;
            break;
        }
    }
}
