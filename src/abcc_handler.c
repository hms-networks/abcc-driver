/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implementation of operation mode independent parts of the abcc handler.
********************************************************************************
*/

#include "abcc_config.h"
#include "abcc_types.h"
#include "abcc_driver_interface.h"
#include "abp.h"
#include "abcc.h"
#include "abcc_link.h"
#include "abcc_command_sequencer.h"
#include "abcc_command_sequencer_interface.h"
#include "abcc_memory.h"
#include "abcc_hardware_abstraction.h"
#include "abcc_log.h"
#include "abcc_handler.h"
#include "abcc_timer.h"
#include "abcc_setup.h"
#include "abcc_port.h"
#include "abcc_segmentation.h"

#if ABCC_CFG_DRV_SPI_ENABLED
#include "spi/abcc_driver_spi_interface.h"
#endif
#if ABCC_CFG_DRV_PARALLEL_ENABLED
#include "par/abcc_driver_parallel_interface.h"
#endif
#if ABCC_CFG_DRV_SERIAL_ENABLED
#include "serial/abcc_driver_serial_interface.h"
#endif

#if !ABCC_CFG_OP_MODE_GETTABLE
   #ifdef ABCC_CFG_ABCC_OP_MODE
      #if ( ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_SPI ) &&             \
            ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_16_BIT_PARALLEL ) && \
            ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_8_BIT_PARALLEL ) &&  \
            ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_SERIAL_19_2 ) &&     \
            ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_SERIAL_57_6 ) &&     \
            ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_SERIAL_115_2 ) &&    \
            ( ABCC_CFG_ABCC_OP_MODE != ABP_OP_MODE_SERIAL_625 ) )
         #error "No valid Operation Mode has been selected for ABCC operation! Check abcc_driver_config.h."
      #endif
   #endif
#endif

/*
** Registerd handler functions
*/
EXTFUNC void ABCC_SpiRunDriver( void );
EXTFUNC void ABCC_SpiISR( void );

EXTFUNC void ABCC_ParRunDriver( void );
EXTFUNC void ABCC_ParISR( void );

EXTFUNC void ABCC_SerRunDriver( void );
EXTFUNC void ABCC_SerISR( void );

/*
** Registered handler functions
*/
void ( *ABCC_ISR )( void );
void ( *ABCC_TriggerWrPdUpdate )( void );

/*
** The interrupt mask that has been set to the ABCC at start up.
*/
UINT16 ABCC_iInterruptEnableMask;

/*
** Registerd driver functions
*/
void ( *pnABCC_DrvRun )( void );
void  ( *pnABCC_DrvInit )( UINT8 bOpmode );
UINT16 ( *pnABCC_DrvISR )( void );
void ( *pnABCC_DrvRunDriverTx )( void );
ABP_MsgType* ( *pnABCC_DrvRunDriverRx )( void );
void ( *pnABCC_DrvPrepareWriteMessage ) ( ABP_MsgType* psWriteMsg );
BOOL ( *pnABCC_DrvWriteMessage ) ( ABP_MsgType* psWriteMsg );
void ( *pnABCC_DrvWriteProcessData )( void* pbProcessData );
BOOL ( *pnABCC_DrvISReadyForWrPd )( void );
BOOL ( *pnABCC_DrvISReadyForWriteMessage )( void );
BOOL ( *pnABCC_DrvISReadyForCmd )( void );
void ( *pnABCC_DrvSetNbrOfCmds )( UINT8 bNbrOfCmds );
void ( *pnABCC_DrvSetAppStatus )( ABP_AppStatusType eAppStatus );
void ( *pnABCC_DrvSetPdSize )( const UINT16 iReadPdSize, const UINT16 iWritePdSize );
void ( *pnABCC_DrvSetIntMask )( const UINT16 iIntMask );
void* ( *pnABCC_DrvGetWrPdBuffer )( void );
UINT16 ( *pnABCC_DrvGetModCap )( void );
UINT16 ( *pnABCC_DrvGetLedStatus )( void );
UINT16 ( *pnABCC_DrvGetIntStatus )( void );
UINT8 ( *pnABCC_DrvGetAnybusState )( void );
void* ( *pnABCC_DrvReadProcessData )( void );
ABP_MsgType* ( *pnABCC_DrvReadMessage )( void );
BOOL ( *pnABCC_DrvIsSupervised )( void );
UINT8 ( *pnABCC_DrvGetAnbStatus )( void );

#if ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
BOOL fAbccUserSyncMeasurementIp;
#endif

static volatile UINT8 abcc_bAnbState = 0xff;

static ABCC_MainStateType abcc_eMainState = ABCC_DRV_INIT;

/*
** Last error.
*/
static ABCC_ErrorCodeType abcc_eLastErrorCode;
static UINT32 abcc_lLastAdditionalInfo;

#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
static BOOL abcc_fFwUpdateAttempted;
#endif

/*
** Pointer to WRPD buffer.
*/
static void* abcc_pbWrPdBuffer;

/*
** Tmo handler for
*/
static ABCC_TimerHandle abcc_TmoHandle;

/*
** Indicate ready for communication
*/
static BOOL abcc_fReadyForCommunicationTmo = FALSE;
static BOOL abcc_fReadyForCommunication = FALSE;

/*
** Current operation mode
*/
static UINT8 abcc_bOpmode = 0;

#if ( ABCC_CFG_DRV_SPI_ENABLED || ABCC_CFG_DRV_SERIAL_ENABLED )
/*
** Flag to indicate that WrPD update shall be done
*/
static BOOL abcc_fDoWrPdUpdate = FALSE;
#endif

/*
** The Application status register value of the Anybus module
*/
static volatile ABP_AppStatusType abcc_eAppStatus = ABP_APPSTAT_NO_ERROR;

/*
** Message channel size.
*/
static UINT16 abcc_iMessageChannelSize = 0;

