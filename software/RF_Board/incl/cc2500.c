#include<cc2500.h>

#include <msp430.h>
#include <serial.h>

char cc2500_read(unsigned char addr) {
    return SPI_RX((addr & 0x3F) | 0x80);
}
