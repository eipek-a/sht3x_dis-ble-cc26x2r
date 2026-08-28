#include <string.h>
#include "i2c_bus.h"
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/I2C.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>

/* Driver configuration */
#include "ti_drivers_config.h"



/* global handle */
static I2C_Handle i2c = NULL;
bool i2c_bus_init(void)
{
    I2C_Params i2c_par;
I2C_init();
    I2C_Params_init(&i2c_par);

    i2c = I2C_open(CONFIG_I2C_0, &i2c_par);

    return (i2c != NULL);
}
I2C_Handle i2c_bus_get(void){
    return i2c;
}