static void TriggerWrPdUpdateNow( void )
{
   if( ABCC_GetMainState() == ABCC_DRV_RUNNING )
   {
      /*
      ** Send new "write process data" to the Anybus-CC.
      ** The data format of the process data is network specific.
      ** The application converts the data accordingly.
      */

      if( ABCC_CbfUpdateWriteProcessData( abcc_pbWrPdBuffer ) )
      {
         pnABCC_DrvWriteProcessData( abcc_pbWrPdBuffer );
#if ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
         if( ABCC_GetOpmode() == ABP_OP_MODE_SPI )
         {
            fAbccUserSyncMeasurementIp = TRUE;
         }
         else
         {
            ABCC_HAL_GpioReset();
         }
#endif
      }
   }
}

static void SetMainState( ABCC_MainStateType eState )
{
#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_INFO_ENABLED
   static const char* pacMainStateToText[] =
   {
      "ABCC_DRV_INIT",
      "ABCC_DRV_SHUTDOWN",
      "ABCC_DRV_ERROR",
      "ABCC_DRV_WAIT_COMMUNICATION_RDY",
      "ABCC_DRV_SETUP",
      "ABCC_DRV_RUNNING"
   };
#endif
   ABCC_LOG_INFO( "Driver main state: %s\n", pacMainStateToText[ eState ] );
   abcc_eMainState = eState;
}

#if ( ABCC_CFG_DRV_SPI_ENABLED || ABCC_CFG_DRV_SERIAL_ENABLED )
static void TriggerWrPdUpdateLater( void )
{
   abcc_fDoWrPdUpdate = TRUE;
}
#endif

static BOOL IsInterruptInUse( void )
{
   BOOL fReturn;

   fReturn = FALSE;
#if ABCC_CFG_INT_ENABLED
   switch( abcc_bOpmode )
   {
   case ABP_OP_MODE_16_BIT_PARALLEL:
   case ABP_OP_MODE_8_BIT_PARALLEL:
   case ABP_OP_MODE_SPI:
      fReturn = TRUE;
      break;

   default:
      break;
   }

   return( fReturn );
#else
   return( fReturn );
#endif /* End of #if ABCC_CFG_INT_ENABLED */
}

static BOOL IsPolledInterruptInUse( void )
{
   BOOL fReturn;

   fReturn = FALSE;
#if ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED
   switch( abcc_bOpmode )
   {
   case ABP_OP_MODE_16_BIT_PARALLEL:
   case ABP_OP_MODE_8_BIT_PARALLEL:
   case ABP_OP_MODE_SPI:
      fReturn = TRUE;
      break;

   default:

      break;
   }

   return( fReturn );
#else
   return( fReturn );
#endif /* End of #if ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED */
}

static void SetReadyForCommunicationTmo( void )
{
   abcc_fReadyForCommunicationTmo = TRUE;
}

#if ( ABCC_CFG_DRV_SPI_ENABLED || ABCC_CFG_DRV_SERIAL_ENABLED )
void ABCC_CheckWrPdUpdate( void )
{
   if( abcc_fDoWrPdUpdate && pnABCC_DrvISReadyForWrPd() )
   {
      abcc_fDoWrPdUpdate = FALSE;
      TriggerWrPdUpdateNow();
   }
}
#endif

void ABCC_SetReadyForCommunication( void )
{
   abcc_fReadyForCommunication = TRUE;
}

void ABCC_SetError( ABCC_LogSeverityType eSeverity, ABCC_ErrorCodeType eErrorCode, UINT32 lAdditionalInfo )
{
   if( eSeverity == ABCC_LOG_SEVERITY_ERROR )
   {
      abcc_eLastErrorCode = eErrorCode;
      abcc_lLastAdditionalInfo = lAdditionalInfo;

      SetMainState( ABCC_DRV_ERROR );
   }

   ABCC_CbfDriverError( eSeverity, eErrorCode, lAdditionalInfo );

   if( eSeverity == ABCC_LOG_SEVERITY_FATAL )
   {
      ABCC_PORT_printf( "FATAL: Enter endless loop\n" );
      while( 1 );
   }
}

ABCC_MainStateType ABCC_GetMainState( void )
{
   return( abcc_eMainState );
}

UINT16 ABCC_GetMessageChannelSize( void )
{
   return( abcc_iMessageChannelSize );
}

UINT16 ABCC_GetMaxMessageSize( void )
{
   return( abcc_iMessageChannelSize < ABCC_CFG_MAX_MSG_SIZE ?
      abcc_iMessageChannelSize : ABCC_CFG_MAX_MSG_SIZE );
}

void ABCC_TriggerAnbStatusUpdate( void )
{
   UINT8 bAnbState;

   bAnbState = pnABCC_DrvGetAnybusState();
   if( bAnbState != abcc_bAnbState )
   {
      abcc_bAnbState = bAnbState;
      ABCC_LOG_DEBUG_MSG_GENERAL( "HEXDUMP_STATE:%02x\n", abcc_bAnbState );
      ABCC_CbfAnbStateChanged( (ABP_AnbStateType)bAnbState );
   }
}

void ABCC_TriggerTransmitMessage( void )
{
   ABCC_LinkCheckSendMessage();
}

#if ( ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED || ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED )
void ABCC_GpioReset( void )
{
   ABCC_HAL_GpioReset();
}
#endif

#if ( ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED || ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED )
void ABCC_GpioSet( void )
{
   ABCC_HAL_GpioSet();
}
#endif

ABCC_ErrorCodeType ABCC_HwInit( void )
{
   if( !ABCC_HAL_HwInit() )
   {
      return( ABCC_EC_HW_INIT_FAILED );
   }
   return( ABCC_EC_NO_ERROR );
}


