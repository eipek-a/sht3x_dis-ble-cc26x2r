#ifndef i2c_bus_h
#define i2c_bus_h
#include <ti/drivers/I2C.h>
#include <stdbool.h>
bool i2c_bus_init(void);
I2C_Handle i2c_bus_get(void);

#endif
