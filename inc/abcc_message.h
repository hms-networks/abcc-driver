/*******************************************************************************
** Copyright 2025-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** ABCC message API used by the application.
********************************************************************************
*/

#ifndef ABCC_MESSAGE_H_
#define ABCC_MESSAGE_H_

#include "abp.h"
#include "abcc_types.h"

/*------------------------------------------------------------------------------
** Function types used by user to deliver messages to the application.
**------------------------------------------------------------------------------
*/
typedef void (*ABCC_MsgHandlerFuncType)( ABP_MsgType* psMsg );

/*------------------------------------------------------------------------------
** Function to indicate that a segmentation session has finished and the message
** has been completely sent.
** See description of ABCC_StartServerRespSegmentationSession().
**------------------------------------------------------------------------------
** Arguments:
**       pxObject - User defined (Supplied in
**                  ABCC_StartServerRespSegmentationSession())
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
typedef void (*ABCC_SegMsgHandlerDoneFuncType)( void* pxObject );

/*------------------------------------------------------------------------------
** Function callback to fetch next block of data to be sent by the segmentation
** handler. Note that the size of the block is unrelated to the segment size
** used by the driver i.e. the segment size does not need to be known by the
** callback function.
**
** See ABCC_StartServerRespSegmentationSession
**------------------------------------------------------------------------------
** Arguments:
**       pxObject - User defined (Supplied in
**                  ABCC_StartServerRespSegmentationSession)
**       plSize   - Pointer to size of returned data block (updated by user).
**                  Don't care if return buffer is NULL.
**
**
** Returns:
**       Pointer to next data block. NULL means no data.
**------------------------------------------------------------------------------
*/
typedef UINT8* (*ABCC_SegMsgHandlerNextBlockFuncType)( void* pxObject, UINT32* plSize );

/*------------------------------------------------------------------------------
** Macros for basic endian swap. Used by conversion macros below.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_iEndianSwap
#define ABCC_iEndianSwap( iFoo )    (UINT16)( ( (UINT16)(iFoo) >> 8 ) | ( (UINT16)(iFoo) << 8 ) )
#endif

#ifndef ABCC_lEndianSwap
#define ABCC_lEndianSwap( lFoo )      (UINT32)( ( (UINT32)ABCC_iEndianSwap( (UINT16)( (UINT32)(lFoo) ) ) << 16 ) | (UINT32)ABCC_iEndianSwap( (UINT32)(lFoo) >> 16 ) )
#endif

#if ( ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED || ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED )
#ifndef ABCC_l64EndianSwap
#define ABCC_l64EndianSwap( lFoo )  (UINT64)( ( (UINT64)ABCC_lEndianSwap( (UINT32)( (UINT64)(lFoo) ) ) << 32 ) | (UINT64)ABCC_lEndianSwap( (UINT64)(lFoo) >> 32 ) )
#endif
#endif

/*------------------------------------------------------------------------------
** Macros for reading/writing byte to/from a 16 bit word.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_iSetLSB
#define ABCC_iSetLSB( iDest, iSrc )                               \
do                                                                \
{                                                                 \
   (iDest) &= (UINT16)0xFF00;                                     \
   (iDest) |= (UINT16)(iSrc) & (UINT16)0x00FF;                    \
}                                                                 \
while( 0 )
#endif

#ifndef ABCC_iSetMSB
#define ABCC_iSetMSB( iDest, iSrc )                               \
do                                                                \
{                                                                 \
   (iDest) &=  (UINT16)0x00FF;                                    \
   (iDest) |=  (UINT16)(iSrc) << 8;                               \
}                                                                 \
while( 0 )
#endif

#ifndef ABCC_iLSB
#define ABCC_iLSB( iFoo )           (UINT16)( (iFoo) & 0x00FF )
#endif

#ifndef ABCC_iMSB
#define ABCC_iMSB( iFoo )           (UINT16)( (UINT16)(iFoo) >> 8 )
#endif

/*------------------------------------------------------------------------------
** Endian dependent macros:
**------------------------------------------------------------------------------
** Macros to convert to/from native endian to/from specified endian:
**
** iBeTOi( iBeFoo )   - 16 bit big endian    -> native endian
** iTOiBe( iFoo )     - 16 bit native endian -> big endian
** iLeTOi( iLeFoo )   - 16 bit little endian -> native endian
** iTOiLe( iFoo )     - 16 bit native endian -> little endian
**                    -
** lBeTOl( lBeFoo )   - 32 bit big endian    -> native endian
** lTOlBe( lFoo )     - 32 bit native endian -> big endian
** lLeTOl( lLeFoo )   - 32 bit little endian -> native endian
** lTOlLe( lFoo )     - 32 bit native endian -> little endian
**                    -
** lBeTOl64( lBeFoo ) - 64 bit big endian    -> native endian
** lTOlBe64( lFoo )   - 64 bit native endian -> big endian
** lLeTOl64( lLeFoo ) - 64 bit little endian -> native endian
** lTOlLe64( lFoo )   - 64 bit native endian -> little endian
**------------------------------------------------------------------------------
** Macros to set/get low/high address octet from a word:
**
** ABCC_GetLowAddrOct( iFoo )
** ABCC_GetHighAddrOct( iFoo )
** ABCC_SetLowAddrOct( iDest, iSrc )
** ABCC_SetHighAddrOct( iDest, iSrc )
**------------------------------------------------------------------------------
*/
#ifdef ABCC_SYS_BIG_ENDIAN

   #define iBeTOi( iBeFoo )                     (UINT16)(iBeFoo)
   #define iTOiBe( iFoo )                       (UINT16)(iFoo)
   #define iLeTOi( iLeFoo )                     ABCC_iEndianSwap( iLeFoo )
   #define iTOiLe( iFoo )                       ABCC_iEndianSwap( iFoo )

   #define lBeTOl( lBeFoo )                     (UINT32)(lBeFoo)
   #define lTOlBe( lFoo )                       (UINT32)(lFoo)
   #define lLeTOl( lLeFoo )                     ABCC_lEndianSwap( lLeFoo )
   #define lTOlLe( lFoo )                       ABCC_lEndianSwap( lFoo )


   #define ABCC_GetLowAddrOct( iFoo )           ABCC_iMSB( iFoo )
   #define ABCC_GetHighAddrOct( iFoo )          ABCC_iLSB( iFoo )

   #define ABCC_SetLowAddrOct( iDest, iSrc )    ABCC_iSetMSB( iDest, iSrc )
   #define ABCC_SetHighAddrOct( iDest, iSrc )   ABCC_iSetLSB( iDest, iSrc )

