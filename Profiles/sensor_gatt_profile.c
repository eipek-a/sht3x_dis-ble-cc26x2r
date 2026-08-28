#include <string.h>
#include <icall.h>
#include "util.h"
/* This Header file contains all BLE API and icall structure definition */
#include "icall_ble_api.h"


#include "sensor_gatt_profile.h"


#ifdef SYSCFG
#ifdef USE_GATT_BUILDER
#include "ti_ble_gatt_service.h"
#endif

#endif

#ifndef USE_GATT_BUILDER

//service
CONST uint8 SensorProfileServUUID2[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(SERV2), HI_UINT16(SERV2)
};
//characteristics
CONST uint8 SensorProfileTEMPUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(TEMP_UUID), HI_UINT16(TEMP_UUID)
};
CONST uint8 SensorProfileHUMIDUUID[ATT_BT_UUID_SIZE] =
{
  LO_UINT16(HUMID_UUID), HI_UINT16(HUMID_UUID)
};

static CONST gattAttrType_t Service2 = {ATT_BT_UUID_SIZE, SensorProfileServUUID2};

//temperature
static uint8 tempprops = GATT_PROP_READ | GATT_PROP_NOTIFY ;
static uint8 temp[TEMP_LEN] = {0,0};
static gattCharCfg_t *SensorProfileTempConfig;
static uint8 SensorProfileTEMPUserDesp[20] = "Temperature";


//humidity
static uint8 humidprops = GATT_PROP_READ | GATT_PROP_NOTIFY ;
static uint8 humid[HUMID_LEN] = {0,0};
static gattCharCfg_t *SensorProfileHumidConfig;
static uint8 SensorProfilehumidUserDesp[20] = "Humidity";


static gattAttribute_t SensorProfileAttrTbl[9] =
{

   {
    { ATT_BT_UUID_SIZE, primaryServiceUUID }, /* type */
    GATT_PERMIT_READ,                         /* permissions */
    0,                                        /* handle */
    (uint8 *)&Service2            /* pValue */
  },
    //temp declaration
      {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ,
      0,
      &tempprops
    },

    // temp value
     {
        { ATT_BT_UUID_SIZE, SensorProfileTEMPUUID },
        GATT_PERMIT_READ,
        0,
        temp
      },

    //temp config
    {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE ,
        0,
        (uint8 *)&SensorProfileTempConfig
      },
      //temp user description
        {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        SensorProfileTEMPUserDesp     
        },

      //humid declaration
      {
      { ATT_BT_UUID_SIZE, characterUUID },
      GATT_PERMIT_READ ,
      0,
      &humidprops
    },

    // humid value
     {
        { ATT_BT_UUID_SIZE, SensorProfileHUMIDUUID },
        GATT_PERMIT_READ,
        0,
        humid
      },

    //humid config
    {
        { ATT_BT_UUID_SIZE, clientCharCfgUUID },
        GATT_PERMIT_READ | GATT_PERMIT_WRITE,
        0,
        (uint8 *)&SensorProfileHumidConfig     
         },
      //humid user description
        {
        { ATT_BT_UUID_SIZE, charUserDescUUID },
        GATT_PERMIT_READ,
        0,
        SensorProfilehumidUserDesp     
        },
    

};
#endif

/*********************************************************************
 * LOCAL FUNCTIONS
 */
bStatus_t SensorProfile_ReadAttrCB(uint16_t connHandle,
                                          gattAttribute_t *pAttr,
                                          uint8_t *pValue, uint16_t *pLen,
                                          uint16_t offset, uint16_t maxLen,
                                          uint8_t method);
bStatus_t SensorProfile_WriteAttrCB(uint16_t connHandle,
                                           gattAttribute_t *pAttr,
                                           uint8_t *pValue, uint16_t len,
                                           uint16_t offset, uint8_t method);


#ifndef USE_GATT_BUILDER
CONST gattServiceCBs_t SensorProfileCBs =
{
  SensorProfile_ReadAttrCB,
  SensorProfile_WriteAttrCB,
  NULL
};



