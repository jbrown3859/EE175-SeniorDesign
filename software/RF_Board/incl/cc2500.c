#include<cc2500.h>

#include <msp430.h>
#include <serial.h>
#include <util.h>

extern char RX_done;
//char timeout_flag;

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

/* burst read FIFO */
void cc2500_burst_read_fifo(char* buffer, unsigned char len) {
    unsigned char i;

    hardware_timeout(100);

    P4OUT &= ~(1 << 4); //NSS low
    __no_operation();

    while ((UCB1IFG & UCTXIFG) == 0 && timeout_flag == 0);
    UCB1TXBUF = 0xFF; //burst RX FIFO address

    while ((UCB1IFG & UCRXIFG) == 0 && timeout_flag == 0); //wait until addr byte data shifted out

    for(i=0;i<len;i++) {
        while ((UCB1IFG & UCTXIFG) == 0 && timeout_flag == 0);
        UCB1TXBUF = 0x00; //transmit nothing

        while ((UCB1IFG & UCRXIFG) == 0 && timeout_flag == 0);
        buffer[i] = UCB1RXBUF; //get next byte
    }

    __no_operation();
    while ((UCB1STATW & UCBUSY) == 1 ); //wait until not busy
    P4OUT |= (1 << 4); //NSS high

    timeout_flag = 0;
    hardware_timeout(0);
}

/* write register */
void cc2500_write(const unsigned char addr, const char data) {
    SPI_TX((addr & 0x3F), data);
}

/* burst write fifo */
void cc2500_burst_write_fifo(const char* buffer, unsigned char len) {
    unsigned char i;

    P4OUT &= ~(1 << 4); //NSS low
    __no_operation();

    while ((UCB1IFG & UCTXIFG) == 0);
    UCB1TXBUF = 0x7F; //burst TX FIFO address

    for(i=0;i<len;i++) {
        while ((UCB1IFG & UCTXIFG) == 0);
        UCB1TXBUF = buffer[i];
    }

    while ((UCB1STATW & UCBUSY) == 1 ); //wait until not busy
    P4OUT |= (1 << 4); //NSS high
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

void cc2500_set_crc(const char crc_en, const char crc_autoflush, const char crc_append) {
    char PKTCTRL0 = cc2500_read(0x08) & ~(0x04);
    char PKTCTRL1 = cc2500_read(0x07) & ~(0x0C);
    cc2500_write(0x08, PKTCTRL0 | crc_en);
    cc2500_write(0x07, PKTCTRL1 | crc_autoflush | crc_append);
}

/* GDO configuration */
void cc2500_configure_gdo(const unsigned char pin, const unsigned char config) {
    if (pin <= 0x02) {
        cc2500_write(pin, config);
    }
}

/* output power setting (programs PATABLE index 0 only, will not work for OOK)
 *  0x00 = undefined, -55 dBm or less
 *  0x50 = -30 dBm
 *  0xFF = +1 dBm
 */
void cc2500_set_tx_power(const unsigned char power) {
    char FREND0 = cc2500_read(0x22) & 0xF8;
    cc2500_write(0x3E, power);
    cc2500_write(0x22, FREND0); //sets PA_POWER to 0x00
}

/* transmit */
void cc2500_transmit(const char* data, const char size) {
    cc2500_configure_gdo(GDO2, GDO_HI_Z); //disable GDO2 to avoid interrupt
    cc2500_configure_gdo(GDO0, TX_RX_ACTIVE); //set I/O pin to detect when transmit is complete

    cc2500_write(0x3f, size); //write size
    cc2500_burst_write_fifo(data, size);

    cc2500_command_strobe(STROBE_STX); //enter transmit mode
    while ((P2IN & 0x01) != 0x01); //detect start of transmit
    while ((P2IN & 0x01) == 0x01); //wait for end
    cc2500_command_strobe(STROBE_SFTX); //reset and enter IDLE

    cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //RX detect
}

/* receive */
unsigned char cc2500_receive(char* buffer) {
    unsigned char len;

    if ((cc2500_read(0x3B) & 0x7F) != 0) { //if RX is not empty
        len = cc2500_read(0xbf); //FIFO RX access
        cc2500_burst_read_fifo(buffer, len);
        cc2500_command_strobe(STROBE_SFRX);
        return len;
    }
    else {
        cc2500_command_strobe(STROBE_SFRX);
        return 0;
    }
}