#else
   #define iBeTOi( iBeFoo )                     ABCC_iEndianSwap( iBeFoo )
   #define iTOiBe( iFoo )                       ABCC_iEndianSwap( iFoo )
   #define iLeTOi( iLeFoo )                     (UINT16)(iLeFoo)
   #define iTOiLe( iFoo )                       (UINT16)(iFoo)
   #define lBeTOl( lBeFoo )                     ABCC_lEndianSwap( lBeFoo )
   #define lTOlBe( lFoo )                       ABCC_lEndianSwap( lFoo )
   #define lLeTOl( lLeFoo )                     (UINT32)(lLeFoo)
   #define lTOlLe( lFoo )                       (UINT32)(lFoo)

   #define ABCC_GetLowAddrOct( iFoo )           ABCC_iLSB( iFoo )
   #define ABCC_GetHighAddrOct( iFoo )          ABCC_iMSB( iFoo )

   #define ABCC_SetLowAddrOct( iDest, iSrc )    ABCC_iSetLSB( iDest, iSrc )
   #define ABCC_SetHighAddrOct( iDest, iSrc )   ABCC_iSetMSB( iDest, iSrc )
#endif

#if ( ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED || ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED )
#ifdef ABCC_SYS_BIG_ENDIAN
#define lBeTOl64( lBeFoo )  (UINT64)(lBeFoo)
#define lTOlBe64( lFoo )    (UINT64)(lFoo)
#define lLeTOl64( lLeFoo )  ABCC_l64EndianSwap( lLeFoo )
#define lTOlLe64( lFoo )    ABCC_l64EndianSwap( lFoo )
#else
#define lBeTOl64( lBeFoo )  ABCC_l64EndianSwap( lBeFoo )
#define lTOlBe64( lFoo )    ABCC_l64EndianSwap( lFoo )
#define lLeTOl64( lLeFoo )  (UINT64)(lLeFoo)
#define lTOlLe64( lFoo )    (UINT64)(lFoo)
#endif
#endif

