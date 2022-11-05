#include <msp430.h>
#include <serial.h>
#include <rfm95w.h>

void rfm95w_register_dump(void) {
    char c;
    unsigned char i;
    for (i = 0; i < 128; i++) {
        putchars("Address: ");
        print_hex(i);
        c = SPI_RX(i);
        putchars(" Data: ");
        print_hex(c);
        putchars("\n\r");
    }
}
