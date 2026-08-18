/*******************************************************************************
********************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
********************************************************************************
** File Description:
** Implementation of the serial driver ping pong protocol.
**
** The UART interface communicates using the legacy wire format defined by
** ABP_Msg255Type. Internally, the driver utilizes the expanded ABP_MsgType
** structure. The serial driver handles the conversion between these two
** formats on the fly, including byte packing and unpacking required for 16-bit char
** architectures.
**
** Header Structure Comparison:
**
** Legacy                   ABP_MsgHeaderType        ABP_MsgHeaderType16
** ===================      ===================      ==========================
**                          UINT16   iDataSize;      UINT16   iDataSize;
**                          UINT16   iReserved;      UINT16   iReserved;
** UINT8    bSourceId;      UINT8    bSourceId;      UINT16   iSourceIdDestObj;
** UINT8    bDestObj;       UINT8    bDestObj;     
** UINT16   iInstance;      UINT16   iInstance;      UINT16   iInstance;
** UINT8    bCmd;           UINT8    bCmd;           UINT16   iCmdReserved;
** UINT8    bDataSize;      UINT8    bReserved;    
** UINT8    bCmdExt0;       UINT8    bCmdExt0;       UINT16   iCmdExt0CmdExt1;
** UINT8    bCmdExt1;       UINT8    bCmdExt1;     
**
** For 16 bit char architectures, the UINT16 message header elements
** need to be unpacked before transmission and packed upon reception.
** This transformation is performed in drv_GetWriteFrag() and drv_AddReadFrag().
********************************************************************************
*/
#include "abcc_config.h"

#if ABCC_CFG_DRV_SERIAL_ENABLED

#include "abcc_types.h"
#include "abp.h"
#include "abcc.h"
#include "abcc_log.h"
#include "abcc_hardware_abstraction.h"
#include "../abcc_timer.h"
#include "../abcc_driver_interface.h"
#include "../abcc_memory.h"
#include "abcc_crc16.h"
#include "abcc_hardware_abstraction_serial.h"
#include "abcc_port.h"
#include "abcc_driver_serial_interface.h"

#if ( ABCC_CFG_MAX_MSG_SIZE < 16 )
#error "ABCC_CFG_MAX_MSG_SIZE must be at least a 16 bytes"
#endif

/*
** Telegram field sizes in bytes.
*/
#define SER_CMD_STAT_REG_LEN ( ABP_UINT8_SIZEOF )
#define SER_MSG_FRAG_LEN     ( 16 * ABP_UINT8_SIZEOF )
#define SER_MSG_HEADER_LEN   ( 8 * ABP_UINT8_SIZEOF )
#define SER_CRC_LEN          ( ABP_UINT16_SIZEOF )

typedef struct WrMsgFragType
{
  UINT8*             pbCurrPtr;           /* Pointer to the current position in the send buffer. */
  INT16              iNumBytesLeft;       /* Number of bytes left to send. */
  UINT16             iFragLength;         /* Current fragmentation block length. */
} WrMsgFragType;

typedef struct RdMsgFragType
{
  UINT8*             pbCurrPtr;           /* Pointer to the current position in the receive buffer. */
  UINT16             iNumBytesReceived;   /* Number of bytes received. */
  UINT16             iFragLength;         /* Current fragmentation block length. */
  UINT16             iMaxLength;          /* Max number of bytes to receive. */
} RdMsgFragType;

ABCC_SYS_PACK_ON
typedef struct SerTxTelegramType
{
   UINT8 bControl;
   UINT8 abWrMsg[ SER_MSG_FRAG_LEN ];
   UINT8 abData[ ABCC_CFG_MAX_PROCESS_DATA_SIZE + SER_CRC_LEN ];
}
PACKED_STRUCT SerTxTelegramType;
ABCC_SYS_PACK_OFF

ABCC_SYS_PACK_ON
typedef struct SerRxTelegramType
{
   UINT8 bStatus;
   UINT8 abRdMsg[ SER_MSG_FRAG_LEN ];
   UINT8 abData[ ABCC_CFG_MAX_PROCESS_DATA_SIZE + SER_CRC_LEN ];
}
PACKED_STRUCT SerRxTelegramType;
ABCC_SYS_PACK_OFF

