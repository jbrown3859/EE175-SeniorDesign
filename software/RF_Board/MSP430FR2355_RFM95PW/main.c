#include <msp430.h>

#include <serial.h>
#include <rfm95w.h>
#include <util.h>

char rxbuf[128];
char rx_ptr = 0;
char rx_len = 0;

char DIO0_mode;
char TX_done;
char RX_done;

char TX_timeout = 0;

void init_GPIO(void) {
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode

    /* LED */
    P1SEL0 &= ~(0b1); //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero


    /* temp, delete me later */
    //P1DIR = 0xFF;
    //P1REN = 0xFF;
    //P1OUT = 0x00;
    P1DIR &= ~(0b1 << 5); //set P1.5 to input
    P1SEL0 &= ~(0b1 << 5); //set P1.5 to GPIO
}

/* ISRs must be defined for serial controllers or a trap will be thrown */
/* UART ISR vector */
#pragma vector=USCI_A0_VECTOR
__interrupt void USCI_A0_ISR(void) {
    switch(__even_in_range(UCA0IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG:
                //while(!(UCA0IFG&UCTXIFG));
                //rxchar = UCA0RXBUF;
                /*
                rxbuf[rx_ptr] = UCA0RXBUF;
                if (rx_ptr < 31){
                    rx_ptr++;
                }
                __no_operation();
                */
              break;
        case USCI_UART_UCTXIFG:
            break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}

/* SPI vector */
#pragma vector=USCI_B1_VECTOR
__interrupt void USCI_B1_ISR(void) {
    switch(__even_in_range(UCB1IV,USCI_UART_UCTXCPTIFG))
    {
        case USCI_NONE: break;
        case USCI_UART_UCRXIFG: //this is always set on every transmit
             break;
        case USCI_UART_UCTXIFG:
            break;
        case USCI_UART_UCSTTIFG: break;
        case USCI_UART_UCTXCPTIFG: break;
    }
}


/**
 * main.c
 */
int main(void) {
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    init_clock();
    init_GPIO();
    init_UART(115200);
    init_SPI_master();
    rfm95w_init();
    __bis_SR_register(GIE); //enable interrupts

    putchars("\n\rProgramming LoRa Registers\n\r");
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
    rfm95w_set_payload_length(0x08);
    rfm95w_set_max_payload_length(0xFF);
    rfm95w_set_sync_word(0x34);

    rfm95w_register_dump();

    __no_operation(); //debug
    RX_done = 0;
    TX_done = 0;

    char mode = 2;

    if (((P1IN >> 5) & 0xb1) == 0) { //TX mode
        rfm95w_set_DIO_mode(DIO0_TXDONE); //set DIO
        rfm95w_set_mode(MODE_STDBY);
        mode = 0;
    }
    if (((P1IN >> 5) & 0xb1) == 1) { //RX mode
        rfm95w_set_DIO_mode(DIO0_RXDONE);
        rfm95w_set_mode(MODE_RXCONTINUOUS);
        mode = 1;
    }

    char msg[8] = {'H','e','l','l','o','!','!','\n'};
    int i;
    char pkt = 0;
    for(;;) {
        if (mode == 0) {
            for (i=0;i<10;i++) {
                //msg[6] = (i + 48); //make digit
                rfm95w_transmit_fixed_packet(msg); //len = 8

                set_TX_timer(1); //enable timeout
                while(TX_done == 0 && TX_timeout == 0);
                set_TX_timer(0); //disable timeout

                TX_timeout = 0; //reset flag
                TX_done = 0; //reset

                putchars("Transmission Complete\n\r");
                hardware_delay(30000);
            }
        }
        else if (mode == 1) {
            while(RX_done == 0) {
                //rfm95w_display_register(0x12);
                //rfm95w_display_register(0x13);
                //rfm95w_display_register(0x12);
                //rfm95w_display_register(0x18);
                //rfm95w_display_register(0x1B); //rssi
            }
            putchars("Got Packet#: ");
            print_hex(pkt);
            putchars("\n\r");
            rx_len = rfm95w_read_fifo(rxbuf); //get data
            rxbuf[rx_len] = '\0'; //null-terminate
            putchars(rxbuf); //print msg
            putchars("\n\rHamming Distance: ");
            print_hex(hamming_distance(rxbuf, msg, 8));
            putchars(" RSSI: ");
            print_hex(rfm95w_get_packet_rssi());
            putchars("\n\r");
            clear_bytes(rxbuf, 8);
            pkt++;
            RX_done = 0;
        }
    }

    return 0;
}
