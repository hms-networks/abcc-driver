/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implements message queuing and write message flow control.
**. and response handler functionality
********************************************************************************
*/

#include "abcc_config.h"
#include "abcc_types.h"
#include "abcc_log.h"
#include "abcc.h"
#include "abcc_link.h"
#include "abcc_driver_interface.h"
#include "abcc_memory.h"
#include "abcc_hardware_abstraction.h"
#include "abcc_timer.h"
#include "abcc_handler.h"
#include "abcc_port.h"

/*
** Max number of messages in each send queue.
*/
#define LINK_MAX_NUM_CMDS_IN_Q            ABCC_CFG_MAX_NUM_APPL_CMDS
#define LINK_MAX_NUM_RESP_IN_Q            ABCC_CFG_MAX_NUM_ABCC_CMDS

/*
** Total number of message resources.
*/
#define LINK_NUM_MSG_IN_POOL              ABCC_CFG_MAX_NUM_MSG_RESOURCES

/*
** Max num message handlers for responses.
** The max number of outstanding commands counter is decremented
** before the handler is invoked therefore we need to handle 1 extra
** handler
*/
#define LINK_MAX_NUM_MSG_HDL ( LINK_MAX_NUM_CMDS_IN_Q + 1 )

/*
** Message queue type for queueing cmds and responses.
*/
typedef struct MsgQueueType
{
   ABP_MsgType** queue;
   INT8 bReadIndex;
   INT8 bQueueSize;
   INT8 bNumInQueue;
} MsgQueueType;

/*
** Callback function used by serial driver to indicate that a read remap is ready
** and the RdPd size can be updated.
*/
#if ABCC_CFG_DRV_SERIAL_ENABLED
void ( *pnABCC_DrvCbfReadRemapDone )( const ABP_MsgType* const psMsg );
#endif

/*
** Largest supported message size.
*/
static UINT16 link_iMaxMsgSize;

/*
** Command and response queues
*/
static ABP_MsgType* link_psCmds[LINK_MAX_NUM_CMDS_IN_Q ];
static ABP_MsgType* link_psResponses[LINK_MAX_NUM_RESP_IN_Q];

static MsgQueueType link_sCmdQueue;
static MsgQueueType link_sRespQueue;

/*
** Response handlers
*/
static ABCC_MsgHandlerFuncType link_pnMsgHandler[ LINK_MAX_NUM_MSG_HDL ];
static UINT8              link_bMsgSrcId[ LINK_MAX_NUM_MSG_HDL ];

static ABCC_LinkNotifyIndType pnMsgSentHandler;
static ABP_MsgType* link_psNotifyMsg;

/*
** Max number of outstanding commands ( no received response yet )
*/
static UINT8 link_bNumberOfOutstandingCommands = 0;

/*
** Flag used to ensure that a context have exclusive access to
** the driver write message interface. The flag is used as an
** alternative to a long critical section.
*/
static BOOL link_fDrvWriteMsgLock = FALSE;

static ABP_MsgType* link_DeQueue( MsgQueueType* psMsgQueue )
{
   ABP_MsgType* psMsg = NULL;
   if( psMsgQueue->bNumInQueue != 0 )
   {
      psMsg = psMsgQueue->queue[ psMsgQueue->bReadIndex++];
      psMsgQueue->bNumInQueue--;
      psMsgQueue->bReadIndex %= psMsgQueue->bQueueSize;
   }

   if( psMsgQueue->bNumInQueue < 0 )
   {
      ABCC_LOG_FATAL( ABCC_EC_ASSERT_FAILED,
         psMsgQueue->bNumInQueue,
         "Queue underflow (%" PRId8 ")\n",
         psMsgQueue->bNumInQueue );
   }

   return( psMsg );
}


static BOOL link_EnQueue( MsgQueueType* psMsgQueue, ABP_MsgType* psMsg )
{
   if( psMsgQueue->bNumInQueue <  psMsgQueue->bQueueSize )
   {
      psMsgQueue->queue[  ( psMsgQueue->bNumInQueue + psMsgQueue->bReadIndex ) %  psMsgQueue->bQueueSize ] = psMsg;
      psMsgQueue->bNumInQueue++;
      return( TRUE );
   }
   return( FALSE );
}