/*------------------------------------------------------------------------------
** Internal states.
**------------------------------------------------------------------------------
*/
typedef enum drv_SerStateType
{
   SM_SER_INIT = 0,
   SM_SER_RDY_TO_SEND_PING,
   SM_SER_WAITING_FOR_PONG
} drv_SerStateType;

/*------------------------------------------------------------------------------
** General privates.
**------------------------------------------------------------------------------
*/
static drv_SerStateType     drv_eState;                 /* Serial driver state. */
static UINT8                drv_bStatus;                /* Latest received status. */
   
static ABP_MsgType*         drv_psWriteMessage;         /* Pointer to active write message. */
   
static ABP_MsgType*         drv_psReadMessage;          /* Pointer to the read message. */
   
static BOOL                 drv_fNewReadMessage;        /* Indicate that new message has arrived. */
   
static UINT16               drv_iWritePdSize;           /* Current write PD size. */
static UINT16               drv_iReadPdSize;            /* Current read PD size. */
   
static UINT16               drv_iTxFrameSize;           /* Current ping frame size. */
static UINT16               drv_iRxFrameSize;           /* Current pong frame size. */
   
static UINT8                drv_bNbrOfCmds;             /* Number of commands that can be received by the application. */
   
static SerRxTelegramType    drv_sRxTelegram;            /* Place holder for Rx telegram. */
static SerTxTelegramType    drv_sTxTelegram;            /* Place holder for Tx telegram. */

static WrMsgFragType        sTxFragHandle;
static RdMsgFragType        sRxFragHandle;
static BOOL                 fSendWriteMessageEndMark;   /* Indicate end of message. */
   
static BOOL                 drv_fNewRxTelegramReceived; /* Serial driver has a complete message. */
static UINT8*               drv_bpRdPd;                 /* Pointer to valid read process data. */
   
static UINT16               drv_iCrcErrorCount;         /* CRC error counter. */
/*
** Timers and watchdogs
*/
static ABCC_TimerHandle     xWdTmoHandle;
static BOOL                 fWdTmo;                     /* Current wd timeout status */
static ABCC_TimerHandle     xTelegramTmoHandle;
static BOOL                 fTelegramTmo;               /* Current telegram timeout status */
static UINT16               iTelegramTmoMs;             /* Telegram timeout  */


/*******************************************************************************
** Private forward declarations.
********************************************************************************
*/
static void DrvSerSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg );

/*------------------------------------------------------------------------------
** Callback from the physical layer to indicate that a RX telegran was received.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void drv_RxTelegramReceived( void )
{
   drv_fNewRxTelegramReceived = TRUE;
}

/*------------------------------------------------------------------------------
** Init write message fragmentation.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to write fragmentation information.
**       psMsg          Pointer to start of message.
**       iMsgSize       Message length.
**       iFragLength    Fragment length.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void drv_WriteFragInit( WrMsgFragType* psFragHandle, UINT8* psMsg, UINT16 iMsgSize, UINT16 iFragLength )
{
    psFragHandle->iNumBytesLeft = iMsgSize;
    psFragHandle->iFragLength = iFragLength;
    psFragHandle->pbCurrPtr = psMsg;
}

/*------------------------------------------------------------------------------
** Get current write message fragment.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to write fragmentation information.
**       pbBuffer       Pointer to destination buffer.
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void drv_GetWriteFrag( WrMsgFragType* const psFragHandle, UINT8* const pbBuffer )
{
   /*
   ** Copy the message into the Tx frame buffer.
   */
   if( psFragHandle->iNumBytesLeft <= 0 )
   {
      ABCC_LOG_FATAL( ABCC_EC_ASSERT_FAILED,
                      0,
                      "No data to copy (%" PRId16 ")\n",
                      psFragHandle->iNumBytesLeft );
      return;
   }

   ABCC_PORT_StrCpyToNative( pbBuffer,
                             psFragHandle->pbCurrPtr,
                             0,
                             psFragHandle->iFragLength );
}