/*------------------------------------------------------------------------------
** 8/16 bit char platform dependent macros for reading ABP message type members
**------------------------------------------------------------------------------
** ABCC_GetMsgDataSize( psMsg )               - Message data size (in octets)
** ABCC_SetMsgDataSize( psMsg, iDataSizeVal ) - Set message data size
**                                              (in octets)
** ABCC_GetMsgInstance( psMsg )               - Message instance
** ABCC_GetMsgSourceId( psMsg )               - Message source id
** ABCC_GetMsgDestObj( psMsg )                - Destination object
** ABCC_IsCmdMsg( psMsg )                     - Message command bit
** ABCC_GetMsgCmdBits( psMsg )                - Message command
** ABCC_GetMsgCmdExt0( psMsg )                - Command extension 0
** ABCC_SetMsgCmdExt0( psMsg, bCmdExt0Val )   - Set command extension 0
** ABCC_GetMsgCmdExt1( psMsg )                - Command extension 1
** ABCC_SetMsgCmdExt1( psMsg, bCmdExt1Val )   - Set command extension 1
** ABCC_GetMsgCmdExt( psMsg )                 - Get extension 0 and 1
**                                              16 bit type
** ABCC_SetMsgCmdExt( psMsg , iExtVal)        - Set extension 0 and 1
**                                              16 bit type
** ABCC_GetMsgDataPtr( psMsg )                - Message data pointer
** ABCC_GetErrorCode( psMsg )                 - Error code
**------------------------------------------------------------------------------
*/

