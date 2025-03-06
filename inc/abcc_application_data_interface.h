/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Typedefs provided by ABCC_driver, used to specify ADI mapping tables.
********************************************************************************
*/

#ifndef ABCC_AD_IF_H_
#define ABCC_AD_IF_H_

#include "abcc_config.h"
#include "abcc_types.h"

/*
** Type for defining the direction of the process data map.
** PD_END_MAP is used to indicate end of default map list.
*/
typedef enum
{
   PD_READ,
   PD_WRITE,
   PD_END_MAP
} PD_DirType;

/*-------------------------------------------------------------------------------
**   AD_MapType.
**   Note! The example at the end of this file shows how to use this type.
**
**   iInstance:      AD instance in map structure.
**                   If instance == AD_MAP_PAD_ADI (0), bNumElem indicates
**                   number of bits to pad with.
**
**   eDir:           Direction of map. Set to PD_END_MAP to indicate end of
**                   map list.
**   bNumElem:       Number of elements to map.
**                   Can only be > 1 for arrays or structures.
**                   AD_MAP_ALL_ELEM indicates that all elements shall
**                   be mapped. If instance == AD_MAP_PAD_ADI, bNumElem
**                   indicates number of bits to pad with.
**
**   bElemStartIndex: Element start index within an array or structure
**-------------------------------------------------------------------------------
*/
#define AD_MAP_END_ENTRY         0xFFFF, PD_END_MAP, 0, 0
#define AD_MAP_ALL_ELEM          0
#define AD_MAP_PAD_ADI           0

typedef struct AD_Map
{
  UINT16     iInstance;
  PD_DirType eDir;
  UINT8      bNumElem;
  UINT8      bElemStartIndex;
}
AD_MapType;

/*
** For backwards compatibility.
*/
typedef AD_MapType AD_DefaultMapType;
#define AD_DEFAULT_MAP_END_ENTRY AD_MAP_END_ENTRY
#define AD_DEFAULT_MAP_ALL_ELEM  AD_MAP_ALL_ELEM

/*------------------------------------------------------------------------------
** ADI type property defines (min/max/default/(enum strings)).
**
** AD_MinMaxDefaultIndexType - Index to access min, max or default in the
**                             xMinMaxDefault array.
**
** AD_UINT8Type      - ABP_UINT8    property type
** AD_CHARType       - ABP_CHAR     property type
** AD_BOOL8Type      - ABP_BOOL     property type
** AD_SINT8Type      - ABP_SINT8    property type
** AD_UINT16Type     - ABP_UINT16   property type
** AD_SINT16Type     - ABP_SINT16   property type
** AD_UINT32Type     - ABP_UINT32   property type
** AD_SINT32Type     - ABP_SINT32   property type
** AD_FLOAT32Type    - ABP_FLOAT    property type
** AD_FLOAT64Type    - ABP_DOUBLE   property type
** AD_ENUMType       - ABP_ENUM     property type
** AD_UINT64Type     - ABP_UINT64   property type
** AD_SINT64Type     - ABP_SINT64   property type
**
** AD_ENUMStrType    - Enumeration string type, used with AD_ENUMType
**                     to specify value/string mappings.
**------------------------------------------------------------------------------
*/
typedef enum AD_MinMaxDefaultIndex
{
   AD_MIN_VALUE_INDEX = 0,
   AD_MAX_VALUE_INDEX = 1,
   AD_DEFAULT_VALUE_INDEX = 2
}
AD_MinMaxDefaultIndexType;

typedef struct AD_UINT8
{
   UINT8 bMinMaxDefault[ 3 ];
}
AD_UINT8Type, AD_CHARType, AD_BOOL8Type, AD_OctetType;

typedef struct AD_SINT8
{
   INT8 bMinMaxDefault[ 3 ];
}
AD_SINT8Type;

typedef struct AD_UINT16
{
   UINT16 iMinMaxDefault[ 3 ];
}
AD_UINT16Type;

