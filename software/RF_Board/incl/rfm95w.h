#ifndef RFM95W_H_
#define RFM95W_H_

#define F_XOSC 32000000

#define MODE_FSK 0x00
#define MODE_LORA 0x80

#define OP_MODE_SLEEP 0b000
#define OP_MODE_STDBY 0b001
#define OP_MODE_FSTX 0b010
#define OP_MODE_TX 0b011
#define OP_MODE_FSRX 0b100
#define OP_MODE_RXCONTINUOUS 0b101
#define OP_MODE_RXSINGLE 0b110
#define OP_MODE_CAD 0b111

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
#define DIO0_NONE 0xC0

#define FLAG_RXTIMEOUT 0x80
#define FLAG_RXDONE 0x40
#define FLAG_CRCERROR 0x20
#define FLAG_VALIDHEADER 0x10
#define FLAG_TXDONE 0x08
#define FLAG_CADDONE 0x04
#define FLAG_FHSSCHANGECHANNEL 0x02
#define FLAG_CADDETECTED 0x01

#define EXPLICIT_HEADER 0x00
#define IMPLICIT_HEADER 0x01

#define CRC_DISABLE 0x00
#define CRC_ENABLE 0x04

#define CR_4_5 0b0010
#define CR_4_6 0b0100
#define CR_4_7 0b0110
#define CR_4_8 0b1000

#define IQ_STD 0x00
#define IQ_INV 0x01

#define MODE_LF 0x08
#define MODE_HF 0x00

/* globals */
extern char DIO0_mode;
//extern char TX_done;
//extern char RX_done;

/* TX interrupt */
void set_TX_timer(char mode);

/* init */
void rfm95w_init(void);
void rfm95w_reset(void);

/* SPI I/O */
char rfm95w_read(const char addr);
void rfm95w_write(const char addr, char c);

/* debug */
void rfm95w_display_register(const char addr);
void rfm95w_register_dump(void);

/* device programming */
char rfm95w_set_mode(const char mode);
char rfm95w_get_mode(void);
void rfm95w_set_lora_mode(const char lora_mode);
void rfm95w_set_frequency_mode(const char m);

void rfm95w_set_carrier_frequency(const unsigned long long frequency);
unsigned long long rfm95w_get_carrier_frequency(void);

void rfm95w_set_tx_power(const char boost, const char max, const char power);
void rfm95w_write_fifo(const char c);

void rfm95w_set_lora_bandwidth(const char b);
unsigned long rfm95w_get_lora_bandwidth(void);

void rfm95w_set_spreading_factor(const char sf);
unsigned char rfm95w_get_spreading_factor(void);

void rfm95w_set_coding_rate(const char cr);
unsigned char rfm95w_get_coding_rate(void);

void rfm95w_agc_auto_on(const char a);
void rfm95w_set_lna_gain(const char gain, const char boost_lf, const char boost_hf);
void rfm95w_LDR_optimize(const char ldr);
void rfm95w_clear_flag(const char f);
void rfm95w_clear_all_flags(void);
char rfm95w_read_flag(const char f);
void rfm95w_set_DIO_mode(const char m);
void rfm95w_set_payload_length(const char l);
void rfm95w_set_max_payload_length(const char l);
unsigned char rfm95w_get_payload_length(void);
void rfm95w_set_preamble_length(const int l);
void rfm95w_set_header_mode(const char m);
void rfm95w_set_crc(const char c);
void rfm95w_set_IQ(const char m);
void rfm95w_set_sync_word(const char s);
unsigned char rfm95w_get_packet_rssi(void);

unsigned char rfm95w_read_fifo(char* buffer);
void rfm95w_transmit_n_chars(const char* data, unsigned int len);
void rfm95w_transmit_fixed_packet(const char* data);

void rfm95w_save_registers(char* registers);
void rfm95w_load_registers(char* registers);

#endif /* RFM95W_H_ */
