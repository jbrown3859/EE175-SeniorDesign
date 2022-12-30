#include <msp430.h>
#include <backend.h>
#include <serial.h>
#include <util.h>

#define RADIOTYPE_SBAND 1
#define RX_SIZE 1024
#define RX_PACKETS 128
#define TX_SIZE 1024
#define TX_PACKETS 128

/* UART data structure */
char UART_RXBUF[256];
unsigned char UART_RX_PTR = 0;
unsigned char UART_RX_BASE = 0;

char timeout_flag = 0;
unsigned long long timestamp = 0;

/* helper functions */
unsigned int get_buffer_distance(unsigned int bottom, unsigned int top, unsigned int max) {
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

unsigned int get_UART_FIFO_size(void) {
    return get_buffer_distance(UART_RX_BASE, UART_RX_PTR, 256);
}

/* buffer functions */
unsigned int get_buffer_data_size(struct packet_buffer* buffer) {
    return get_buffer_distance(buffer->data_base, buffer->data_head, buffer->max_data);
}

unsigned int get_buffer_packet_count(struct packet_buffer* buffer) {
    return get_buffer_distance(buffer->ptr_base, buffer->ptr_head, buffer->max_packets);
}

unsigned int get_next_buffer_packet_size(struct packet_buffer* buffer) {
    unsigned int next = buffer->pointers[buffer->ptr_base];
    return get_buffer_distance(buffer->data_base, next, buffer->max_data);
}

void write_packet_buffer(struct packet_buffer* buffer, char* data, const unsigned char len) {
    unsigned char i;

    if (get_buffer_data_size(buffer) + len >= (buffer->max_data)) { //data overflow
        buffer->flags |= 0x01;
    }
    else {
        buffer->flags &= ~0x01;
    }

    if (get_buffer_packet_count(buffer) == (buffer->max_packets - 1)) { //packet overflow
        buffer->flags |= 0x02;
    }
    else {
        buffer->flags &= ~0x02;
    }

    if ((buffer->flags & 0x03) == 0) { //if no overflows
        for(i=0;i<len;i++) {
            buffer->data[buffer->data_head] = data[i];
            buffer->data_head = buffer->data_head < (buffer->max_data - 1) ? buffer->data_head + 1 : 0;
        }

        buffer->pointers[buffer->ptr_head] = buffer->data_head; //push pointer to queue
        buffer->ptr_head = buffer->ptr_head < (buffer->max_packets - 1) ? buffer->ptr_head + 1 : 0;
    }
}

unsigned int read_packet_buffer(struct packet_buffer* buffer, char* dest) {
    unsigned int i;
    unsigned int j = 0;
    if (buffer->ptr_head != buffer->ptr_base) { //if not empty
        for(i=buffer->data_base;i!=buffer->pointers[buffer->ptr_base];i = (i < (buffer->max_data-1) ? i+1 : 0)) { //eat up from bottom
            dest[j] = buffer->data[buffer->data_base];
            buffer->data_base = buffer->data_base < (buffer->max_data-1) ? buffer->data_base + 1 : 0;
            j++;
        }
        buffer->ptr_base = buffer->ptr_base < (buffer->max_packets - 1) ? buffer->ptr_base + 1 : 0;
    }
    return j;
}

void burst_read_packet_buffer(struct packet_buffer* buffer, unsigned char packet_size, unsigned char packet_num) {
    unsigned char i;
    unsigned char j;
    char rx[256];
    unsigned int len = 0;

    for(i=0;i<packet_num;i++) {
        if ((get_next_buffer_packet_size(buffer) != packet_size) || (get_buffer_packet_count(buffer) == 0)) { //exit condition
            break;
        }
        len = read_packet_buffer(buffer,rx);
        for(j=0;j<len;j++) {
            putchar(rx[j]);
        }
    }
}

void main_loop(void) {
    enum State state = INIT;
    unsigned int temp;
    struct RadioInfo info;

    /* temporary variables*/
    char pkt[256];
    unsigned int pkt_len = 0;
    unsigned int pkt_num = 0;
    char command;

    unsigned int i = 0;
    unsigned int j = 0;

    /* RX buffer */
    char RXbuf_data[RX_SIZE];
    unsigned int RXbuf_ptrs[RX_PACKETS];
    struct packet_buffer RXbuf;
    RXbuf.data = RXbuf_data;
    RXbuf.max_data = RX_SIZE;
    RXbuf.pointers = RXbuf_ptrs;
    RXbuf.max_packets = RX_PACKETS;
    RXbuf.data_base = 0;
    RXbuf.data_head = 0;
    RXbuf.ptr_base = 0;
    RXbuf.ptr_head = 0;
    RXbuf.flags = 0;

    /* TX buffer */
    char TXbuf_data[TX_SIZE];
    unsigned int TXbuf_ptrs[TX_PACKETS];
    struct packet_buffer TXbuf;
    TXbuf.data = TXbuf_data;
    TXbuf.max_data = TX_SIZE;
    TXbuf.pointers = TXbuf_ptrs;
    TXbuf.max_packets = TX_PACKETS;
    TXbuf.data_base = 0;
    TXbuf.data_head = 0;
    TXbuf.ptr_base = 0;
    TXbuf.ptr_head = 0;
    TXbuf.flags = 0;

    for (;;) {
        //state actions
        switch(state) {
        case INIT:
            hardware_timeout(100);
            info.frequency = 2450000000;
            state = WAIT;
            break;
        case WAIT:
            if (get_UART_FIFO_size() != 0) { //await command
                command = read_UART_FIFO();

                switch(command) {
                case 0x61: //get info
                    state = GET_RADIO_INFO;
                    break;
                case 0x62: //send RX buffer size
                    state = GET_RX_BUF_STATE;
                    break;
                case 0x63: //read RX packet
                    state = READ_RX_BUF;
                    break;
                case 0x64: //burst read RX packets
                    state = BURST_READ_RX;
                    break;
                case 0x65: //flush RX buffer
                    state = FLUSH_RX;
                    break;
                case 0x66: //send TX buffer state
                    state = GET_TX_BUF_STATE;
                    break;
                case 0x67: //write to TX
                    state = WRITE_TX_BUF;
                    break;
                case 0x68: //burst write TX packets
                    state = BURST_WRITE_TX;
                    break;
                case 0x69: //flush TX buffer
                    state = FLUSH_TX;
                    break;
                case 0x70: //write RX buffer (debug)
                    state = WRITE_RX_BUF;
                    break;
                case 0x71: //read TX buffer (debug)
                    state = READ_TX_BUF;
                    break;
                case 0x72:
                    state = CLEAR_RX_FLAGS;
                    break;
                case 0x73:
                    state = CLEAR_TX_FLAGS;
                    break;
                default:
                    state = WAIT;
                    break;
                }
            }
            break;
        case GET_RADIO_INFO:
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
        case GET_RX_BUF_STATE:
            putchar(RXbuf.flags);
            putchar((char)get_buffer_packet_count(&RXbuf));

            temp = get_buffer_data_size(&RXbuf);
            putchar((char)((temp >> 8) & 0xFF));
            putchar((char)(temp & 0xFF));

            if (temp > 0) {
                putchar((char)(get_next_buffer_packet_size(&RXbuf) & 0xFF));
            }
            else {
                putchar(0x00);
            }
            state = WAIT;
            break;
        case READ_RX_BUF:
            pkt_len = read_packet_buffer(&RXbuf, pkt);
            pkt[pkt_len] = '\0';
            putchars(pkt);
            state = WAIT;
            break;
        case BURST_READ_RX:
            while (get_UART_FIFO_size() < 2); //await data
            pkt_len = read_UART_FIFO(); //packet size
            pkt_num = read_UART_FIFO(); //packet number
            burst_read_packet_buffer(&RXbuf, pkt_len, pkt_num);
            state = WAIT;
            break;
        case FLUSH_RX:
            while (get_buffer_packet_count(&RXbuf) != 0) {
                pkt_len = read_packet_buffer(&RXbuf, pkt);
                pkt[pkt_len] = '\0';
                putchars(pkt);
            }
            state = WAIT;
            break;
        case GET_TX_BUF_STATE:
            putchar(TXbuf.flags);
            putchar((char)get_buffer_packet_count(&TXbuf));

            temp = get_buffer_data_size(&TXbuf);
            putchar((char)((temp >> 8) & 0xFF));
            putchar((char)(temp & 0xFF));

            if (temp > 0) {
                putchar((char)(get_next_buffer_packet_size(&TXbuf) & 0xFF));
            }
            else {
                putchar(0x00);
            }

            state = WAIT;
            break;
        case WRITE_TX_BUF:
            while (get_UART_FIFO_size() == 0); //await data
            pkt_len = read_UART_FIFO(); //packet size

            for(i=0;i<pkt_len;i++) {
                while (get_UART_FIFO_size() == 0);
                pkt[i] = read_UART_FIFO();
            }

            write_packet_buffer(&TXbuf, pkt, pkt_len);
            putchar(TXbuf.flags);
            state = WAIT;
            break;
        case BURST_WRITE_TX: //this is not its own buffer function due to RAM space constraints (cannot write all incoming data at once)
            while (get_UART_FIFO_size() < 2); //await data
            pkt_len = read_UART_FIFO();
            pkt_num = read_UART_FIFO();

            for(i=0;i<pkt_num;i++) {
                for(j=0;j<pkt_len;j++) {
                    while (get_UART_FIFO_size() == 0);
                    pkt[j] = read_UART_FIFO();
                }
                write_packet_buffer(&TXbuf, pkt, pkt_len);
            }

            putchar(TXbuf.flags);
            state = WAIT;
            break;
        case FLUSH_TX:
            while (get_buffer_packet_count(&TXbuf) != 0) {
                pkt_len = read_packet_buffer(&TXbuf, pkt);
                pkt[pkt_len] = '\0';
                putchars(pkt);
            }
            state = WAIT;
            break;
        case WRITE_RX_BUF:
            while (get_UART_FIFO_size() == 0); //await data
            pkt_len = read_UART_FIFO(); //packet size

            for(i=0;i<pkt_len;i++) {
                while (get_UART_FIFO_size() == 0);
                pkt[i] = read_UART_FIFO();
            }

            write_packet_buffer(&RXbuf, pkt, pkt_len);
            putchar(RXbuf.flags);
            state = WAIT;
            break;
        case READ_TX_BUF:
            pkt_len = read_packet_buffer(&TXbuf, pkt);
            pkt[pkt_len] = '\0';
            putchars(pkt);
            state = WAIT;
            break;
        case CLEAR_RX_FLAGS:
            RXbuf.flags = 0x00;
            putchar(RXbuf.flags);
            state = WAIT;
            break;
        case CLEAR_TX_FLAGS:
            TXbuf.flags = 0x00;
            putchar(TXbuf.flags);
            state = WAIT;
            break;
        }
    }
}