typedef struct AD_SINT16
{
   INT16 iMinMaxDefault[ 3 ];
}
AD_SINT16Type;

typedef struct AD_UINT32
{
   UINT32 lMinMaxDefault[ 3 ];
}
AD_UINT32Type;

typedef struct AD_SINT32
{
   INT32 lMinMaxDefault[ 3 ];
}
AD_SINT32Type;

typedef struct AD_FLOAT32
{
   FLOAT32 rMinMaxDefault[ 3 ];
}
AD_FLOAT32Type;

#if ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED
typedef struct AD_FLOAT64
{
   FLOAT64 dMinMaxDefault[ 3 ];
}
AD_FLOAT64Type;
#endif

typedef struct AD_ENUMStr
{
   UINT8  eValue;
   char*  acEnumStr;
}
AD_ENUMStrType;

typedef struct AD_ENUM
{
   UINT8 eMinMaxDefault[ 3 ];
   UINT8             bNumOfEnumStrings;
   AD_ENUMStrType*   pasEnumStrings;
}
AD_ENUMType;

#if ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED
typedef struct AD_UINT64
{
   UINT64 lMinMaxDefault[ 3 ];
}
AD_UINT64Type;

typedef struct AD_SINT64
{
   INT64 lMinMaxDefault[ 3 ];
}
AD_SINT64Type;
#endif


   typedef union
   {
      struct
      {
         void* pxValuePtr;
         void* pxValueProps;
      } sVOID;
      struct
      {
         UINT8*         pbValuePtr;
         AD_UINT8Type*  psValueProps;
      } sUINT8, sCHAR, sBOOL;
      struct
      {
         UINT8*       pbValuePtr;
         AD_ENUMType* psValueProps;
      }sENUM;
      struct
      {
         INT8*         pbValuePtr;
         AD_SINT8Type*  psValueProps;
      } sSINT8;
      struct
      {
         UINT16*        piValuePtr;
         AD_UINT16Type* psValueProps;
      } sUINT16;
      struct
      {
         INT16*        piValuePtr;
         AD_SINT16Type* psValueProps;
      } sSINT16;
      struct
      {
         UINT32*        plValuePtr;
         AD_UINT32Type* psValueProps;
      } sUINT32;
      struct
      {
         INT32*        plValuePtr;
         AD_SINT32Type* psValueProps;
      } sSINT32;
      struct
      {
         FLOAT32*        prValuePtr;
         AD_FLOAT32Type* psValueProps;
      } sFLOAT32;
#if ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED
      struct
      {
         FLOAT64*        pdValuePtr;
         AD_FLOAT64Type* psValueProps;
      } sFLOAT64;
#endif
#if ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED
      struct
      {
         UINT64*        plValuePtr;
         AD_UINT64Type* psValueProps;
      } sUINT64;
      struct
      {
         INT64*        plValuePtr;
         AD_SINT64Type* psValueProps;
      } sSINT64;
#endif
   } uDataType;

