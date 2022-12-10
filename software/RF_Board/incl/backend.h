#ifndef BACKEND_H_
#define BACKEND_H_

enum State{INIT, WAIT, SEND_INFO};

struct RadioInfo {
    unsigned long frequency;
};

int read_UART_FIFO(void);
unsigned char get_UART_FIFO_size(void);

void main_loop(void);

#endif
