#include <msp430.h>
#include <serial.h>
#include <rfm95w.h>

extern char DIO0_mode;
extern char TX_done;
extern char RX_done;

/* vector for interrupt on P2.0 triggered by DIO0 on LoRa */
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    switch(DIO0_mode) {
    case DIO0_RXDONE:
        rfm95w_write(0x12, 0x40); //clear flag
        RX_done = 1;
        break;
    case DIO0_TXDONE:
        rfm95w_write(0x12, 0x08); //clear flag
        TX_done = 1;
        break;
    case DIO0_CADDONE:
        break;
    }

    P2IFG &= ~(0x01); //clear interrupt flag
}

void rfm95w_init(void) {
    P2DIR &= ~(0x01); //set P2.0 to input
    P2REN &= ~(0x01); //disable pull resistors
    P2IES &= ~(0x01); //trigger on rising edge
    P2IE |= 0x01; //enable interrupt
}

char rfm95w_read(const char addr) {
    return SPI_RX(addr);
}

void rfm95w_write(char addr, const char c) {
    addr |= 0x80;
    SPI_TX(addr, c); //transmit write bit along with addr
}

void rfm95w_display_register(const char addr) {
    char c;
    putchars("Address: ");
    print_hex(addr);
    c = rfm95w_read(addr);
    putchars(" Data: ");
    print_hex(c);
    putchars("\n\r");
}

void rfm95w_register_dump(void) {
    unsigned char i;
    for (i = 0; i < 128; i++) {
        rfm95w_display_register(i);
    }
}

/* set device mode (sleep, stdby, TX, RX, etc.) */
void rfm95w_set_mode(const char mode) {
    char r = rfm95w_read(0x01) & ~(0b00000111); //clear mode bits
    rfm95w_write(0x01, (r | mode)); //write new mode
}

char rfm95w_get_mode(void) {
    return (rfm95w_read(0x01) & 0b111); //return only mode bits
}

/* set register 0x01 to configure radio modulation and put radio into standby */
void rfm95w_set_lora_mode(const char lora_mode) {
    rfm95w_set_mode(MODE_SLEEP); //device must be in sleep to change modulation

    char r = rfm95w_read(0x01) & ~(0b10000000); //clear lora bit
    r |= lora_mode; //set lora mode
    rfm95w_write(0x01, r);

    rfm95w_set_mode(MODE_STDBY);
}

/* set carrier frequency and put radio into standby */
void rfm95w_set_carrier_frequency(const unsigned long long frequency) {
    const unsigned long long frf = ((frequency * 524288)/F_XOSC); //from datasheet formula

    rfm95w_set_mode(MODE_STDBY); //must be in standby or sleep to program frequency

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
void rfm95w_set_lora_bandwidth(char b) {
    char r = rfm95w_read(0x1D) & 0x0F; //clear high bits
    rfm95w_write(0x1D, (r | b));
}

/* set spreading factor (will only write if sf is in range 6-12) */
void rfm95w_set_spreading_factor(char sf) {
    char r = rfm95w_read(0x1E) & 0x0F; //clear high bits

    if (sf >= 6 && sf <= 12) {
        rfm95w_write(0x1E, (r | (sf << 4)));
    }
}

/* load chars into FIFO until null terminator is encountered and then transmit */
void rfm95w_transmit_chars(const char* data) {
    char c;
    unsigned char i = 0;
    char tx_ptr = rfm95w_read(0x0E);
    rfm95w_write(0x0D, tx_ptr); //set FIFO pointer to transmit buffer region

    rfm95w_set_mode(MODE_STDBY); //must be in stdby to fill fifo

    c = data[i];
    while(c != '\0') {
        rfm95w_write_fifo(c);
        i++;
        c = data[i];
    }

    rfm95w_write(0x22, i); //write payload length
    rfm95w_set_mode(MODE_TX); //set to transmit mode
    while(rfm95w_get_mode() != MODE_TX); //poll until TX is entered
}

/* read fifo into provided buffer following a successful RX (returns buffer length) */
unsigned char rfm95w_read_fifo(char* buffer) {
    char current_addr;
    unsigned char num_bytes;
    unsigned char i;

    current_addr = rfm95w_read(0x10);
    rfm95w_write(0x0d, current_addr); //write current address to pointer

    num_bytes = (unsigned char)rfm95w_read(0x13);


    for (i=0;i<num_bytes;i++) { //read fifo num_bytes times
        buffer[i] = rfm95w_read(0x00);
    }

    rfm95w_write(0x0d, 0x00); //reset pointer (may be redundant?)
    return num_bytes;
}

/* tx done flag (prefer to do this in hardware with DIO0 instead) */
char rfm95w_tx_done(void) {
    return ((rfm95w_read(0x12) >> 3) & 0x01);
}

char rfm95w_rx_done(void) {
    return ((rfm95w_read(0x12) >> 6) & 0x01);
}

void rfm95w_set_DIO_mode(const char m) {
    rfm95w_write(0x40, m);
    DIO0_mode = m;
}