/*------------------------------------------------------------------------------
** AD_StructDataType (describes a member of a structured data type).
**
** The structured data type must contain packed data. All non bit-data types
** (including ABP_BITS8, ABP_BITS16 and ABP_BITS32) must be at least 8 bit
** aligned. ABP_PADx data types can be used for padding.
**
** Note! The example at the end of this file shows how to use this type.
**
**------------------------------------------------------------------------------
** 1. pacElementName          - Name of each element as a character string.
**                              NULL means not implemented, i.e. that no element
**                              name will be returned.
**
** 2. bDataType               - Element data type format, which can be one of the
**                              following:
**                                  ABP_BOOL:    Boolean.
**                                  ABP_SINT8:   8-bit signed integer.
**                                  ABP_SINT16:  16-bit signed integer.
**                                  ABP_SINT32:  32-bit signed integer.
**                                  ABP_UINT8:   8-bit unsigned integer.
**                                  ABP_UINT16:  16-bit unsigned integer.
**                                  ABP_UINT32:  32-bit unsigned integer.
**                                  ABP_CHAR:    Character.
**                                  ABP_ENUM:    Enumeration.
**                                  ABP_SINT64:  64-bit signed integer.
**                                  ABP_UINT64:  64-bit unsigned integer.
**                                  ABP_FLOAT:   floating point value (32-bits).
**                                  ABP_DOUBLE   floating point value (64-bits).
**                                  ABP_OCTET:   8 bit data
**                                  ABP_BITS8:   8 bit-data type
**                                  ABP_BITS16:  16 bit-data type
**                                  ABP_BITS32:  32 bit-data type
**                                  ABP_BIT1     1 bit-data type
**                                  ABP_BIT2     2 bit-data type
**                                     :
**                                  ABP_BIT7     7 bit.data type
**                                  ABP_PAD0     0 pad-bit data type
**                                  ABP_PAD1     1 pad-bit data type
**                                     :
**                                  ABP_BIT7     16 pad-bit data type
**                                  ABP_BOOL1    1 bit boolean
**
** 3. iNumSubElem             - Number of sub elements the specified data type (1).
**                              Only allowed to be larger than 1 for ABP_CHAR or ABP_OCTET.
**
** 4. bDesc                   - Entry descriptor; bits filled with the following
**                              configurations:
**                                  ABP_APPD_DESCR_GET_ACCESS: Get service is
**                                  allowed on value attribute.
**                                  ABP_APPD_DESCR_SET_ACCESS: Set service is
**                                  allowed on value attribute.
**                                  ABP_APPD_DESCR_MAPPABLE_WRITE_PD: Set if
**                                  possible to map.
**                                  ABP_APPD_DESCR_MAPPABLE_READ_PD:Set if
**                                  possible to map.
**
** 5. bBitOffset              - Offset to where the bit data type is located relative
**                              the position given by (5). This could be used used to
**                              pack bit data types in the ADI place holder.
**                              For 8 bit char platforms the offset must be within
**                              an octet ( 0 -7 ). For a 16 bit char system the offset
**                              must be within a 16 bit word.
**                              Note that the bit type is allowed cross octet boundaries.
**
** 6. pxValuePtr              - Pointer to the local value variable.
**
** 7. psValueProps            - Pointer to the local value properties struct.
**                              If NULL, no properties are applied (max/min/default).
** --------------------------------------------------------------------------------------------------------------------
** | 1. pacElementName | 2. bDataType | 3. iNumSubElem | 4. bDesc | 5. bBitOffset | 6. pxValuePtr | 7. pxValuePropPtr |
** --------------------------------------------------------------------------------------------------------------------
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_STRUCT_DATA_TYPE_ENABLED
typedef struct AD_StructDataType
{
   char*    pacElementName;
   UINT8    bDataType;
   UINT16   iNumSubElem;
   UINT8    bDesc;
   UINT8    bBitOffset;
   uDataType uData;
} AD_StructDataType;
#endif


#if ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED

/*------------------------------------------------------------------------------
** Forward declaration of ADI entry type.
**------------------------------------------------------------------------------
*/
struct AD_AdiEntry;

/*------------------------------------------------------------------------------
** ABCC_GetAdiValueFuncType
** This function will be called to indicate that the ABCC has requested to read
** of the ADI, either through explicit or implicit messaging. The function call
** could be used to trigger an update of the ADI to hold the latest value.
**------------------------------------------------------------------------------
** Arguments:
**    psAdiEntry               - Pointer to ADI entry
**    bNumElements             - Number of elements to update.
**    bStartIndex              - Start index of first element to update.
** Returns:
**    None
**------------------------------------------------------------------------------
*/
typedef void (*ABCC_GetAdiValueFuncType)( const struct AD_AdiEntry* psAdiEntry,
                                          UINT8 bNumElements,
                                          UINT8 bStartIndex );

