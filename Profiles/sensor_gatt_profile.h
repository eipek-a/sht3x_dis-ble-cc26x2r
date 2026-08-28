#ifndef SENSOR_GATT_PROFILE_H
#define SENSOR_GATT_PROFILE_H
#include "bcomdef.h"
#ifdef __cplusplus
extern "C"
{
#endif
#define SENSORPROFILE_SERVICE                0X00000001

#define TEMP        5
#define HUMID       6
#define SERV2  0xFFF7
#define TEMP_UUID  0xFFF6
#define HUMID_UUID  0xFFF8
#define TEMP_LEN    2
#define HUMID_LEN  2
bStatus_t SensorProfile_AddService( uint32 services );
bStatus_t SensorProfile_SetParameter( uint8 param, uint8 len, void *value );
bStatus_t SensorProfile_GetParameter( uint8 param, void *value );
#ifdef __cplusplus
}
#endif

#endif /* SENSOR_GATT_PROFILE_H */
