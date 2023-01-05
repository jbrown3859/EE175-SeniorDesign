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
    //cc2500_set_packet_length(32);
    cc2500_set_data_whitening(WHITE_OFF);
    //cc2500_set_sync_word(0xBAAD);
    cc2500_set_data_rate(131,8); //9600 baud
    cc2500_write(0x26, 0x11); //value from smartrf studio
    cc2500_register_dump();

    cc2500_init_gpio(); //init after programming to avoid false interrupts

	/* infinite loop */
    char buffer[256];
    unsigned char len;
    unsigned char i;

    putchars("Entering main loop\n\r");
    cc2500_command_strobe(STROBE_SRX);
	for(;;){
	    /*
	    putchars("Transmitting\n\r");
	    cc2500_burst_tx("I have never met Napoleon", 25);
	    cc2500_burst_tx("But I plan to find the time", 27);
        */
	    /*
        print_binary(cc2500_read(0x38)); //for debug
        putchars("\n\r");
        print_hex(cc2500_read(0x3B));
        putchars("\n\n\r");
        */

	    if (RX_done == 1) {
	        //putchars("Packet Detected, RX buffer: ");
	        //print_hex(cc2500_read(0x3B));
	        //putchars("\n\r");
	        //len = cc2500_receive(buffer);
	        len = cc2500_burst_rx(buffer);
	        buffer[len] = '\0';
	        //putchars("Got Packet, length=");
	        //print_hex(len);
	        //putchars("\n\r");
	        putchars(buffer);
	        putchars("\n\r");
	        cc2500_command_strobe(STROBE_SRX);
	        RX_done = 0;
	    }


	    __no_operation();
	}

    //main_loop();

	return 0;
}