/*------------------------------------------------------------------------------
** ABCC_SetAdiValueFuncType
** This function is called to indicate that the ABCC has updated the ADI either
** through explicit or implicit messaging. The function call
** could be used to trigger action based on the ADI update.
**------------------------------------------------------------------------------
** Arguments:
**    psAdiEntry               - Pointer to ADI entry
**    bNumElements             - Number of elements that has been updated.
**    bStartIndex              - Start index of first updated element.
** Returns:
**    None
**------------------------------------------------------------------------------
*/
typedef void (*ABCC_SetAdiValueFuncType)( const struct AD_AdiEntry* psAdiEntry,
                                          UINT8 bNumElements,
                                          UINT8 bStartIndex );
#endif

#if ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED
/*------------------------------------------------------------------------------
** Function pointer definition for functions used for transparent setting of
** data towards ADIs with Set_Attribute or Set_Attribute_Indexed commands.
**------------------------------------------------------------------------------
** Arguments:
**    psAdiEntry               - Pointer to ADI entry that received the set
**                               command.
**    bNumElements             - Number of elements that is updated with the
**                               set command.
**    bStartIndex              - Start index of first element in the set
**                               command.
**    iDataLength              - Length of the set data in pxData.
**    pxData                   - Pointer to the data of the set command.
**
** Returns:
**    Error code indicating the result of the operation.
**------------------------------------------------------------------------------
*/
typedef ABP_MsgErrorCodeType (*ABCC_AdiTransparentSetFuncType)( const struct AD_AdiEntry* psAdiEntry,
                                                                UINT8                     bNumElements,
                                                                UINT8                     bStartIndex,
                                                                UINT16                    iDataLength,
                                                                const void*               pxData );
#endif /* #if ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED */

/*------------------------------------------------------------------------------
** AD_AdiEntryType (describing an ADI).
**
** Structured data types are enabled by defining
** ABCC_CFG_STRUCT_DATA_TYPE_ENABLED to 1 in abcc_driver_config.h
**
** Get/Set callbacks are enabled by defining
** ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED to 1 in abcc_driver_config.h
**
** Transparent set callback is enabled by defining
** ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED to 1 in abcc_driver_config.h
**
** Note! The example at the end of this file shows how to use this type.
**------------------------------------------------------------------------------
** 1. iInstance               - ADI instance number (1-65535); 0 is reserved for
**                              the Class.
**                              NOTE: The entries in the ADI list cannot be
**                              placed in arbitrary order, they must be sorted
**                              in ascending order for all lookup functions in
**                              the driver and the Application Data Object to
**                              work as intended!
**
** 2. pabName                 - Name of each ADI as a character string
**                              (ADI instance attribute #1).
**                              If NULL, a 0 length name will be returned.
**
** 3. bDataType               - ADI data type format, which can be one of the
**                              following (Ignored for structured data types.
**                              Structures have a separate type indicator for
**                              each element, see struct type definition above):
**                                  ABP_BOOL:    Boolean.
**                                  ABP_SINT8:   8-bit signed integer.
**                                  ABP_SINT16:  16-bit signed integer.
**                                  ABP_SINT32:  32-bit signed integer.
**                                  ABP_UINT8:   8-bit unsigned integer.
**                                  ABP_UINT16:  16-bit unsigned integer.
**                                  ABP_UINT32:  32-bit unsigned integer.
**                                  ABP_CHAR:    Character.
**                                  ABP_ENUM:    Enumeration.
**                                  ABP_SINT64:  64-bit signed integer.
**                                  ABP_UINT64:  64-bit unsigned integer.
**                                  ABP_FLOAT:   floating point value (32-bits).
**                                  ABP_DOUBLE:  floating point value (64-bits).
**                                  ABP_OCTET:   8 bit data
**                                  ABP_BITS8:   8 bit-data type
**                                  ABP_BITS16:  16 bit-data type
**                                  ABP_BITS32:  32 bit-data type
**                                  ABP_BIT1     1 bit-data type
**                                  ABP_BIT2     2 bit-data type
**                                     :
**                                  ABP_BIT7     7 bit.data type
**                                  ABP_PAD0     0 pad-bit data type
**                                  ABP_PAD1     1 pad-bit data type
**                                     :
**                                  ABP_BIT7     16 pad-bit data type
**                                  ABP_BOOL1    1 bit boolean
**
** 4. bNumOfElements          - For arrays ( psStruct (8) is NULL ):
**                                  Number of elements of the specified data type ( 3 )
**
**                            - For structured data type ( psStruct (8) != NULL ):
**                                  Number of elements in the structure
**
** 5. bDesc                   - Ignored for structured data types ( psStruct (8) != NULL )
**                              All other data types:
**                              Entry descriptor; bits filled with the following
**                              configurations:
**                                  ABP_APPD_DESCR_GET_ACCESS: Get service is
**                                  allowed on value attribute.
**                                  ABP_APPD_DESCR_SET_ACCESS: Set service is
**                                  allowed on value attribute.
**                                  ABP_APPD_DESCR_MAPPABLE_WRITE_PD: Set if
**                                  possible to map.
**                                  ABP_APPD_DESCR_MAPPABLE_READ_PD:Set if
**                                  possible to map.
**
** 6. pxValuePtr              - Ignored for structured data type ( psStruct (8) != NULL )
**                              All other data types:
**                              Pointer to the local value variable.
**
** 7. psValueProps            - Ignored for structured data type ( psStruct (8) != NULL )
**                              All other data types:
**                              Pointer to the local value properties struct.
**                              If NULL, no properties are applied (max/min/default).
**
** 8. psStruct(optional)      - Pointer to an AD_StructDataType.
**                              Set to NULL for non structured data type. This field is
**                              enabled by defining ABCC_CFG_STRUCT_DATA_TYPE_ENABLED.
**
** 9. pnGetAdiValue (optional)- Pointer to an ABCC_GetAdiValueFuncType
**                              Called when getting ADI value if defined
**
** 10. pnSetAdiValue (optional)- Pointer to an ABCC_SetAdiValueFuncType
**                               Called when setting ADI value if defined
** 11. pnSetAdiValueTransparent (optional) - Pointer to an
**                                           ABCC_AdiTransparentSetFuncType.
**                                           Called for setting the ADI Value
**                                           in a transparent way. Can be used
**                                           if the application e.g. needs to
**                                           validate the ADI data.
** ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
** | 1. iInstance | 2. pabName | 3. bDataType | 4. bNumOfElements | 5. bDesc | 6. pxValuePtr | 7. pxValuePropPtr | 8. psStruct | 9. pnGetAdiValue | 10. pnSetAdiValue | 11. pnSetAdiValueTransparent |
** ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
**------------------------------------------------------------------------------
*/
typedef struct AD_AdiEntry
{
   UINT16   iInstance;
   char*    pacName;
   UINT8    bDataType;
   UINT8    bNumOfElements;
   UINT8    bDesc;
   uDataType uData;
#if ABCC_CFG_STRUCT_DATA_TYPE_ENABLED
   const AD_StructDataType *psStruct;
#endif
#if ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED
   ABCC_GetAdiValueFuncType pnGetAdiValue;
   ABCC_SetAdiValueFuncType pnSetAdiValue;
#endif
#if ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED
   ABCC_AdiTransparentSetFuncType pnSetAdiValueTransparent;
#endif
} AD_AdiEntryType;

#if 0
/*******************************************************************************
** Example of ADI mapping without structured data type support i.e.
** ABCC_CFG_STRUCT_DATA_TYPE_ENABLED is defined to 0.
********************************************************************************
*/

/*
** Example with all access flags set.
*/
#define ADI_DESCR_ALL_ACCESS  ( ABP_APPD_DESCR_GET_ACCESS | ABP_APPD_DESCR_SET_ACCESS | ABP_APPD_DESCR_MAPPABLE_WRITE_PD | ABP_APPD_DESCR_MAPPABLE_READ_PD )

