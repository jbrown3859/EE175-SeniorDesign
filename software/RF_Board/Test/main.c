#include <msp430.h> 
#include <util.h>
#include <serial.h>

/**
 * main.c
 */
int main(void)
{
	WDTCTL = WDTPW | WDTHOLD;	// stop watchdog timer
	PM5CTL0 &= ~LOCKLPM5; //disable high-impedance GPIO mode
    __bis_SR_register(GIE); //enable interrupts

    init_clock();
    init_serial_timer(1000); //set I/O timeout
    init_UART(115200);
    init_SPI_master();


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

	return 0;
}