/*------------------------------------------------------------------------------
** Update to next write fragment. Returns TRUE if nothing to send.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to write fragmentation information.
**       pbBuffer       Pointer to destination buffer.
** Returns:
**       TRUE if the whole message is sent.
**------------------------------------------------------------------------------
*/
static BOOL drv_PrepareNextWriteFrag( WrMsgFragType* const psFragHandle )
{
   BOOL fFragDone = FALSE;

   if( psFragHandle->iNumBytesLeft > 0 )
   {
      psFragHandle->pbCurrPtr         += psFragHandle->iFragLength;
      psFragHandle->iNumBytesLeft     -= psFragHandle->iFragLength;
   }

   if( psFragHandle->iNumBytesLeft <= 0 )
   {
      fFragDone = TRUE;
   }

   return( fFragDone );
}

/*------------------------------------------------------------------------------
** Check if write message sending is in progress.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to write fragmentation information.
** Returns:
**       TRUE if sending is ongoing.
**------------------------------------------------------------------------------
*/
static BOOL drv_isWrMsgSendingInprogress( WrMsgFragType* const psFragHandle )
{
   return( psFragHandle->pbCurrPtr != 0 );
}

/*------------------------------------------------------------------------------
** Init read message fragmentation.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to read fragmentation information.
**       psMsg          Pointer to start of message.
**       iFragLength    Fragment length.
**       iMaxMsgLength  Maximum length of message.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void drv_InitReadFrag( RdMsgFragType* psFragHandle, UINT8* psMsg, UINT16 iFragLength, UINT16 iMaxMsgLength )
{
    psFragHandle->iNumBytesReceived = 0;
    psFragHandle->iFragLength = iFragLength;
    psFragHandle->pbCurrPtr = psMsg;
    psFragHandle->iMaxLength = iMaxMsgLength;
}

/*------------------------------------------------------------------------------
** Add read message fragment.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to read fragmentation information.
**       pbBuffer       Pointer to source buffer.
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void drv_AddReadFrag( RdMsgFragType* const psFragHandle, UINT8* const pbBuffer )
{
   if( ( psFragHandle->iNumBytesReceived + psFragHandle->iFragLength ) > psFragHandle->iMaxLength )
   {
      /*
      ** Message size exceeds buffer. Don't fill up the buffer;
      ** this is handled in higher layers.
      */
      return;
   }

   /*
   ** Copy the Rx message into fragmentation buffer.
   */
   psFragHandle->iNumBytesReceived += psFragHandle->iFragLength;

   ABCC_PORT_StrCpyToPacked( psFragHandle->pbCurrPtr,
                             0,
                             pbBuffer,
                             psFragHandle->iFragLength );

   psFragHandle->pbCurrPtr += psFragHandle->iFragLength;
}

/*------------------------------------------------------------------------------
** Check if read message receiving is in progress.
**------------------------------------------------------------------------------
** Arguments:
**       psFragHandle   Pointer to read fragmentation information.
** Returns:
**       TRUE if receiving is ongoing.
**------------------------------------------------------------------------------
*/
static BOOL drv_isRdMsgReceiveInprogress( RdMsgFragType* const psFragHandle )
{
   return( psFragHandle->pbCurrPtr != 0 );
}

static void drv_WdTimeoutHandler( void )
{
   fWdTmo = TRUE;
   ABCC_CbfWdTimeout();
}

static void drv_TelegramTimeoutHandler( void )
{
   fTelegramTmo = TRUE;
}