/*------------------------------------------------------------------------------
** ADI instance allocation without 'struct' or 'callback'.
**------------------------------------------------------------------------------
*/
const AD_AdiEntryType AD_asADIEntryList[] =
{
   /*
   ** --------------------------------------------------------------------------------------------------------------
   ** | 1. iInstance | 2. pabName | 3. bDataType | 4. bNumOfElements | 5. bDesc | 6.pxValuePtr | 7. pxValuePropPtr |
   ** --------------------------------------------------------------------------------------------------------------
   */
   {  1,    "RadOnOff_BOOL8",            ABP_BOOL,   1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_fRadOnOff,            &VAPP_BOOL8Props_fRadOnOff           } } },
   {  2,    "RadSetpointTemp_FLOAT",     ABP_FLOAT,  1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_rRadSetpointTemp,     &VAPP_FLOAT32Props_rRadSetpointTemp  } } },
   {  3,    "RadErrCode_ENUM",           ABP_ENUM,   1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_eRadErrCode,          &VAPP_ENUMProps_RadErrCode           } } },
   {  4,    "Fan_OnOff_BOOL8",           ABP_BOOL,   1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_fFanOnOff,            &VAPP_BOOL8Props_fFanOnOff           } } },
   {  5,    "FanOpErrCode_ENUM",         ABP_ENUM,   1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_eFanErrCode,          &VAPP_ENUMProps_FanErrCode           } } },
   {  6,    "ActTemp_FLOAT",             ABP_FLOAT,  1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_rActTemp,             &VAPP_FLOAT32Props_rRadSetpointTemp  } } },
   {  500,  "ActTempErrCode_ENUM",       ABP_ENUM,   1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_eActTempErrCode,      &VAPP_ENUMProps_ActTempErrCode       } } },
   {  501,  "InputTemp_FLOAT",           ABP_FLOAT,  1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_rInputTemp,           NULL                                 } } },
   {  502,  "HeatEffect_deg/sec_FLOAT",  ABP_FLOAT,  1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_rRadHeatPowerPerSec,  NULL                                 } } },
   {  503,  "SystemTripBits_UINT8",      ABP_UINT8,  1,  ADI_DESCR_ALL_ACCESS,  { {  &VAPP_bTripBits,            NULL                                 } } }
};


/*------------------------------------------------------------------------------
** ADI read/write process data mappings.
**------------------------------------------------------------------------------
** Array of mapping elements containing ADI in AD_asADIEntryList
** in native mapping order and direction of the map.
** Note! Mapping sequence is terminated by AD_DEFAULT_MAP_ENTRY  and
** MUST be present at end of LIST.
**------------------------------------------------------------------------------
*/
AD_MapType sDefaultMap[] =
{
   /*
   ** -------------------------------------------------------------
   ** | 1. iInstance | 2. eDir | 3. bNumElem | 4. bElemStartIndex |
   ** -------------------------------------------------------------
   */
   { 3,    PD_WRITE,  AD_MAP_ALL_ELEM,  0 },
   { 5,    PD_WRITE,  AD_MAP_ALL_ELEM,  0 },
   { 6,    PD_WRITE,  AD_MAP_ALL_ELEM,  0 },
   { 1,    PD_READ,   AD_MAP_ALL_ELEM,  0 },
   { 2,    PD_READ,   AD_MAP_ALL_ELEM,  0 },
   { 500,  PD_WRITE,  AD_MAP_ALL_ELEM,  0 },
   { 501,  PD_WRITE,  AD_MAP_ALL_ELEM,  0 },
   { 502,  PD_WRITE,  AD_MAP_ALL_ELEM,  0 },
   { 4,    PD_READ,   AD_MAP_ALL_ELEM,  0 },
   { 503,  PD_READ,   AD_MAP_ALL_ELEM,  0 },
   { AD_MAP_END_ENTRY                     }
};


