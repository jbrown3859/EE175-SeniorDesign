#include <msp430.h> 
#include <util.h>
#include <serial.h>

#include <cc2500.h>
#include <rfm95w.h>

/* compare arrays and find different elements (for debugging mass register programming) */
void compare_arrays(char* a, char* b, unsigned char len) {
    unsigned int i;
    for (i=0;i<len;i++) {
        if (a[i] != b[i]) {
            putchars("Index:");
            print_hex(i);
            putchars(" a:");
            print_hex(a[i]);
            putchars(" b:");
            print_hex(b[i]);
            putchars("\n\r");
        }
    }
}

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    /* GPIO */
    //cc2500_init_frontend();

    init_clock();
    init_serial_timer(1000); //set I/O timeout
    init_UART(115200);
    init_SPI_master();

    //ADC test
    //set P1.5 to ADC input
    /*
    P1SEL0 |= (1 << 5);
    P1SEL1 |= (1 << 5);

    init_ADC(5); //init for channel #5
    */


    //radio programming
    cc2500_set_frontend(RX_SHUTDOWN);
    cc2500_command_strobe(STROBE_SRES); //reset chip
    cc2500_register_dump();
    hardware_delay(100);
    //cc2500_set_base_frequency(2450000000);
    //cc2500_set_IF_frequency(457000);
    cc2500_set_vco_autocal(AUTOCAL_FROM_IDLE);
    cc2500_set_packet_length(40);
    cc2500_set_data_whitening(WHITE_OFF);
    cc2500_set_fifo_thresholds(0x0A);
    //cc2500_set_sync_word(0xBAAD);
    cc2500_set_data_rate(MAN_300,EXP_300);
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
        cc2500_set_base_frequency(2450000000);
        print_hex(cc2500_read(0x0D));
        print_hex(cc2500_read(0x0E));
        print_hex(cc2500_read(0x0F));
        putchars("  ");
        print_dec(cc2500_get_frequency(), 10);
        putchars("  ");
        print_hex(cc2500_read(0x0A));
        putchars("\n\r");
    }


    /*
    char msg[5] = {'T','E','S','T'};

    unsigned char txpower = 0xE0;
    unsigned int i;
    //power test
    for(;;) {
        txpower = 0xFF;
        cc2500_set_tx_power(txpower);
        putchars("Transmit Power Register: ");
        print_hex(txpower);
        putchars("\n\r");

        cc2500_set_frontend(TX);
        cc2500_write(0x3F, 22); //write size
        cc2500_burst_write_fifo("\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0", 22);
        while (cc2500_get_status() != STATUS_STATE_TX && cc2500_get_status() != STATUS_STATE_FSTXON) {
            cc2500_command_strobe(STROBE_STX);
        }
        for(i=0;i<20;i++) {
            print_dec(ADC_to_millivolts(get_ADC_result()), 4); //print ADC value
            putchars("\n\r");
            hardware_delay(100);
        }
        hardware_delay(60000); //wait until done
        cc2500_command_strobe(STROBE_SFTX);
        cc2500_set_frontend(RX_SHUTDOWN);
        putchars("\n\r");

        __no_operation();
        txpower += 0x03;
    }

    //frontend test
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
    */

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

    char regs[48];
    char after[48];

    putchars("Programmed Registers: \n\r");
    rfm95w_register_dump();
    rfm95w_save_registers(regs);
    rfm95w_reset();
    putchars("After Reset: \n\r");
    rfm95w_register_dump();
    rfm95w_load_registers(regs);
    hardware_delay(100);
    putchars("After Load: \n\r");
    rfm95w_register_dump();
    rfm95w_save_registers(after);
    compare_arrays(regs, after, 0x27);

    for (;;) {}
    */

    /*
    for (;;) {
        putchars("LoRa Bandwidth: ");
        print_dec(rfm95w_get_lora_bandwidth(), 6);
        putchars("\n\r");

        putchars("Spreading Factor: ");
        print_dec(rfm95w_get_spreading_factor(), 2);
        putchars("\n\r");

        putchars("Coding Rate: ");
        print_dec(rfm95w_get_coding_rate(), 2);
        putchars("\n\r");
    }
    */

	return 0;
}