#ifdef ABCC_SYS_16_BIT_CHAR
#define ABCC_GetMsgDataSize( psMsg               ) ( iLeTOi(               ((ABP_MsgHeaderType16*) (psMsg))->iDataSize                                ) )
#define ABCC_SetMsgDataSize( psMsg, iDataSizeVal ) (                       ((ABP_MsgHeaderType16*) (psMsg))->iDataSize ) = iTOiLe( iDataSizeVal )
#define ABCC_GetMsgInstance( psMsg               ) ( iLeTOi(               ((ABP_MsgHeaderType16*) (psMsg))->iInstance                                ) )
#define ABCC_SetMsgInstance( psMsg, iInstanceVal ) (                       ((ABP_MsgHeaderType16*) (psMsg))->iInstance ) = iTOiLe( iInstanceVal )
#define ABCC_GetMsgSourceId( psMsg               ) ( ABCC_GetLowAddrOct(   ((ABP_MsgHeaderType16*) (psMsg))->iSourceIdDestObj                         ) )
#define ABCC_GetMsgDestObj(  psMsg               ) ( ABCC_GetHighAddrOct(  ((ABP_MsgHeaderType16*) (psMsg))->iSourceIdDestObj                         ) )
#define ABCC_IsCmdMsg(       psMsg               ) ( ABCC_GetLowAddrOct(   ((ABP_MsgHeaderType16*) (psMsg))->iCmdReserved ) & ABP_MSG_HEADER_C_BIT    )
#define ABCC_GetMsgCmdBits(  psMsg               ) ( ABCC_GetLowAddrOct(   ((ABP_MsgHeaderType16*) (psMsg))->iCmdReserved ) & ABP_MSG_HEADER_CMD_BITS )
#define ABCC_GetMsgCmdExt0(  psMsg               ) ( ABCC_GetLowAddrOct(   ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1                          ) )
#define ABCC_SetMsgCmdExt0(  psMsg, bCmdExt0Val  )   ABCC_SetLowAddrOct(   ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1, bCmdExt0Val             )
#define ABCC_GetMsgCmdExt1(  psMsg               ) ( ABCC_GetHighAddrOct(  ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1                          ) )
#define ABCC_SetMsgCmdExt1(  psMsg, bCmdExt1Val  )   ABCC_SetHighAddrOct(  ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1, bCmdExt1Val             )
#define ABCC_GetMsgCmdExt(   psMsg               ) (              iLeTOi(  ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1                          ) )
#define ABCC_SetMsgCmdExt(   psMsg, iExtVal      ) (                       ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1 = iTOiLe( iExtVal        ) )
#define ABCC_GetMsgDataPtr(  psMsg               ) (                                               (psMsg)->aiData                                    )
#define ABCC_GetErrorCode(   psMsg               ) ( ABCC_GetLowAddrOct(                           (psMsg)->aiData[ 0 ]                               ) )
#else
#define ABCC_GetMsgDataSize( psMsg               ) ( iLeTOi( ((ABP_MsgHeaderType*)   (psMsg))->iDataSize                          ) )
#define ABCC_SetMsgDataSize( psMsg, iDataSizeVal ) (         ((ABP_MsgHeaderType*)   (psMsg))->iDataSize ) = iTOiLe( iDataSizeVal )
#define ABCC_GetMsgInstance( psMsg               ) ( iLeTOi( ((ABP_MsgHeaderType*)   (psMsg))->iInstance                          ) )
#define ABCC_SetMsgInstance( psMsg, iInstanceVal ) (         ((ABP_MsgHeaderType*)   (psMsg))->iInstance ) = iTOiLe( iInstanceVal )
#define ABCC_GetMsgSourceId( psMsg               ) (         ((ABP_MsgHeaderType*)   (psMsg))->bSourceId                          )
#define ABCC_GetMsgDestObj(  psMsg               ) (         ((ABP_MsgHeaderType*)   (psMsg))->bDestObj                           )
#define ABCC_IsCmdMsg(       psMsg               ) (         ((ABP_MsgHeaderType*)   (psMsg))->bCmd & ABP_MSG_HEADER_C_BIT        )
#define ABCC_GetMsgCmdBits(  psMsg               ) (         ((ABP_MsgHeaderType*)   (psMsg))->bCmd & ABP_MSG_HEADER_CMD_BITS     )
#define ABCC_GetMsgCmdExt0(  psMsg               ) (         ((ABP_MsgHeaderType*)   (psMsg))->bCmdExt0                           )
#define ABCC_SetMsgCmdExt0(  psMsg, bCmdExt0Val  ) (         ((ABP_MsgHeaderType*)   (psMsg))->bCmdExt0        = (bCmdExt0Val)    )
#define ABCC_GetMsgCmdExt1(  psMsg               ) (         ((ABP_MsgHeaderType*)   (psMsg))->bCmdExt1                           )
#define ABCC_SetMsgCmdExt1(  psMsg, bCmdExt1Val  ) (         ((ABP_MsgHeaderType*)   (psMsg))->bCmdExt1        = (bCmdExt1Val)    )
#define ABCC_GetMsgCmdExt(   psMsg               ) ( iLeTOi( ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1                    ) )
#define ABCC_SetMsgCmdExt(   psMsg, iExtVal      ) (         ((ABP_MsgHeaderType16*) (psMsg))->iCmdExt0CmdExt1 = iTOiLe( iExtVal  ) )
#define ABCC_GetMsgDataPtr(  psMsg               ) (                                 (psMsg)->abData                              )
#define ABCC_GetErrorCode(   psMsg               ) (                                 (psMsg)->abData[ 0 ]                         )
#endif


