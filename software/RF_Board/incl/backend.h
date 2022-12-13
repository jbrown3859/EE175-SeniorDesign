#ifndef BACKEND_H_
#define BACKEND_H_

enum State{INIT, WAIT, SEND_INFO, SEND_RX_BUF_STATE, SEND_RX_PACKET, BURST_SEND_RX};

struct RadioInfo {
    unsigned long frequency;
};

unsigned int get_queue_distance(unsigned int bottom, unsigned int top, unsigned int max);

int read_UART_FIFO(void);
unsigned char get_UART_FIFO_size(void);

void write_RX_FIFO(char* data, unsigned char len);
void read_RX_FIFO(void);
void burst_read_RX_FIFO(unsigned char packet_size, unsigned char packet_num);
unsigned char get_RX_FIFO_packet_count(void);
unsigned int get_RX_FIFO_size(void);
unsigned int get_next_RX_packet_size(void);

void main_loop(void);

#endif