/*******************************************************************************
** Example of ADI mapping structured data type
** (ABCC_CFG_STRUCT_DATA_TYPE_ENABLED defined to 1 in abcc_driver_config.h )
********************************************************************************
*/


/*------------------------------------------------------------------------------
** Variable place holders for the ADI:s
**------------------------------------------------------------------------------
*/
/*
** The content of struct1 is described by  ABCC_API_AdiStruct1 below
*/
struct
{
   UINT16   iData;
   UINT32   lData;
} struct1;

/*
** The content of struct2 is described by  ABCC_API_AdiStruct2 below
*/
struct
{
   UINT8    bBitData[ 2 ];
   UINT16   iData;
} struct2;

/*
** Basic 8 bit ADI
*/
UINT8    bData;


/*------------------------------------------------------------------------------
** ABCC_API_AdiStruct1 describes a structured ADI with two members. ( struct1 )
**------------------------------------------------------------------------------
*/
static const AD_StructDataType ABCC_API_AdiStruct1[] =
{
   /*
   ** --------------------------------------------------------------------------------------------------------------------
   ** | 1. pacElementName | 2. bDataType | 3. iNumSubElem | 4. bDesc | 5. bBitOffset | 6. pxValuePtr | 7. pxValuePropPtr |
   ** --------------------------------------------------------------------------------------------------------------------
   */
   /* Index: 0 */ {  "Element01",  ABP_UINT16,  1,  ADI_DESCR_ALL_ACCESS,  0,  { {  &struct1.iData,  NULL  } } },
   /* Index: 1 */ {  "Element02",  ABP_SINT32,  1,  ADI_DESCR_ALL_ACCESS,  0,  { {  &struct1.lData,  NULL  } } }
};


/*
**------------------------------------------------------------------------------
**   ABCC_API_AdiStruct2 describes a structured ADI with 4 members (struct2). The
**   structure includes bit data types and padding. The picture below shows the
**   packed ADI result. The numbers are referring to the index in
**   ABCC_API_AdiStruct2.
**
**
**   Bit:     7   6   5   4   3   2   1   0
**          ---------------------------------
**          | 1 | 1 | 1 | 0 | 0 | 0 | 0 | 0 |   struct2.bBitData[ 0 ]
**          ---------------------------------
**          ---------------------------------
**          | 2 | 2 | 2 | 2 | 2 | 2 | 2 | 1 |   struct2.bBitData[ 1 ]
**          ---------------------------------
**          ---------------------------------
**          | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |   struct2.iData
**          ---------------------------------
**          ---------------------------------
**          | 3 | 3 | 3 | 3 | 3 | 3 | 3 | 3 |   struct2.iData
**          ---------------------------------
**------------------------------------------------------------------------------
*/
static const AD_StructDataType ABCC_API_AdiStruct2[] =
{
   /*
   ** --------------------------------------------------------------------------------------------------------------------
   ** | 1. pacElementName | 2. bDataType | 3. iNumSubElem | 4. bDesc | 5. bBitOffset | 6. pxValuePtr | 7. pxValuePropPtr |
   ** --------------------------------------------------------------------------------------------------------------------
   */
   /* Index: 0 */ {  "Element01",  ABP_BIT5,    1,  ADI_DESCR_ALL_ACCESS,  0,  { {  &struct2.bBitData[ 0 ],  NULL  } } }, /* First 5 bits in struct2.bBitData[ 0 ]                                       */
   /* Index: 1 */ {  "Element02",  ABP_BIT4,    1,  ADI_DESCR_ALL_ACCESS,  5,  { {  &struct2.bBitData[ 0 ],  NULL  } } }, /* 4 bits starting at position 5 in struct2.bBitData[ 0 ]                      */
   /* Index: 2 */ {  "Element03",  ABP_PAD7,    1,  ADI_DESCR_ALL_ACCESS,  1,  { {  NULL,                    NULL  } } }, /* 7 bits padding is needed starting at position 1 in  struct2.bBitData[ 1 ]   */
   /* Index: 3 */ {  "Element04",  ABP_UINT16,  1,  ADI_DESCR_ALL_ACCESS,  0,  { {  &struct2.iData,          NULL  } } }  /* 16 bit type must at least be on a 8 bit aligned position                    */
};


