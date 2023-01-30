#include <msp430.h>
#include <backend.h>
#include <serial.h>
#include <util.h>

#define RADIOTYPE_SBAND 1
#define RX_SIZE 1024
#define RX_PACKETS 128
#define TX_SIZE 1024
#define TX_PACKETS 128

#ifdef RADIOTYPE_SBAND
#include <cc2500.h>
#endif
#ifdef RADIOTYPE_UHF
#include <rfm95w.h>
#endif

/* Globals */
struct packet_buffer RXbuf;
struct packet_buffer TXbuf;
struct RadioInfo info;

char RX_done = 0;

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

/* ISR for TX/RX detection */
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    #ifdef RADIOTYPE_SBAND

    if (info.radio_mode == RX_ACTIVE) {
        info.radio_mode = RX_DONE;
    }
    else if (info.radio_mode == TX_ACTIVE) {
        cc2500_command_strobe(STROBE_SFTX);
        info.radio_mode = TX_WAIT;
    }

    #endif

   // WDTCTL = WDTPW | WDTCNTCL; //reset watchdog count
    P2IFG &= ~(0x04);
}

void main_loop(void) {
    enum State state = INIT;
    unsigned int temp;

    /* temporary variables*/
    char pkt[256];
    unsigned int pkt_len = 0;
    unsigned int pkt_num = 0;
    unsigned char command;

    unsigned int i = 0;
    unsigned int j = 0;

    char addr;
    char data;

    char radio_state;

    /* RX buffer */
    char RXbuf_data[RX_SIZE];
    unsigned int RXbuf_ptrs[RX_PACKETS];
    //struct packet_buffer RXbuf;
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
    //struct packet_buffer TXbuf;
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
        /* background tasks */
        //TX packet if buffer is not empty and radio is enabled
        if (get_buffer_packet_count(&TXbuf) != 0) {
            #ifdef RADIOTYPE_SBAND
            if (info.radio_mode == TX_WAIT) { //if not already in TX
                pkt_len = read_packet_buffer(&TXbuf, pkt);
                cc2500_write(0x3F, pkt_len); //write packet size
                cc2500_burst_write_fifo(pkt, pkt_len); //write packet

                cc2500_command_strobe(STROBE_STX); //enter transmit mode
                info.radio_mode = TX_ACTIVE;
            }
            #endif
        }

        //RX packet handling
        if (info.radio_mode == RX_DONE) {
            #ifdef RADIOTYPE_SBAND
            pkt_len = cc2500_receive(pkt);
            if (pkt_len > 0) { //filter failed CRCs
                write_packet_buffer(&RXbuf, pkt, pkt_len);
            }
            cc2500_command_strobe(STROBE_SFRX);
            cc2500_command_strobe(STROBE_SRX);
            info.radio_mode = RX_ACTIVE;
            #endif
        }

        /* state machine */
        switch(state) {
        case INIT:
            info.frequency = 2450000000;
            info.radio_mode = RX_ACTIVE;

            #ifdef RADIOTYPE_SBAND
            P2IES |= 0x04; //trigger P2.2 on falling edge
            P2IE |= 0x04; //enable interrupt
            cc2500_command_strobe(STROBE_SRX);
            #endif

            state = WAIT;
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
            //putchar(TXbuf.flags);
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

            //putchar(TXbuf.flags);
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
            //putchar(RXbuf.flags);
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
        case PROG_RADIO_REG:
            while (get_UART_FIFO_size() < 2);
            addr = read_UART_FIFO();
            data = read_UART_FIFO();

            #ifdef RADIOTYPE_SBAND
            cc2500_write(addr, data);
            putchar(cc2500_read(addr));
            #endif

            state = WAIT;
            break;
        case READ_RADIO_REG:
            while (get_UART_FIFO_size() == 0);
            addr = read_UART_FIFO();

            #ifdef RADIOTYPE_SBAND
            data = cc2500_read(addr);
            putchar(data);
            #endif

            state = WAIT;
            break;
        case MODE_IDLE:
            #ifdef RADIOTYPE_SBAND
            radio_state = cc2500_get_status();

            if (radio_state == STATUS_STATE_RX || radio_state == STATUS_STATE_RXOVERFLOW) {
                cc2500_command_strobe(STROBE_SFRX); //this fixes it for some reason and I also don't know why
            }
            cc2500_command_strobe(STROBE_SIDLE); //CAUSES UART TO HANG SOMETIMES AND I DON'T KNOW WHY

            radio_state = cc2500_get_status();
            putchar(radio_state);
            #endif

            info.radio_mode = IDLE;
            state = WAIT;
            break;
        case MODE_RX:
            #ifdef RADIOTYPE_SBAND
            cc2500_command_strobe(STROBE_SFRX);
            cc2500_command_strobe(STROBE_SRX);
            radio_state = cc2500_get_status();
            putchar(radio_state);
            #endif

            info.radio_mode = RX_ACTIVE;
            state = WAIT;
            break;
        case MODE_TX:
            #ifdef RADIOTYPE_SBAND
            cc2500_command_strobe(STROBE_SFRX);
            cc2500_command_strobe(STROBE_SIDLE);
            radio_state = cc2500_get_status();
            putchar(radio_state);
            #endif

            info.radio_mode = TX_WAIT;
            state = WAIT;
            break;
        default:
        case WAIT:
            if (get_UART_FIFO_size() != 0) { //handle command
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
                case 0x80:
                    state = PROG_RADIO_REG;
                    break;
                case 0x81:
                    state = READ_RADIO_REG;
                    break;
                case 0x82:
                    state = MODE_IDLE;
                    break;
                case 0x83:
                    state = MODE_RX;
                    break;
                case 0x84:
                    state = MODE_TX;
                    break;
                default:
                    state = WAIT;
                    break;
                }
            }
            break;
        }
    }
}
