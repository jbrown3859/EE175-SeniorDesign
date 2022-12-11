#include<util.h>

#include <msp430.h>

extern char timeout_flag; //must be reset by user

/* timeout vector */
#pragma vector=TIMER0_B0_VECTOR
__interrupt void TIMER0_B0_VECTOR_ISR (void) {
    timeout_flag = 1;
    TB0CCTL0 &= ~CCIFG; //reset interrupt
}

void hardware_timeout(unsigned int d) {
    if (d > 0) { //enable timer
            TB0CTL |= MC__UP | TBCLR;

            TB0CCTL0 = CCIE; //interrupt mode
            TB0CCR0 = d; //trigger value
            TB0CTL = TBSSEL__ACLK | MC__UP | ID_3 | TBCLR; //slow clock, count up, divide by 8, clear at start
    }
    else { //disable timer
        TB0CTL |= TBCLR;
        TB0CTL &= ~(0b11 << 4); //stop timer
    }
}

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

/* power function */
unsigned long long pow(unsigned long long base, unsigned long long exp) {
    unsigned long long result = 1;
    unsigned long long i;

    for(i=0;i<exp;i++) {
        result *= base;
    }

    return result;
}
