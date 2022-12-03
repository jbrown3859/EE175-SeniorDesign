#include<cc2500.h>

#include <msp430.h>
#include <serial.h>

/* read register */
char cc2500_read(const unsigned char addr) {
    if (addr >= 0x30 && addr <= 0x3D) { //status registers require burst bit to be set
        return SPI_RX((addr & 0x3F) | 0xC0);
    }
    else {
        return SPI_RX((addr & 0x3F) | 0x80);
    }
}

/* write register */
void cc2500_write(const unsigned char addr, const char data) {
    SPI_TX((addr & 0x3F), data);
}

/* display register addr and value over uart for debug */
void cc2500_display_register(const char addr) {
    char c;
    putchars("Address: ");
    print_hex(addr);
    c = cc2500_read(addr);
    putchars(" Data: ");
    print_hex(c);
    putchars("\n\r");
}

/* dump all registers over serial */
void cc2500_register_dump(void) {
    unsigned char i;
    for (i = 0; i <= 0x3D; i++) {
        cc2500_display_register(i);
    }
}

/* set base frequency (equal to channel 0 frequency) */
void cc2500_set_base_frequency(const unsigned long long freq) {
    unsigned long long f;
    f = (65536 * freq) / (XTAL_FREQ);

    cc2500_write(0x0D, (char)((f >> 16) & 0xFF)); //high byte
    cc2500_write(0x0E, (char)((f >> 8) & 0xFF)); //middle byte
    cc2500_write(0x0F, (char)(f & 0xFF)); //low byte
}

/* set intermediate frequency for RX */
void cc2500_set_IF_frequency(const unsigned long long freq ) {
    unsigned char f;
    f = (1024 * freq) / (XTAL_FREQ);
    if (f <= 31) { //bits 0-4
        cc2500_write(0x0B, f);
    }
}

/* set channel number */
void cc2500_set_channel(const unsigned char channel) {
    cc2500_write(0x0A, channel);
}

/* issue command strobe */
void cc2500_command_strobe(const unsigned char strobe) {
    if (strobe >= 0x30 && strobe <= 0x3D) {
        SPI_TX(strobe, 0x00);
    }
}
