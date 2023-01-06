#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <cc2500.h>

#include <backend.h>

char RX_done;

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    init_UART(115200);
    init_SPI_master();

    /* LED */
    P1SEL0 &= ~(0b1); //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero

    putchars("\n\rResetting Chip\n\r");
    cc2500_command_strobe(STROBE_SRES); //reset chip
    cc2500_register_dump();
    putchars("\n\rRegister Dump\n\r");
    //cc2500_set_base_frequency(2405000000);
    //cc2500_set_IF_frequency(457000);
    cc2500_set_vco_autocal(AUTOCAL_FROM_IDLE);
    cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //RX detect
    cc2500_configure_gdo(GDO0, GDO_HI_Z);
    cc2500_set_packet_length(32);
    cc2500_set_data_whitening(WHITE_OFF);
    //cc2500_set_sync_word(0xBAAD);
    cc2500_set_data_rate(MAN_76800,EXP_76800);
    cc2500_set_crc(CRC_ENABLED, CRC_AUTOFLUSH, CRC_APPEND);
    cc2500_write(0x26, 0x11); //value from smartrf studio
    cc2500_set_tx_power(0xC6);
    cc2500_register_dump();

    cc2500_init_gpio(); //init after programming to avoid false interrupts

	/* infinite loop */
    char buffer[256];
    unsigned char len;
    unsigned char i;

    putchars("Entering main loop\n\r");
    cc2500_command_strobe(STROBE_SRX);
	for(;;) {
	    char message[] = "Radio Test Packet #0";
	    /*
	    for (i=0;i<10;i++) {
	        message[19] = 48 + i;
	        cc2500_transmit(message, 20);
	    }
	    */
	    if (RX_done == 1) {
	        len = cc2500_receive(buffer);
	        cc2500_command_strobe(STROBE_SRX);
	        buffer[len] = '\0';
	        RX_done = 0;

	        putchars(buffer);
	        putchars("\n\r");
	    }

	    __no_operation();
	}

    //main_loop();

	return 0;
}