void ABCC_DrvSerInit( UINT8 bOpmode )
{
   if( ( bOpmode != ABP_OP_MODE_SERIAL_19_2 ) &&
       ( bOpmode != ABP_OP_MODE_SERIAL_57_6 ) &&
       ( bOpmode != ABP_OP_MODE_SERIAL_115_2 ) &&
       ( bOpmode != ABP_OP_MODE_SERIAL_625 ) )
   {
      ABCC_LOG_FATAL( ABCC_EC_INCORRECT_OPERATING_MODE,
                      (UINT32)bOpmode,
                      "Incorrect operating mode %" PRIu8 "\n",
                       bOpmode );
   }

   /*
   ** Initialize privates and states.
   */
   fSendWriteMessageEndMark = FALSE;

   drv_psReadMessage = 0;
   drv_psWriteMessage = 0;

   drv_bNbrOfCmds = 0;
   drv_eState = SM_SER_INIT;

   drv_iWritePdSize = 0;
   drv_iReadPdSize = 0;

   drv_iTxFrameSize = SER_CMD_STAT_REG_LEN + SER_MSG_FRAG_LEN + drv_iWritePdSize;
   drv_iRxFrameSize = SER_CMD_STAT_REG_LEN + SER_MSG_FRAG_LEN + drv_iReadPdSize;

   drv_bpRdPd = NULL;

   drv_sTxTelegram.bControl = 0;
   drv_sRxTelegram.bStatus = 0;
   drv_bStatus = 0;

   drv_InitReadFrag( &sRxFragHandle, 0, 0, 0 );
   drv_WriteFragInit( &sTxFragHandle, 0, 0, 0 );

   drv_fNewRxTelegramReceived = FALSE;
   drv_fNewReadMessage = FALSE;

   xTelegramTmoHandle = ABCC_TimerCreate( drv_TelegramTimeoutHandler );
   fTelegramTmo = FALSE;

   switch( bOpmode )
   {
   case ABP_OP_MODE_SERIAL_19_2:
      iTelegramTmoMs = ABCC_CFG_SERIAL_TMO_19_2;
      break;
   case ABP_OP_MODE_SERIAL_57_6:
      iTelegramTmoMs = ABCC_CFG_SERIAL_TMO_57_6;
      break;
   case ABP_OP_MODE_SERIAL_115_2:
      iTelegramTmoMs = ABCC_CFG_SERIAL_TMO_115_2;
      break;
   case ABP_OP_MODE_SERIAL_625:
      iTelegramTmoMs = ABCC_CFG_SERIAL_TMO_625;
      break;
   default:
      ABCC_LOG_FATAL( ABCC_EC_INCORRECT_OPERATING_MODE,
                      (UINT32)bOpmode,
                      "Incorrect operating mode %" PRIu8 "\n",
                      bOpmode );
      break;
   }

   xWdTmoHandle = ABCC_TimerCreate( drv_WdTimeoutHandler );
   fWdTmo = FALSE;

   /*
   ** Register the PONG indicator for the physical serial driver.
   */
   ABCC_HAL_SerRegDataReceived( drv_RxTelegramReceived );
}

