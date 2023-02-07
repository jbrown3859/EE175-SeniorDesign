#include <msp430.h>
#include <serial.h>
#include <rfm95w.h>
#include <util.h>

char DIO0_mode;
//extern char TX_done;
//extern char RX_done;

//extern char TX_timeout;

/* vector for interrupt on P2.0 triggered by DIO0 on LoRa */
/*
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    switch(DIO0_mode) {
    case DIO0_RXDONE:
        rfm95w_clear_flag(FLAG_RXDONE);
        rfm95w_clear_flag(FLAG_VALIDHEADER);
        RX_done = 1;
        break;
    case DIO0_TXDONE:
        rfm95w_clear_flag(FLAG_TXDONE);
        TX_done = 1;
        break;
    case DIO0_CADDONE:
        break;
    }
    P2IFG &= ~(0x01); //clear interrupt flag
}
*/

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
    for (i = 0; i < 128; i++) {
        rfm95w_display_register(i);
    }
}

/* get device mode */
char rfm95w_get_mode(void) {
    return (rfm95w_read(0x01) & 0b111); //return only mode bits
}

/* set device mode (sleep, stdby, TX, RX, etc.) */
void rfm95w_set_mode(const char mode) {
    char r = rfm95w_read(0x01) & ~(0b00000111); //clear mode bits
    rfm95w_write(0x01, (r | mode)); //write new mode
    while(rfm95w_get_mode() != mode); //poll until commanded mode is entered
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

/* set spreading factor (will only write if sf is in range 6-12) */
void rfm95w_set_spreading_factor(const char sf) {
    char r = rfm95w_read(0x1E) & 0x0F; //clear high bits

    if (sf >= 6 && sf <= 12) {
        rfm95w_write(0x1E, (r | (sf << 4)));
    }
}

/* write a 1 to the desired flag, clearing it */
void rfm95w_clear_flag(const char f) {
    rfm95w_write(0x12, f);
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
void rfm95w_transmit_chars(const char* data) {
    char c;
    unsigned char i = 0;
    char tx_ptr = rfm95w_read(0x0E);
    rfm95w_write(0x0D, tx_ptr); //set FIFO pointer to transmit buffer region

    rfm95w_set_mode(OP_MODE_STDBY); //must be in stdby to fill fifo

    c = data[i];
    while(c != '\0') {
        rfm95w_write_fifo(c);
        i++;
        c = data[i];
    }

    rfm95w_set_payload_length(i);
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

    current_addr = rfm95w_read(0x10);
    rfm95w_write(0x0d, current_addr); //write current address to pointer

    num_bytes = (unsigned char)rfm95w_read(0x13); //num bytes register

    for (i=0;i<num_bytes;i++) { //read fifo num_bytes times
        buffer[i] = rfm95w_read(0x00);
    }

    rfm95w_write(0x0d, 0x00); //reset pointer (may be redundant?)
    return num_bytes;
}
