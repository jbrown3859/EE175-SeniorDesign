#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <cc2500.h>

#include <backend.h>

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    init_UART(115200);
    init_SPI_master();

    /*
    putchars("\n\rResetting Chip\n\r");
    cc2500_command_strobe(STROBE_SRES); //reset chip
    cc2500_register_dump();
    putchars("\n\rRegister Dump\n\r");
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
    cc2500_register_dump();
    cc2500_init_gpio(); //init after programming to avoid false interrupts
    */

	/* infinite loop */
    putchars("Entering main loop\n\r");

    //SPI test
    unsigned int i;
    for (;;) {
        putchars("SPI mode 00\n\r");
        set_SPI_mode(0,0);

        putchars("SPI mode 01\n\r");
        set_SPI_mode(0,1);
        SPI_RX(0xAA);
        putchars("SPI mode 10\n\r");
        set_SPI_mode(1,0);
        SPI_RX(0xAA);
        putchars("SPI mode 11\n\r");
        set_SPI_mode(1,1);
        SPI_RX(0xAA);
    }


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
    /*
    cc2500_command_strobe(STROBE_SRX);
	for(;;) {
	    char message[] = "Radio Test Packet #0";

	    for (i=0;i<10;i++) {
	        message[19] = 48 + i;
	        cc2500_transmit(message, 20);
	    }

	    if (cc2500_TXRX_done == 1) {
	        len = cc2500_receive(buffer);
	        cc2500_command_strobe(STROBE_SRX);
	        buffer[len] = '\0';
	        cc2500_TXRX_done = 0;

	        putchars(buffer);
	        putchars("\n\r");
	    }

	    __no_operation();
	}
    */
    main_loop();

	return 0;
}
