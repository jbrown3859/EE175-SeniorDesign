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
    //cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //RX detect
    cc2500_configure_gdo(GDO0, GDO_HI_Z);
    cc2500_set_packet_length(40);
    cc2500_set_data_whitening(WHITE_OFF);
    cc2500_set_fifo_thresholds(0x0A);
    //cc2500_set_sync_word(0xBAAD);
    cc2500_set_data_rate(MAN_28800,EXP_28800);
    //cc2500_set_crc(CRC_ENABLED, CRC_AUTOFLUSH, 0x00);
    cc2500_set_crc(0x00,0x00,0x00);
    cc2500_write(0x26, 0x11); //value from smartrf studio
    cc2500_set_tx_power(0xFF);
    //cc2500_register_dump();
    cc2500_init_gpio(); //init after programming to avoid false interrupts


    /*
    char image_packet[18] = {0,0,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64,64};
    unsigned int img_ptr = 0;

    for (;;) {
        //hardware_delay(2000);
        putchars("Transmitting\n\r");
        image_packet[0] = ((img_ptr >> 8) & 0xFF) | 0x80;
        image_packet[1] = (img_ptr) & 0xFF;
        cc2500_transmit(image_packet, 18);
        img_ptr = (img_ptr < 1200) ? img_ptr + 1 : 0;
    }
    */

    //char len;
    //char buffer[64];
    unsigned int i;
    //cc2500_command_strobe(STROBE_SRX);


    P2IES |= 0x04; //trigger P2.2 on falling edge
    P2IE |= 0x04; //enable interrupt
    /*
	for(;;) {

	    char message[] = "Radio Test Packet #0";

	    for (i=0;i<10;i++) {
	        message[19] = 48 + i;
	        cc2500_transmit(message, 20);
	        putchars("Transmitted Packet\n\r");
	    }

	    __no_operation();
	}
    */

    main_loop();

	return 0;
}
