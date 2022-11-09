#include <msp430.h>
#include <serial.h>
#include <rfm95w.h>

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

    c = data[i];
    while(c != '\0') {
        rfm95w_write_fifo(c);
        i++;
        c = data[i];
    }

    rfm95w_write(0x22, i); //write payload length
    rfm95w_set_mode(MODE_TX); //set to transmit mode
}

/* tx done flag (prefer to do this in hardware with DIO0 instead) */
char rfm95w_tx_done(void) {
    return ((rfm95w_read(0x12) >> 3) & 0x01);
}
