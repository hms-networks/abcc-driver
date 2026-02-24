/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implementation of abcc setup state machine
********************************************************************************
*/

#include "abcc_config.h"
#include "abcc_types.h"
#include "abp.h"
#include "abcc.h"
#include "abcc_command_sequencer_interface.h"
#include "abcc_handler.h"
#include "abcc_driver_interface.h"
#include "abcc_log.h"

/*
** Invalid ADI index.
*/
#define AD_INVALID_ADI_INDEX           ( 0xffff )

#if !ABCC_CFG_DRV_CMD_SEQ_ENABLED
typedef enum CmdSetupState
{
   SETUP_BEFORE_USER_INIT,
   SETUP_USER_INIT,
   SETUP_AFTER_USER_INIT,
   SETUP_DONE
}
CmdSetupStateType;
#endif

#if !ABCC_CFG_DRV_CMD_SEQ_ENABLED
static void SendSetupCommand( ABP_MsgType* psMsg );
#endif
#if ABCC_CFG_DEBUG_GET_FLOG
static ABCC_CmdSeqCmdStatusType GetFatalLogCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType GetFatalLogResp( ABP_MsgType* psMsg, void* pxUserData );
#if ABCC_CFG_DEBUG_CLR_FLOG
static ABCC_CmdSeqCmdStatusType ClearFatalLogCmd( ABP_MsgType* psMsg, void* pxUserData );
#endif
#endif
static ABCC_CmdSeqCmdStatusType DataFormatCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType DataFormatResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType ParamSupportCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType ParamSupportResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType ModuleTypeCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType ModuleTypeResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType NetworkTypeCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType NetworkTypeResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType FirmwareVersionCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType FirmwareVersionResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType PreparePdMapping( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType ReadWriteMapCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType ReadWriteMapResp( ABP_MsgType* psMsg, void* pxUserData );

static void TriggerUserInit( const ABCC_CmdSeqResultType eSeqResult, void* pxUserData );

static ABCC_CmdSeqCmdStatusType RdPdSizeCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType RdPdSizeResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType WrPdSizeCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType WrPdSizeResp( ABP_MsgType* psMsg, void* pxUserData );

static ABCC_CmdSeqCmdStatusType SetupCompleteCmd( ABP_MsgType* psMsg, void* pxUserData );
static ABCC_CmdSeqRespStatusType SetupCompleteResp( ABP_MsgType* psMsg, void* pxUserData );

static void SetupDone( const ABCC_CmdSeqResultType eSeqResult, void* pxUserData );

/*
** Command sequence until user setup.
*/
static const ABCC_CmdSeqType SetupSeqBeforeUserInit[] =
{
#if ABCC_CFG_DEBUG_GET_FLOG
   ABCC_CMD_SEQ( GetFatalLogCmd,     GetFatalLogResp ),
#if ABCC_CFG_DEBUG_CLR_FLOG
   ABCC_CMD_SEQ( ClearFatalLogCmd,   NULL ),
#endif
#endif
   ABCC_CMD_SEQ( DataFormatCmd,      DataFormatResp ),
   ABCC_CMD_SEQ( ParamSupportCmd,    ParamSupportResp ),
   ABCC_CMD_SEQ( ModuleTypeCmd,      ModuleTypeResp ),
   ABCC_CMD_SEQ( NetworkTypeCmd,     NetworkTypeResp ),
   ABCC_CMD_SEQ( FirmwareVersionCmd, FirmwareVersionResp ),
   ABCC_CMD_SEQ( PreparePdMapping,   NULL ),
   ABCC_CMD_SEQ( ReadWriteMapCmd,    ReadWriteMapResp ),
   ABCC_CMD_SEQ_END()
};

/*
** Command sequence after user setup.
*/
static const ABCC_CmdSeqType SetupSeqAfterUserInit[] =
{
   ABCC_CMD_SEQ( RdPdSizeCmd,      RdPdSizeResp ),
   ABCC_CMD_SEQ( WrPdSizeCmd,      WrPdSizeResp ),
   ABCC_CMD_SEQ( SetupCompleteCmd, SetupCompleteResp ),
   ABCC_CMD_SEQ_END()
};


#if ABCC_CFG_DEBUG_GET_FLOG && ABCC_CFG_DEBUG_CLR_FLOG
static BOOL abcc_fClearFatalLog;
#endif

/*------------------------------------------------------------------------------
** abcc_iModuleType       - ABCC module type (read out during SETUP state)
** abcc_iNetworkType      - ABCC network type (read out during SETUP state)
** abcc_sFwVersion        - ABCC firmware version (read out during SETUP state)
** abcc_eNetFormat        - Data endian format of the network
**                          (read out during SETUP state)
** abcc_eParameterSupport - Parameter support (read out during SETUP state)
** abcc_eCmdState         - Current command message state during initialization
** abcc_psAdiEntry        - Pointer to list of ADIs
** abcc_psDefaultMap      - Pointer to list of default mapped ADIs
** abcc_iNumAdi           - Number of ADIs in abcc_psAdiEntry list
** abcc_MappingIndex      - Index of next ADI to map from abcc_psDefaultMap
** abcc_iPdReadSize       - Read process data size in octets
** abcc_iPdWriteSize      - Write process data size in octets
** abcc_iPdWriteBitSize   - Write process data size in bits
** abcc_iPdReadBitSize    - Read process data size in bits
**------------------------------------------------------------------------------
*/
static UINT16                    abcc_iModuleType;
static UINT16                    abcc_iNetworkType;
static ABCC_FwVersionType        abcc_sFwVersion;
static ABCC_NetFormatType        abcc_eNetFormat;
static ABCC_ParameterSupportType abcc_eParameterSupport;

/*
** Must be set to TRUE before the first command is sent to the ABCC and then
** set to FALSE again in the response handler for that command.
*/
static BOOL abcc_fFirstCommandPending = FALSE;

/*
** Help varibales for ADI mapping servcie
*/
static AD_AdiEntryType*     abcc_psAdiEntry    = NULL;
static AD_MapType*          abcc_psDefaultMap  = NULL;
static UINT16               abcc_iNumAdi       = 0;
static UINT16               abcc_iMappingIndex = 0;

/*
** Currently used process data sizes
*/
static UINT16               abcc_iPdReadSize   = 0;
static UINT16               abcc_iPdWriteSize  = 0;

/*
** Current wrpd sizes divided in octets and bits
*/
static UINT16   abcc_iPdWriteBitSize = 0;

/*
** Current rdpd sizes divided in octets and bits
*/
static UINT16   abcc_iPdReadBitSize = 0;

#if !ABCC_CFG_DRV_CMD_SEQ_ENABLED

/*
** Current state main setup state
*/
static CmdSetupStateType eSetupState;

/*
** Sub states for main setup state: SETUP_BEFORE_USER_INIT:
** DataFormatCmd        0
** ParamSupportCmd      1
** ModuleTypeCmd        2
** NetworkTypeCmd       3
** FirmwareVersionCmd   4
** ReadWriteMapCmd      5
** PreparePdMapping     6
**
** SETUP_USER_INIT:     Has no sub states
**
** Sub states for main setup state: SETUP_AFTER_USER_INIT:
** RdPdSizeCmd          0
** WrPdSizeCmd          1
** SetupCompleteCmd     2
**
** SETUP_DONE:       Has no sub states
**
*/
static UINT8 bSetupSubState = 0;

/*
** Pointer to current main setup state sequence.
** SETUP_BEFORE_USER_INIT: SetupSeqBeforeUserInit[]
** SETUP_AFTER_USER_INIT:  SetupSeqAfterUserInit[]
*/
static const ABCC_CmdSeqType* pasSetupSeq;
#endif

/*------------------------------------------------------------------------------
** Find ADI entry table index for the specified instance number.
**------------------------------------------------------------------------------
** Arguments:
**    iInstance         -  Instance number.
**
** Returns:
**    AD_INVALID_ADI_INDEX      - Instance was not found.
**------------------------------------------------------------------------------
*/
static UINT16 GetAdiIndex( UINT16 iInstance )
{
   UINT16   iLow;
   UINT16   iMid;
   UINT16   iHigh;

   if( abcc_iNumAdi == 0 )
   {
      return( AD_INVALID_ADI_INDEX );
   }

   iLow = 0;
   iHigh = abcc_iNumAdi - 1;

   while( iLow != iHigh )
   {
      iMid = iLow + ( ( iHigh - iLow + 1 ) / 2 );
      if( abcc_psAdiEntry[ iMid ].iInstance > iInstance )
      {
         iHigh = iMid - 1;
      }
      else
      {
         iLow = iMid;
      }
   }

   if( abcc_psAdiEntry[ iLow ].iInstance != iInstance )
   {
      iLow = AD_INVALID_ADI_INDEX;
   }

   return( iLow );
}

static UINT16 abcc_GetAdiMapSizeInBits( const AD_AdiEntryType* psAdiEntry, UINT8 bNumElem, UINT8 bElemStartIndex )
{
   UINT16 iSize;
#if ABCC_CFG_STRUCT_DATA_TYPE_ENABLED
   UINT16 i;
   if( psAdiEntry->psStruct == NULL )
   {
      iSize = ABCC_GetDataTypeSizeInBits( psAdiEntry->bDataType ) * bNumElem;
   }
   else
   {
      iSize = 0;
      for( i = bElemStartIndex; i < ( bNumElem + bElemStartIndex ); i++ )
      {
         iSize += ABCC_GetDataTypeSizeInBits( psAdiEntry->psStruct[ i ].bDataType );
      }
   }
#else
      (void)bElemStartIndex;
      iSize = ABCC_GetDataTypeSizeInBits( psAdiEntry->bDataType ) * bNumElem;
#endif

   return( iSize );
}

static void abcc_FillMapExtCommand( ABP_MsgType16* psMsg16, UINT16 iAdi, UINT8 bAdiTotNumElem, UINT8 bElemStartIndex, UINT8 bNumElem, UINT8 bDataType )
{
   psMsg16->aiData[ 0 ] = iTOiLe( iAdi );                               /* ADI Instance number. */
   ABCC_SetLowAddrOct( psMsg16->aiData[ 1 ], bAdiTotNumElem );          /* Total number of elements in ADI. */
   ABCC_SetHighAddrOct( psMsg16->aiData[ 1 ], bElemStartIndex );
   ABCC_SetLowAddrOct( psMsg16->aiData[ 2 ], bNumElem );
   ABCC_SetHighAddrOct( psMsg16->aiData[ 2 ], 1 );                      /* Number of type descriptors. */
   ABCC_SetLowAddrOct( psMsg16->aiData[ 3 ], bDataType );               /* ADI element data type. */
   psMsg16->sHeader.iDataSize = iTOiLe( 7 );                            /* The number of used octets in aiData. (The bytes written below). */
}

void ABCC_SetupInit( void )
{
   abcc_sFwVersion.bMajor = 0xFF;
   abcc_sFwVersion.bMinor = 0xFF;
   abcc_sFwVersion.bBuild = 0xFF;
   abcc_iModuleType = 0xFFFF;
   abcc_iNetworkType = 0xFFFF;
   abcc_eNetFormat = NET_UNKNOWN;
   abcc_eParameterSupport = PARAMETER_UNKNOWN;

   abcc_psAdiEntry     = NULL;
   abcc_psDefaultMap   = NULL;
   abcc_iNumAdi = 0;
   abcc_iMappingIndex  = 0;
   abcc_iPdReadSize    = 0;
   abcc_iPdWriteSize   = 0;
   abcc_iPdWriteBitSize  = 0;
   abcc_iPdReadBitSize   = 0;
}

#if ABCC_CFG_DEBUG_GET_FLOG
/*------------------------------------------------------------------------------
** Get fatal log command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType GetFatalLogCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_ANB, 1, ABP_ANB_IA_FATAL_EVENT,
                      ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Get fatal log response.
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType GetFatalLogResp( ABP_MsgType* psMsg, void* pxUserData )
{
   UINT16 iSize;
   UINT16 iIndex;
   UINT8  bTemp;

   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   ABCC_PORT_printf( "Fatal log: " );
   iSize = ABCC_GetMsgDataSize( psMsg );
   for( iIndex = 0; iIndex < iSize; iIndex++ )
   {
      ABCC_GetMsgData8( psMsg, &bTemp, iIndex );
      ABCC_PORT_printf( "%02"PRIx8, bTemp );
   }
   ABCC_PORT_printf( "\n" );

   if( ( ABCC_GetMsgDataSize( psMsg ) != 40 ) &&
       ( ABCC_GetMsgDataSize( psMsg ) != 42 ) )
   {
      /*
      ** A fatal log is supposed to be either 40 bytes (ABCC30) or 42 bytes
      ** (ABCC40), so abort the startup procedure if the returned data has a
      ** different size.
      */
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

#if ABCC_CFG_DEBUG_CLR_FLOG
   /*
   ** Check the FW revision field to see if an event has been recorded, it
   ** should be non-zero then.
   **
   ** NOTE: The format of the fatal log is platform-specific! The present
   ** method works with the existing ABCC30 and ABCC40 modules, but may very
   ** well have to be tweaked if ABCCs based on other platforms appear.
   */
   abcc_fClearFatalLog = FALSE;
   ABCC_GetMsgData8( psMsg, &bTemp, 6 );
   if( bTemp != 0 )
   {
      ABCC_GetMsgData8( psMsg, &bTemp, 7 );
      if( bTemp != 0 )
      {
         abcc_fClearFatalLog = TRUE;
      }
   }
#endif

   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

#if ABCC_CFG_DEBUG_CLR_FLOG
/*------------------------------------------------------------------------------
** Clear fatal log command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType ClearFatalLogCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   if( abcc_fClearFatalLog )
   {
      ABCC_SetByteAttribute( psMsg, ABP_OBJ_NUM_ANB, 1, ABP_ANB_IA_FATAL_EVENT,
                             0, ABCC_GetNewSourceId() );
      return( ABCC_CMDSEQ_CMD_SEND );
   }

   return( ABCC_CMDSEQ_CMD_SKIP );
}
#endif
#endif

/*------------------------------------------------------------------------------
** Data format command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType DataFormatCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_NW, 1,
                      ABP_NW_IA_DATA_FORMAT, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

BOOL ABCC_IsFirstCommandPending( void )
{
   return( abcc_fFirstCommandPending );
}

/*------------------------------------------------------------------------------
** Data format response
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType DataFormatResp( ABP_MsgType* psMsg, void* pxUserData )
{
   UINT8 bFormat;

   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   abcc_fFirstCommandPending = FALSE;

   ABCC_GetMsgData8( psMsg, &bFormat, 0 );
   switch( bFormat )
   {
   case ABP_NW_DATA_FORMAT_LSB_FIRST:
      abcc_eNetFormat = NET_LITTLEENDIAN;
      break;
   case ABP_NW_DATA_FORMAT_MSB_FIRST:
      abcc_eNetFormat = NET_BIGENDIAN;
      break;
   default:
      ABCC_LOG_ERROR( ABCC_EC_UNKNOWN_ENDIAN,
         (UINT32)abcc_eNetFormat,
         "Unknown endian %" PRIu8 "\n",
         bFormat );
      break;
   }
   ABCC_LOG_INFO( "RSP MSG_DATA_FORMAT: %d\n", abcc_eNetFormat );
   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Parameter support command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType ParamSupportCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_NW, 1,
                      ABP_NW_IA_PARAM_SUPPORT, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Parameter support response
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType ParamSupportResp( ABP_MsgType* psMsg, void* pxUserData )
{
   UINT8 bParamSupport;
   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   ABCC_GetMsgData8( psMsg, &bParamSupport, 0 );
   if( bParamSupport == FALSE )
   {
      abcc_eParameterSupport = NOT_PARAMETER_SUPPORT;
   }
   else
   {
      abcc_eParameterSupport = PARAMETER_SUPPORT;
   }
   ABCC_LOG_INFO( "RSP MSG_GET_PARAM_SUPPORT: %d\n", abcc_eParameterSupport );
   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Module type command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType ModuleTypeCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_ANB, 1,
                      ABP_ANB_IA_MODULE_TYPE, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Module type response.
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType ModuleTypeResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   ABCC_GetMsgData16( psMsg, &abcc_iModuleType, 0 );
   ABCC_LOG_INFO( "RSP MSG_GET_MODULE_ID: 0x%x\n", abcc_iModuleType );
   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Network type command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType NetworkTypeCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_NW, 1,
                      ABP_NW_IA_NW_TYPE, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Network type response.
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType NetworkTypeResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   ABCC_GetMsgData16( psMsg, &abcc_iNetworkType, 0 );
   ABCC_LOG_INFO( "RSP MSG_GET_NETWORK_ID: 0x%x\n", abcc_iNetworkType );
   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Firmware version command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType FirmwareVersionCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_ANB, 1,
                      ABP_ANB_IA_FW_VERSION, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Firmware version response.
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType FirmwareVersionResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   ABCC_GetMsgData8( psMsg, &abcc_sFwVersion.bMajor, 0 );
   ABCC_GetMsgData8( psMsg, &abcc_sFwVersion.bMinor, 1 );
   ABCC_GetMsgData8( psMsg, &abcc_sFwVersion.bBuild, 2 );
   ABCC_LOG_INFO( "RSP MSG_GET_FW_VERSION: %" PRIu8 ".%" PRIu8 ".%" PRIu8 "\n",
         abcc_sFwVersion.bMajor,
         abcc_sFwVersion.bMinor,
         abcc_sFwVersion.bBuild );
   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Actions required before mapping.
** Part of a command sequence and implements function callback.
** ABCC_CmdRespCmdHandler (abcc.h)
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType PreparePdMapping( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;
   (void)psMsg;
   abcc_iNumAdi = ABCC_CbfAdiMappingReq( (const AD_AdiEntryType**)&abcc_psAdiEntry,
                                         (const AD_MapType**)&abcc_psDefaultMap );
   /*
   ** No command shall be sent.
   */
   return( ABCC_CMDSEQ_CMD_SKIP );
}

