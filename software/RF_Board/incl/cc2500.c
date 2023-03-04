#include<cc2500.h>

#include <msp430.h>
#include <serial.h>
#include <util.h>

/* init GDO gpio pins (P2.0->GDO0 and P2.2->GDO2) */
void cc2500_init_gpio(enum cc2500_interrupt_setting interrupts) {
    P2SEL0 &= ~(0b101); //set pins 0 and 2 to GPIO
    P2DIR &= ~(0b101); //set to input
    P2REN &= ~(0b101); //disable pullup/pulldown

    //P2IES |= 0x04; //trigger P2.2 on falling edge
    //P2IE |= 0x04; //enable interrupt

    switch(interrupts) {
    case INT_NONE:
        P2IE &= ~(0x05); //disable interrupts
        break;
    case INT_GDO0:
        P2IES |= 0x01; //trigger P2.0 on falling edge
        P2IE |= 0x01; //enable interrupt
        break;
    case INT_GDO2:
        P2IES |= 0x04; //trigger P2.2 on falling edge
        P2IE |= 0x04; //enable interrupt
        break;
    case INT_BOTH:
        P2IES |= 0x05; //trigger both on falling edge
        P2IE |= 0x05; //enable interrupt
        break;
    }

    //cc2500_configure_gdo(GDO0, GDO_HI_Z);
    //cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //TX/RX detect
}

/* configure GPIO for RF frontend (RF board PCB only) */
void cc2500_init_frontend(void) {
    //TX/RX Switch
    P3SEL0 &= ~(0x3E); //set 3.1-3.5 to I/O
    P3SEL1 &= ~(1 << 4); //enable I/O on P3.4
    P3DIR |= (0x3E); //set 3.1-3.5 to output

    P3OUT |= (1 << 1); //set P3.1 to 1 (bypass LNA A)
    P3OUT |= (1 << 2); //set P3.2 to 1 (bypass LNA B)
    P3OUT |= (1 << 3); //set P3.3 to 1 (shutdown LNAs)
    P3OUT &= ~(1 << 4); //set P3.4 to zero (PA off)
    P3OUT &= ~(1 << 5); //set P3.5 to zero (RX mode)
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

    hardware_timeout(10); //hardware timeout instead of serial timeout because bursts can take longer

    P4OUT &= ~(1 << 4); //NSS low
    __no_operation();

    while ((UCB1IFG & UCTXIFG) == 0 && timeout_flag == 0);
    UCB1TXBUF = 0xFF; //burst RX FIFO address

    while ((UCB1IFG & UCRXIFG) == 0 && timeout_flag == 0); //wait until addr byte data shifted out
    buffer[0] = UCB1RXBUF; //throwaway

    for(i=0;i<len;i++) {
        while ((UCB1IFG & UCTXIFG) == 0 && timeout_flag == 0);
        UCB1TXBUF = 0x00; //transmit nothing

        while ((UCB1IFG & UCRXIFG) == 0 && timeout_flag == 0);
        buffer[i] = UCB1RXBUF; //get next byte
    }

    __no_operation();
    while ((UCB1STATW & UCBUSY) == 1 && timeout_flag == 0); //wait until not busy
    P4OUT |= (1 << 4); //NSS high

    hardware_timeout(0);
    timeout_flag = 0;
}

/* write register */
void cc2500_write(const unsigned char addr, const char data) {
    SPI_TX((addr & 0x3F), data);
}

