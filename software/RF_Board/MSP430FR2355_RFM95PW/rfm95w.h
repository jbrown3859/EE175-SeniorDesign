#ifndef RFM95W_H_
#define RFM95W_H_

#define F_XOSC 32000000

#define MODE_FSK 0x00
#define MODE_LORA 0x80

#define MODE_SLEEP 0b000
#define MODE_STDBY 0b001
#define MODE_FSTX 0b010
#define MODE_TX 0b011
#define MODE_FSRX 0b100
#define MODE_RXCONTINUOUS 0b101
#define MODE_RXSINGLE 0b110
#define MODE_CAD 0b111

#define PA_RFO 0x00
#define PA_BOOST 0x80

#define BW_7_8 0x00
#define BW_10_4 0x10
#define BW_15_6 0x20
#define BW_20_8 0x30
#define BW_32_25 0x40
#define BW_41_7 0x50
#define BW_62_5 0x60
#define BW_125 0x70
#define BW_250 0x80
#define BW_500 0x90

#define LNA_G1 0x20
#define LNA_G2 0x40
#define LNA_G3 0x60
#define LNA_G4 0x80
#define LNA_G5 0xA0
#define LNA_G6 0xC0
#define LNA_BOOST_LF_OFF 0x00
#define LNA_BOOST_HF_OFF 0x00
#define LNA_BOOST_HF_ON 0x03

#define AGC_OFF 0x00
#define AGC_ON 0x04

#define LDR_DISABLE 0x00
#define LDR_ENABLE 0x08

#define DIO0_RXDONE 0x00
#define DIO0_TXDONE 0x40
#define DIO0_CADDONE 0x80


/* init */
void rfm95w_init(void);

/* SPI I/O */
char rfm95w_read(const char addr);
void rfm95w_write(char addr, char c);

/* debug */
void rfm95w_display_register(const char addr);
void rfm95w_register_dump(void);

/* device programming */
void rfm95w_set_mode(const char mode);
char rfm95w_get_mode(void);
void rfm95w_set_lora_mode(const char lora_mode);
void rfm95w_set_carrier_frequency(const unsigned long long frequency);
void rfm95w_set_tx_power(const char boost, const char max, const char power);
void rfm95w_write_fifo(const char c);
void rfm95w_set_lora_bandwidth(char b);
void rfm95w_set_spreading_factor(char sf);
void rfm95w_agc_auto_on(const char a);
void rfm95w_set_lna_gain(const char gain, const char boost_lf, const char boost_hf);
void rfm95w_LDR_optimize(const char ldr);

void rfm95w_transmit_chars(const char* data);
char rfm95w_tx_done(void);
char rfm95w_rx_done(void);

void rfm95w_set_DIO_mode(const char m);

unsigned char rfm95w_read_fifo(char* buffer);

#endif /* RFM95W_H_ */