/*------------------------------------------------------------------------------
** Read write mapping command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType ReadWriteMapCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   UINT16 iLocalMapIndex;
   UINT16 iLocalSize;
   ABCC_MsgType pMsgSendBuffer;

   (void)pxUserData;

   pMsgSendBuffer.psMsg = psMsg;
   iLocalMapIndex = 0;

   /*
   ** Unique source id for each mapping command
   */
   ABCC_SetLowAddrOct( pMsgSendBuffer.psMsg16->sHeader.iSourceIdDestObj, ABCC_GetNewSourceId() );


   if( abcc_psAdiEntry && abcc_psDefaultMap && ( abcc_psDefaultMap[ abcc_iMappingIndex ].eDir != PD_END_MAP ) )
   {
      if( abcc_psDefaultMap[ abcc_iMappingIndex ].iInstance != AD_MAP_PAD_ADI )
      {
         iLocalMapIndex = GetAdiIndex( abcc_psDefaultMap[ abcc_iMappingIndex ].iInstance );

         if( iLocalMapIndex == AD_INVALID_ADI_INDEX )
         {
            ABCC_LOG_ERROR( ABCC_EC_DEFAULT_MAP_ERR,
               (UINT32)abcc_psDefaultMap[ abcc_iMappingIndex ].iInstance,
               "Error in default map, instance %" PRIu16 " doesn't exist\n",
               abcc_psDefaultMap[ abcc_iMappingIndex ].iInstance );
            return( ABCC_CMDSEQ_CMD_ABORT );
         }
      }
   }
   else
   {
      return( ABCC_CMDSEQ_CMD_SKIP );
   }

   if( ABCC_ReadModuleId() == ABP_MODULE_ID_ACTIVE_ABCC40 )
   {
      UINT8 bNumElemToMap;
      UINT8 bElemMapStartIndex;

      /*
      ** Implement mapping according to the extended command for ABCC.
      */
      ABCC_SetHighAddrOct( pMsgSendBuffer.psMsg16->sHeader.iSourceIdDestObj, ABP_OBJ_NUM_NW );
      pMsgSendBuffer.psMsg16->sHeader.iInstance            = iTOiLe( 1 );

      /*
      ** Number of mapping items to add.
      */
      ABCC_SetLowAddrOct( pMsgSendBuffer.psMsg16->sHeader.iCmdExt0CmdExt1, 1 );

      /*
      ** Reserved
      */
      ABCC_SetHighAddrOct( pMsgSendBuffer.psMsg16->sHeader.iCmdExt0CmdExt1, 0 );

      if( abcc_psDefaultMap[ abcc_iMappingIndex ].iInstance != AD_MAP_PAD_ADI )
      {
         if( abcc_psDefaultMap[ abcc_iMappingIndex ].bNumElem == AD_MAP_ALL_ELEM )
         {
            bNumElemToMap = abcc_psAdiEntry[ iLocalMapIndex ].bNumOfElements;
            bElemMapStartIndex = 0;
         }
         else
         {
            bNumElemToMap = abcc_psDefaultMap[ abcc_iMappingIndex ].bNumElem;
            bElemMapStartIndex = abcc_psDefaultMap[ abcc_iMappingIndex ].bElemStartIndex;
         }

         abcc_FillMapExtCommand( pMsgSendBuffer.psMsg16,
                                 abcc_psAdiEntry[ iLocalMapIndex ].iInstance,      /* Adi */
                                 abcc_psAdiEntry[ iLocalMapIndex ].bNumOfElements, /* Adi total num elements */
                                 bElemMapStartIndex,                               /* Mapping  start index */
                                 bNumElemToMap,                                    /* Num elements to map */
                                 abcc_psAdiEntry[ iLocalMapIndex ].bDataType );    /* Data type */
         iLocalSize = abcc_GetAdiMapSizeInBits( &abcc_psAdiEntry[ iLocalMapIndex ],
                                                bNumElemToMap, bElemMapStartIndex );

#if ABCC_CFG_STRUCT_DATA_TYPE_ENABLED
         if( abcc_psAdiEntry[ iLocalMapIndex ].psStruct != NULL )
         {
            UINT16 iDescOffset;
            iDescOffset = 0;
            ABCC_SetHighAddrOct( pMsgSendBuffer.psMsg16->aiData[ 2 ], bNumElemToMap );

            while( iDescOffset < bNumElemToMap )
            {
               ABCC_SetLowAddrOct( pMsgSendBuffer.psMsg16->aiData[ ( iDescOffset >> 1 ) + 3 ], abcc_psAdiEntry[ iLocalMapIndex ].psStruct[ iDescOffset + bElemMapStartIndex].bDataType );
               iDescOffset++;
               if( iDescOffset < bNumElemToMap )
               {
                  ABCC_SetHighAddrOct( pMsgSendBuffer.psMsg16->aiData[ ( iDescOffset >> 1 ) + 3 ], abcc_psAdiEntry[ iLocalMapIndex ].psStruct[ iDescOffset + bElemMapStartIndex].bDataType );
                  iDescOffset++;
               }
            }
            pMsgSendBuffer.psMsg16->sHeader.iDataSize = iTOiLe( 6 + iDescOffset );
         }
#endif
      }
      else
      {
          abcc_FillMapExtCommand( pMsgSendBuffer.psMsg16,
                                  0,                                                /* Adi */
                                  abcc_psDefaultMap[ abcc_iMappingIndex ].bNumElem, /* Adi total num elements */
                                  0,                                                /* Mapping  start index */
                                  abcc_psDefaultMap[ abcc_iMappingIndex ].bNumElem, /* Num elements to map */
                                  ABP_PAD1 );                                       /* Data type */
         iLocalSize = abcc_psDefaultMap[ abcc_iMappingIndex ].bNumElem;

      }

      if( abcc_psDefaultMap[ abcc_iMappingIndex ].eDir == PD_READ )
      {
         ABCC_SetLowAddrOct( pMsgSendBuffer.psMsg16->sHeader.iCmdReserved, ABP_MSG_HEADER_C_BIT | ABP_NW_CMD_MAP_ADI_READ_EXT_AREA );
         abcc_iPdReadBitSize += iLocalSize;
         abcc_iPdReadSize = ( abcc_iPdReadBitSize + 7 ) / 8;
      }
      else
      {
         ABCC_SetLowAddrOct( pMsgSendBuffer.psMsg16->sHeader.iCmdReserved, ABP_MSG_HEADER_C_BIT | ABP_NW_CMD_MAP_ADI_WRITE_EXT_AREA );
         abcc_iPdWriteBitSize += iLocalSize;
         abcc_iPdWriteSize = ( abcc_iPdWriteBitSize + 7 ) / 8;
      }
      abcc_iMappingIndex++;
   }

   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Read write mapping response
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType ReadWriteMapResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_LOG_INFO( "RSP MSG_MAP_IO_****\n" );

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   if( abcc_psAdiEntry && abcc_psDefaultMap && ( abcc_psDefaultMap[ abcc_iMappingIndex ].eDir == PD_END_MAP ) )
   {
      /*
      ** This was the last mapping command. Proceed with next command.
      */
      return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
   }

   /*
   ** Next mapping
   */
   return( ABCC_CMDSEQ_RESP_EXEC_CURRENT );
}