/*------------------------------------------------------------------------------
** 8/16 bit char platform dependent macros to read and write message data.
** Two version of each get/set function exist:
** ABCC_GetMsgData<x> takes the abcc message pointer as input.
** ABCC_GetData<x> takes the abcc message payload pointer as input.
**------------------------------------------------------------------------------
** ABCC_Set<x>String()  - Copy native string to ABCC message payload
** ABCC_Get<x>String()  - Copy ABCC message payload string to native string
** ABCC_Set<x>Data8()   - Write 8 bit data to ABCC message payload
** ABCC_Set<x>Data16()  - Write 16 bit data to ABCC message payload
** ABCC_Set<x>Data32()  - Write 32 bit data to ABCC message payload
** ABCC_Set<x>Data64()  - Write 64 bit data to ABCC message payload
** ABCC_Get<x>Data8()   - Read 8 bit data from an ABCC message payload
** ABCC_Get<x>Data16()  - Read 16 bit data from an ABCC message payload
** ABCC_Get<x>Data32()  - Read 32 bit data from an ABCC message payload
** ABCC_Get<x>Data64()  - Read 64 bit data from an ABCC message payload
**------------------------------------------------------------------------------
** ABCC_SetMsgString( psMsg, pcString, iNumChar, iOctetOffset )
**    psMsg - Pointer to message
** ABCC_SetString( pxDst, pcString, iNumChar, iOctetOffset )
**    pxDst - Pointer to message payload
**    pcString - String to be written
**    iNumChar - Number of chars in the string
**    iOctetOffset - Offset to where the string shall be written.
**
** ABCC_GetMsgString( pxSrc, pcString, iNumChar, iOctetOffset )
**    psMsg - Pointer to message
** ABCC_GetString( psMsg, pcString, iNumChar, iOctetOffset )
**    pxSrc - Pointer to message payload
**    pcString - String to be written
**    iNumChar - Number of chars in the string
**    iOctetOffset - Offset to where the string shall be read.
**
** ABCC_SetMsgDataX( psMsg, Data, iOctetOffset )
**    psMsg - Pointer to message
** ABCC_SetDataX( pxDst, Data, iOctetOffset )
**    pxDst - Pointer to message payload
**    Data  - Data to be set
**    iOctetOffset - Offset to where data shall be written.
**
** ABCC_GetMsgDataX( psMsg, Data, iOctetOffset )
**    psMsg - Pointer to message
** ABCC_GetDataX( pxSrc, Data, iOctetOffset )
**    pxSrc - Pointer to message
**    Data  - Read data variable
**    iOctetOffset - Offset to where data shall be read.
**------------------------------------------------------------------------------
*/
#define ABCC_LOG_OVERRUN( xSize )                   \
ABCC_LOG_ERROR( ABCC_EC_MSG_BUFFER_OVERRUN,         \
   xSize,                                           \
   "Message buffer overrun prevented %d > %d\n",    \
   xSize,                                           \
   ABCC_GetMaxMessageSize() )

void ABCC_GetString( void* pxSrc, char* pcString, UINT16 iNumChar, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_GetMsgString( psMsg, pcString, iNumChar, iOctetOffset )                   \
   if( ( iOctetOffset + iNumChar ) > ABCC_GetMaxMessageSize() )                        \
   {                                                                                   \
      ABCC_LOG_OVERRUN( iOctetOffset + iNumChar );                                     \
   }                                                                                   \
   else                                                                                \
   {                                                                                   \
      ABCC_GetString( ABCC_GetMsgDataPtr( psMsg ), pcString, iNumChar, iOctetOffset ); \
   }
#else
#define ABCC_GetMsgString( psMsg, pcString, iNumChar, iOctetOffset )                   \
   ABCC_GetString( ABCC_GetMsgDataPtr( psMsg ), pcString, iNumChar, iOctetOffset )
#endif

void ABCC_SetString( void* pxDst, const char* pcString, UINT16 iNumChar, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_SetMsgString( psMsg, pcString, iNumChar, iOctetOffset )                   \
   if( ( iOctetOffset + iNumChar ) > ABCC_GetMaxMessageSize() )                        \
   {                                                                                   \
      ABCC_LOG_OVERRUN( iOctetOffset + iNumChar );                                     \
   }                                                                                   \
   else                                                                                \
   {                                                                                   \
      ABCC_SetString( ABCC_GetMsgDataPtr( psMsg ), pcString, iNumChar, iOctetOffset ); \
   }
#else
#define ABCC_SetMsgString( psMsg, pcString, iNumChar, iOctetOffset )                   \
   ABCC_SetString( ABCC_GetMsgDataPtr( psMsg ), pcString, iNumChar, iOctetOffset )
#endif

void ABCC_GetData8( void* pxSrc, UINT8* pbData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_GetMsgData8( psMsg, pbData, iOctetOffset )                       \
   if( ( iOctetOffset + ABP_UINT8_SIZEOF ) > ABCC_GetMaxMessageSize() )       \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT8_SIZEOF );                    \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_GetData8( ABCC_GetMsgDataPtr( psMsg ), pbData, iOctetOffset );     \
   }
#else
#define ABCC_GetMsgData8( psMsg, pbData, iOctetOffset )                       \
   ABCC_GetData8( ABCC_GetMsgDataPtr( psMsg ), pbData, iOctetOffset )
#endif

void ABCC_SetData8( void* pxDst, UINT8 bData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_SetMsgData8( psMsg, bData, iOctetOffset )                        \
   if( ( iOctetOffset + ABP_UINT8_SIZEOF ) > ABCC_GetMaxMessageSize() )       \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT8_SIZEOF );                    \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_SetData8( ABCC_GetMsgDataPtr( psMsg ), bData, iOctetOffset );      \
   }
