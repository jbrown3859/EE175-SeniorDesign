#include <msp430.h>
#include <serial.h>
#include <rfm95w.h>
#include <util.h>

char DIO0_mode;

/* init */
void rfm95w_init(void) {
    P2DIR &= ~(0x01); //set P2.0 to input
    P2REN &= ~(0x01); //disable pull resistors
    P2IES &= ~(0x01); //trigger on rising edge
    P2IE |= 0x01; //enable interrupt

    //P2DIR |= (1 << 2); //set P2.2 to output
    //P2REN &= ~(1 << 2); //disable pull resistors
    //P2OUT |= (1 << 2); //set reset high

    P1DIR |= (1 << 4); //reset pin init
    P1REN &= ~(1 << 4);
    P1OUT |= (1 << 4);
}

/* reset the radio */
void rfm95w_reset(void) {
    //P2OUT &= ~(1 << 2); //pull reset low
    P1OUT &= ~(1 << 4);
    hardware_delay(1000);
    //P2OUT |= (1 << 2); //pull high
    P1OUT |= (1 << 4);
    hardware_delay(1000);
}

/* read over SPI */
char rfm95w_read(const char addr) {
    return SPI_RX(addr);
}

/* write over SPI */
void rfm95w_write(const char addr, const char c) {
    SPI_TX((addr | 0x80), c); //transmit write bit along with addr
}

/* display register addr and value over uart for debug */
void rfm95w_display_register(const char addr) {
    char c;
    putchars("Address: ");
    print_hex(addr);
    c = rfm95w_read(addr);
    putchars(" Data: ");
    print_hex(c);
    putchars("\n\r");
}

/* dump all registers over serial */
void rfm95w_register_dump(void) {
    unsigned char i;

    putchars("LoRa Register Dump:\n\r");
    for (i = 0; i < 128; i++) {
        rfm95w_display_register(i);
    }
    putchars("\n\r");
}

/* get device mode */
char rfm95w_get_mode(void) {
    return (rfm95w_read(0x01) & 0b111); //return only mode bits
}

/* set device mode (sleep, stdby, TX, RX, etc.) */
char rfm95w_set_mode(const char mode) {
    rfm95w_clear_all_flags(); //clear flags to ensure a valid switch
    char r = rfm95w_read(0x01) & ~(0b00000111); //clear mode bits
    rfm95w_write(0x01, (r | mode)); //write new mode

    hardware_timeout(1000);
    while(rfm95w_get_mode() != mode && timeout_flag == 0); //poll until commanded mode is entered
    hardware_timeout(0);
    timeout_flag = 0;

    return rfm95w_get_mode();
}

/* set register 0x01 to configure radio modulation and put radio into standby */
void rfm95w_set_lora_mode(const char lora_mode) {
    rfm95w_set_mode(OP_MODE_SLEEP); //device must be in sleep to change modulation

    char r = rfm95w_read(0x01) & ~(0b10000000); //clear lora bit
    r |= lora_mode; //set lora mode
    rfm95w_write(0x01, r);

    rfm95w_set_mode(OP_MODE_STDBY);
}

void rfm95w_set_frequency_mode(const char m) {
    char r = rfm95w_read(0x01) & ~(0x08); //clear LF/HF bit
    rfm95w_write(0x01, (r | m));
}

/* set carrier frequency and put radio into standby */
void rfm95w_set_carrier_frequency(const unsigned long long frequency) {
    const unsigned long long frf = ((frequency * 524288)/F_XOSC); //from datasheet formula

    rfm95w_set_mode(OP_MODE_STDBY); //must be in standby or sleep to program frequency

    rfm95w_write(0x06, (char)((frf >> 16) & 0xFF)); //MSB
    rfm95w_write(0x07, (char)((frf >> 8) & 0xFF)); //middle byte
    rfm95w_write(0x08, (char)(frf & 0xFF)); //LSB
}

/* get carrier frequency */
unsigned long long rfm95w_get_carrier_frequency(void) {
    unsigned long long frf;
    unsigned long long freq;

    frf = ((unsigned long long)rfm95w_read(0x06) << 16) | ((unsigned long long)rfm95w_read(0x07) << 8) | rfm95w_read(0x08);
    freq = (frf * F_XOSC)/524288;

    return freq;
}