/*------------------------------------------------------------------------------
** Trigger user init when mapping is done.
** Part of a command sequence and implements function callback
** ABCC_CmdSeqDoneHandler (abcc.h)
**------------------------------------------------------------------------------
*/
static void TriggerUserInit( const ABCC_CmdSeqResultType eSeqResult, void* pxUserData )
{
   (void)pxUserData;

   switch( eSeqResult )
   {
   case ABCC_CMDSEQ_RESULT_COMPLETED:
      ABCC_LOG_INFO( "Mapped PD size, RdPd %d WrPd: %d\n", abcc_iPdReadSize, abcc_iPdWriteSize );
      ABCC_CbfUserInitReq();
      break;

   case ABCC_CMDSEQ_RESULT_ABORT_INT:
      ABCC_LOG_WARNING( ABCC_EC_SETUP_FAILED,
         (UINT32)eSeqResult,
         "TriggerUserInit reported internally aborted command sequence.\n" );
      break;

   case ABCC_CMDSEQ_RESULT_ABORT_EXT:
      ABCC_LOG_WARNING( ABCC_EC_SETUP_FAILED,
         (UINT32)eSeqResult,
         "TriggerUserInit reported externally aborted command sequence.\n" );
      break;

   default:
      ABCC_LOG_WARNING( ABCC_EC_SETUP_FAILED,
         (UINT32)eSeqResult,
         "TriggerUserInit reported aborted command sequence.\n" );
      break;
   }
}

