#include <msp430.h>

#include <serial.h>
#include <rfm95w.h>

char rxbuf[128];
char rx_ptr = 0;
char rx_len = 0;

char DIO0_mode;
char TX_done;
char RX_done;

void init_GPIO(void) {
    PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode

    /* LED */
    P1SEL0 |= 0b0; //set P1 to I/O
    P1DIR |= 0b1; //set P1.0 to output
    P1OUT &= ~(0b1); //set P1.0 to zero

    P2SEL0 = 0x00;
    P2REN = 0x00;
    P2DIR |= (1 << 2);

    /* temp, delete me later */
    P1DIR = 0xFF;
    P1REN = 0xFF;
    P1OUT = 0x00;
}

void init_Timer_B0() {
    TB0CCTL0 = CCIE; //interrupt mode
    TB0CCR0 = 8000; //trigger value
    TB0CTL = TBSSEL__ACLK | MC__UP | ID_0 | TBCLR; //slow clock, count up, no division, clear at start
}

#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_VECTOR_ISR (void) {
    P1OUT ^= 0x01; //toggle led
    TB0CCTL0 &= ~CCIFG; //reset interrupt
    //SPI_TX('a');
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
    init_Timer_B0();
    rfm95w_init();
    __bis_SR_register(GIE); //enable interrupts

    putchars("\n\rProgramming LoRa Registers\n\r");
    rfm95w_reset();
    rfm95w_set_lora_mode(MODE_LORA);
    rfm95w_set_frequency_mode(0x00);
    rfm95w_set_carrier_frequency(433500000);

    rfm95w_set_tx_power(PA_BOOST, 0x0, 0xF);

    rfm95w_set_lora_bandwidth(BW_32_25);
    rfm95w_set_spreading_factor(12);

    rfm95w_agc_auto_on(AGC_ON);
    //rfm95w_set_lna_gain(LNA_G6,LNA_BOOST_LF_OFF,LNA_BOOST_HF_OFF);
    rfm95w_LDR_optimize(LDR_ENABLE);

    rfm95w_set_preamble_length(0x0010);
    rfm95w_set_crc(CRC_DISABLE);
    rfm95w_set_coding_rate(CR_4_5);
    rfm95w_set_header_mode(EXPLICIT_HEADER);
    rfm95w_set_payload_length(0x08);
    rfm95w_set_max_payload_length(0xFF);
    rfm95w_set_sync_word(0x34);

    /*
    rfm95w_set_DIO_mode(DIO0_TXDONE); //set DIO
    //rfm95w_set_IQ(IQ_TX);
    rfm95w_set_mode(MODE_STDBY);
    */


    rfm95w_set_DIO_mode(DIO0_RXDONE);
    //rfm95w_set_IQ(IQ_RX);
    rfm95w_set_mode(MODE_RXCONTINUOUS);


    rfm95w_register_dump();

    __no_operation(); //debug
    RX_done = 0;
    TX_done = 0;
    for(;;) {
        /*
        rfm95w_transmit_fixed_packet("hello!!\n"); //len = 8
        while(TX_done == 0);
        putchars("Transmission Complete\n\r");
        TX_done = 0; //reset

        unsigned long long i;
        for(i=0;i<600000;i++);
        */


        //await packet
        while(RX_done == 0) {
            //rfm95w_display_register(0x12);
            //rfm95w_display_register(0x13);
            //rfm95w_display_register(0x12);
            //rfm95w_display_register(0x18);
            //rfm95w_display_register(0x1B); //rssi
        }
        rx_len = rfm95w_read_fifo(rxbuf); //get data
        rxbuf[rx_len] = '\0'; //null-terminate
        putchars(rxbuf); //print msg
        putchars("\n\r");
        RX_done = 0;

    }

    return 0;
}
