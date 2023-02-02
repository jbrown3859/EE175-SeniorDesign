#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <cc2500.h>
//#include <rfm95w.h>
#include <backend.h>


/* ISR for RX detection */
/*
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    char buffer[64];
    unsigned int len;

   len = cc2500_receive(buffer);
    if (len > 0) { //filter failed CRCs
        buffer[len] = '\0';

        putchars(buffer);
        putchars("\n\r");
    }

    cc2500_command_strobe(STROBE_SRX);
    P2IFG &= ~(0x04);
}
*/

/*
#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void) {
    char pkt[64];
    unsigned int pkt_len;
    unsigned int i;

    pkt_len = cc2500_receive(pkt);
    if (pkt_len > 0) { //filter failed CRCs
        for (i = 0; i < 16; i++) {
            cc2500_transmit(pkt, pkt_len); //repeat message
        }
    }

    cc2500_command_strobe(STROBE_SRX);

   // WDTCTL = WDTPW | WDTCNTCL; //reset watchdog count
    P2IFG &= ~(0x04);
}
*/

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    //WDTCTL = WDTPW | 0b1011 | (1 << 5); //reset after 16s
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    init_UART(115200);
    init_SPI_master();


    //putchars("\n\rResetting Chip\n\r");
    cc2500_command_strobe(STROBE_SRES); //reset chip
    hardware_delay(100);
    //cc2500_register_dump();
    //putchars("\n\rRegister Dump\n\r");
    //cc2500_set_base_frequency(2405000000);
    //cc2500_set_IF_frequency(457000);
    cc2500_set_vco_autocal(AUTOCAL_FROM_IDLE);
    cc2500_set_packet_length(40);
    cc2500_set_data_whitening(WHITE_OFF);
    cc2500_set_fifo_thresholds(0x0A);
    //cc2500_set_sync_word(0xBAAD);
    cc2500_set_data_rate(MAN_38400,EXP_38400);
    //cc2500_set_crc(CRC_ENABLED, CRC_AUTOFLUSH, 0x00);
    cc2500_set_crc(0x00,0x00,0x00);
    cc2500_write(0x26, 0x11); //value from smartrf studio
    cc2500_set_tx_power(0xFF);
    //cc2500_register_dump();
    cc2500_set_rxoff_mode(RXOFF_IDLE);
    cc2500_set_txoff_mode(TXOFF_IDLE);
    cc2500_configure_gdo(GDO0, GDO_HI_Z);
    cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //TX/RX detect

    cc2500_init_gpio(INT_GDO2); //init after programming to avoid false interrupts
    //cc2500_init_gpio(INT_NONE);

    main_loop();

    /* for test only */
    char image_packet[18] = {0,0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64};
    unsigned int img_ptr = 0;

    unsigned int i;
    for (;;) {
        hardware_delay(200);
        putchars("Transmitting\n\r");

        for (i=2;i<18;i++) {
            if (img_ptr > 100 && img_ptr < 300 && img_ptr % 10 >= 2 && img_ptr % 10 <= 7) {
                image_packet[i] = 0xF3;
            }
            else if (img_ptr > 300 && img_ptr < 500 && img_ptr % 10 >= 2 && img_ptr % 10 <= 7) {
                image_packet[i] = 0x0F;
            }
            else if (img_ptr > 500 && img_ptr < 700 && img_ptr % 10 >= 2 && img_ptr % 10 <= 7) {
                image_packet[i] = 0x0C;
            }
            else if (img_ptr > 700 && img_ptr < 900 && img_ptr % 10 >= 2 && img_ptr % 10 <= 7) {
                image_packet[i] = 0x03;
            }
            else if (img_ptr > 900 && img_ptr < 1100 && img_ptr % 10 >= 2 && img_ptr % 10 <= 7) {
                image_packet[i] = 0xFC;
            }
            else {
                image_packet[i] = 0xF0;
            }
        }
        image_packet[0] = ((img_ptr >> 8) & 0xFF) | 0x80;
        image_packet[1] = (img_ptr) & 0xFF;
        cc2500_transmit(image_packet, 18);

        img_ptr = (img_ptr < 1200) ? img_ptr + 1 : 0;
    }

	return 0;
}
