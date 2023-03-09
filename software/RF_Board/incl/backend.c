#include <msp430.h>
#include <backend.h>
#include <serial.h>
#include <util.h>

#define RADIOTYPE_SBAND 1
//#define RADIOTYPE_UHF 1

#define RX_SIZE 2048
#define RX_PACKETS 160
#define TX_SIZE 256
#define TX_PACKETS 32

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

char get_flag_from_state(enum RadioState state) {
    char flag;

    switch (state) {
    case IDLE:
        flag = 0x00;
        break;
    case RX:
        flag = 0x40;
        break;
    case TX_WAIT:
        flag = 0x80;
        break;
    case TX_ACTIVE:
        flag = 0xC0;
        break;
    default:
        flag = 0x00;
        break;
    }

    return flag;
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

/* Get n bytes and time out */
unsigned char get_UART_bytes(char* bytes, unsigned char size, unsigned int timeout) {
    unsigned char i;
    unsigned char len = 0;

    timeout_flag = 0;
    hardware_timeout(timeout);
    while (get_UART_FIFO_size() < size && timeout_flag == 0); //await data
    hardware_timeout(0);

    if (timeout_flag == 1) { //timed out before got all data
        size = get_UART_FIFO_size(); //return as many bytes as we got
        timeout_flag = 0;
    }

    for (i = 0; i < size; i++) {
        bytes[i] = read_UART_FIFO();
        len++;
    }

    return len;
}

void flush_UART_FIFO(void) {
    UART_RX_PTR = UART_RX_BASE;
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
    unsigned char pkt_len;
    char pkt[256];

    if (info.radio_mode == RX) {
        #ifdef RADIOTYPE_SBAND
        pkt_len = cc2500_receive(pkt);
        if (pkt_len > 0) { //filter failed CRCs
           write_packet_buffer(&RXbuf, pkt, pkt_len);
        }
        cc2500_command_strobe(STROBE_SFRX);
        cc2500_command_strobe(STROBE_SRX);
        #endif
        #ifdef RADIOTYPE_UHF
        pkt_len = rfm95w_read_fifo(pkt);
        if (pkt_len > 0) {
            write_packet_buffer(&RXbuf, pkt, pkt_len);
        }
        rfm95w_clear_flag(FLAG_RXDONE);
        rfm95w_clear_flag(FLAG_VALIDHEADER);
        #endif

        info.radio_mode = RX;
    }
    else if (info.radio_mode == TX_ACTIVE) {
        #ifdef RADIOTYPE_UHF
        rfm95w_clear_flag(FLAG_TXDONE);
        #endif

        info.radio_mode = TX_WAIT;
    }

    P2IFG &= ~(0x05); //reset interrupt flags
    //P2IFG &= ~(0x01);
}

void main_loop(void) {
    WDTCTL = WDTPW | WDTSSEL_1 | WDTCNTCL | WDTIS_4; //watchdog on, slow clock source, reset watchdog, 1s expiration

    enum State state = INIT;
    unsigned int temp;

    /* temporary variables*/
    char pkt[256];
    unsigned int pkt_len = 0;
    unsigned int pkt_num = 0;
    unsigned char command;
    char args[16];
    char regs[48];

    unsigned int i = 0;
    //unsigned int j = 0;

    char addr;
    char data;

    char radio_state;

    /* RX buffer */
    char RXbuf_data[RX_SIZE];
    unsigned int RXbuf_ptrs[RX_PACKETS];
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
    TXbuf.data = TXbuf_data;
    TXbuf.max_data = TX_SIZE;
    TXbuf.pointers = TXbuf_ptrs;
    TXbuf.max_packets = TX_PACKETS;
    TXbuf.data_base = 0;
    TXbuf.data_head = 0;
    TXbuf.ptr_base = 0;
    TXbuf.ptr_head = 0;
    TXbuf.flags = 0;

    /* GPIO inits */
    #ifdef RADIOTYPE_SBAND
    cc2500_init_frontend();
    #endif

    for (;;) {
        /* Radio state machine*/
        switch(info.radio_mode) {
        case IDLE:
            break;
        case RX:
            break;
        case TX_WAIT:
            if (get_buffer_packet_count(&TXbuf) != 0) {
                #ifdef RADIOTYPE_SBAND
                cc2500_command_strobe(STROBE_SFTX); //flush if not already cleared
                pkt_len = read_packet_buffer(&TXbuf, pkt);
                cc2500_write(0x3F, pkt_len); //write packet size
                cc2500_burst_write_fifo(pkt, pkt_len); //write packet

                cc2500_command_strobe(STROBE_STX); //enter transmit mode
                #endif

                #ifdef RADIOTYPE_UHF
                pkt_len = read_packet_buffer(&TXbuf, pkt);
                pkt[pkt_len] = '\n'; //packet MUST end '\n' to trigger ISR (idk man I didn't design the chip)
                pkt[pkt_len + 1] = '\0'; //null-terminate
                rfm95w_transmit_chars(pkt);
                #endif

                info.radio_mode = TX_ACTIVE;
            }
            break;
        case TX_ACTIVE:
            break;
        }

        /* UART state machine */
        switch(state) {
        case INIT:
            info.frequency = 0; //dummy values, should never actually be this
            info.data_rate = 0;
            info.bandwidth = 0;
            info.spreading_factor = 0;
            info.coding_rate = 0;

            #ifdef RADIOTYPE_SBAND
            cc2500_command_strobe(STROBE_SRX);
            cc2500_set_frontend(RX_SINGLE_BYPASS);
            #endif
            #ifdef RADIOTYPE_UHF
            rfm95w_set_DIO_mode(DIO0_RXDONE);
            rfm95w_set_mode(OP_MODE_RXCONTINUOUS);
            #endif

            info.radio_mode = RX;
            state = WAIT;
            break;
        case GET_RADIO_INFO:
            putchar(0xAA); //send preamble to reduce the chance of the groundstation application getting a false positive on device detection
            putchar(0xAA);

            #ifdef RADIOTYPE_SBAND
            info.frequency = cc2500_get_frequency();
            info.data_rate = cc2500_get_data_rate();
            putchar('S');
            #endif
            #ifdef RADIOTYPE_UHF
            info.frequency = rfm95w_get_carrier_frequency();
            info.bandwidth = rfm95w_get_lora_bandwidth();
            info.spreading_factor = rfm95w_get_spreading_factor();
            info.coding_rate = rfm95w_get_coding_rate();
            putchar('U');
            #endif

            print_dec(info.frequency, 10);
            print_dec(info.data_rate, 6);
            print_dec(info.bandwidth, 6);
            print_dec(info.spreading_factor, 2);
            print_dec(info.coding_rate, 2);
            state = WAIT;
            break;
        case GET_RX_BUF_STATE:
            putchar(RXbuf.flags | get_flag_from_state(info.radio_mode));
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
            putnchars(pkt, pkt_len);
            state = WAIT;
            break;
        case BURST_READ_RX:
            if (get_UART_bytes(args, 2, 10000) == 2) {
                burst_read_packet_buffer(&RXbuf, args[0], args[1]);
            }
            else {
                flush_UART_FIFO();
            }

            state = WAIT;
            break;
        case FLUSH_RX:
            while (get_buffer_packet_count(&RXbuf) != 0) {
                pkt_len = read_packet_buffer(&RXbuf, pkt);

                if (pkt_len != 0) {
                    putchar((unsigned char)((pkt_len >> 8) & 0xFF)); //add length to each packet
                    putchar((unsigned char)(pkt_len & 0xFF));
                    putnchars(pkt, pkt_len);
                }
            }
            state = WAIT;
            break;
        case GET_TX_BUF_STATE:
            putchar(TXbuf.flags | get_flag_from_state(info.radio_mode));
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
            if (get_UART_bytes(args, 1, 10000) == 1) {
                pkt_len = args[0];

                if (get_UART_bytes(pkt, pkt_len, 64000) == pkt_len) {
                    write_packet_buffer(&TXbuf, pkt, pkt_len);
                }
                else {
                    flush_UART_FIFO();
                }
            }
            state = WAIT;
            break;
        case BURST_WRITE_TX: //this is not its own buffer function due to RAM space constraints (cannot write all incoming data at once)
            if (get_UART_bytes(args, 2, 10000) == 2) {
                pkt_len = args[0];
                pkt_num = args[1];

                for(i=0;i<pkt_num;i++) {
                    if (get_UART_bytes(pkt, pkt_len, 64000) == pkt_len) {
                        write_packet_buffer(&TXbuf, pkt, pkt_len);
                    }
                }
            }
            else {
                flush_UART_FIFO();
            }
            state = WAIT;
            break;
        case FLUSH_TX:
            while (get_buffer_packet_count(&TXbuf) != 0) {
                pkt_len = read_packet_buffer(&TXbuf, pkt);

                if (pkt_len != 0) {
                    putchar((unsigned char)((pkt_len >> 8) & 0xFF)); //add length to each packet
                    putchar((unsigned char)(pkt_len & 0xFF));
                    putnchars(pkt, pkt_len);
                }
            }
            state = WAIT;
            break;
        case WRITE_RX_BUF:
            if (get_UART_bytes(args, 1, 10000) == 1) {
                pkt_len = args[0];

                if (get_UART_bytes(pkt, pkt_len, 64000) == pkt_len) {
                    write_packet_buffer(&RXbuf, pkt, pkt_len);
                }
                else {
                    flush_UART_FIFO();
                }
            }
            state = WAIT;
            break;
        case READ_TX_BUF:
            pkt_len = read_packet_buffer(&TXbuf, pkt);
            putnchars(pkt, pkt_len);
            state = WAIT;
            break;
        case CLEAR_RX_FLAGS:
            RXbuf.flags = 0x00;
            putchar(RXbuf.flags | get_flag_from_state(info.radio_mode));
            state = WAIT;
            break;
        case CLEAR_TX_FLAGS:
            TXbuf.flags = 0x00;
            putchar(TXbuf.flags | get_flag_from_state(info.radio_mode));
            state = WAIT;
            break;
        case GET_RADIO_STATE:
            putchar(get_flag_from_state(info.radio_mode));
            state = WAIT;
            break;
        case PROG_RADIO_REG:
            if (get_UART_bytes(args, 2, 10000) == 2) {
                addr = args[0];
                data = args[1];

                #ifdef RADIOTYPE_SBAND
                cc2500_write(addr, data);
                putchar(cc2500_read(addr));
                #endif
                #ifdef RADIOTYPE_UHF
                rfm95w_write(addr, data);
                putchar(rfm95w_read(addr));
                #endif
            }
            else {
                flush_UART_FIFO();
            }

            state = WAIT;
            break;
        case READ_RADIO_REG:
            if (get_UART_bytes(args, 1, 10000) == 1) {
                #ifdef RADIOTYPE_SBAND
                data = cc2500_read(args[0]);
                putchar(data);
                #endif
                #ifdef RADIOTYPE_UHF
                data = rfm95w_read(args[0]);
                putchar(data);
                #endif
            }
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

            if (radio_state == STATUS_STATE_IDLE) {
                cc2500_set_frontend(RX_SHUTDOWN);
                info.radio_mode = IDLE;
            }
            #endif
            #ifdef RADIOTYPE_UHF
            rfm95w_clear_all_flags();
            //rfm95w_set_DIO_mode(DIO0_NONE);
            radio_state = rfm95w_set_mode(OP_MODE_STDBY);

            if (radio_state == OP_MODE_STDBY) {
                info.radio_mode = IDLE; //switch state only if the modem successfully switches
            }
            #endif

            putchar(get_flag_from_state(info.radio_mode));
            state = WAIT;
            break;
        case MODE_RX:
            #ifdef RADIOTYPE_SBAND
            cc2500_command_strobe(STROBE_SFRX);
            cc2500_command_strobe(STROBE_SRX);
            radio_state = cc2500_get_status();

            if (radio_state == STATUS_STATE_RX || radio_state == STATUS_STATE_RXOVERFLOW || radio_state == STATUS_STATE_SETTLING || radio_state == STATUS_STATE_CALIBRATE) { //may not be in RX state if actively getting packets
                cc2500_set_frontend(RX_SINGLE_BYPASS);
                info.radio_mode = RX;
            }
            #endif
            #ifdef RADIOTYPE_UHF
            rfm95w_clear_all_flags();
            rfm95w_set_DIO_mode(DIO0_RXDONE);
            radio_state = rfm95w_set_mode(OP_MODE_RXCONTINUOUS);

            if (radio_state == OP_MODE_RXCONTINUOUS) {
                info.radio_mode = RX; //switch state only if the modem successfully switches
            }
            #endif

            putchar(get_flag_from_state(info.radio_mode));
            state = WAIT;
            break;
        case MODE_TX:
            #ifdef RADIOTYPE_SBAND
            cc2500_command_strobe(STROBE_SFRX);
            cc2500_command_strobe(STROBE_SIDLE);
            radio_state = cc2500_get_status();

            if (radio_state == STATUS_STATE_IDLE) {
                cc2500_set_frontend(TX);
                info.radio_mode = TX_WAIT;
            }
            #endif
            #ifdef RADIOTYPE_UHF
            rfm95w_clear_all_flags();
            rfm95w_set_DIO_mode(DIO0_TXDONE); //set DIO
            radio_state = rfm95w_set_mode(OP_MODE_STDBY);

            if (radio_state == OP_MODE_STDBY) {
                info.radio_mode = TX_WAIT; //switch state only if the modem successfully switches
            }
            #endif

            putchar(get_flag_from_state(info.radio_mode));
            state = WAIT;
            break;
        case MANUAL_RESET: //reset chip and save register states
            #ifdef RADIOTYPE_SBAND
            cc2500_save_registers(regs);
            cc2500_command_strobe(STROBE_SRES);
            hardware_delay(100);
            cc2500_load_registers(regs);

            switch(info.radio_mode) {
            case IDLE:
                cc2500_set_frontend(RX_SHUTDOWN);
                break;
            case RX:
                cc2500_set_frontend(RX_SINGLE_BYPASS);
                break;
            case TX_WAIT:
                cc2500_set_frontend(TX);
                break;
            case TX_ACTIVE:
                info.radio_mode = TX_WAIT;
                cc2500_set_frontend(TX);
                break;
            }
            #endif
            #ifdef RADIOTYPE_UHF
            rfm95w_save_registers(regs);
            rfm95w_reset();
            hardware_delay(100);
            rfm95w_load_registers(regs);

            if (info.radio_mode == TX_ACTIVE) {
                info.radio_mode = TX_WAIT;
            }
            #endif

            putchar(get_flag_from_state(info.radio_mode));
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
                case 0x74:
                    state = GET_RADIO_STATE;
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
                case 0x85:
                    state = MANUAL_RESET;
                    break;
                default:
                    flush_UART_FIFO(); //flush to avoid hanging
                    state = WAIT;
                    break;
                }
            }
            break;
        }
        WDTCTL = WDTPW | WDTSSEL_1 | WDTCNTCL | WDTIS_4; //reset watchdog count
    }
}
