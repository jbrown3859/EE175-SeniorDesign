#ifndef CC2500_H_
#define CC2500_H_

#define XTAL_FREQ 26000000 //frequency of off-chip crystal

/* command strobes */
#define STROBE_SRES 0x30 //reset
#define STROBE_SFSTXON 0x31
#define STROBE_SXOFF 0x32
#define STROBE_SCAL 0x33
#define STROBE_SRX 0x34
#define STROBE_STX 0x35
#define STROBE_SIDLE 0x36
#define STROBE_SWOR 0x38
#define STROBE_SPWD 0x39
#define STROBE_SFRX 0x3A
#define STROBE_SFTX 0x3B
#define STROBE_SWORRST 0x3C
#define STROBE_SNOP 0x3D

/* autocal settings */
#define AUTOCAL_OFF 0x00
#define AUTOCAL_FROM_IDLE 0x10
#define AUTOCAL_TO_IDLE 0x20
#define AUTOCAL_FOURTH_IDLE 0x30

/* gdo settings */
#define GDO2 0x00
#define GDO0 0x02

#define RX_END_OF_PACKET 0x01
#define RX_FIFO_OVERFLOW 0x04
#define TX_FIFO_UNDERFLOW 0x05
#define TX_RX_ACTIVE 0x06
#define GDO_HI_Z 0x2E

/* MARCSTATEs */
#define STATE_SLEEP 0x00
#define STATE_IDLE 0x01
#define STATE_XOFF 0x02
#define STATE_VCOON_MC 0x03
#define STATE_REGON_MC 0x04
#define STATE_MANCAL 0x05
#define STATE_VCOON 0x06
#define STATE_REGON 0x07
#define STATE_STARTCAL 0x08
#define STATE_BWBOOST 0x09
#define STATE_FS_LOCK 0x0A
#define STATE_IFADCON 0x0B
#define STATE_ENDCAL 0x0C
#define STATE_RX 0x0D
#define STATE_RX_END 0x0E
#define STATE_RX_RST 0x0F
#define STATE_TXRX_SWITCH 0x10
#define STATE_RXFIFO_OVERFLOW 0x11
#define STATE_FSTXON 0x12
#define STATE_TX 0x13
#define STATE_TX_END 0x14
#define STATE_RXTX_SWITCH 0x15
#define STATE_TXFIFO_UNDERFLOW 0x16

/* Data whitening */
#define WHITE_OFF 0x00
#define WHITE_ON 0x40

/* control functions */
void cc2500_init_gpio(void);
char cc2500_read(const unsigned char addr);
void cc2500_write(const unsigned char addr, const char data);
void cc2500_display_register(const char addr);
void cc2500_register_dump(void);
void cc2500_set_base_frequency(const unsigned long long freq);
void cc2500_set_IF_frequency(const unsigned long long freq);
void cc2500_set_channel(const unsigned char channel);
void cc2500_set_data_rate(const unsigned char mantissa, const unsigned char exponent);
void cc2500_command_strobe(const unsigned char strobe);
void cc2500_set_vco_autocal(const unsigned char autocal);
void cc2500_set_fifo_thresholds(const unsigned char threshold);
void cc2500_set_packet_length(const unsigned char pktlen);
void cc2500_set_data_whitening(const unsigned char white);
void cc2500_set_sync_word(const unsigned short sync);
void cc2500_configure_gdo(const unsigned char pin, const unsigned char config);

void cc2500_transmit(const char* data, const char size);
unsigned char cc2500_receive(char* buffer);

#endif