ABCC_ErrorCodeType ABCC_StartDriver( UINT32 lMaxStartupTimeMs )
{
   UINT8 bModuleId;

   if( lMaxStartupTimeMs == 0 )
   {
      lMaxStartupTimeMs = ABCC_CFG_STARTUP_TIME_MS;
   }

   bModuleId = ABCC_ReadModuleId();

#if ( ABCC_CFG_DRV_SERIAL_ENABLED )
   if( bModuleId != ABP_MODULE_ID_ACTIVE_ABCC40  )
#elif( ABCC_CFG_DRV_SPI_ENABLED || ABCC_CFG_DRV_PARALLEL_ENABLED )
   if( bModuleId != ABP_MODULE_ID_ACTIVE_ABCC40 )
#endif
   {
      ABCC_LOG_ERROR( ABCC_EC_MODULE_ID_NOT_SUPPORTED,
         bModuleId,
         "Module ID not supported: 0x%" PRIx8 "\n",
         bModuleId );

      return( ABCC_EC_MODULE_ID_NOT_SUPPORTED );
   }

   abcc_bOpmode = ABCC_GetOpmode();

   switch( abcc_bOpmode )
   {
#if ABCC_CFG_DRV_SERIAL_ENABLED
   case ABP_OP_MODE_SERIAL_19_2:
   case ABP_OP_MODE_SERIAL_57_6:
   case ABP_OP_MODE_SERIAL_115_2:
   case ABP_OP_MODE_SERIAL_625:

      ABCC_ISR                   = NULL;
      ABCC_TriggerWrPdUpdate     = &TriggerWrPdUpdateLater;

      pnABCC_DrvRun              = &ABCC_SerRunDriver;
      pnABCC_DrvInit               = &ABCC_DrvSerInit;
      pnABCC_DrvISR                = &ABCC_DrvSerISR;
      pnABCC_DrvRunDriverTx        = &ABCC_DrvSerRunDriverTx;
      pnABCC_DrvRunDriverRx        = &ABCC_DrvSerRunDriverRx;
      pnABCC_DrvPrepareWriteMessage = NULL;
      pnABCC_DrvWriteMessage       = &ABCC_DrvSerWriteMessage;
      pnABCC_DrvWriteProcessData   = &ABCC_DrvSerWriteProcessData;
      pnABCC_DrvISReadyForWrPd     = &ABCC_DrvSerIsReadyForWrPd;
      pnABCC_DrvISReadyForWriteMessage = &ABCC_DrvSerIsReadyForWriteMessage;
      pnABCC_DrvISReadyForCmd      = &ABCC_DrvSerIsReadyForCmd;
      pnABCC_DrvSetNbrOfCmds       = &ABCC_DrvSerSetNbrOfCmds;
      pnABCC_DrvSetAppStatus       = &ABCC_DrvSerSetAppStatus;
      pnABCC_DrvSetPdSize          = &ABCC_DrvSerSetPdSize;
      pnABCC_DrvSetIntMask         = &ABCC_DrvSerSetIntMask;
      pnABCC_DrvGetWrPdBuffer      = &ABCC_DrvSerGetWrPdBuffer;
      pnABCC_DrvGetModCap          = &ABCC_DrvSerGetModCap;
      pnABCC_DrvGetLedStatus       = &ABCC_DrvSerGetLedStatus;
      pnABCC_DrvGetIntStatus       = &ABCC_DrvSerGetIntStatus;
      pnABCC_DrvGetAnybusState     = &ABCC_DrvSerGetAnybusState;
      pnABCC_DrvReadProcessData    = &ABCC_DrvSerReadProcessData;
      pnABCC_DrvReadMessage        = &ABCC_DrvSerReadMessage;
      pnABCC_DrvIsSupervised       = &ABCC_DrvSerIsSupervised;
      pnABCC_DrvGetAnbStatus       = &ABCC_DrvSerGetAnbStatus;

      ABCC_iInterruptEnableMask = 0;
      abcc_iMessageChannelSize = ABP_MAX_MSG_255_DATA_BYTES;

      break;
#endif /* End of #if ABCC_CFG_DRV_SERIAL_ENABLED */
#if ABCC_CFG_DRV_SPI_ENABLED
   case ABP_OP_MODE_SPI:

      ABCC_ISR                   = &ABCC_SpiISR;
      ABCC_TriggerWrPdUpdate     = &TriggerWrPdUpdateLater;

      pnABCC_DrvRun                = &ABCC_SpiRunDriver;
      pnABCC_DrvInit               = &ABCC_DrvSpiInit;
      pnABCC_DrvISR                = NULL;
      pnABCC_DrvRunDriverTx        = &ABCC_DrvSpiRunDriverTx;
      pnABCC_DrvRunDriverRx        = &ABCC_DrvSpiRunDriverRx;
      pnABCC_DrvPrepareWriteMessage = NULL;
      pnABCC_DrvWriteMessage       = &ABCC_DrvSpiWriteMessage;
      pnABCC_DrvWriteProcessData   = &ABCC_DrvSpiWriteProcessData;
      pnABCC_DrvISReadyForWrPd     = &ABCC_DrvSpiIsReadyForWrPd;
      pnABCC_DrvISReadyForWriteMessage = &ABCC_DrvSpiIsReadyForWriteMessage;
      pnABCC_DrvISReadyForCmd      = &ABCC_DrvSpiIsReadyForCmd;
      pnABCC_DrvSetNbrOfCmds       = &ABCC_DrvSpiSetNbrOfCmds;
      pnABCC_DrvSetAppStatus       = &ABCC_DrvSpiSetAppStatus;
      pnABCC_DrvSetPdSize          = &ABCC_DrvSpiSetPdSize;
      pnABCC_DrvSetIntMask         = &ABCC_DrvSpiSetIntMask;
      pnABCC_DrvGetWrPdBuffer      = &ABCC_DrvSpiGetWrPdBuffer;
      pnABCC_DrvGetModCap          = &ABCC_DrvSpiGetModCap;
      pnABCC_DrvGetLedStatus       = &ABCC_DrvSpiGetLedStatus;
      pnABCC_DrvGetIntStatus       = &ABCC_DrvSpiGetIntStatus;
      pnABCC_DrvGetAnybusState     = &ABCC_DrvSpiGetAnybusState;
      pnABCC_DrvReadProcessData    = &ABCC_DrvSpiReadProcessData;
      pnABCC_DrvReadMessage        = &ABCC_DrvSpiReadMessage;
      pnABCC_DrvIsSupervised       = &ABCC_DrvSpiIsSupervised;
      pnABCC_DrvGetAnbStatus       = &ABCC_DrvSpiGetAnbStatus;

      ABCC_iInterruptEnableMask = ABCC_CFG_INT_ENABLE_MASK_SPI;
      abcc_iMessageChannelSize = ABP_MAX_MSG_DATA_BYTES;

      break;
#endif /* End of #if ABCC_CFG_DRV_SPI_ENABLED */
#if ABCC_CFG_DRV_PARALLEL_ENABLED
   case ABP_OP_MODE_8_BIT_PARALLEL:
   case ABP_OP_MODE_16_BIT_PARALLEL:

      ABCC_ISR                   = &ABCC_ParISR;
      ABCC_TriggerWrPdUpdate     = &TriggerWrPdUpdateNow;

      pnABCC_DrvRun                = &ABCC_ParRunDriver;
      pnABCC_DrvInit               = &ABCC_DrvParInit;
      pnABCC_DrvISR                = &ABCC_DrvParISR;
      pnABCC_DrvRunDriverTx        = NULL;
      pnABCC_DrvRunDriverRx        = &ABCC_DrvParRunDriverRx;
      pnABCC_DrvPrepareWriteMessage = &ABCC_DrvParPrepareWriteMessage;
      pnABCC_DrvWriteMessage       = &ABCC_DrvParWriteMessage;
      pnABCC_DrvWriteProcessData   = &ABCC_DrvParWriteProcessData;
      pnABCC_DrvISReadyForWrPd     = &ABCC_DrvParIsReadyForWrPd;
      pnABCC_DrvISReadyForWriteMessage = &ABCC_DrvParIsReadyForWriteMessage;
      pnABCC_DrvISReadyForCmd      = &ABCC_DrvParIsReadyForCmd;
      pnABCC_DrvSetNbrOfCmds       = &ABCC_DrvParSetNbrOfCmds;
      pnABCC_DrvSetAppStatus       = &ABCC_DrvParSetAppStatus;
      pnABCC_DrvSetPdSize          = &ABCC_DrvParSetPdSize;
      pnABCC_DrvSetIntMask         = &ABCC_DrvParSetIntMask;
      pnABCC_DrvGetWrPdBuffer      = &ABCC_DrvParGetWrPdBuffer;
      pnABCC_DrvGetModCap          = &ABCC_DrvParGetModCap;
      pnABCC_DrvGetLedStatus       = &ABCC_DrvParGetLedStatus;
      pnABCC_DrvGetIntStatus       = &ABCC_DrvParGetIntStatus;
      pnABCC_DrvGetAnybusState     = &ABCC_DrvParGetAnybusState;
      pnABCC_DrvReadProcessData    = &ABCC_DrvParReadProcessData;
      pnABCC_DrvReadMessage        = &ABCC_DrvParReadMessage;
      pnABCC_DrvIsSupervised       = &ABCC_DrvParIsSupervised;
      pnABCC_DrvGetAnbStatus       = &ABCC_DrvParGetAnbStatus;

      abcc_iMessageChannelSize = ABP_MAX_MSG_DATA_BYTES;

#if ABCC_CFG_INT_ENABLED
      ABCC_iInterruptEnableMask = ABCC_CFG_INT_ENABLE_MASK_PAR;

#if ABCC_CFG_SYNC_ENABLED && !ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED
      ABCC_iInterruptEnableMask |= ABP_INTMASK_SYNCIEN;
#endif
#else
      ABCC_iInterruptEnableMask = 0;
#endif

      break;
#endif /* End of #if ABCC_CFG_DRV_PARALLEL_ENABLED */
   default:
      ABCC_LOG_ERROR( ABCC_EC_INCORRECT_OPERATING_MODE, (UINT32)abcc_bOpmode, "Incorrect operating mode: %" PRIu8 "\n", abcc_bOpmode );

      return( ABCC_EC_INCORRECT_OPERATING_MODE );
   }

   if( !( ( abcc_eMainState == ABCC_DRV_INIT ) ||
          ( abcc_eMainState == ABCC_DRV_SHUTDOWN ) ) )
   {
      ABCC_LOG_ERROR( ABCC_EC_INCORRECT_STATE, (UINT32)abcc_eMainState, "Incorrect state: %d\n", abcc_eMainState );
      SetMainState( ABCC_DRV_ERROR );

      return( ABCC_EC_INCORRECT_STATE );
   }

   if( !ABCC_HAL_Init() )
   {
      return( ABCC_EC_INTERNAL_ERROR );
   }

   ABCC_TimerInit();
   pnABCC_DrvInit( abcc_bOpmode );

   ABCC_LinkInit();
#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
   ABCC_CmdSequencerInit();
#endif
   ABCC_SetupInit();
   ABCC_SegmentationInit();

   abcc_bAnbState = 0xff;

   abcc_TmoHandle = ABCC_TimerCreate( SetReadyForCommunicationTmo );

   abcc_pbWrPdBuffer = pnABCC_DrvGetWrPdBuffer();

   if( !ABCC_ModuleDetect() )
   {
      ABCC_LOG_ERROR( ABCC_EC_MODULE_NOT_DECTECTED, 0, "Module not detected\n" );

      return( ABCC_EC_MODULE_NOT_DECTECTED );
   }

#if ABCC_CFG_OP_MODE_SETTABLE
   ABCC_HAL_SetOpmode( abcc_bOpmode );
#endif

   abcc_fReadyForCommunicationTmo = FALSE;
   abcc_fReadyForCommunication = FALSE;
#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
   abcc_fFwUpdateAttempted = FALSE;
#endif

#if ( ABCC_CFG_SYNC_ENABLED && ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED )
   ABCC_HAL_SyncInterruptEnable();
#endif

#if ABCC_CFG_INT_ENABLED
   if( IsInterruptInUse() )
   {
         ABCC_HAL_AbccInterruptEnable();
   }
#endif /* End of #if ABCC_CFG_INT_ENABLED */

   SetMainState( ABCC_DRV_WAIT_COMMUNICATION_RDY );

   ABCC_TimerStart( abcc_TmoHandle, lMaxStartupTimeMs );

   return( ABCC_EC_NO_ERROR );
}

