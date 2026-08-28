#include <string.h>
#include "sensor.h"
#include <stdint.h>
#include <stddef.h>

/* Driver Header files */
#include <ti/drivers/I2C.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>


/* Driver configuration */
#include "ti_drivers_config.h"
#include "i2c_bus.h"


bool sensor_read(float *temp, float *humid)
{
I2C_Handle i2c = i2c_bus_get();
if (i2c == NULL) return false;
    I2C_Transaction i2cTransaction = {0};
    uint8_t measureCommand[2] = {0x24, 0x00};   
    uint8_t readbuf[6] = {0};

    if (i2c == NULL)
    {
        return false;
    }

    i2cTransaction.targetAddress = sensor_addr;
    i2cTransaction.writeBuf      = measureCommand;
    i2cTransaction.writeCount    = 2;
    i2cTransaction.readBuf       = NULL;
    i2cTransaction.readCount     = 0;

    if (!I2C_transfer(i2c, &i2cTransaction))
    {
        return false;
    }

   Task_sleep(20000 / Clock_tickPeriod);

    memset(&i2cTransaction, 0, sizeof(i2cTransaction));
    i2cTransaction.targetAddress = sensor_addr;
    i2cTransaction.writeBuf      = NULL;
    i2cTransaction.writeCount    = 0;
    i2cTransaction.readBuf       = readbuf;
    i2cTransaction.readCount     = 6;

    if (!I2C_transfer(i2c, &i2cTransaction))
    {
        return false;
    }

    {
        uint16_t rawTemperature = ((uint16_t)readbuf[0] << 8) | readbuf[1];
        uint16_t rawHumidity    = ((uint16_t)readbuf[3] << 8) | readbuf[4];

        *temp  = -45.0f + 175.0f * ((float)rawTemperature / 65535.0f);
        *humid = 100.0f * ((float)rawHumidity / 65535.0f);
    }



    return true;
}