/*------------------------------------------------------------------------------
**  Handles preparation and sending of Tx telegram.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       -
**------------------------------------------------------------------------------
*/
void ABCC_DrvSerRunDriverTx( void )
{
   UINT16 iCrc;
   BOOL   fHandleWriteMsg = FALSE;

   ABCC_PORT_UseCritical();

   if( drv_eState == SM_SER_RDY_TO_SEND_PING )
   {
      drv_eState = SM_SER_WAITING_FOR_PONG;
      drv_sTxTelegram.bControl &= ABP_CTRL_T_BIT;

      if( !fTelegramTmo )
      {
         /*
         ** Everything is OK. Reset retransmission and toggle the T bit.
         */
         drv_sTxTelegram.bControl ^= ABP_CTRL_T_BIT;

         ABCC_PORT_EnterCritical();

         if( ( drv_psWriteMessage != 0 ) && !drv_isWrMsgSendingInprogress( &sTxFragHandle ) )
         {
            ABCC_SetMsgReserved( drv_psWriteMessage, (UINT8)ABCC_GetMsgDataSize( drv_psWriteMessage ) );
#ifdef ABCC_SYS_16_BIT_CHAR
            drv_WriteFragInit( &sTxFragHandle,
                               (UINT8*)( &drv_psWriteMessage->sHeader.iSourceIdDestObj ),
                               (UINT16)ABCC_GetMsgReserved( drv_psWriteMessage ) + SER_MSG_HEADER_LEN,
                               SER_MSG_FRAG_LEN );
#else
            drv_WriteFragInit( &sTxFragHandle,
                               &drv_psWriteMessage->sHeader.bSourceId,
                               (UINT16)ABCC_GetMsgReserved( drv_psWriteMessage ) + SER_MSG_HEADER_LEN,
                               SER_MSG_FRAG_LEN );
#endif            
         }

         ABCC_PORT_ExitCritical();
      }

      fTelegramTmo = FALSE;

      /*---------------------------------------------------------------------------
      ** Write message handling.
      **---------------------------------------------------------------------------
      */
      if( drv_isWrMsgSendingInprogress( &sTxFragHandle ) )
      {
         fHandleWriteMsg = TRUE;
      }

      drv_sTxTelegram.bControl &= ~ABP_CTRL_M_BIT;

      if( fHandleWriteMsg )
      {
         if( !fSendWriteMessageEndMark )
         {
            drv_GetWriteFrag( &sTxFragHandle, drv_sTxTelegram.abWrMsg );
            drv_sTxTelegram.bControl |= ABP_CTRL_M_BIT;
         }
         else
         {
            /*
            ** Position to update the Rx frame size to match the length of
            ** the new RdPd size after a read remap.
            ** The last fragment of the remap response has been sent
            ** and the ABCC will adjust the length in the next frame.
            */
            if( ( ABCC_GetMsgDestObj( drv_psWriteMessage ) == ABP_OBJ_NUM_APPD ) && ( ABCC_GetMsgCmdBits( drv_psWriteMessage ) == ABP_APPD_REMAP_ADI_READ_AREA ) )
            {
               if( pnABCC_DrvCbfReadRemapDone != NULL )
               {
                  pnABCC_DrvCbfReadRemapDone( drv_psWriteMessage );
               }
            }
         }
      }

      if( drv_bNbrOfCmds > 0 )
      {
         drv_sTxTelegram.bControl |= ABP_CTRL_R_BIT;
      }

      drv_bpRdPd = NULL;

      /*
      ** Apply the CRC checksum.
      */
      iCrc = CRC_Crc16( (UINT8*)&drv_sTxTelegram, drv_iTxFrameSize );

      drv_sTxTelegram.abData[ drv_iWritePdSize + 1] = (UINT8)( iCrc & 0xFF );
      drv_sTxTelegram.abData[ drv_iWritePdSize  ] = (UINT8)( iCrc >> 8 );

      /*
      ** Send TX telegram and receive Rx telegram.
      */
      ABCC_LOG_DEBUG_UART_HEXDUMP_TX( (UINT8*)&drv_sTxTelegram, drv_iTxFrameSize + SER_CRC_LEN );
      ABCC_TimerStart( xTelegramTmoHandle, iTelegramTmoMs );
      ABCC_HAL_SerSendReceive( (UINT8*)&drv_sTxTelegram,  (UINT8*)&drv_sRxTelegram, drv_iTxFrameSize + SER_CRC_LEN, drv_iRxFrameSize + SER_CRC_LEN );
   }
}

