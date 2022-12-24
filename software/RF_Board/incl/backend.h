#ifndef BACKEND_H_
#define BACKEND_H_

enum State{INIT, WAIT, SEND_INFO, SEND_RX_BUF_STATE, SEND_RX_PACKET, BURST_SEND_RX};

struct RadioInfo {
    unsigned long frequency;
};

/* data structure for RX and TX packet buffers */
struct packet_buffer {
    char* data;
    unsigned int max_data;
    unsigned int data_head;
    unsigned int data_base;

    unsigned int* pointers;
    unsigned int max_packets;
    unsigned int ptr_head;
    unsigned int ptr_base;

    char flags;
};

unsigned int get_buffer_distance(unsigned int bottom, unsigned int top, unsigned int max);

int read_UART_FIFO(void);
unsigned int get_UART_FIFO_size(void);

unsigned int get_buffer_data_size(struct packet_buffer* buffer);
unsigned int get_buffer_packet_count(struct packet_buffer* buffer);
unsigned int get_next_buffer_packet_size(struct packet_buffer* buffer);
void write_packet_buffer(struct packet_buffer* buffer, char* data, const unsigned char len);
unsigned int read_packet_buffer(struct packet_buffer* buffer, char* dest);
void burst_read_packet_buffer(struct packet_buffer* buffer, unsigned char packet_size, unsigned char packet_num);

void main_loop(void);

#endif