#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
BOOL ABCC_WaitForFwUpdate( UINT32 lTimeoutMs )
{
   if( abcc_fFwUpdateAttempted )
   {
      return( FALSE );
   }
   abcc_fFwUpdateAttempted = TRUE;

   SetMainState( ABCC_DRV_WAIT_COMMUNICATION_RDY );
   abcc_fReadyForCommunication = FALSE;
   abcc_fReadyForCommunicationTmo = FALSE;
#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
   ABCC_CmdSeqAbort( NULL );
#endif
   ABCC_TimerStart( abcc_TmoHandle, lTimeoutMs );

   return( TRUE );
}
#endif

ABCC_CommunicationStateType ABCC_isReadyForCommunication( void )
{
   if( abcc_eMainState > ABCC_DRV_WAIT_COMMUNICATION_RDY )
   {
      return( ABCC_READY_FOR_COMMUNICATION );
   }

   if( abcc_eMainState < ABCC_DRV_WAIT_COMMUNICATION_RDY )
   {
      return( ABCC_NOT_READY_FOR_COMMUNICATION );
   }

   if( abcc_fReadyForCommunicationTmo == TRUE )
   {
      if( IsInterruptInUse() || IsPolledInterruptInUse() )
      {
#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
         if( abcc_fFwUpdateAttempted )
#endif
         {
            return( ABCC_STARTUP_TIMEOUT );
         }
#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
         else
         {
            return( ABCC_ASSUME_FW_UPDATE );
         }
#endif
      }
      else
      {
         abcc_fReadyForCommunication = TRUE;
      }
   }

#if ( ( !ABCC_CFG_INT_ENABLED ) && ( ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED ) )
   if( IsPolledInterruptInUse() )
   {
      abcc_fReadyForCommunication = ABCC_HAL_IsAbccInterruptActive();
   }
#endif

   if( abcc_fReadyForCommunication == TRUE )
   {
      pnABCC_DrvSetIntMask( ABCC_iInterruptEnableMask );
      SetMainState( ABCC_DRV_RUNNING );
      pnABCC_DrvSetNbrOfCmds( ABCC_CFG_MAX_NUM_APPL_CMDS );

      ABCC_StartSetup();
      return( ABCC_READY_FOR_COMMUNICATION );
   }

   return( ABCC_NOT_READY_FOR_COMMUNICATION );
}