/*------------------------------------------------------------------------------
**  Handle the reception of the Rx telegram.
**------------------------------------------------------------------------------
** Arguments:
**       psResp:  Pointer to the response message.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
ABP_MsgType* ABCC_DrvSerRunDriverRx( void )
{
   UINT16 iCalcCrc;
   UINT16 iReceivedCrc;

   ABP_MsgType* psWriteMsg = NULL;

   if( drv_eState == SM_SER_WAITING_FOR_PONG )
   {
      if( !drv_fNewRxTelegramReceived )
      {
         if( fTelegramTmo )
         {
            ABCC_HAL_SerRestart();
            drv_eState = SM_SER_RDY_TO_SEND_PING;
         }

         /*
         ** Nothing has happened. No pong was received.
         */
         return( NULL );
      }

      /*
      ** Telegram received.
      */
      drv_fNewRxTelegramReceived = FALSE;

      ABCC_LOG_DEBUG_UART_HEXDUMP_RX( (UINT8*)&drv_sRxTelegram, drv_iRxFrameSize + SER_CRC_LEN );

      iReceivedCrc = CRC_Crc16( (UINT8*)&drv_sRxTelegram, drv_iRxFrameSize );

      /*
      ** Read the CRC of the Rx telegram.
      */
      iCalcCrc = (UINT16)drv_sRxTelegram.abData[ drv_iReadPdSize ] << 8;
      iCalcCrc |= (UINT16)drv_sRxTelegram.abData[ drv_iReadPdSize + 1 ];

      if( ( ( drv_bStatus & ABP_CTRL_T_BIT ) ==
            ( drv_sRxTelegram.bStatus & ABP_CTRL_T_BIT ) ) ||
          ( iCalcCrc != iReceivedCrc ) )
      {
         drv_iCrcErrorCount++;
         ABCC_LOG_WARNING( ABCC_EC_CHECKSUM_MISMATCH,
                           drv_iCrcErrorCount,
                           "CRC check failed for received message (error count: %" PRIu16 ")\n",
                           drv_iCrcErrorCount );
         ABCC_HAL_SerRestart();
         return( NULL );
      }

      if( fWdTmo )
      {
         ABCC_CbfWdTimeoutRecovered();
      }

      /*
      ** Correct telegram received.
      */
      ABCC_TimerStop( xTelegramTmoHandle );
      fTelegramTmo = FALSE;

      ABCC_TimerStop( xWdTmoHandle );
      fWdTmo = FALSE;

      /*
      ** Restart watchdog.
      */
      ABCC_TimerStart( xWdTmoHandle, ABCC_CFG_WD_TIMEOUT_MS );

      /*
      ** Save the current Anybus status.
      */
      drv_bStatus = drv_sRxTelegram.bStatus;
      drv_bpRdPd = drv_sRxTelegram.abData;

      /*---------------------------------------------------------------------------
      ** Write message handling.
      **---------------------------------------------------------------------------
      */
      if( drv_isWrMsgSendingInprogress( &sTxFragHandle ) )
      {
         /*
         ** End mark is successfully sent.
         */

         if( fSendWriteMessageEndMark )
         {
            fSendWriteMessageEndMark = FALSE;
            drv_WriteFragInit( &sTxFragHandle, 0, 0, 0 );
            psWriteMsg = drv_psWriteMessage;

            /*
            ** Update application flow control.
            */
            if( ( ABCC_GetMsgCmdField( drv_psWriteMessage ) & ABP_MSG_HEADER_C_BIT ) == 0 )
            {
               drv_bNbrOfCmds++;
            }

            drv_psWriteMessage = 0;
         }
         else
         {
            fSendWriteMessageEndMark = drv_PrepareNextWriteFrag( &sTxFragHandle );
         }
      }

      /*---------------------------------------------------------------------------
      ** Read message handling.
      ** --------------------------------------------------------------------------
      */
      if( drv_bStatus & ABP_STAT_M_BIT  )
      {
         if( !drv_isRdMsgReceiveInprogress( &sRxFragHandle ) )
         {
            if( drv_psReadMessage == NULL )
            {
               DrvSerSetMsgReceiverBuffer( ABCC_MemAlloc() );

               if( drv_psReadMessage == NULL )
               {
                  ABCC_LOG_WARNING( ABCC_EC_OUT_OF_MSG_BUFFERS,
                                    0,
                                    "Out of message buffers when attempting to read a message\n" );
                  return( NULL );
               }
            }

            /*
            ** Start receiving on legacy start position which corresponds to &drv_psReadMessage->sHeader.bSourceId.
            */
#ifdef ABCC_SYS_16_BIT_CHAR
            drv_InitReadFrag( &sRxFragHandle,
                              (UINT8*)(&drv_psReadMessage->sHeader.iSourceIdDestObj),
                              SER_MSG_FRAG_LEN,
                              ABCC_CFG_MAX_MSG_SIZE + SER_MSG_HEADER_LEN );
#else
            drv_InitReadFrag( &sRxFragHandle,
                              &drv_psReadMessage->sHeader.bSourceId,
                              SER_MSG_FRAG_LEN,
                              ABCC_CFG_MAX_MSG_SIZE + SER_MSG_HEADER_LEN );
#endif
         }

         drv_AddReadFrag( &sRxFragHandle, drv_sRxTelegram.abRdMsg );
      }
      else
      {
         if( drv_isRdMsgReceiveInprogress( &sRxFragHandle ) )
         {
            /*
            ** Empty message endmarker received.
            ** Copy message payload data size from legacy
            ** ABP_Msg255HeaderType to ABP_MsgHeaderType format 
            ** used by the driver.
            */
            ABCC_SetMsgDataSize( drv_psReadMessage, ABCC_GetMsgReserved( drv_psReadMessage ) );

            /*
            ** Update application flow control.
            */
            if( ABCC_GetMsgCmdField( drv_psReadMessage ) & ABP_MSG_HEADER_C_BIT )
            {
               drv_bNbrOfCmds--;
            }
            drv_fNewReadMessage = TRUE;
            drv_InitReadFrag( &sRxFragHandle, 0, 0, 0 );
         }
      }
      drv_eState = SM_SER_RDY_TO_SEND_PING;
   }
   else if( drv_eState == SM_SER_INIT )
   {
      ABCC_TimerStart( xWdTmoHandle, ABCC_CFG_WD_TIMEOUT_MS );
      drv_eState = SM_SER_RDY_TO_SEND_PING;
   }
   return( psWriteMsg );
}

