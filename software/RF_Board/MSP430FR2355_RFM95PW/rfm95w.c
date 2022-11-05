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
    return rfm95w_read(0x01) & 0b111; //return only mode bits
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