/*------------------------------------------------------------------------------
** Read map size command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType RdPdSizeCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_NW, 1,
                      ABP_NW_IA_READ_PD_SIZE, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Read map size response
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType RdPdSizeResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   if( abcc_psDefaultMap == NULL )
   {
      /*
      ** Use received read size
      */
      ABCC_GetMsgData16( psMsg, &abcc_iPdReadSize, 0 );
   }
   else
   {
      UINT16 iSize;

      /*
      ** Verify that ABCC and driver has the same view
      */
      ABCC_GetMsgData16( psMsg, &iSize, 0 );

      if( abcc_iPdReadSize != iSize )
      {
         ABCC_LOG_ERROR( ABCC_EC_PD_SIZE_MISMATCH,
            iSize,
            "Read PD size mismatch, ABCC: %d Driver: %d\n",
            abcc_iPdReadSize,
            iSize );

         return( ABCC_CMDSEQ_RESP_ABORT );
      }
   }

   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Write map size command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType WrPdSizeCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_GetAttribute( psMsg, ABP_OBJ_NUM_NW, 1,
                      ABP_NW_IA_WRITE_PD_SIZE, ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Write map size response
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType WrPdSizeResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;
   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   if( abcc_psDefaultMap == NULL )
   {
      /*
      ** Use received write size
      */
      ABCC_GetMsgData16( psMsg, &abcc_iPdWriteSize, 0 );
   }
   else
   {
      UINT16 iSize;

      /*
      ** Verify that ABCC and driver has the same view
      */
      ABCC_GetMsgData16( psMsg, &iSize, 0 );
      if( abcc_iPdWriteSize != iSize )
      {
         ABCC_LOG_ERROR( ABCC_EC_PD_SIZE_MISMATCH,
            iSize,
            "Write PD size mismatch, ABCC: %d Driver: %d\n",
            abcc_iPdWriteSize,
            iSize );

         return( ABCC_CMDSEQ_RESP_ABORT );
      }
   }

   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

/*------------------------------------------------------------------------------
** Setup complete command
**
** This function is a part of a command sequence. See description of
** ABCC_CmdSeqCmdHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqCmdStatusType SetupCompleteCmd( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   ABCC_SetByteAttribute(  psMsg, ABP_OBJ_NUM_ANB, 1,
                           ABP_ANB_IA_SETUP_COMPLETE, TRUE,
                           ABCC_GetNewSourceId() );
   return( ABCC_CMDSEQ_CMD_SEND );
}

/*------------------------------------------------------------------------------
** Setup complete response
**
** Part of a command sequence and implements function callback
** ABCC_CmdSeqRespHandler type in cmd_seq_if.h
**------------------------------------------------------------------------------
*/
static ABCC_CmdSeqRespStatusType SetupCompleteResp( ABP_MsgType* psMsg, void* pxUserData )
{
   (void)pxUserData;

   if( ABCC_VerifyMessage( psMsg ) != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( ABCC_EC_RESP_MSG_E_BIT_SET,
         (UINT32)ABCC_GetErrorCode( psMsg ),
         "Unexpected error response %" PRIu8 "\n",
         ABCC_GetErrorCode( psMsg ) );
      return( ABCC_CMDSEQ_RESP_ABORT );
   }

   pnABCC_DrvSetPdSize( abcc_iPdReadSize, abcc_iPdWriteSize );
   ABCC_LOG_INFO( "RSP MSG_SETUP_COMPLETE\n" );
   return( ABCC_CMDSEQ_RESP_EXEC_NEXT );
}

static void SetupDone( const ABCC_CmdSeqResultType eSeqResult, void* pxUserData )
{
   (void)pxUserData;

   switch( eSeqResult )
   {
   case ABCC_CMDSEQ_RESULT_COMPLETED:
      ABCC_LOG_INFO( "Mapped PD size, RdPd %" PRIu16 " WrPd: %" PRIu16 "\n", abcc_iPdReadSize, abcc_iPdWriteSize );
      break;

   case ABCC_CMDSEQ_RESULT_ABORT_INT:
      ABCC_LOG_WARNING( ABCC_EC_SETUP_FAILED,
         (UINT32)eSeqResult,
         "SetupDone reported internally aborted command sequence. PD mapping can be incomplete.\n" );
      break;

   case ABCC_CMDSEQ_RESULT_ABORT_EXT:
      ABCC_LOG_WARNING( ABCC_EC_SETUP_FAILED,
         (UINT32)eSeqResult,
         "SetupDone reported externally aborted command sequence. PD mapping can be incomplete.\n" );
      break;

   default:
      ABCC_LOG_WARNING( ABCC_EC_SETUP_FAILED,
         (UINT32)eSeqResult,
         "SetupDone reported aborted command sequence. PD mapping can be incomplete.\n" );
      break;
   }
}

#if !ABCC_CFG_DRV_CMD_SEQ_ENABLED
/*------------------------------------------------------------------------------
** Handles responses for setup messages.
** Depending on main setup state and sub state the corresponding response
** handler is called.
** In state SETUP_BEFORE_USER_INIT SetupSeqBeforeUserInit[] sequence is used.
** In state SETUP_AFTER_USER_INIT SetupSeqAfterUserInit[] sequence is used.
** bSetupSubState is used as index in the sequence array.
**
** When the response is handled the next setup command is triggered.
**------------------------------------------------------------------------------
** Arguments:
**    psMsg                   - Pointer to response buffer
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
static void HandleSetupRespone( ABP_MsgType* psMsg )
{
   if( pasSetupSeq[ bSetupSubState ].pnRespHandler( psMsg, NULL ) == ABCC_CMDSEQ_RESP_EXEC_NEXT )
   {
      bSetupSubState++;
   }
   SendSetupCommand( psMsg );
}

/*------------------------------------------------------------------------------
** Send the next command in the setup sequence.
** Depending on main setup state and sub state the corresponding command handler
** is called.
** In state SETUP_BEFORE_USER_INIT SetupSeqBeforeUserInit[] sequence is used.
** In state SETUP_AFTER_USER_INIT SetupSeqAfterUserInit[] sequence is used.
** bSetupSubState is used as index in the sequence array.
**
** At the end of SETUP_BEFORE_USER_INIT ABCC_CbfUserInitReq() callback
** is triggered.
** At the end of SETUP_AFTER_USER_INIT the setup is done.
**------------------------------------------------------------------------------
** Arguments:
**    psMsg                   - Pointer to response buffer
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
static void SendSetupCommand( ABP_MsgType* psMsg )
{
   while( pasSetupSeq[ bSetupSubState ].pnCmdHandler != NULL )
   {
      if( pasSetupSeq[ bSetupSubState ].pnCmdHandler( psMsg, NULL ) == ABCC_CMDSEQ_CMD_SKIP )
      {
         bSetupSubState++;
      }
      else
      {
         ABCC_SendCmdMsg( psMsg, HandleSetupRespone );
         break;
      }
   }

   if( pasSetupSeq[ bSetupSubState ].pnCmdHandler == NULL )
   {
      if( eSetupState == SETUP_BEFORE_USER_INIT )
      {
         eSetupState = SETUP_USER_INIT;
         TriggerUserInit( ABCC_CMDSEQ_RESULT_COMPLETED, NULL );
      }
      else
      {
         eSetupState = SETUP_DONE;
         SetupDone( ABCC_CMDSEQ_RESULT_COMPLETED, NULL );
      }
   }
}
#endif

#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
void ABCC_StartSetup( void )
{
   abcc_fFirstCommandPending = TRUE;
   ABCC_CmdSeqAdd( SetupSeqBeforeUserInit, TriggerUserInit, NULL, NULL );
}

void ABCC_UserInitComplete( void )
{
   ABCC_CmdSeqAdd( SetupSeqAfterUserInit, SetupDone, NULL, NULL );
}
#else
void ABCC_StartSetup( void )
{
   ABP_MsgType* psMsg;
   abcc_fFirstCommandPending = TRUE;
   eSetupState = SETUP_BEFORE_USER_INIT;
   bSetupSubState = 0;
   pasSetupSeq = SetupSeqBeforeUserInit;
   psMsg = ABCC_GetCmdMsgBuffer();
   if( !psMsg )
   {
      ABCC_LOG_ERROR( ABCC_EC_NO_RESOURCES, 0, "No resources for setup\n" );
      return;
   }
   SendSetupCommand( psMsg );
}

void ABCC_UserInitComplete( void )
{
   ABP_MsgType* psMsg;
   psMsg = ABCC_GetCmdMsgBuffer();
if( !psMsg )
   {
      ABCC_LOG_ERROR( ABCC_EC_NO_RESOURCES, 0, "No resources for setup\n" );
      return;
   }
   eSetupState = SETUP_AFTER_USER_INIT;
   bSetupSubState = 0;
   pasSetupSeq = SetupSeqAfterUserInit;
   SendSetupCommand( psMsg );
}
#endif

ABCC_FwVersionType ABCC_FirmwareVersion( void )
{
   return( abcc_sFwVersion );
}

UINT16 ABCC_NetworkType( void )
{
   return( abcc_iNetworkType );
}

UINT16 ABCC_ModuleType( void )
{
   return( abcc_iModuleType );
}

ABCC_NetFormatType ABCC_NetFormat( void )
{
   return( abcc_eNetFormat );
}

ABCC_ParameterSupportType ABCC_ParameterSupport( void )
{
   return( abcc_eParameterSupport );
}
