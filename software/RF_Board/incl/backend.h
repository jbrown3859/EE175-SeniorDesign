#ifndef BACKEND_H_
#define BACKEND_H_

enum State{INIT, WAIT, GET_RADIO_INFO,
            GET_RX_BUF_STATE, READ_RX_BUF, BURST_READ_RX, FLUSH_RX,
            GET_TX_BUF_STATE, WRITE_TX_BUF, BURST_WRITE_TX, FLUSH_TX,
            WRITE_RX_BUF, READ_TX_BUF, CLEAR_RX_FLAGS, CLEAR_TX_FLAGS,
            PROG_RADIO_REG, READ_RADIO_REG,
            MODE_IDLE, MODE_RX, MODE_TX};

enum RadioState{IDLE, RX_ACTIVE, RX_DONE, TX_WAIT, TX_ACTIVE};

struct RadioInfo {
    unsigned long frequency;
    enum RadioState radio_mode;
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
unsigned char get_UART_bytes(char* bytes, unsigned char size, unsigned int timeout);
void flush_UART_FIFO(void);

unsigned int get_buffer_data_size(struct packet_buffer* buffer);
unsigned int get_buffer_packet_count(struct packet_buffer* buffer);
unsigned int get_next_buffer_packet_size(struct packet_buffer* buffer);
void write_packet_buffer(struct packet_buffer* buffer, char* data, const unsigned char len);
unsigned int read_packet_buffer(struct packet_buffer* buffer, char* dest);
void burst_read_packet_buffer(struct packet_buffer* buffer, unsigned char packet_size, unsigned char packet_num);

void main_loop(void);

#endif
