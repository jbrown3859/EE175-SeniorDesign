#include <msp430.h> 
#include <util.h>
#include <serial.h>

#include <cc2500.h>

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    /* GPIO */
    P3SEL0 &= ~(1 << 5); //set P3.5 to I/O
    P3DIR |= (1 << 5); //set P3.5 to output
    P3OUT &= ~(1 << 5); //set P3.5 to zero

    init_clock();
    init_serial_timer(1000); //set I/O timeout
    init_UART(115200);
    init_SPI_master();

    /* radio programming */
    cc2500_command_strobe(STROBE_SRES); //reset chip
    hardware_delay(100);
    //cc2500_set_base_frequency(2450000000);
    //cc2500_set_IF_frequency(457000);
    cc2500_set_vco_autocal(AUTOCAL_FROM_IDLE);
    cc2500_set_packet_length(40);
    cc2500_set_data_whitening(WHITE_OFF);
    cc2500_set_fifo_thresholds(0x0A);
    //cc2500_set_sync_word(0xBAAD);
    cc2500_set_data_rate(MAN_38400,EXP_38400);
    //cc2500_set_crc(CRC_ENABLED, CRC_AUTOFLUSH, 0x00);
    cc2500_set_crc(0x00,0x00,0x00);
    //cc2500_write(0x26, 0x11); //value from smartrf studio
    cc2500_set_tx_power(0xFF);
    cc2500_set_rxoff_mode(RXOFF_IDLE);
    cc2500_set_txoff_mode(TXOFF_IDLE);
    cc2500_configure_gdo(GDO0, GDO_HI_Z);
    cc2500_configure_gdo(GDO2, TX_RX_ACTIVE); //TX/RX detect

    cc2500_init_gpio(INT_NONE); //init after programming to avoid false interrupts

    cc2500_register_dump();

    __no_operation();

    for (;;) {
        putchars("Setting TX/RX to 1\n\r");
        P3OUT |= (1 << 5); //set to one
        __no_operation();
        putchars("Setting TX/RX to 0\n\r");
        P3OUT &= ~(1 << 5); //set P3.5 to zero
        __no_operation();
    }

    /*
    unsigned int i;
	for(;;) {
	    for (i=0;i<2000;i++) {
	        SPI_TX(0x00, 'M');
	    }
	    putchars("I haue read the truest computer of Times,\n\r");
	    putchars("and the best Arithmetician that euer breathed,\n\r");
	    putchars("and he reduceth thy dayes into a short number\n\r");
	    putchars("\n\r");
	    //hardware_delay(10000);
	}
	*/

	return 0;
}