static void link_CheckNotification( const ABP_MsgType* const psMsg )
{
   if( ( pnMsgSentHandler != NULL ) && ( psMsg == link_psNotifyMsg ) )
   {
      pnMsgSentHandler();
      pnMsgSentHandler = NULL;
   }
}

void ABCC_LinkInit( void )
{
   UINT16 iCount;

   link_fDrvWriteMsgLock = FALSE;
   /*
   ** Init Queue structures.
   */
   link_sCmdQueue.bNumInQueue = 0;
   link_sCmdQueue.bQueueSize = LINK_MAX_NUM_CMDS_IN_Q;
   link_sCmdQueue.bReadIndex = 0;
   link_sCmdQueue.queue = link_psCmds;

   link_sRespQueue.bNumInQueue = 0;
   link_sRespQueue.bQueueSize = LINK_MAX_NUM_RESP_IN_Q;
   link_sRespQueue.bReadIndex = 0;
   link_sRespQueue.queue = link_psResponses;

   ABCC_MemCreatePool();

   for( iCount = 0; iCount < LINK_MAX_NUM_CMDS_IN_Q; iCount++  )
   {
      link_pnMsgHandler[ iCount ] = 0;
      link_bMsgSrcId[iCount ] = 0;
   }

   /*
   ** Initialize driver privates and states to default values.
   */
   link_bNumberOfOutstandingCommands = 0;

   pnMsgSentHandler = NULL;
   link_psNotifyMsg = NULL;

   link_iMaxMsgSize = ABCC_GetMessageChannelSize();

   /*
   ** link_CheckNotification will be called by serial driver when a read remap
   ** sequence is ready for a RdPd size update.
   */
#if ABCC_CFG_DRV_SERIAL_ENABLED
   pnABCC_DrvCbfReadRemapDone = &link_CheckNotification;
#endif

}


ABP_MsgType* ABCC_LinkReadMessage( void )
{
   ABCC_MsgType psReadMessage;
   ABCC_PORT_UseCritical();

   psReadMessage.psMsg = pnABCC_DrvReadMessage();

   if( psReadMessage.psMsg != NULL )
   {
      if( ( ABCC_GetLowAddrOct( psReadMessage.psMsg16->sHeader.iCmdReserved ) & ABP_MSG_HEADER_C_BIT ) == 0 )
      {
         /*
         ** Decrement number of outstanding commands if a response is received
         */
         ABCC_PORT_EnterCritical();
         link_bNumberOfOutstandingCommands--;
         ABCC_PORT_ExitCritical();
         ABCC_LOG_DEBUG_MSG_GENERAL( "Outstanding commands: %" PRIu8 "\n",
                                     link_bNumberOfOutstandingCommands );
      }
   }
   return( psReadMessage.psMsg );
}