void ABCC_TriggerRdPdUpdate( void )
{
   void* bpRdPd;

#if ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED
   ABCC_HAL_GpioSet();
#endif

   bpRdPd = pnABCC_DrvReadProcessData();

   if( bpRdPd )
   {
      if( pnABCC_DrvGetAnybusState() == ABP_ANB_STATE_PROCESS_ACTIVE  )
      {
         /*
         ** The "read process data" is only valid in the PROCESS_ACTIVE state.
         ** Retrieve the new "read process data" from the Anybus-CC.
         ** The data format of the process data is network specific.
         ** Convert it to our native format.
         */
         ABCC_CbfNewReadPd( bpRdPd );
      }
   }

#if ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED
   /*
   ** This is the Output Valid point (for OuputValidTime = 0). The
   ** applications has received data and handled it. Thus we reset the
   ** ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED measurement.
   */
   ABCC_HAL_GpioReset();
#endif
}

void ABCC_TriggerReceiveMessage ( void )
{
   ABCC_MsgType sRdMsg;

   sRdMsg.psMsg = ABCC_LinkReadMessage();

   if( sRdMsg.psMsg == NULL )
   {
      return;
   }

   ABCC_LOG_DEBUG_HEXDUMP_MSG_RX( sRdMsg.psMsg );
   ABCC_LOG_DEBUG_MSG_CONTENT( sRdMsg.psMsg, "Msg received\n" );
   /*
   ** Set buffer status to indicate that the buffer is handed over to the
   ** application.
   */
   ABCC_MemSetBufferStatus( sRdMsg.psMsg, ABCC_MEM_BUFSTAT_IN_APPL_HANDLER );
   /*
   ** A new message is available.
   */
   if( ABCC_GetLowAddrOct( sRdMsg.psMsg16->sHeader.iCmdReserved ) & ABP_MSG_HEADER_C_BIT )
   {
      /*
      ** Check so that messages exceeding ABCC_CFG_MAX_MSG_SIZE are handled. The
      ** actual buffer protection is done in the operating mode driver.
      */
      if( ABCC_GetMsgDataSize( sRdMsg.psMsg ) > ABCC_CFG_MAX_MSG_SIZE )
      {
         ABCC_LOG_WARNING( ABCC_EC_RCV_CMD_SIZE_EXCEEDS_BUFFER,
            (UINT32)ABCC_GetMsgDataSize( sRdMsg.psMsg ),
            "Received command size exceeds buffer size: %" PRIu16 "\n",
            ABCC_GetMsgDataSize( sRdMsg.psMsg ) );
         ABP_SetMsgErrorResponse( sRdMsg.psMsg, 1, ABP_ERR_NO_RESOURCES );
         ABCC_SendRespMsg( sRdMsg.psMsg );
      }
      else
      {

         /*
         ** Check if there is a related active segmentation session.
         */
         if( !ABCC_HandleSegmentAck( sRdMsg.psMsg ) )
         {
            /*
            ** The message is a new command, let the application respond.
            */
            ABCC_CbfHandleCommandMessage( sRdMsg.psMsg );
         }
      }
   }
   else
   {
      /*
      ** Check so that messages exceeding ABCC_CFG_MAX_MSG_SIZE are handled.
      */
      if( ABCC_GetMsgDataSize( sRdMsg.psMsg ) > ABCC_CFG_MAX_MSG_SIZE )
      {
         (void)ABCC_LinkGetMsgHandler( ABCC_GetLowAddrOct( sRdMsg.psMsg16->sHeader.iSourceIdDestObj ) );
         ABCC_LOG_WARNING( ABCC_EC_RCV_RESP_SIZE_EXCEEDS_BUFFER,
            (UINT32)ABCC_GetMsgDataSize( sRdMsg.psMsg ),
            "Received response size exceeds buffer size: %" PRIu16 "\n",
            ABCC_GetMsgDataSize( sRdMsg.psMsg ) );
      }
      else
      {
         ABCC_MsgHandlerFuncType pnMsgHandler = 0;
         pnMsgHandler = ABCC_LinkGetMsgHandler( ABCC_GetLowAddrOct( sRdMsg.psMsg16->sHeader.iSourceIdDestObj ) );

         if( pnMsgHandler )
         {
            ABCC_LOG_DEBUG_MSG_EVENT( sRdMsg.psMsg, "Routing response to registered response handler: " );
            pnMsgHandler( sRdMsg.psMsg );
         }
         else
         {
            ABCC_LOG_DEBUG_MSG_EVENT( sRdMsg.psMsg, "No response handler found" );
         }
      }
   }

   if( ABCC_MemGetBufferStatus( sRdMsg.psMsg ) == ABCC_MEM_BUFSTAT_IN_APPL_HANDLER )
   {
      /*
      ** The status has not been changed while the user processed the response
      ** message. Then this buffer shall be freed by the driver.
      */
      ABCC_ReturnMsgBuffer( &sRdMsg.psMsg );
   }
}