/*------------------------------------------------------------------------------
** ADI instance allocation with structured ADI:s. The last member
** ( 8. psStruct ) is referring the structured data types described above.
** Note that member 3,5,6 and 7 are ignored if psStruct ( 8.) is a valid
** pointer.
**------------------------------------------------------------------------------
*/
const AD_AdiEntryType AD_asADIEntryList[] =
{
   /*
   ** ----------------------------------------------------------------------------------------------------------------------------
   ** | 1. iInstance | 2. pabName | 3. bDataType | 4. bNumOfElements | 5. bDesc | 6.pxValuePtr | 7. pxValuePropPtr | 8. psStruct |
   ** ----------------------------------------------------------------------------------------------------------------------------
   */
   {  10,  "Struct1",   ABP_BOOL,   2,  ADI_DESCR_ALL_ACCESS,  { { NULL,    NULL } },  ABCC_API_AdiStruct1  },
   {  20,  "Struct2",   ABP_BOOL,   4,  ADI_DESCR_ALL_ACCESS,  { { NULL,    NULL } },  ABCC_API_AdiStruct2  },
   {  30,  "NoStruct",  ABP_UINT8,  1,  ADI_DESCR_ALL_ACCESS,  { { &bData,  NULL } },  NULL  }
};


/*------------------------------------------------------------------------------
**  sDefaultMap is an example of a default map using bit data types, structured
**  ADI:s. and process data padding.
**  The mapping result is shown below.
**
**  Read mapping where ( OctetOffset + Bitoffset ) is the offset from RDPD
**  buffer start.
**
**  -------------------------------------------------------------
**  Octetoffset | Bitoffset |  Data type |  ADI | Place holder  |
**  -------------------------------------------------------------
**     0              0       ABP_UINT16    10   struct1 element 0
**     2              0       ABP_UINT32    10   struct1 element 1
**     6              0       ABP_BIT4      20   struct2 element 1
**     6              4       ABP_PAD1x4    0    No implementation
**     7              0       ABP_UINT8     30   bData
**
**  Write mapping where ( OctetOffset + Bitoffset ) is the offset from WRPD
**  buffer start.
**
**  -------------------------------------------------------------
**  Octetoffset | Bitoffset |  Data type |  ADI | Place holder  |
**  -------------------------------------------------------------
**     0              0       ABP_BIT5      20   struct2 element 0
**     0              5       ABP_BIT4      20   struct2 element 1
**     1              1       ABP_PAD1x7    0    No implementation
**------------------------------------------------------------------------------
*/
AD_MapType sDefaultMap[] =
{
   /*
   ** -------------------------------------------------------------
   ** | 1. iInstance | 2. eDir | 3. bNumElem | 4. bElemStartIndex |
   ** -------------------------------------------------------------
   */
   {  10,              PD_READ,   AD_MAP_ALL_ELEM,  0  },  /* ADI 10  is mapped. ( All elements )                  */
   {  20,              PD_READ,   1,                        1  },  /* ADI 20  element 1 (index) is mapped ( 4 Bits )       */
   {  AD_MAP_PAD_ADI,  PD_READ,   4,                        0  },  /* PAD process data with 4 bits for correct alignment   */
   {  30,              PD_READ,   1,                        0  },  /* Map ADI 30                                           */
   {  20,              PD_WRITE,  2,                        0  },  /* ADI 20 element 0 and 1 is mapped ( 9 bits )          */
   {  AD_MAP_PAD_ADI,  PD_WRITE,  7,                        0  },  /* PAD process data with 7 bits for correct alignment   */
   {  AD_MAP_END_ENTRY                                 }
};


#endif /* Example ADIs */

#endif  /* inclusion lock */
