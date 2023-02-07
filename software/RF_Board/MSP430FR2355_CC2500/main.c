#include <msp430.h>

#include <util.h>
#include <serial.h>
#include <backend.h>

//#define RADIOTYPE_SBAND 1
#define RADIOTYPE_UHF 1

#ifdef RADIOTYPE_SBAND
#include <cc2500.h>
#endif
#ifdef RADIOTYPE_UHF
#include <rfm95w.h>
#endif

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    init_serial_timer(1000); //set I/O timeout
    init_UART(115200);
    init_SPI_master();

    #ifdef RADIOTYPE_SBAND
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

    cc2500_init_gpio(INT_GDO2); //init after programming to avoid false interrupts
    #endif

    #ifdef RADIOTYPE_UHF
    rfm95w_init();
    rfm95w_reset();

    rfm95w_set_lora_mode(MODE_LORA);
    rfm95w_set_frequency_mode(0x00);
    rfm95w_set_carrier_frequency(433500000);

    rfm95w_set_tx_power(PA_BOOST, 0x0, 0x0);
    rfm95w_agc_auto_on(AGC_ON);

    rfm95w_set_lora_bandwidth(BW_41_7);
    rfm95w_set_spreading_factor(12);
    rfm95w_LDR_optimize(LDR_ENABLE);

    rfm95w_set_preamble_length(0x0010);
    rfm95w_set_crc(CRC_DISABLE);
    rfm95w_set_coding_rate(CR_4_5);
    rfm95w_set_header_mode(EXPLICIT_HEADER);
    //rfm95w_set_payload_length(0x08);
    rfm95w_set_max_payload_length(0x20);
    rfm95w_set_sync_word(0x34);
    #endif

    main_loop();

	return 0;
}