/* burst write fifo */
void cc2500_burst_write_fifo(const char* buffer, unsigned char len) {
    unsigned char i;

    hardware_timeout(10);

    P4OUT &= ~(1 << 4); //NSS low
    __no_operation();

    while ((UCB1IFG & UCTXIFG) == 0 && timeout_flag == 0);
    UCB1TXBUF = 0x7F; //burst TX FIFO address

    for(i=0;i<len;i++) {
        while ((UCB1IFG & UCTXIFG) == 0 && timeout_flag == 0);
        UCB1TXBUF = buffer[i];
    }

    while ((UCB1STATW & UCBUSY) == 1 && timeout_flag == 0); //wait until not busy
    P4OUT |= (1 << 4); //NSS high

    hardware_timeout(0);
    timeout_flag = 0;
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

    putchars("CC2500 Register Dump:\n\r");
    for (i = 0; i <= 0x3D; i++) {
        cc2500_display_register(i);
    }
    putchars("\n\r");
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

/* calculate operating frequency from register settings */
unsigned long long cc2500_get_frequency(void) {
    unsigned long long high = ((unsigned long long)cc2500_read(0x0D) << 16) & 0xFF0000;
    unsigned long long med = ((unsigned long long)cc2500_read(0x0E) << 8) & 0xFF00;
    unsigned long long low = cc2500_read(0x0F);
    unsigned long long freq = high | med | low;

    unsigned long long chan = cc2500_read(0x0A);

    long long chanspc_e = cc2500_read(0x13) & 0x03;
    unsigned long long chanspc_m = cc2500_read(0x14);

    return (XTAL_FREQ * (freq + chan * ((256 + chanspc_m) * pow(2, chanspc_e - 2))))/65536;
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
char cc2500_command_strobe(const unsigned char strobe) {
    P1OUT |= (0b1);
    char status_byte;

    set_serial_timer(1);
    if (strobe >= 0x30 && strobe <= 0x3D) {
       P4OUT &= ~(1 << 4); //NSS low
       __no_operation();

       while ((UCB1IFG & UCTXIFG) == 0 && SERIAL_TIMEOUT == 0);
       UCB1TXBUF = strobe;

       while ((UCB1IFG & UCRXIFG) == 0 && SERIAL_TIMEOUT == 0);
       status_byte = UCB1RXBUF;

       __no_operation();
       while ((UCB1STATW & UCBUSY) == 1 && SERIAL_TIMEOUT == 0); //wait until not busy
       P4OUT |= (1 << 4); //NSS high
    }
    set_serial_timer(0);
    SERIAL_TIMEOUT = 0;

    P1OUT &= ~(0b1);
    return status_byte;
}

/* get chip status */
char cc2500_get_status(void) {
    return cc2500_command_strobe(STROBE_SNOP) & 0x70; //issue SNOP strobe to return state bits
}

/* VCO settings */
void cc2500_set_vco_autocal(const unsigned char autocal) {
    char MCSM0 = cc2500_read(0x18) & ~(0x30);
    cc2500_write(0x18, MCSM0 | autocal);
}

/* RXOFF mode */
void cc2500_set_rxoff_mode(const char mode) {
    char MCSM1 = cc2500_read(0x17) & ~(0x0C);
    cc2500_write(0x17, MCSM1 | mode);
}

/* TXOFF mode */
void cc2500_set_txoff_mode(const char mode) {
    char MCSM1 = cc2500_read(0x17) & ~(0x03);
    cc2500_write(0x17, MCSM1 | mode);
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

/* switch frontend mode */
void cc2500_set_frontend(enum cc2500_frontend_setting setting) {
    switch (setting) {
    case RX_SHUTDOWN:
        P3OUT &= ~(1 << 4); //set P3.4 to zero (PA off)

        P3OUT |= (1 << 3); //set P3.3 to 1 (shutdown LNAs)
        P3OUT &= ~(1 << 1); //both bypasses off
        P3OUT &= ~(1 << 2);

        P3OUT &= ~(1 << 5); //set P3.5 to zero (RX mode)
        break;
    case RX_DUAL_BYPASS:
        P3OUT &= ~(1 << 4); //set P3.4 to zero (PA off)

        P3OUT &= ~(1 << 3); //set P3.3 to 0 (enable LNAs)
        P3OUT |= (1 << 1); //set P3.1 to 1 (bypass LNA A)
        P3OUT |= (1 << 2); //set P3.2 to 1 (bypass LNA B)

        P3OUT &= ~(1 << 5); //set P3.5 to zero (RX mode)
        break;
    case RX_SINGLE_BYPASS:
        P3OUT &= ~(1 << 4); //set P3.4 to zero (PA off)

        P3OUT &= ~(1 << 3); //set P3.3 to 0 (enable LNAs)
        P3OUT |= (1 << 1); //set P3.1 to 1 (bypass LNA A)
        P3OUT &= ~(1 << 2); //set P3.2 to 0 (enable LNA B)

        P3OUT &= ~(1 << 5); //set P3.5 to zero (RX mode)
        break;
    case RX_NO_BYPASS:
        P3OUT &= ~(1 << 4); //set P3.4 to zero (PA off)

        P3OUT &= ~(1 << 3); //set P3.3 to 0 (enable LNAs)
        P3OUT &= ~(1 << 1); //set P3.1 to 0 (enable LNA A)
        P3OUT &= ~(1 << 2); //set P3.2 to 0 (enable LNA B)

        P3OUT &= ~(1 << 5); //set P3.5 to zero (RX mode)
        break;
    case TX:
        P3OUT |= (1 << 3); //set P3.3 to 1 (shutdown LNAs)
        P3OUT &= ~(1 << 1); //both bypasses off
        P3OUT &= ~(1 << 2);

        P3OUT |= (1 << 5); //set P3.5 to one (TX mode)
        P3OUT |= (1 << 4); //set P3.4 to zero (PA on)
        break;
    }
}

/* transmit */
void cc2500_transmit(const char* data, const char size) {
    cc2500_write(0x3F, size); //write size
    cc2500_burst_write_fifo(data, size);

    cc2500_command_strobe(STROBE_STX); //enter transmit mode
    while ((P2IN & 0x04) != 0x04); //detect start of transmit
    while ((P2IN & 0x04) == 0x04); //wait for end
    cc2500_command_strobe(STROBE_SFTX); //reset and enter IDLE
}

/* receive */
unsigned char cc2500_receive(char* buffer) {
    unsigned char len;
    char status;

    status = cc2500_read(0x3B);

    if ((status & 0x7F) != 0) { //if RX is not empty
        len = cc2500_read(0xbf); //FIFO RX access
        cc2500_burst_read_fifo(buffer, len);
        return len;
    }
    else {
        return 0;
    }
}

/* calculate data rate from register settings */
unsigned long long cc2500_get_data_rate(void) {
    unsigned long drate_e = cc2500_read(0x10) & 0x0F;
    unsigned long drate_m = cc2500_read(0x11);

    return (((256 + drate_m) * pow(2, drate_e))* XTAL_FREQ)/268435456;
}

/* save registers to array */
void cc2500_save_registers(char* registers) {
    unsigned int i;
    for(i=0;i<0x2E;i++) {
        registers[i] = cc2500_read(i);
    }
}

/* load registers from array */
void cc2500_load_registers(char* registers) {
    unsigned int i;
    for(i=0;i<0x2E;i++) {
        cc2500_write(i, registers[i]);
    }
}