#else
#define ABCC_SetMsgData8( psMsg, bData, iOctetOffset )                        \
   ABCC_SetData8( ABCC_GetMsgDataPtr( psMsg ), bData, iOctetOffset )
#endif

void ABCC_GetData16( void* pxSrc, UINT16* piData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_GetMsgData16( psMsg, piData, iOctetOffset )                      \
   if( ( iOctetOffset + ABP_UINT16_SIZEOF ) > ABCC_GetMaxMessageSize() )      \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT16_SIZEOF );                   \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_GetData16( ABCC_GetMsgDataPtr( psMsg ), piData, iOctetOffset );    \
   }
#else
#define ABCC_GetMsgData16( psMsg, piData, iOctetOffset )                      \
   ABCC_GetData16( ABCC_GetMsgDataPtr( psMsg ), piData, iOctetOffset )
#endif

void ABCC_SetData16( void* pxDst, UINT16 iData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_SetMsgData16( psMsg, iData, iOctetOffset )                       \
   if( ( iOctetOffset + ABP_UINT16_SIZEOF ) > ABCC_GetMaxMessageSize() )      \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT16_SIZEOF );                   \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_SetData16( ABCC_GetMsgDataPtr( psMsg ), iData, iOctetOffset );     \
   }
#else
#define ABCC_SetMsgData16( psMsg, iData, iOctetOffset )                       \
   ABCC_SetData16( ABCC_GetMsgDataPtr( psMsg ), iData, iOctetOffset )
#endif

void ABCC_GetData32( void* pxSrc, UINT32* plData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_GetMsgData32( psMsg, plData, iOctetOffset )                      \
   if( ( iOctetOffset + ABP_UINT32_SIZEOF ) > ABCC_GetMaxMessageSize() )      \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT32_SIZEOF );                   \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_GetData32( ABCC_GetMsgDataPtr( psMsg ), plData, iOctetOffset );    \
   }
#else
#define ABCC_GetMsgData32( psMsg, plData, iOctetOffset )                      \
   ABCC_GetData32( ABCC_GetMsgDataPtr( psMsg ), plData, iOctetOffset )
#endif

void ABCC_SetData32( void* pxDst, UINT32 lData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_SetMsgData32( psMsg, lData, iOctetOffset )                       \
   if( ( iOctetOffset + ABP_UINT32_SIZEOF ) > ABCC_GetMaxMessageSize() )      \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT32_SIZEOF );                   \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_SetData32( ABCC_GetMsgDataPtr( psMsg ), lData, iOctetOffset );     \
   }
#else
#define ABCC_SetMsgData32( psMsg, lData, iOctetOffset )                       \
   ABCC_SetData32( ABCC_GetMsgDataPtr( psMsg ), lData, iOctetOffset )
#endif

#if ( ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED || ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED )
void ABCC_GetData64( void* pxSrc, UINT64* plData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_GetMsgData64( psMsg, plData, iOctetOffset )                      \
   if( ( iOctetOffset + ABP_UINT64_SIZEOF ) > ABCC_GetMaxMessageSize() )      \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT64_SIZEOF );                   \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_GetData64( ABCC_GetMsgDataPtr( psMsg ), plData, iOctetOffset );    \
   }
#else
#define ABCC_GetMsgData64( psMsg, plData, iOctetOffset )                      \
   ABCC_GetData64( ABCC_GetMsgDataPtr( psMsg ), plData, iOctetOffset )
#endif

void ABCC_SetData64( void* pxDst, UINT64 lData, UINT16 iOctetOffset );
#if ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
#define ABCC_SetMsgData64( psMsg, lData, iOctetOffset )                       \
   if( ( iOctetOffset + ABP_UINT64_SIZEOF ) > ABCC_GetMaxMessageSize() )      \
   {                                                                          \
      ABCC_LOG_OVERRUN( iOctetOffset + ABP_UINT64_SIZEOF );                   \
   }                                                                          \
   else                                                                       \
   {                                                                          \
      ABCC_SetData64( ABCC_GetMsgDataPtr( psMsg ), lData, iOctetOffset );     \
   }
#else
#define ABCC_SetMsgData64( psMsg, lData, iOctetOffset )                       \
   ABCC_SetData64( ABCC_GetMsgDataPtr( psMsg ), lData, iOctetOffset )
#endif
#endif

#endif /* inclusion lock */
