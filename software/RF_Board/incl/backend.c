#include <msp430.h>
#include <backend.h>
#include <serial.h>
#include <util.h>

#define RADIOTYPE_SBAND 1
#define RX_FIFO_SIZE 2048
#define RX_FIFO_PACKETS 256

/* UART data structure */
char UART_RXBUF[256];
unsigned char UART_RX_PTR = 0;
unsigned char UART_RX_BASE = 0;

/* Radio data structure*/
char RADIO_RXBUF[RX_FIFO_SIZE];
unsigned int RF_RX_HEAD = 0;
unsigned int RF_RX_BASE = 0;

unsigned int RADIO_RX_PTRS[RX_FIFO_PACKETS]; //pointers to packet locations inside the RX queue
unsigned char RF_RX_PTR_HEAD = 0;
unsigned char RF_RX_PTR_BASE = 0;

char rx_buffer_flags;

char timeout_flag = 0;

/* helper functions */
unsigned int get_queue_distance(unsigned int bottom, unsigned int top, unsigned int max) {
    if (top >= bottom) {
        return top - bottom;
    }
    else {
        return (max - bottom) + top;
    }
}

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
    if (UART_RX_PTR >= UART_RX_BASE) {
        return UART_RX_PTR - UART_RX_BASE;
    }
    else  {
        return (256 - UART_RX_BASE) + UART_RX_PTR;
    }
}

/* RX Queue */
void write_RX_FIFO(char* data, unsigned char len) {
    unsigned char i;

    if (get_RX_FIFO_size() + len >= (RX_FIFO_SIZE-1)) { //data overflow
        rx_buffer_flags |= 0x01;
    }
    else {
        rx_buffer_flags &= ~0x01;
    }

    if (get_RX_FIFO_packet_count() == (RX_FIFO_PACKETS-1)) { //packet overflow
        rx_buffer_flags |= 0x02;
    }
    else {
        rx_buffer_flags &= ~0x02;
    }

    if ((rx_buffer_flags & 0x03) == 0) { //if no overflows
        for(i=0;i<len;i++) {
            RADIO_RXBUF[RF_RX_HEAD] = data[i];
            RF_RX_HEAD = RF_RX_HEAD < (RX_FIFO_SIZE-1) ? RF_RX_HEAD + 1 : 0;
        }

        RADIO_RX_PTRS[RF_RX_PTR_HEAD] = RF_RX_HEAD; //push pointer to queue
        RF_RX_PTR_HEAD++;
    }
}

void read_RX_FIFO(void) {
    unsigned int i;
    if (RF_RX_PTR_HEAD != RF_RX_PTR_BASE) { //if not empty
        for(i=RF_RX_BASE;i!=RADIO_RX_PTRS[RF_RX_PTR_BASE];i = (i < (RX_FIFO_SIZE-1) ? i+1 : 0)) { //eat up from bottom
            putchar(RADIO_RXBUF[RF_RX_BASE]);
            RF_RX_BASE = RF_RX_BASE < (RX_FIFO_SIZE-1) ? RF_RX_BASE + 1 : 0;
        }
        RF_RX_PTR_BASE++;
    }
}

void burst_read_RX_FIFO(unsigned char packet_size, unsigned char packet_num) {
    unsigned char i;
    for(i=0;i<packet_num;i++) {
        if ((get_next_RX_packet_size() != packet_size) || (get_RX_FIFO_packet_count() == 0)) { //exit condition
            break;
        }
        read_RX_FIFO();
    }
}

unsigned char get_RX_FIFO_packet_count(void) {
    if (RF_RX_PTR_HEAD >= RF_RX_PTR_BASE) {
        return RF_RX_PTR_HEAD - RF_RX_PTR_BASE;
    }
    else {
        return (RX_FIFO_PACKETS - RF_RX_PTR_BASE) + RF_RX_PTR_HEAD;
    }
}

unsigned int get_RX_FIFO_size(void) {
    if (RF_RX_HEAD >= RF_RX_BASE) {
        return RF_RX_HEAD - RF_RX_BASE;
    }
    else {
        return (RX_FIFO_SIZE - RF_RX_BASE) + RF_RX_HEAD;
    }
}

unsigned int get_next_RX_packet_size(void) {
    unsigned int next = RADIO_RX_PTRS[RF_RX_PTR_BASE];//RADIO_RX_PTRS[RF_RX_PTR_BASE < (RX_FIFO_PACKETS-1) ? RF_RX_PTR_BASE + 1 : 0];

    if (next >= RF_RX_BASE) {
        return next - RF_RX_BASE;
    }
    else {
        return (RX_FIFO_SIZE - RF_RX_BASE) + next;
    }
}

void main_loop(void) {
    enum State state = INIT;
    char command;
    char temp;
    unsigned char args[2];

    struct RadioInfo info;

    /* test */
    unsigned int i = 0;
    char packet[18];

    for (i=2;i<18;i++) {
        packet[i] = 0x03; //blue
        //write_RX_FIFO("WHAT HATH GOD WROUGHT", 21);
    }
    i=0;

    for (;;) {
        //state actions
        switch(state) {
        case INIT:
            hardware_timeout(30);
            info.frequency = 2450000000;
            state = WAIT;
            break;
        case WAIT:
            if (get_UART_FIFO_size() != 0) { //await command
                command = read_UART_FIFO();

                switch(command) {
                case 0x61: //get info
                    state = SEND_INFO;
                    break;
                case 0x62: //send RX buffer size
                    state = SEND_RX_BUF_STATE;
                    break;
                case 0x63: //read RX packet
                    state = SEND_RX_PACKET;
                    break;
                case 0x64: //burst read RX packets
                    state = BURST_SEND_RX;
                    break;
                default:
                    state = WAIT;
                    break;
                }
            }

            if (timeout_flag == 1) {
                packet[0] = 0x80 | (char)((i >> 8) & 0xFF);
                packet[1] = (char)(i & 0xFF);
                write_RX_FIFO(packet, 18);

                i = (i < (1200 - 1)) ? i+1 : 0;
                timeout_flag = 0;
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
            state = WAIT;
            break;
        case SEND_RX_BUF_STATE:
            putchar(rx_buffer_flags);
            putchar(get_RX_FIFO_packet_count());

            temp = get_RX_FIFO_size();
            putchar((char)((temp >> 8) & 0xFF));
            putchar((char)(temp & 0xFF));

            if (temp > 0) {
                putchar((char)(get_next_RX_packet_size() & 0xFF));
            }
            else {
                putchar(0x00);
            }
            state = WAIT;
            break;
        case SEND_RX_PACKET:
            read_RX_FIFO();
            state = WAIT;
            break;
        case BURST_SEND_RX:
            while (get_UART_FIFO_size() < 2); //await data
            args[0] = read_UART_FIFO();
            args[1] = read_UART_FIFO();
            burst_read_RX_FIFO(args[0], args[1]);
            state = WAIT;
            break;
        }
    }
}