UINT16 ABCC_DrvSerISR( void )
{
   return( 0 );
}

BOOL ABCC_DrvSerWriteMessage( ABP_MsgType* psWriteMsg )
{
   ABCC_PORT_UseCritical();
   if( !psWriteMsg )
   {
      ABCC_LOG_FATAL( ABCC_EC_UNEXPECTED_NULL_PTR,
                      0,
                      "Unexpected NULL pointer\n" );
   }

   ABCC_PORT_EnterCritical();

   if( drv_psWriteMessage )
   {
      ABCC_LOG_FATAL( ABCC_EC_INCORRECT_STATE,
                      (UINT32)drv_psWriteMessage,
                      "Expected drv_psWriteMessage to be NULL, was %p\n",
                      (void*)drv_psWriteMessage );
   }

   drv_psWriteMessage = psWriteMsg;
   ABCC_PORT_ExitCritical();

   /*
   ** The serial driver owns the buffer.
   */
   return( FALSE );
}

void ABCC_DrvSerWriteProcessData( void* pxProcessData )
{
   (void)pxProcessData;
   /*
   ** Nothing needs to be done here since the buffer is already updated by the application.
   */
   if( drv_eState != SM_SER_RDY_TO_SEND_PING )
   {
      ABCC_LOG_ERROR( ABCC_EC_INCORRECT_STATE,
                      (UINT32)drv_eState,
                      "Wrong driver state (%d)\n",
                      drv_eState );
   }
}

/*
** This function must be called from a critical section.
*/
BOOL ABCC_DrvSerIsReadyForWriteMessage( void )
{
   BOOL fRdyForWrMsg = FALSE;

   if( drv_psWriteMessage == NULL )
   {
      fRdyForWrMsg = TRUE;
   }
   return( fRdyForWrMsg );
}

/*
** This function must be called from a critical section.
*/
BOOL ABCC_DrvSerIsReadyForCmd( void )
{
   return( pnABCC_DrvISReadyForWriteMessage() && ( drv_bStatus & ABP_STAT_R_BIT ) );
}

void ABCC_DrvSerSetNbrOfCmds( UINT8 bNbrOfCmds )
{
   drv_bNbrOfCmds = bNbrOfCmds;
}

void ABCC_DrvSerSetAppStatus( ABP_AppStatusType eAppStatus )
{
   (void)eAppStatus;
}