ABCC_ErrorCodeType ABCC_SendCmdMsg( ABP_MsgType*  psCmdMsg, ABCC_MsgHandlerFuncType pnMsgHandler )
{
   ABCC_ErrorCodeType eResult;
   ABCC_MsgType sMsg;

   sMsg.psMsg = psCmdMsg;

   /*
   ** Register function to handle response.
   ** Must be done before sending the message to avoid race condition.
   */
   if( ABCC_LinkMapMsgHandler( ABCC_GetLowAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj ),
                               pnMsgHandler ) == ABCC_EC_NO_ERROR )
   {
      eResult = ABCC_LinkWriteMessage( sMsg.psMsg );
      if( eResult != ABCC_EC_NO_ERROR )
      {
         /*
         ** Free message handler resource
         */
         (void)ABCC_LinkGetMsgHandler( ABCC_GetLowAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj ) );
      }
   }
   else
   {
      eResult = ABCC_EC_NO_RESOURCES;

      /*
      ** Report error
      */
      ABCC_LOG_WARNING( ABCC_EC_NO_RESOURCES, 0, "No resources available to map response handler\n" );
   }

   return( eResult );
}

UINT16 ABCC_GetCmdQueueSize( void )
{
   return( ABCC_LinkGetNumCmdQueueEntries() );
}


ABCC_ErrorCodeType ABCC_SendRespMsg( ABP_MsgType* psMsgResp )
{
   return( ABCC_LinkWriteMessage( psMsgResp ) );
}

ABP_MsgType* ABCC_GetCmdMsgBuffer( void )
{
   if( ABCC_GetCmdQueueSize() == 0 )
   {
      return( NULL );
   }
   return( ABCC_MemAlloc() );
}

ABCC_ErrorCodeType ABCC_ReturnMsgBuffer( ABP_MsgType** ppsBuffer )
{
   ABCC_LinkFree( ppsBuffer );

   return( ABCC_EC_NO_ERROR );
}

void ABCC_TakeMsgBufferOwnership( ABP_MsgType* psMsg )
{
   ABCC_MemSetBufferStatus( psMsg, ABCC_MEM_BUFSTAT_OWNED );
}

void ABCC_SetPdSize( const UINT16 iReadPdSize, const UINT16 iWritePdSize )
{
   ABCC_LOG_INFO( "New process data sizes RdPd %" PRIu16 " WrPd %" PRIu16 "\n", iReadPdSize, iWritePdSize );
   pnABCC_DrvSetPdSize( iReadPdSize, iWritePdSize );
}

ABCC_ErrorCodeType ABCC_RunDriver( void )
{
   if( abcc_eMainState == ABCC_DRV_ERROR )
   {
      return( abcc_eLastErrorCode );
   }

   pnABCC_DrvRun();

   if( abcc_eMainState == ABCC_DRV_ERROR )
   {
      return( abcc_eLastErrorCode );
   }

   return( ABCC_EC_NO_ERROR );
}

void ABCC_HWReset( void )
{
   ABCC_LOG_INFO( "HW Reset\n" );
   ABCC_ShutdownDriver();
   ABCC_HAL_HWReset();
}


void ABCC_ShutdownDriver( void )
{
   ABCC_LOG_INFO( "Enter Shutdown state\n" );

#if ( ABCC_CFG_SYNC_ENABLED && ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED )
   ABCC_HAL_SyncInterruptDisable();
#endif

#if ABCC_CFG_INT_ENABLED
   ABCC_HAL_AbccInterruptDisable();
#endif
   ABCC_HAL_Close();
   ABCC_TimerDisable();
   SetMainState( ABCC_DRV_SHUTDOWN );
}


BOOL ABCC_ModuleDetect( void )
{
#if ABCC_CFG_MOD_DETECT_PINS_CONN
   return( ABCC_HAL_ModuleDetect() );
#else
   return( TRUE );
#endif
}

UINT16 ABCC_ModCap( void )
{
   return( pnABCC_DrvGetModCap() );
}

UINT16 ABCC_LedStatus()
{
   return( pnABCC_DrvGetLedStatus() );
}

UINT8 ABCC_AnbState( void )
{
   return( pnABCC_DrvGetAnybusState() );
}

BOOL ABCC_IsSupervised( void )
{
   return( pnABCC_DrvIsSupervised() );
}

void ABCC_HWReleaseReset( void )
{
   ABCC_LOG_INFO( "Release hardware reset\n" );
   ABCC_HAL_HWReleaseReset();
}

ABP_AppStatusType ABCC_GetAppStatus( void )
{
   return( abcc_eAppStatus );
}

void ABCC_SetAppStatus( ABP_AppStatusType eAppStatus )
{
   if( abcc_eAppStatus != eAppStatus )
   {
      abcc_eAppStatus = eAppStatus;
      pnABCC_DrvSetAppStatus( eAppStatus );
   }
}

