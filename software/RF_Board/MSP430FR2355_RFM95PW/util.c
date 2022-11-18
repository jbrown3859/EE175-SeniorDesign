#include<util.h>

#include <msp430.h>

/* delay ISR */
#pragma vector=TIMER2_B0_VECTOR
__interrupt void TIMER2_B0_VECTOR_ISR (void) {
    TB2CTL |= TBCLR;
    TB2CTL &= ~(0b11 << 4); //stop timer

    TB2CCTL0 &= ~CCIFG; //reset interrupt
    __bic_SR_register_on_exit(LPM3_bits); //exit sleep mode
}

/* slow delay clock, f = 32.768 kHz, p = 30.518us*/
void hardware_delay(unsigned int d) {
    TB2CCTL0 = CCIE; //interrupt mode
    TB2CCR0 = d; //trigger value
    TB2CTL = TBSSEL__ACLK | MC__UP | ID_0 | TBCLR; //slow clock, count up, no division, clear at start

    __bis_SR_register(LPM3_bits | GIE); //sleep until interrupt is triggered
}

/* get the bitwise hamming distance between two arrays of chars (for evaluating link quality) */
unsigned int hamming_distance(char* s1, char* s2, unsigned char l) {
    unsigned char i;
    unsigned char j;
    char diff;
    unsigned int d = 0;

    for (i=0;i<l;i++) {
        diff = (s1[i] ^ s2[i]);
        for (j=0;j<8;j++) {
            if (((diff >> j) & 0x01) == 1) {
                d++;
            }
        }
    }
    return d;
}

/* clear n bytes to zero in an array starting from index 0 */
void clear_bytes(char* s, unsigned char l) {
    unsigned char i;
    for (i=0;i<l;i++) {
        s[i] = 0x00;
    }
}
