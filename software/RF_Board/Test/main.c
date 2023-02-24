#include <msp430.h> 
#include <util.h>
#include <serial.h>

#include <cc2500.h>
#include <rfm95w.h>

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    /* GPIO */
    cc2500_init_frontend();

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

    char msg[5] = {'T','E','S','T'};

    for (;;) {
        putchars("Setting to TX\n\r");
        cc2500_set_frontend(TX);
        __no_operation();
        cc2500_transmit(msg, 5);
        putchars("Transmission complete\n\r");
        __no_operation();
        cc2500_set_frontend(RX_SHUTDOWN);
        putchars("Set to IDLE\n\r");
        __no_operation();
        cc2500_set_frontend(RX_DUAL_BYPASS);
        putchars("Set to RX dual bypass\n\r");
        __no_operation();
        cc2500_set_frontend(RX_SINGLE_BYPASS);
        putchars("Set to single bypass\n\r");
        __no_operation();
        cc2500_set_frontend(RX_NO_BYPASS);
        putchars("Set to no bypass\n\r");
        __no_operation();
    }

    /*
    rfm95w_init();
    rfm95w_reset();

    rfm95w_set_lora_mode(MODE_LORA);
    rfm95w_set_frequency_mode(0x00);
    rfm95w_set_carrier_frequency(433500000);

    rfm95w_set_tx_power(PA_BOOST, 0x0, 0x0);
    rfm95w_agc_auto_on(AGC_ON);

    rfm95w_set_lora_bandwidth(BW_125);
    rfm95w_set_spreading_factor(9);
    //rfm95w_LDR_optimize(LDR_ENABLE);

    rfm95w_set_preamble_length(0x0010);
    rfm95w_set_crc(CRC_DISABLE);
    rfm95w_set_coding_rate(CR_4_5);
    rfm95w_set_header_mode(EXPLICIT_HEADER);
    //rfm95w_set_payload_length(0x08);
    rfm95w_set_max_payload_length(0x20);
    rfm95w_set_sync_word(0x34);

    for(;;) {
        putchars("Register Dump:\n\r");
        rfm95w_register_dump();
        hardware_delay(60000);
        putchars("\n\r");
    }
    */

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
