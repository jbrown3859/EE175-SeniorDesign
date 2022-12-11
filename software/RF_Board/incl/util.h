#ifndef UTIL_H_
#define UTIL_H_

void hardware_delay(unsigned int d);
void hardware_timeout(unsigned int d);
unsigned int hamming_distance(char* s1, char* s2, unsigned char l);
void clear_bytes(char* s, unsigned char l);

unsigned long long pow(unsigned long long base, unsigned long long exp);

#endif /* UTIL_H_ */