/* set the transmit power of the device*/
void rfm95w_set_tx_power(const char boost, const char max, const char power) {
    rfm95w_write(0x09, (boost | ((max & 0x07) << 4) | (power & 0x0F)));
}

/* enable/disable the AGC */
void rfm95w_agc_auto_on(const char a) {
    char r = rfm95w_read(0x26) & ~(0x04); //clear agc bit
    rfm95w_write(0x26, (r|a));
}

/* enable/disable the low data rate mode (symbol length > 16ms) */
void rfm95w_LDR_optimize(const char ldr) {
    char r = rfm95w_read(0x26) & ~(0x08); //clear ldr bit
    rfm95w_write(0x26, (r|ldr));
}

/* set LNA power of the device*/
void rfm95w_set_lna_gain(const char gain, const char boost_lf, const char boost_hf) {
    rfm95w_write(0x0C, (gain | boost_lf | boost_hf));
}

/* write fifo */
void rfm95w_write_fifo(const char c) {
    rfm95w_write(0x00, c);
}

/* set bandwidth */
void rfm95w_set_lora_bandwidth(const char b) {
    char r = rfm95w_read(0x1D) & 0x0F; //clear high bits
    rfm95w_write(0x1D, (r | b));
}

/* get bandwidth in Hz */
unsigned long rfm95w_get_lora_bandwidth(void) {
    char b = rfm95w_read(0x1D) & 0xF0;
    unsigned long bw;

    switch(b) {
        case BW_7_8:
            bw = 7800;
            break;
        case BW_10_4:
            bw = 10400;
            break;
        case BW_15_6:
            bw = 15600;
            break;
        case BW_20_8:
            bw = 20800;
            break;
        case BW_32_25:
            bw = 32250;
            break;
        case BW_41_7:
            bw = 41700;
            break;
        case BW_62_5:
            bw = 62500;
            break;
        case BW_125:
            bw = 125000;
            break;
        case BW_250:
            bw = 250000;
            break;
        case BW_500:
            bw = 500000;
            break;
        default:
            bw = 0;
            break;
    }
    return bw;
}

/* set spreading factor (will only write if sf is in range 6-12) */
void rfm95w_set_spreading_factor(const char sf) {
    char r = rfm95w_read(0x1E) & 0x0F; //clear high bits

    if (sf >= 6 && sf <= 12) {
        rfm95w_write(0x1E, (r | (sf << 4)));
    }
}

/* get spreading factor */
unsigned char rfm95w_get_spreading_factor(void) {
    char sf = rfm95w_read(0x1E) & 0xF0;

    return (sf >> 4) & 0x0F;
}

/* write a 1 to the desired flag, clearing it */
void rfm95w_clear_flag(const char f) {
    rfm95w_write(0x12, f);
}

/* clear all flags */
void rfm95w_clear_all_flags(void) {
    rfm95w_write(0x12, 0xFF);
}

/* read from flags register */
char rfm95w_read_flag(const char f) {
    return (rfm95w_read(0x12) & f);
}

/* set the flag that a given DIO pin reports */
void rfm95w_set_DIO_mode(const char m) {
    rfm95w_write(0x40, m);
    DIO0_mode = m;
}

/* write length of payload */
void rfm95w_set_payload_length(const char l) {
    rfm95w_write(0x22, l);
}

/* write max paylod length */
void rfm95w_set_max_payload_length(const char l) {
    rfm95w_write(0x23, l);
}

/* return payload length */
unsigned char rfm95w_get_payload_length(void) {
    return (unsigned char)rfm95w_read(0x22);
}

/* set length of preamble */
void rfm95w_set_preamble_length(const int l) {
    rfm95w_write(0x20, (char)((l >> 8) & 0xFF)); //MSB
    rfm95w_write(0x21, (char)(l & 0xFF));
}

/* set explicit or implicit header */
void rfm95w_set_header_mode(const char m) {
    char r;
    r = rfm95w_read(0x1d) & ~(0x01); //read register and clear mode bit
    rfm95w_write(0x1d, (r | m));
}