void ABCC_LinkCheckSendMessage( void )
{
   BOOL fMsgWritten;
   ABP_MsgType* psWriteMessage;
   ABCC_PORT_UseCritical();

   psWriteMessage = NULL;

   ABCC_PORT_EnterCritical();

   /*
   ** Check that no other context are in progress with sending a message.
   */
   if( !link_fDrvWriteMsgLock )
   {
      /*
      ** Check if any messages are queued and the driver is ready to send
      ** the message.
      ** If the queue index > 0 then there are messages in the queue.
      ** Response messages are prioritized over command messages.
      */
      if( ( link_sRespQueue.bNumInQueue > 0 ) && pnABCC_DrvISReadyForWriteMessage() )
      {
         /*
         ** At this point it is sure that we will send a message. Lock the
         ** driver to ensure exclusive access after leaving this critical
         ** section.
         */
         link_fDrvWriteMsgLock = TRUE;
         psWriteMessage = link_DeQueue( &link_sRespQueue );
         ABCC_LOG_DEBUG_MSG_EVENT( psWriteMessage, "Response dequeued: " );
         ABCC_LOG_DEBUG_MSG_GENERAL( "RespQ status: %" PRIu8 "(%" PRIu8 ")\n",
               link_sRespQueue.bNumInQueue,
               link_sRespQueue.bQueueSize );
      }
      else if( ( link_sCmdQueue.bNumInQueue > 0 ) && pnABCC_DrvISReadyForCmd() )
      {
         /*
         ** At this point it is sure that we will send a message. Lock the
         ** driver to ensure exclusive access after leaving this critical
         ** section.
         */
         link_fDrvWriteMsgLock = TRUE;
         psWriteMessage = link_DeQueue( &link_sCmdQueue );
         ABCC_LOG_DEBUG_MSG_EVENT( psWriteMessage, "Command dequeued: " );
         ABCC_LOG_DEBUG_MSG_GENERAL( "CmdQ status: %" PRIu8 "(%" PRIu8 ")\n",
               link_sCmdQueue.bNumInQueue,
               link_sCmdQueue.bQueueSize );
      }
   }
   ABCC_PORT_ExitCritical();

   if( psWriteMessage != NULL )
   {
      /*
      ** Only call ABCC_DrvPrepareWriteMessage if it's implemented by the
      ** driver. Note that this function will not deliver the message to the
      ** ABCC just copy the message data to the memory.
      */
      if( pnABCC_DrvPrepareWriteMessage != NULL )
      {
         pnABCC_DrvPrepareWriteMessage( psWriteMessage );
      }

      ABCC_PORT_EnterCritical();
      /*
      ** Do the actual write of the message and unlock the driver to enable
      ** use from other contexts. Both these actions need to be done within the
      ** same critical section.
      */
      fMsgWritten = pnABCC_DrvWriteMessage( psWriteMessage );
      link_fDrvWriteMsgLock = FALSE;
      ABCC_PORT_ExitCritical();

      if( fMsgWritten )
      {
         /*
         ** The message was successfully written and can be deallocated now.
         */
         ABCC_LOG_DEBUG_HEXDUMP_MSG_TX( psWriteMessage );
         ABCC_LOG_DEBUG_MSG_CONTENT( psWriteMessage, "Msg sent\n" );
         link_CheckNotification( psWriteMessage );
         ABCC_LinkFree( &psWriteMessage );
      }
   }
}


void ABCC_LinkRunDriverRx( void )
{
   ABP_MsgType* psSentMsg;

   psSentMsg = pnABCC_DrvRunDriverRx();
   /*
   ** If a write message was sent, free the buffer.
   */
   if( psSentMsg )
   {
      ABCC_LOG_DEBUG_HEXDUMP_MSG_TX( psSentMsg );
      ABCC_LOG_DEBUG_MSG_CONTENT( psSentMsg, "Msg sent\n" );
      link_CheckNotification( psSentMsg );
      ABCC_LinkFree( &psSentMsg );
   }
}


UINT16 ABCC_LinkGetNumCmdQueueEntries( void )
{
   UINT16 iQEntries;
   iQEntries =  LINK_MAX_NUM_CMDS_IN_Q - link_bNumberOfOutstandingCommands;
   return( iQEntries );
}


