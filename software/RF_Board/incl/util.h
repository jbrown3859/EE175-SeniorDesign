#ifndef UTIL_H_
#define UTIL_H_

extern char timeout_flag; //must be reset by user

void hardware_delay(unsigned int d);
void hardware_timeout(unsigned int d);
unsigned int hamming_distance(char* s1, char* s2, unsigned char l);
void clear_bytes(char* s, unsigned char l);

unsigned long long pow(unsigned long long base, unsigned long long exp);

void init_ADC(const char channel);
unsigned int get_ADC_result(void);
unsigned int ADC_to_millivolts(unsigned int adcval);
unsigned int get_ADC_average(const unsigned char n);

#endif /* UTIL_H_ */