UINT8 ABCC_ReadModuleId( void )
{
#if ABCC_CFG_MODULE_ID_PINS_CONN
   return( ABCC_HAL_ReadModuleId() );
#else
   return( ABP_MODULE_ID_ACTIVE_ABCC40 );
#endif
}

void ABCC_RunTimerSystem( const INT16 iDeltaTimeMs )
{
   ABCC_TimerTick( iDeltaTimeMs );
}

UINT64 ABCC_GetUptimeMs( void )
{
   return( ABCC_TimerGetUptimeMs() );
}

UINT8 ABCC_GetNewSourceId( void )
{
   static UINT8 bSourceId = 0;
   UINT8 bTempSrcId;
   ABCC_PORT_UseCritical();

   do
   {
      ABCC_PORT_EnterCritical();
      bTempSrcId = ++bSourceId;
      ABCC_PORT_ExitCritical();
   } while( ABCC_LinkIsSrcIdUsed( bTempSrcId ) );

   return( bTempSrcId );
}

UINT8 ABCC_GetOpmode( void )
{
#if ABCC_CFG_OP_MODE_GETTABLE
   return( ABCC_HAL_GetOpmode() );
#elif ( defined( ABCC_CFG_ABCC_OP_MODE ) )
   return( ABCC_CFG_ABCC_OP_MODE );
#elif defined( ABCC_CFG_ABCC_OP_MODE )
   return( ABCC_CFG_ABCC_OP_MODE );
#else
   /*
   ** The user has not configured any way to determine the operating mode
   */
   #error "No method to determine the operating mode is available. Either set ABCC_CFG_OP_MODE_GETTABLE to 1 or any of ABCC_CFG_ABCC_OP_MODE_X. See descriptions in abcc_config.h for details."
#endif /* End of #if defined( ABCC_CFG_OP_MODE_HW_CONF ) */
}


void ABCC_GetAttribute( ABP_MsgType* psMsg,
                        UINT8 bObject,
                        UINT16 iInstance,
                        UINT8 bAttribute,
                        UINT8 bSourceId )
{
   ABCC_MsgType sMsg;
   sMsg.psMsg = psMsg;

   ABCC_SetLowAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj, bSourceId ); /* SourceId */
   ABCC_SetHighAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj, bObject );  /* bObject */
   psMsg->sHeader.iInstance = iTOiLe( iInstance );                          /* Instance */
   ABCC_SetLowAddrOct(  sMsg.psMsg16->sHeader.iCmdReserved,
                  ABP_MSG_HEADER_C_BIT | ABP_CMD_GET_ATTR );                /* Command */

   sMsg.psMsg16->sHeader.iDataSize = 0;                                     /* Data size           */
   ABCC_SetLowAddrOct( sMsg.psMsg16->sHeader.iCmdExt0CmdExt1, bAttribute ); /* CmdExt0 (Attribute) */
   ABCC_SetHighAddrOct( sMsg.psMsg16->sHeader.iCmdExt0CmdExt1, 0 );         /* CmdExt1 (reserved)  */
}

void ABCC_SetByteAttribute(ABP_MsgType* psMsg,
                           UINT8 bObject,
                           UINT16 iInstance,
                           UINT8 bAttribute,
                           UINT8 bVal,
                           UINT8 bSourceId )
{
   ABCC_MsgType sMsg;
   sMsg.psMsg = psMsg;

   ABCC_SetLowAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj, bSourceId );  /* SourceId */
   ABCC_SetHighAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj, bObject );   /* bObject */
   psMsg->sHeader.iInstance = iTOiLe( iInstance );                           /* Instance */
   ABCC_SetLowAddrOct(  sMsg.psMsg16->sHeader.iCmdReserved,
                  ABP_MSG_HEADER_C_BIT | ABP_CMD_SET_ATTR );                 /* Command */

   sMsg.psMsg16->sHeader.iDataSize = iTOiLe( 1 );                            /* Data size           */
   ABCC_SetLowAddrOct( sMsg.psMsg16->sHeader.iCmdExt0CmdExt1, bAttribute );  /* CmdExt0 (Attribute) */
   ABCC_SetHighAddrOct( sMsg.psMsg16->sHeader.iCmdExt0CmdExt1, 0 );          /* CmdExt1 (reserved)  */
   ABCC_SetLowAddrOct( sMsg.psMsg16->aiData[ 0 ], bVal );                    /* Data                */
}

void ABCC_SetMsgHeader( ABP_MsgType* psMsg,
                        UINT8 bObject,
                        UINT16 iInstance,
                        UINT8 bAttribute,
                        ABP_MsgCmdType eService,
                        UINT16 iDataSize,
                        UINT8 bSourceId )
{
   ABCC_MsgType sMsg;
   sMsg.psMsg = psMsg;

   ABCC_SetLowAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj, bSourceId );  /* SourceId */
   ABCC_SetHighAddrOct( sMsg.psMsg16->sHeader.iSourceIdDestObj, bObject );   /* bObject */
   psMsg->sHeader.iInstance = iTOiLe( iInstance );                           /* Instance */
   ABCC_SetLowAddrOct(  sMsg.psMsg16->sHeader.iCmdReserved,
                        ABP_MSG_HEADER_C_BIT | eService );                   /* Command */

   sMsg.psMsg16->sHeader.iDataSize = iTOiLe( iDataSize );                    /* Data size           */
   ABCC_SetLowAddrOct( sMsg.psMsg16->sHeader.iCmdExt0CmdExt1, bAttribute );  /* CmdExt0 (Attribute) */
   ABCC_SetHighAddrOct( sMsg.psMsg16->sHeader.iCmdExt0CmdExt1, 0 );          /* CmdExt1 (reserved)  */
}

ABCC_ErrorCodeType ABCC_VerifyMessage( const ABP_MsgType* psMsg )
{
   const ABP_MsgType16* psMsg16 = (const ABP_MsgType16*)psMsg;
   if( ABCC_GetLowAddrOct( psMsg16->sHeader.iCmdReserved ) & ABP_MSG_HEADER_E_BIT )
   {
      return( ABCC_EC_RESP_MSG_E_BIT_SET );
   }
   return( ABCC_EC_NO_ERROR );
}