ABCC_ErrorCodeType ABCC_LinkWriteMessage( ABP_MsgType* psWriteMsg )
{
   BOOL fSendMsg;
   BOOL fMsgWritten;
   ABCC_ErrorCodeType eErrorCode;
#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_WARNING_ENABLED
   UINT32 lAddErrorInfo;
#endif

   ABCC_PORT_UseCritical();

   eErrorCode = ABCC_EC_NO_ERROR;
#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_WARNING_ENABLED
   lAddErrorInfo = 0;
#endif
   fSendMsg = FALSE;

   if( ABCC_GetMsgDataSize( psWriteMsg ) > link_iMaxMsgSize )
   {
      eErrorCode = ABCC_EC_WRMSG_SIZE_ERR;
      ABCC_LOG_WARNING( eErrorCode, iLeTOi( psWriteMsg->sHeader.iDataSize ),
                        "Message size exceeds max size: %" PRIu16 "\n",
                        iLeTOi( psWriteMsg->sHeader.iDataSize ) );
      return( eErrorCode );
   }

   ABCC_PORT_EnterCritical();

   /*
   ** Check if it is possible to message immediately or if it has to be queued.
   */
   if( !ABCC_IsCmdMsg( psWriteMsg ) )
   {
      if( !link_fDrvWriteMsgLock && ( link_sRespQueue.bNumInQueue == 0 ) && pnABCC_DrvISReadyForWriteMessage() )
      {
         /*
         ** At this point it is sure that we will send a message. Lock the
         ** driver to ensure exclusive access after leaving this critical
         ** section.
         */
         fSendMsg = TRUE;
         link_fDrvWriteMsgLock = TRUE;
      }
      else if( link_EnQueue( &link_sRespQueue, psWriteMsg ) )
      {
         ABCC_LOG_DEBUG_MSG_EVENT( psWriteMsg, "Response msg queued: " );
         ABCC_LOG_DEBUG_MSG_GENERAL( "RespQ status: %" PRIu8 "(%" PRIu8 ")\n",
               link_sRespQueue.bNumInQueue,
               link_sRespQueue.bQueueSize );
      }
      else
      {
         ABCC_LOG_DEBUG_MSG_EVENT( psWriteMsg, "Response queue full: " );
         ABCC_LOG_DEBUG_MSG_GENERAL( "RespQ status: %" PRIu8 "(%" PRIu8 ")\n",
               link_sRespQueue.bNumInQueue,
               link_sRespQueue.bQueueSize );
         eErrorCode = ABCC_EC_LINK_RESP_QUEUE_FULL;
#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_WARNING_ENABLED
         lAddErrorInfo = (UINT32)psWriteMsg;
#endif
      }
   }
   else
   {
      if( !link_fDrvWriteMsgLock &&
          ( ( link_sCmdQueue.bNumInQueue + link_sRespQueue.bNumInQueue ) == 0 ) &&
          pnABCC_DrvISReadyForCmd() )
      {
         /*
         ** At this point it is sure that we will send a message. Lock the
         ** driver to ensure exclusive access after leaving this critical
         ** section.
         */
         fSendMsg = TRUE;
         link_fDrvWriteMsgLock = TRUE;
         link_bNumberOfOutstandingCommands++;
      }
      else if( link_EnQueue( &link_sCmdQueue, psWriteMsg ) )
      {
         ABCC_LOG_DEBUG_MSG_EVENT( psWriteMsg, "Command queued: " );
         ABCC_LOG_DEBUG_MSG_GENERAL( "CmdQ status: %" PRIu8 "(%" PRIu8 ")\n",
            link_sCmdQueue.bNumInQueue,
            link_sCmdQueue.bQueueSize );

         link_bNumberOfOutstandingCommands++;
         ABCC_LOG_DEBUG_MSG_GENERAL( "Outstanding commands: %" PRIu8 "\n",
            link_bNumberOfOutstandingCommands );
      }
      else
      {
         ABCC_LOG_DEBUG_MSG_EVENT( psWriteMsg, "Command queue full: " );
         ABCC_LOG_DEBUG_MSG_GENERAL( "CmdQ status: %" PRIu8 "(%" PRIu8 ")\n",
            link_sCmdQueue.bNumInQueue,
            link_sCmdQueue.bQueueSize );
         eErrorCode = ABCC_EC_LINK_CMD_QUEUE_FULL;
      }
   }
   ABCC_PORT_ExitCritical();

   if( eErrorCode == ABCC_EC_NO_ERROR )
   {
      ABCC_MemSetBufferStatus( psWriteMsg, ABCC_MEM_BUFSTAT_SENT );
   }
   else
   {
      ABCC_LinkFree( &psWriteMsg );
   }

   /*
   ** Continue the transmission of the message if both queues were empty.
   ** Else the message will be transmitted at the next ABCC write message event.
   */
   if( fSendMsg )
   {
      /*
      ** Only call ABCC_DrvPrepareWriteMessage if it's implemented by the
      ** driver. Note that this function will not deliver the message to the
      ** ABCC just copy the message data to the memory.
      */
      if( pnABCC_DrvPrepareWriteMessage != NULL )
      {
         pnABCC_DrvPrepareWriteMessage( psWriteMsg );
      }

      ABCC_PORT_EnterCritical();
      /*
      ** Do the actual write of the message and unlock the driver to enable
      ** use from other contexts. Both these actions need to be done within the
      ** same critical section.
      */
      fMsgWritten = pnABCC_DrvWriteMessage( psWriteMsg );
      link_fDrvWriteMsgLock = FALSE;
      ABCC_PORT_ExitCritical();

      if( fMsgWritten )
      {
         /*
         ** The message was successfully written and can be deallocated now.
         */
         ABCC_LOG_DEBUG_HEXDUMP_MSG_TX( psWriteMsg );
         ABCC_LOG_DEBUG_MSG_CONTENT( psWriteMsg, "Msg sent\n" );
         link_CheckNotification( psWriteMsg );
         ABCC_LinkFree( &psWriteMsg );
      }
   }

   if( eErrorCode != ABCC_EC_NO_ERROR )
   {
      ABCC_LOG_WARNING( eErrorCode,
         lAddErrorInfo,
         "Failed to send message (Error code: %d, additional info: %" PRIu32 ")\n",
         eErrorCode,
         lAddErrorInfo );
   }

   return( eErrorCode );
}

