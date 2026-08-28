#ifndef sensor_h
#define sensor_h


#include <stdbool.h>
#define sensor_addr 0x44
bool sensor_read(float *temp, float *humid);




#endif
