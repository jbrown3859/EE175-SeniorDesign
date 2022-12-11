#include <msp430.h>
#include <backend.h>
#include <serial.h>
#include <util.h>

#define RADIOTYPE_SBAND 1

/* UART data structure */
char UART_RXBUF[256];
unsigned char UART_RX_PTR = 0;
unsigned char UART_RX_BASE = 0;

/* Radio data structure*/
char RADIO_RXBUF[2048];
unsigned int RF_RX_HEAD = 0;
unsigned int RF_RX_BASE = 0;

unsigned int RADIO_RX_PTRS[256]; //pointers to packet locations inside the RX queue
unsigned char RF_RX_PTR_HEAD = 0;
unsigned char RF_RX_PTR_BASE = 0;

char timeout_flag = 0;

/* UART comms */
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

/* RF Queue */
void write_RX_FIFO(char* data, unsigned char len) {
    unsigned char i;

    for(i=0;i<len;i++) {
        RADIO_RXBUF[RF_RX_HEAD] = data[i];
        RF_RX_HEAD = RF_RX_HEAD < 2047 ? RF_RX_HEAD + 1 : 0;
    }

    RADIO_RX_PTRS[RF_RX_PTR_HEAD] = RF_RX_HEAD; //push pointer to queue
    RF_RX_PTR_HEAD++;
}

void read_RX_FIFO(void) {
    unsigned int i;
    if (RF_RX_PTR_HEAD != RF_RX_PTR_BASE) { //if not empty
        for(i=RF_RX_BASE;i!=RADIO_RX_PTRS[RF_RX_PTR_BASE];i = (i < 2047 ? i+1 : 0)) { //eat up from bottom
            putchar(RADIO_RXBUF[RF_RX_BASE]);
            RF_RX_BASE = RF_RX_BASE < 2047 ? RF_RX_BASE + 1 : 0;
        }
        RF_RX_PTR_BASE++;
    }
}

void main_loop(void) {
    enum State state = INIT;
    char command;

    struct RadioInfo info;

    for (;;) {
        //state actions
        switch(state) {
        case INIT:
            info.frequency = 2450000000;
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

            /* test */
            /*
            putchars("===\n\r");
            putchars("Writing FIFO");
            write_RF_FIFO(dummyPacket, 16);

            putchars("\n\rRX_BASE     ");
            print_dec(RF_RX_BASE, 4);
            putchars("\n\rRX_HEAD     ");
            print_dec(RF_RX_HEAD, 4);

            putchars("\n\rRX_PTR_BASE ");
            print_dec(RF_RX_PTR_BASE, 4);
            putchars("\n\rRX_PTR_HEAD ");
            print_dec(RF_RX_PTR_HEAD, 4);

            putchars("\n\rReading FIFO\n\r");
            read_RF_FIFO();

            putchars("\n\rRX_BASE     ");
            print_dec(RF_RX_BASE, 4);
            putchars("\n\rRX_HEAD     ");
            print_dec(RF_RX_HEAD, 4);

            putchars("\n\rRX_PTR_BASE ");
            print_dec(RF_RX_PTR_BASE, 4);
            putchars("\n\rRX_PTR_HEAD ");
            print_dec(RF_RX_PTR_HEAD, 4);

            putchars("\n\r===\n\r");
            */
            write_RX_FIFO("hello",5);
            write_RX_FIFO("a",1);
            write_RX_FIFO("hello world 1234", 16);
            write_RX_FIFO("WHAT HATH GOD WROUGHT", 21);

            read_RX_FIFO();
            putchars("\n\r");
            read_RX_FIFO();
            putchars("\n\r");
            read_RX_FIFO();
            putchars("\n\r");
            read_RX_FIFO();
            putchars("\n\r");

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
