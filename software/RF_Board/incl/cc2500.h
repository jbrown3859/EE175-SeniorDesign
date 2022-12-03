#ifndef CC2500_H_
#define CC2500_H_

#define XTAL_FREQ 26000000 //frequency of off-chip crystal

/* command strobes */
#define STROBE_SRES 0x30
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

char cc2500_read(const unsigned char addr);
void cc2500_write(const unsigned char addr, const char data);
void cc2500_display_register(const char addr);
void cc2500_register_dump(void);
void cc2500_set_base_frequency(const unsigned long long freq);
void cc2500_set_IF_frequency(const unsigned long long freq);
void cc2500_set_channel(const unsigned char channel);
void cc2500_command_strobe(const unsigned char strobe);

#endif