/* enable/disable CRC generation/check */
void rfm95w_set_crc(const char c) {
    char r;
    r = rfm95w_read(0x1e) & ~(0x04); //read and clear CRC bit
    rfm95w_write(0x1e, (r | c));
}

/* set error coding rate */
void rfm95w_set_coding_rate(const char cr) {
    char r;
    r = rfm95w_read(0x1d) & ~(0b1110); //read and clear coding rate bits
    rfm95w_write(0x1d, (r | cr));
}

/* get coding rate */
unsigned char rfm95w_get_coding_rate(void) {
    char r = rfm95w_read(0x1d) & (0b1110);
    unsigned char rate;

    switch(r) {
    case CR_4_5:
        rate = 45;
        break;
    case CR_4_6:
        rate = 46;
        break;
    case CR_4_7:
        rate = 47;
        break;
    case CR_4_8:
        rate = 48;
        break;
    }

    return rate;
}

/* set normal/inverted IQ */
void rfm95w_set_IQ(const char m) {
    switch (m) {
    case IQ_STD:
        rfm95w_write(0x33, 0x27);
        rfm95w_write(0x3B, 0x1D);
        break;
    case IQ_INV:
        rfm95w_write(0x33, 0x67);
        rfm95w_write(0x3B, 0x19);
        break;
    }
}

/* set sync word */
void rfm95w_set_sync_word(const char s) {
    rfm95w_write(0x39, s);
}

/* get rssi of last packet */
unsigned char rfm95w_get_packet_rssi(void) {
    return (unsigned char)rfm95w_read(0x1a);
}


/* load chars into FIFO until null terminator is encountered and then transmit */
void rfm95w_transmit_n_chars(const char* data, unsigned int len) {
    unsigned char i = 0;
    char tx_ptr = rfm95w_read(0x0E);
    rfm95w_write(0x0D, tx_ptr); //set FIFO pointer to transmit buffer region

    rfm95w_set_mode(OP_MODE_STDBY); //must be in stdby to fill fifo

    for(i=0;i<len;i++) {
        rfm95w_write_fifo(data[i]);
    }

    rfm95w_set_payload_length(len);
    rfm95w_set_mode(OP_MODE_TX); //set to transmit mode
}

/* transmit fixed size payload in implicit header mode */
void rfm95w_transmit_fixed_packet(const char* data) {
    unsigned char i;
    unsigned char l;
    char tx_ptr = rfm95w_read(0x0E);
    rfm95w_write(0x0D, tx_ptr); //set FIFO pointer to transmit buffer region
    l = rfm95w_get_payload_length();

    rfm95w_set_mode(OP_MODE_STDBY); //must be in stdby to fill fifo

    for (i=0;i<l;i++) {
        rfm95w_write_fifo(data[i]);
    }

    rfm95w_set_mode(OP_MODE_TX); //set to transmit mode
}

/* read fifo into provided buffer following a successful RX (returns buffer length) */
unsigned char rfm95w_read_fifo(char* buffer) {
    char current_addr;
    unsigned char num_bytes;
    unsigned char i;

    num_bytes = (unsigned char)rfm95w_read(0x13); //num bytes register

    current_addr = rfm95w_read(0x10);
    rfm95w_write(0x0d, current_addr); //write current address to pointer

    //num_bytes = (unsigned char)rfm95w_read(0x13); //num bytes register

    for (i=0;i<num_bytes;i++) { //read fifo num_bytes times
        buffer[i] = rfm95w_read(0x00);
    }

    rfm95w_write(0x0d, 0x00); //reset pointer (may be redundant?)
    return num_bytes;
}

/* save registers to reset chip */
void rfm95w_save_registers(char* registers) {
    unsigned char i;

    for(i=0x01;i<=0x5D;i++) {
        registers[i] = rfm95w_read(i);
    }
}

/* load registers to chip */
void rfm95w_load_registers(char* registers) {
    unsigned char i;

    rfm95w_set_mode(OP_MODE_SLEEP); //put into sleep mode
    rfm95w_write(0x01, registers[1] & 0x80); //LoRa/FSK mode setting must be done in sleep

    for(i=0x02;i<=0x5D;i++) {
        rfm95w_write(i, registers[i]);
    }

    rfm95w_write(0x01, registers[1]); //write rest
}