bStatus_t SensorProfile_AddService( uint32 services )
{
  uint8 status;

  // Allocate Client Characteristic Configuration table
  SensorProfileTempConfig = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *
                                                            MAX_NUM_BLE_CONNS );
  if ( SensorProfileTempConfig == NULL )
  {
    return ( bleMemAllocError );
  }

    SensorProfileHumidConfig = (gattCharCfg_t *)ICall_malloc( sizeof(gattCharCfg_t) *
                                                            MAX_NUM_BLE_CONNS );
  if ( SensorProfileHumidConfig == NULL )
  {
    ICall_free(SensorProfileTempConfig);
    SensorProfileTempConfig = NULL;
    return ( bleMemAllocError );


  }
  // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( LINKDB_CONNHANDLE_INVALID, SensorProfileTempConfig );
   // Initialize Client Characteristic Configuration attributes
  GATTServApp_InitCharCfg( LINKDB_CONNHANDLE_INVALID, SensorProfileHumidConfig );

  
  if ( services & SENSORPROFILE_SERVICE )
  {
    // Register GATT attribute list and CBs with GATT Server App
    status = GATTServApp_RegisterService( SensorProfileAttrTbl,
                                          GATT_NUM_ATTRS( SensorProfileAttrTbl ),
                                          GATT_MAX_ENCRYPT_KEY_SIZE,
                                          &SensorProfileCBs );
  }
  else
  {
    status = SUCCESS;
  }

  return ( status );
}
bStatus_t SensorProfile_SetParameter(uint8 param, uint8 len, void *value ){
          bStatus_t ret = SUCCESS;

  switch(param){
case TEMP:
  if (len == TEMP_LEN)
  {
    memcpy(temp, value, TEMP_LEN);
    GATTServApp_ProcessCharCfg(SensorProfileTempConfig, temp, FALSE,
                               SensorProfileAttrTbl, GATT_NUM_ATTRS(SensorProfileAttrTbl),
                               INVALID_TASK_ID, SensorProfile_ReadAttrCB);
  }
  else
  {

    ret = bleInvalidRange;
  }
  break;
case HUMID:
  if (len == HUMID_LEN)
  {
    memcpy(humid, value, HUMID_LEN);
    GATTServApp_ProcessCharCfg(SensorProfileHumidConfig, humid, FALSE,
                               SensorProfileAttrTbl, GATT_NUM_ATTRS(SensorProfileAttrTbl),
                               INVALID_TASK_ID, SensorProfile_ReadAttrCB);
  }
  else
  {
    ret = bleInvalidRange;
  }
  break;
  default: ret = INVALIDPARAMETER; break;
}
return (ret);
}

bStatus_t SensorProfile_GetParameter( uint8 param, void *value )
{
    bStatus_t ret = SUCCESS;
switch(param){

case TEMP:
      VOID memcpy( value, temp, TEMP_LEN );
      break;
case HUMID:
      VOID memcpy( value, humid, HUMID_LEN );
      break;
}
return (ret);}
#endif
bStatus_t SensorProfile_ReadAttrCB(uint16_t connHandle,
                                    gattAttribute_t *pAttr,
                                    uint8_t *pValue, uint16_t *pLen,
                                    uint16_t offset, uint16_t maxLen,
                                    uint8_t method)
{

  bStatus_t status = SUCCESS;
 if ( offset != 0 )
  {
    return ( ATT_ERR_ATTR_NOT_LONG );
  }
if ( pAttr->type.len == ATT_BT_UUID_SIZE )
{
  uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
  switch ( uuid ){
case TEMP_UUID:
  *pLen = TEMP_LEN;
  memcpy(pValue, pAttr->pValue, TEMP_LEN);
  break;

case HUMID_UUID:
  *pLen = HUMID_LEN;
  memcpy(pValue, pAttr->pValue, HUMID_LEN);
  break;
  }
}
return status;

}

 
bStatus_t SensorProfile_WriteAttrCB(uint16_t connHandle,
                                     gattAttribute_t *pAttr,
                                     uint8_t *pValue, uint16_t len,
                                     uint16_t offset, uint8_t method)
  {
  bStatus_t status = SUCCESS;

if ( pAttr->type.len == ATT_BT_UUID_SIZE )
  {
    // 16-bit UUID
    uint16 uuid = BUILD_UINT16( pAttr->type.uuid[0], pAttr->type.uuid[1]);
    switch ( uuid )
    {
case GATT_CLIENT_CHAR_CFG_UUID:
        status = GATTServApp_ProcessCCCWriteReq( connHandle, pAttr, pValue, len,
                                                 offset, GATT_CLIENT_CFG_NOTIFY );
        break;

      default:
      
        status = ATT_ERR_ATTR_NOT_FOUND;
        break;
    }
  }
  else
  {
    // 128-bit UUID
    status = ATT_ERR_INVALID_HANDLE;
  }



  return ( status );
}

    






  