ABCC_ErrorCodeType ABCC_LinkWrMsgWithNotification( ABP_MsgType* psWriteMsg,
                                                   ABCC_LinkNotifyIndType pnHandler )
{
   ABCC_ErrorCodeType eResult;

   /*
   ** Save callback function to call when message is successfully sent.
   */
   if( pnMsgSentHandler != NULL )
   {
      ABCC_LOG_FATAL( ABCC_EC_ASSERT_FAILED,
         (UINT32)pnMsgSentHandler,
         "Message sent handler not NULL (%" PRIx32 ")\n",
         (UINT32)pnMsgSentHandler );
   }

   pnMsgSentHandler = pnHandler;
   link_psNotifyMsg = psWriteMsg;

   eResult = ABCC_LinkWriteMessage( psWriteMsg );

   return( eResult );
}

void ABCC_LinkFree( ABP_MsgType** ppsBuffer )
{
   if( !*ppsBuffer )
   {
      ABCC_LOG_WARNING( ABCC_EC_TRYING_TO_FREE_NULL_POINTER,
         0,
         "ABCC_LinkFree called with NULL pointer\n" );
   }

   ABCC_MemFree( ppsBuffer );
}

ABCC_ErrorCodeType ABCC_LinkMapMsgHandler( UINT8 bSrcId, ABCC_MsgHandlerFuncType  pnMSgHandler )
{
   UINT16 iIndex;
   ABCC_ErrorCodeType eResult = ABCC_EC_NO_RESOURCES;
   ABCC_PORT_UseCritical();

   /*
   ** Find free spot.
   */
   ABCC_PORT_EnterCritical();
   for( iIndex = 0; iIndex < LINK_MAX_NUM_MSG_HDL; iIndex++ )
   {
      if( link_pnMsgHandler[ iIndex ] == 0 )
      {
         link_pnMsgHandler[ iIndex ] = pnMSgHandler;
         link_bMsgSrcId[ iIndex ] = bSrcId;
         eResult = ABCC_EC_NO_ERROR;
         break;
      }
   }
   ABCC_PORT_ExitCritical();
   return( eResult );
}

ABCC_MsgHandlerFuncType ABCC_LinkGetMsgHandler( UINT8 bSrcId )
{
   UINT16 iIndex;
   ABCC_MsgHandlerFuncType pnHandler = NULL;
   ABCC_PORT_UseCritical();

   /*
   ** Find message handler. If not found return NULL.
   */
   ABCC_PORT_EnterCritical();
   for( iIndex = 0; iIndex < LINK_MAX_NUM_MSG_HDL; iIndex++ )
   {
      if( ( link_pnMsgHandler[ iIndex ] != NULL ) && ( link_bMsgSrcId[ iIndex ] == bSrcId ) )
      {
         pnHandler = link_pnMsgHandler[ iIndex ];
         link_pnMsgHandler[ iIndex ] = NULL;
         break;
      }
   }
   ABCC_PORT_ExitCritical();
   return( pnHandler );
}

BOOL ABCC_LinkIsSrcIdUsed( UINT8 bSrcId )
{
   BOOL fFound = FALSE;
   UINT16 iIndex;

   for( iIndex = 0; iIndex < LINK_MAX_NUM_MSG_HDL; iIndex++ )
   {
      if( ( link_pnMsgHandler[ iIndex ] != NULL ) && ( link_bMsgSrcId[ iIndex ] == bSrcId ) )
      {
         fFound = TRUE;
         break;
      }
   }
   return( fFound );
}