/*------------------------------------------------------------------------------
** ABCC_GetDataTypeSizeInBits()
**------------------------------------------------------------------------------
*/
UINT16 ABCC_GetDataTypeSizeInBits( UINT8 bDataType )
{
   UINT16 iSetBitSize;

   if( ABP_Is_PADx( bDataType ) )
   {
      iSetBitSize = bDataType - ABP_PAD0;
   }
   else if( ABP_Is_BITx( bDataType ) )
   {
      iSetBitSize = ( ( bDataType - ABP_BIT1 ) + 1 );
   }
   else if( bDataType == ABP_BOOL1 )
   {
      iSetBitSize = 1;
   }
   else
   {
      iSetBitSize = (UINT16)ABCC_GetDataTypeSize( bDataType );
      iSetBitSize *= 8;
   }

   return( iSetBitSize );
}

/*------------------------------------------------------------------------------
** ABCC_GetDataTypeSize()
**------------------------------------------------------------------------------
*/

UINT8 ABCC_GetDataTypeSize( UINT8 bDataType )
{
   UINT8 bSize;
   switch( bDataType )
   {

   case ABP_UINT8:
   case ABP_BOOL:
   case ABP_SINT8:
   case ABP_ENUM:
   case ABP_BITS8:
   case ABP_CHAR:
   case ABP_OCTET:
      bSize = ABP_UINT8_SIZEOF;
      break;

   case ABP_UINT16:
   case ABP_BITS16:
   case ABP_SINT16:
      bSize = ABP_UINT16_SIZEOF;
      break;

   case ABP_UINT32:
   case ABP_SINT32:
   case ABP_BITS32:
   case ABP_FLOAT:
      bSize = ABP_UINT32_SIZEOF;
      break;

#if ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED
   case ABP_DOUBLE:
      bSize = ABP_DOUBLE_SIZEOF;
      break;

#endif
#if ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED
   case ABP_SINT64:
   case ABP_UINT64:
      bSize = ABP_UINT64_SIZEOF;
      break;

#endif
   case ABP_BOOL1:
   case ABP_BIT1:
   case ABP_BIT2:
   case ABP_BIT3:
   case ABP_BIT4:
   case ABP_BIT5:
   case ABP_BIT6:
   case ABP_BIT7:
      bSize = ABP_UINT8_SIZEOF;
      break;

   case ABP_PAD0:
      bSize = 0;
      break;

   case ABP_PAD1:
   case ABP_PAD2:
   case ABP_PAD3:
   case ABP_PAD4:
   case ABP_PAD5:
   case ABP_PAD6:
   case ABP_PAD7:
   case ABP_PAD8:
      bSize = ABP_UINT8_SIZEOF;
      break;

   case ABP_PAD9:
   case ABP_PAD10:
   case ABP_PAD11:
   case ABP_PAD12:
   case ABP_PAD13:
   case ABP_PAD14:
   case ABP_PAD15:
   case ABP_PAD16:
      bSize = ABP_UINT16_SIZEOF;
      break;

   default:
      ABCC_LOG_WARNING( ABCC_EC_UNSUPPORTED_DATA_TYPE, (UINT32)bDataType, "Unsupported data type: %" PRIu8 "\n", bDataType );
      bSize = 0;
      break;
   }

   return( bSize );
}

void ABCC_GetString( void* pxSrc, char* pcString, UINT16 iNumChar, UINT16 iOctetOffset )
{
   ABCC_PORT_StrCpyToNative( pcString,
                             pxSrc,
                             iOctetOffset,
                             iNumChar );
}

void ABCC_SetString( void* pxDst, const char* pcString, UINT16 iNumChar, UINT16 iOctetOffset )
{
   ABCC_PORT_StrCpyToPacked( pxDst,
                             iOctetOffset,
                             pcString,
                             iNumChar );
}

void ABCC_GetData8( void* pxSrc, UINT8* pbData, UINT16 iOctetOffset )
{
#ifdef ABCC_SYS_16_BIT_CHAR
   *pbData = 0;
#endif
   ABCC_PORT_Copy8( pbData, 0, pxSrc, iOctetOffset );
}

void ABCC_SetData8( void* pxDst, UINT8 bData, UINT16 iOctetOffset )
{
   ABCC_PORT_Copy8( pxDst, iOctetOffset, &bData, 0 );
}

void ABCC_GetData16( void* pxSrc, UINT16* piData, UINT16 iOctetOffset )
{
   ABCC_PORT_Copy16( piData, 0, pxSrc, iOctetOffset );
   *piData = iLeTOi( *piData );
}

void ABCC_SetData16( void* pxDst, UINT16 iData, UINT16 iOctetOffset )
{
   iData = iTOiLe( iData );
   ABCC_PORT_Copy16( pxDst, iOctetOffset, &iData, 0 );
}

void ABCC_GetData32( void* pxSrc, UINT32* plData, UINT16 iOctetOffset )
{
   ABCC_PORT_Copy32( plData, 0, pxSrc, iOctetOffset );
   *plData = lLeTOl( *plData );
}

void ABCC_SetData32( void* pxDst, UINT32 lData, UINT16 iOctetOffset )
{
   lData = lTOlLe( lData );
   ABCC_PORT_Copy32( pxDst, iOctetOffset, &lData, 0 );
}

#if ( ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED || ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED )
void ABCC_GetData64( void* pxSrc, UINT64* plData, UINT16 iOctetOffset )
{
   ABCC_PORT_Copy64( plData, 0, pxSrc, iOctetOffset );
   *plData = lLeTOl64( *plData );
}

void ABCC_SetData64( void* pxDst, UINT64 lData, UINT16 iOctetOffset )
{
   lData = lTOlLe64( lData );
   ABCC_PORT_Copy64( pxDst, iOctetOffset, &lData, 0 );
}
#endif