void ABCC_DrvSerSetPdSize( const UINT16  iReadPdSize, const UINT16  iWritePdSize )
{
   /*
   ** The serial application interface can't handle more than 256 bytes of PD.
   ** Pull the plug if someone orders a larger PD size than that.
   */
   if( iReadPdSize > ABP_MAX_PROCESS_DATA )
   {
      ABCC_LOG_ERROR( ABCC_EC_RDPD_SIZE_ERR,
                      0,
                      "Read PD size is out of range for serial operating mode. PD size error %" PRIu16 ">%d\n",
                      iReadPdSize,
                      ABP_MAX_PROCESS_DATA );

      return;
   }
   else if( iWritePdSize > ABP_MAX_PROCESS_DATA )
   {
      ABCC_LOG_ERROR( ABCC_EC_WRPD_SIZE_ERR,
                      0,
                      "Write PD size is out of range for serial operating mode. PD size error %" PRIu16 ">%d\n",
                      iWritePdSize,
                      ABP_MAX_PROCESS_DATA );

      return;
   }

   /*
   **  Update lengths depending on PD sizes.
   */
   drv_iWritePdSize = iWritePdSize;
   drv_iReadPdSize = iReadPdSize;
   drv_iTxFrameSize = SER_CMD_STAT_REG_LEN + SER_MSG_FRAG_LEN + drv_iWritePdSize;
   drv_iRxFrameSize = SER_CMD_STAT_REG_LEN + SER_MSG_FRAG_LEN + drv_iReadPdSize;
}

static void DrvSerSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg )
{
   /*
   ** The buffer can be NULL if we are out of msg resources.
   */
   drv_psReadMessage = psReadMsg;
}

UINT16 ABCC_DrvSerGetIntStatus( void )
{
   ABCC_LOG_WARNING( ABCC_EC_INTSTATUS_NOT_SUPPORTED_BY_DRV_IMPL,
                     0,
                     "Interrupt status not supported by serial driver\n" );

   return( 0 );
}

UINT8 ABCC_DrvSerGetAnybusState( void )
{
   return( drv_bStatus & ABP_STAT_S_BITS );
}

void* ABCC_DrvSerReadProcessData( void )
{
   return( drv_bpRdPd );
}

ABP_MsgType* ABCC_DrvSerReadMessage( void )
{
   ABP_MsgType* psMsg = NULL;

   if( drv_fNewReadMessage )
   {
      psMsg = drv_psReadMessage;
      drv_fNewReadMessage = FALSE;
      drv_psReadMessage = NULL;
   }

   return( psMsg );
}

void ABCC_DrvSerSetIntMask( const UINT16 iIntMask )
{
   (void)iIntMask;
   /*
   ** Not possible to set interrupt mask for serial driver.
   */
}

void* ABCC_DrvSerGetWrPdBuffer( void )
{
   /*
   ** Return position to WrPd position in Tx telegram.
   */
   return( drv_sTxTelegram.abData );
}

UINT16 ABCC_DrvSerGetModCap( void )
{
   ABCC_LOG_WARNING( ABCC_EC_MODCAP_NOT_SUPPORTED_BY_DRV_IMPL,
                     0,
                     "Module capability not supported by serial driver\n" );
   return( 0 );
}

UINT16 ABCC_DrvSerGetLedStatus( void )
{
   ABCC_LOG_WARNING( ABCC_EC_LEDSTATUS_NOT_SUPPORTED_BY_DRV_IMPL,
                     0,
                     "LED status not supported by serial driver\n" );
   return( 0 );
}

BOOL ABCC_DrvSerIsReadyForWrPd( void )
{
   if( drv_eState == SM_SER_RDY_TO_SEND_PING )
   {
      return( TRUE );
   }
   return( FALSE );
}

BOOL ABCC_DrvSerIsSupervised( void )
{
   return( ( drv_bStatus & ABP_STAT_SUP_BIT ) == ABP_STAT_SUP_BIT );
}

UINT8 ABCC_DrvSerGetAnbStatus( void )
{
   return( drv_bStatus & ( ABP_STAT_SUP_BIT | ABP_STAT_S_BITS ) );
}
#endif /* End of #if ABCC_CFG_DRV_SERIAL_ENABLED. */
