#include<cc2500.h>

#include <msp430.h>
#include <serial.h>

extern char RX_done;

/* ISR for RX detection */
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    RX_done = 1;
    P2IFG &= ~(0x04);
}

/* init GDO gpio pins (P2.0->GDO0 and P2.2->GDO2) */
void cc2500_init_gpio(void) {
    P2SEL0 &= ~(0b101); //set pins 0 and 2 to GPIO
    P2DIR &= ~(0b101); //set to input
    P2REN &= ~(0b101); //disable pullup/pulldown

    P2IES |= 0x04; //trigger P2.2 on falling edge
    P2IE |= 0x04; //enable interrupt
}

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

/* radio programming */
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

/* set data rate */
void cc2500_set_data_rate(const unsigned char mantissa, const unsigned char exponent) {
    char MDMCFG4 = cc2500_read(0x10) & 0xF0;
    cc2500_write(0x10, (MDMCFG4 | (exponent & 0x0F)));
    cc2500_write(0x11, mantissa);
}

/* issue command strobe */
void cc2500_command_strobe(const unsigned char strobe) {
    if (strobe >= 0x30 && strobe <= 0x3D) {
        SPI_TX(strobe, 0x00);
    }
}

/* VCO settings */
void cc2500_set_vco_autocal(const unsigned char autocal) {
    char MCSM0 = cc2500_read(0x18) & ~(0x30);
    cc2500_write(0x18, MCSM0 | autocal);
}

/* FIFO and packet settings */
/* Consult datasheet for splits:
 * 0x00 == 61 TX, 4 RX
 * 0x07 == 33 TX, 32 RX
 * 0x0F == 1 TX, 64 RX
 */
void cc2500_set_fifo_thresholds(const unsigned char threshold) {
    cc2500_write(0x03, threshold);
}

void cc2500_set_packet_length(const unsigned char pktlen) {
    cc2500_write(0x06, pktlen);
}

void cc2500_set_data_whitening(const unsigned char white) {
    char PKTCTRL0 = cc2500_read(0x08) & ~(0x40);
    cc2500_write(0x08, (PKTCTRL0 | white));
}

void cc2500_set_sync_word(const unsigned short sync) {
    cc2500_write(0x04, (char)((sync >> 8) & 0xFF));
    cc2500_write(0x05, (char)(sync & 0xFF));
}

/* GDO configuration */
void cc2500_configure_gdo(const unsigned char pin, const unsigned char config) {
    if (pin <= 0x02) {
        cc2500_write(pin, config);
    }
}

/* Transmit */
void cc2500_transmit(const char* data, const char size) {
    unsigned char i;

    cc2500_configure_gdo(GDO2, GDO_HI_Z); //disable GDO2 to avoid interrupt
    cc2500_configure_gdo(GDO0, TX_RX_ACTIVE); //set I/O pin to detect when transmit is complete

    cc2500_write(0x3f, size); //write size for variable length mode
    for (i=0;i<size;i++) {
        cc2500_write(0x3f, data[i]); //write data
    }

    cc2500_command_strobe(STROBE_STX); //enter transmit mode
    while ((P2IN & 0x01) != 0x01); //detect start of transmit
    while ((P2IN & 0x01) == 0x01); //wait for end
    cc2500_command_strobe(STROBE_SFTX); //reset and enter IDLE

    cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //RX detect
}

/* receive */
unsigned char cc2500_receive(char* buffer) {
    unsigned char len;
    unsigned char i;

    if ((cc2500_read(0x3B) & 0x7F) != 0) { //if RX is not empty
        len = cc2500_read(0xbf); //FIFO RX access

        for(i=0;i<len;i++) {
            if (cc2500_read(0x3B) & 0x7F == 0x00) {
                len = i;
                break; //leave loop if there are no more bytes to grab
            }
            buffer[i] = cc2500_read(0xbf);
        }

        cc2500_command_strobe(STROBE_SFRX);
        return len;
    }
    else { //reset to IDLE if no data in buffer
        cc2500_command_strobe(STROBE_SFRX);
        return 0;
    }

}
