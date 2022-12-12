#ifndef BACKEND_H_
#define BACKEND_H_

enum State{INIT, WAIT, SEND_INFO, SEND_RX_BUF_STATE};

struct RadioInfo {
    unsigned long frequency;
};

int read_UART_FIFO(void);
unsigned char get_UART_FIFO_size(void);

void write_RX_FIFO(char* data, unsigned char len);
void read_RX_FIFO(void);
unsigned char get_RX_FIFO_packet_num(void);
unsigned int get_RX_FIFO_size(void);

void main_loop(void);

#endif
