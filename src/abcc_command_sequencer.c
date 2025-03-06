/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implements a command sequencer for ABCC command messages.
********************************************************************************
*/

#include "abcc_types.h"
#include "abp.h"
#include "abcc.h"
#include "abcc_command_sequencer_interface.h"
#include "abcc_software_port.h"
#include "abcc_log.h"
#include "abcc_link.h"
#include "abcc_memory.h"

#if ABCC_CFG_DRV_CMD_SEQ_ENABLED

#if ( ABCC_CFG_MAX_NUM_CMD_SEQ > 255 )
#error "ABCC_CFG_MAX_NUM_CMD_SEQ larger than 255 not supported"
#endif

/*******************************************************************************
** Typedefs
********************************************************************************
*/
typedef enum CmdSeqState
{
   CMD_SEQ_STATE_NOT_STARTED = 0,
   CMD_SEQ_STATE_BUSY,
   CMD_SEQ_STATE_WAIT_RESP,
   CMD_SEQ_STATE_RETRIGGER,
   CMD_SEQ_STATE_ANY
}
CmdSeqStateType;

typedef struct CmdSeqHandler
{
   const ABCC_CmdSeqType*  pasCmdSeq;
   ABCC_CmdSeqDoneHandler  pnSeqDone;
   CmdSeqStateType         eState;
   UINT8                   bCurrSeqIndex;
   UINT8                   bSourceId;
   UINT8                   bRetryCount;
   void*                   pxUserData;
   ABCC_CmdSeqResultType   eSeqResult;
}
CmdSeqEntryType;

/*******************************************************************************
** Private globals
********************************************************************************
*/
static UINT16 abcc_iNeedReTriggerCount;
static CmdSeqEntryType abcc_asCmdSeq[ ABCC_CFG_MAX_NUM_CMD_SEQ ];

/*******************************************************************************
** Forward declarations
********************************************************************************
*/
static BOOL ExecCmdSequence( CmdSeqEntryType* psCmdSeqHandler, ABP_MsgType* psMsg );
static void HandleResponse( ABP_MsgType* psMsg );
static CmdSeqEntryType* AllocCmdSeqEntry( const ABCC_CmdSeqType* pasCmdSeq );
static BOOL CheckAndSetState( CmdSeqEntryType* psCmdSeqHandler,
                              CmdSeqStateType eCheckState,
                              CmdSeqStateType eNewState );

/*******************************************************************************
** Private services
********************************************************************************
*/
/*------------------------------------------------------------------------------
** Allocate command sequence handler. Returns NULL if no handler is available.
**------------------------------------------------------------------------------
** Arguments:
**    pasCmdSeq        - Pointer to command sequence.
**
** Returns:
**    CmdSeqEntryType* - Pointer to allocated handler. NULL if no resource
**                       is allocated.
**------------------------------------------------------------------------------
*/
static CmdSeqEntryType* AllocCmdSeqEntry( const ABCC_CmdSeqType* pasCmdSeq )
{
   UINT8 i;
   CmdSeqEntryType* psEntry;
   ABCC_PORT_UseCritical();

   psEntry = NULL;

   ABCC_PORT_EnterCritical();

   for( i = 0; i < ABCC_CFG_MAX_NUM_CMD_SEQ; i++ )
   {
      if( abcc_asCmdSeq[ i ].pasCmdSeq == NULL )
      {
         psEntry = &abcc_asCmdSeq[ i ];
         psEntry->pasCmdSeq = pasCmdSeq;
         break;
      }
   }

   ABCC_PORT_ExitCritical();
   return( psEntry );
}

/*------------------------------------------------------------------------------
** Resets handler to initial state. If it's the initial reset after startup the
** reset of state will have no side effect.
**------------------------------------------------------------------------------
** Arguments:
**    psEntry  - Pointer to entry.
**    fInitial - Set to TRUE if it's the initial reset after startup.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
static void ResetCmdSeqEntry( CmdSeqEntryType* psEntry, BOOL fInitial )
{
   if( psEntry != NULL )
   {
      if( fInitial )
      {
         psEntry->eState = CMD_SEQ_STATE_NOT_STARTED;
      }
      else
      {
         /*
         ** This call may affect file global abcc_iNeedReTriggerCount
         */
         if( !CheckAndSetState( psEntry, CMD_SEQ_STATE_ANY, CMD_SEQ_STATE_NOT_STARTED ) )
         {
            ABCC_LOG_FATAL( ABCC_EC_ASSERT_FAILED, 0, "Failed to set command sequence state\n" );
         }
      }

      psEntry->pasCmdSeq = NULL;
      psEntry->pnSeqDone = NULL;
      psEntry->bCurrSeqIndex = 0;
      psEntry->bSourceId = 0;
      psEntry->bRetryCount = 0;
      psEntry->pxUserData = NULL;
      psEntry->eSeqResult = ABCC_CMDSEQ_RESULT_COMPLETED;
   }
}

/*------------------------------------------------------------------------------
** Find entry mapped to source id. Used when response message is mapped to
** command sequence.
**------------------------------------------------------------------------------
** Arguments:
**    bSourceId - Source id
**
** Returns:
**    CmdSeqEntryType* - Mapped handler. NULL if not found.
**------------------------------------------------------------------------------
*/
static CmdSeqEntryType* FindCmdSeqEntryFromSourceId( UINT8 bSourceId )
{
   UINT8 i;

   for( i = 0; i < ABCC_CFG_MAX_NUM_CMD_SEQ; i++ )
   {
      if( ( abcc_asCmdSeq[ i ].eState == CMD_SEQ_STATE_WAIT_RESP ) &&
          ( abcc_asCmdSeq[ i ].pasCmdSeq != NULL ) &&
            abcc_asCmdSeq[ i ].bSourceId == bSourceId )
      {
         return( &abcc_asCmdSeq[ i ] );
      }
   }

   return( NULL );
}

/*------------------------------------------------------------------------------
** Check if the given handle corresponds to an active command sequence.
**------------------------------------------------------------------------------
** Arguments:
**    xHandle - Handle to validate
**
** Returns:
**    TRUE  - Handle was valid
**    FALSE - Handle was invalid
**------------------------------------------------------------------------------
*/
static BOOL ValidateHandle( const ABCC_CmdSeqHandle xHandle )
{
   UINT8 i;

   for( i = 0; i < ABCC_CFG_MAX_NUM_CMD_SEQ; i++ )
   {
      if( xHandle == &abcc_asCmdSeq[ i ] )
      {
         if( abcc_asCmdSeq[ i ].pasCmdSeq != NULL )
         {
            return( TRUE );
         }
         else
         {
            break;
         }
      }
   }

   return( FALSE );
}

/*------------------------------------------------------------------------------
** Set entry in new state. May have side effects when entering or leaving
** CMD_SEQ_STATE_RETRIGGER state (see ABCC_CmdSequencerExec()).
**------------------------------------------------------------------------------
** Arguments:
**    psCmdSeqHandler - Pointer to entry
**    eCheckState     - State will only be updated if current state is this or
**                      if this is CMD_SEQ_STATE_ANY
**    eNewState       - New state
**
** Returns:
**    TRUE: New state set
**    FALSE: Old state kept
**------------------------------------------------------------------------------
*/
static BOOL CheckAndSetState( CmdSeqEntryType* psCmdSeqHandler,
                              CmdSeqStateType eCheckState,
                              CmdSeqStateType eNewState )
{
   BOOL fRet;
   ABCC_PORT_UseCritical();

   fRet = FALSE;
   ABCC_PORT_EnterCritical();

   if( ( eCheckState == CMD_SEQ_STATE_ANY ) ||
       ( eCheckState == psCmdSeqHandler->eState ) )
   {
      if( ( psCmdSeqHandler->eState == CMD_SEQ_STATE_RETRIGGER ) &&
            eNewState != CMD_SEQ_STATE_RETRIGGER )
      {
         abcc_iNeedReTriggerCount--;
      }

      if( ( psCmdSeqHandler->eState != CMD_SEQ_STATE_RETRIGGER ) &&
            eNewState == CMD_SEQ_STATE_RETRIGGER )
      {
         abcc_iNeedReTriggerCount++;
      }

      psCmdSeqHandler->eState = eNewState;
      fRet = TRUE;
   }

   ABCC_PORT_ExitCritical();

   return( fRet );
}

/*------------------------------------------------------------------------------
** Performs abort on handler if it is active.
**------------------------------------------------------------------------------
** Arguments:
**    psEntry - Pointer to handler
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
static void DoAbort( CmdSeqEntryType* psEntry )
{
   BOOL fFreeSourceId;
   UINT8 bSourceId = 0;
   ABCC_CmdSeqDoneHandler pnSeqDone;
   void *pxUserData;
   ABCC_PORT_UseCritical();

   fFreeSourceId = FALSE;
   ABCC_PORT_EnterCritical();

   if( psEntry->eState == CMD_SEQ_STATE_BUSY )
   {
      /*
      ** State not allowed. See header file documentation for
      ** ABCC_CmdSeqAbort()
      */
      ABCC_LOG_FATAL( ABCC_EC_INCORRECT_STATE, psEntry->eState, "Incorrect state (%d)\n", psEntry->eState );
   }
   else if( psEntry->eState != CMD_SEQ_STATE_NOT_STARTED )
   {
      if( psEntry->eState == CMD_SEQ_STATE_RETRIGGER )
      {
         abcc_iNeedReTriggerCount--;
      }
      if( psEntry->eState == CMD_SEQ_STATE_WAIT_RESP )
      {
         bSourceId = psEntry->bSourceId;
         fFreeSourceId = TRUE;
      }

      ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->Aborted\n",
         (void*)psEntry->pasCmdSeq);

      pnSeqDone = psEntry->pnSeqDone;
      pxUserData = psEntry->pxUserData;

      /*
      ** Do reset of handler.
      */
      ResetCmdSeqEntry( psEntry, TRUE );

      if( pnSeqDone != NULL )
      {
         pnSeqDone( ABCC_CMDSEQ_RESULT_ABORT_EXT, pxUserData );
      }
   }
   ABCC_PORT_ExitCritical();

   /*
   ** Free of sourceId is done outside critical section to avoid nested
   ** critical sections. Result can be ignored
   */
   if( fFreeSourceId )
   {
      (void)ABCC_LinkGetMsgHandler( bSourceId );
   }
}

/*------------------------------------------------------------------------------
** Common response handler for all response messages routed to the command
** sequencer. Implements ABCC_MsgHandlerFuncType function callback (abcc.h)
**------------------------------------------------------------------------------
** Arguments:
**    psMsg - Pointer to response message.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
static void HandleResponse( ABP_MsgType* psMsg )
{
   UINT8 bSourceId;
   CmdSeqEntryType* psEntry;
   ABCC_CmdSeqRespStatusType eStatus;


   bSourceId = ABCC_GetMsgSourceId( psMsg );
   psEntry = FindCmdSeqEntryFromSourceId( bSourceId );

   if( psEntry != NULL )
   {
      /*
      ** The corresponding command sequence found
      */
      if( CheckAndSetState( psEntry, CMD_SEQ_STATE_WAIT_RESP, CMD_SEQ_STATE_BUSY ) )
      {
         if( psEntry->pasCmdSeq[ psEntry->bCurrSeqIndex ].pnRespHandler != NULL )
         {
            /*
            ** Pass the response message to the application.
            */
            ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->%s()\n",
               (void*)psEntry->pasCmdSeq,
               psEntry->pasCmdSeq[ psEntry->bCurrSeqIndex ].pcRespName );

            eStatus = psEntry->pasCmdSeq[ psEntry->bCurrSeqIndex ].pnRespHandler( psMsg, psEntry->pxUserData );

            if( eStatus == ABCC_CMDSEQ_RESP_EXEC_NEXT )
            {
               /*
               ** Move to next command in sequence
               */
               psEntry->bCurrSeqIndex++;
            }
            else if( eStatus == ABCC_CMDSEQ_RESP_ABORT )
            {
               ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->Aborted\n",
                     (void*)psEntry->pasCmdSeq );
               /*
               ** Loop until end of sequence.
               */
               while( psEntry->pasCmdSeq[ ++psEntry->bCurrSeqIndex ].pnCmdHandler != NULL );

               psEntry->eSeqResult = ABCC_CMDSEQ_RESULT_ABORT_INT;
            }
            else if( eStatus == ABCC_CMDSEQ_RESP_EXEC_CURRENT )
            {
               ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->Executing same sequence step again\n",
                     (void*)psEntry->pasCmdSeq );
            }
            else
            {
               ABCC_LOG_ERROR( ABCC_EC_PARAMETER_NOT_VALID,
                  (UINT32)eStatus,
                  "Bad return parameter from response handler (%d)\n",
                  eStatus );
               return;
            }
         }
         else
         {
            ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->No response handler\n",
                  (void*)psEntry->pasCmdSeq );

            psEntry->bCurrSeqIndex++;
         }

         if( ABCC_MemGetBufferStatus( psMsg ) != ABCC_MEM_BUFSTAT_IN_APPL_HANDLER )
         {
            /*
            ** The application has used the buffer for other things.
            */
            psMsg = ABCC_GetCmdMsgBuffer();
         }

         /*
         ** Execute next command. The return value is ignored
         ** since the message deallocation will be handled after return of this
         ** function
         */
         (void)ExecCmdSequence( psEntry, psMsg );
      }
   }
}

/*------------------------------------------------------------------------------
** Execute the command sequence
**------------------------------------------------------------------------------
** Arguments:
**    psEntry - Pointer to handler
**    psMsg   - Pointer to allocated command message buffer.
**
** Returns:
**    TRUE    - Command buffer sent or freed.
**    FALSE   - Command buffer not sent or freed.
**------------------------------------------------------------------------------
*/
static BOOL ExecCmdSequence( CmdSeqEntryType* psEntry, ABP_MsgType* psMsg )
{
   BOOL fCmdBufferConsumed;
   const ABCC_CmdSeqType* psCmdSeq;
   ABCC_CmdSeqCmdStatusType eStatus;

   fCmdBufferConsumed = FALSE;

   if( psMsg != NULL )
   {
      psEntry->bRetryCount = 0;
      psCmdSeq = &psEntry->pasCmdSeq[ psEntry->bCurrSeqIndex ];

      while( ( psCmdSeq->pnCmdHandler != NULL ) && !fCmdBufferConsumed )
      {
         ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->%s()\n",
               (void*)psEntry->pasCmdSeq,
               psEntry->pasCmdSeq[ psEntry->bCurrSeqIndex ].pcCmdName );

         eStatus = psCmdSeq->pnCmdHandler( psMsg, psEntry->pxUserData );
         if( eStatus == ABCC_CMDSEQ_CMD_SKIP )
         {
            ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->Command not sent, jump to next sequence step\n",
                  (void*)psEntry->pasCmdSeq );
            /*
            ** User has chosen not to execute this command. Move to next.
            */
            psCmdSeq++;
            psEntry->bCurrSeqIndex++;
         }
         else if( eStatus == ABCC_CMDSEQ_CMD_SEND )
         {
            psEntry->bSourceId = ABCC_GetMsgSourceId( psMsg );
            if( !CheckAndSetState( psEntry, CMD_SEQ_STATE_ANY, CMD_SEQ_STATE_WAIT_RESP ) )
            {
               ABCC_LOG_FATAL( ABCC_EC_ASSERT_FAILED,
                  0,
                  "Failed to set command sequence state\n" );
            }
            (void)ABCC_SendCmdMsg( psMsg, HandleResponse );
            fCmdBufferConsumed = TRUE;
         }
         else if( eStatus == ABCC_CMDSEQ_CMD_ABORT )
         {
            ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->Aborted\n",
                  (void*)psEntry->pasCmdSeq );
            /*
            ** Abort move to end of sequence
            */
            while( (++psCmdSeq)->pnCmdHandler != NULL );
            psEntry->eSeqResult = ABCC_CMDSEQ_RESULT_ABORT_INT;
         }
         else
         {
            ABCC_LOG_ERROR( ABCC_EC_PARAMETER_NOT_VALID,
               (UINT32)eStatus,
               "Bad return parameter from response handler (%d)\n",
               eStatus );
         }
      }

      /*
      ** Check end of sequence
      */
      if( psCmdSeq->pnCmdHandler == NULL )
      {
         ABCC_CmdSeqDoneHandler pnSeqDone;
         void *pxUserData;
         ABCC_CmdSeqResultType eSeqResult;

         /*
         ** Free resource before calling done callback
         */
         ABCC_ReturnMsgBuffer( &psMsg );
         fCmdBufferConsumed = TRUE;

         ABCC_LOG_DEBUG_CMD_SEQ( "CmdSeq(%p)->Done\n",
               (void*)psEntry->pasCmdSeq );
         pnSeqDone = psEntry->pnSeqDone;
         pxUserData = psEntry->pxUserData;
         eSeqResult = psEntry->eSeqResult;

         ResetCmdSeqEntry( psEntry, FALSE );

         if( pnSeqDone != NULL )
         {
            pnSeqDone( eSeqResult, pxUserData );
         }
      }
   }
   else
   {
      /*
      ** Currently out of resource. Try again at next call of
      ** ABCC_CmdSequencerExec().
      */
      if( !CheckAndSetState( psEntry, CMD_SEQ_STATE_ANY, CMD_SEQ_STATE_RETRIGGER ) )
      {
         ABCC_LOG_FATAL( ABCC_EC_ASSERT_FAILED,
            0,
            "Failed to set command sequence state\n" );
      }
   }

   return( fCmdBufferConsumed );
}

ABCC_ErrorCodeType ABCC_CmdSeqAdd(
   const ABCC_CmdSeqType* pasCmdSeq,
   const ABCC_CmdSeqDoneHandler pnCmdSeqDone,
   void *pxUserData,
   ABCC_CmdSeqHandle* pxHandle )
{
   CmdSeqEntryType* psEntry;
   ABP_MsgType* psMsg;

   if( pasCmdSeq == NULL )
   {
      return( ABCC_EC_PARAMETER_NOT_VALID );
   }

   /*
   ** Allocate and init handler.
   */
   psEntry = AllocCmdSeqEntry( pasCmdSeq );
   if( psEntry != NULL )
   {
      psEntry->pnSeqDone = pnCmdSeqDone;
      psEntry->pxUserData = pxUserData;
      if( pxHandle != NULL )
      {
         *pxHandle = (ABCC_CmdSeqHandle)psEntry;
      }

      psMsg = ABCC_GetCmdMsgBuffer();
      (void)ExecCmdSequence( psEntry, psMsg );
   }
   else
   {
      ABCC_LOG_WARNING( ABCC_EC_OUT_OF_CMD_SEQ_RESOURCES, ABCC_CFG_MAX_NUM_CMD_SEQ, "Out of command sequence resources" );
   }

   return( ABCC_EC_NO_ERROR );
}

ABCC_ErrorCodeType ABCC_CmdSeqAbort( const ABCC_CmdSeqHandle xHandle )
{
   UINT8 i;

   if( xHandle == NULL )
   {
      for( i = 0; i < ABCC_CFG_MAX_NUM_CMD_SEQ; i++ )
      {
         DoAbort( &abcc_asCmdSeq[ i ] );
      }
   }
   else
   {
      if( ValidateHandle( xHandle ) )
      {
         DoAbort( (CmdSeqEntryType*)xHandle );
      }
      else
      {
         return( ABCC_EC_PARAMETER_NOT_VALID );
      }
   }

   return( ABCC_EC_NO_ERROR );
}

void ABCC_CmdSequencerInit( void )
{
   UINT8 i;
   for( i = 0; i < ABCC_CFG_MAX_NUM_CMD_SEQ; i++ )
   {
      ResetCmdSeqEntry( &abcc_asCmdSeq[ i ], TRUE );
   }
   abcc_iNeedReTriggerCount = 0;
}

void ABCC_CmdSequencerExec( void )
{
   UINT8 i;
   ABP_MsgType* psMsg;

   /*
   ** Only execute if any sequence requires re-trigger.
   */
   if( abcc_iNeedReTriggerCount > 0 )
   {
      psMsg = NULL;

      for( i = 0; i < ABCC_CFG_MAX_NUM_CMD_SEQ; i++ )
      {
         if( CheckAndSetState( &abcc_asCmdSeq[ i ], CMD_SEQ_STATE_RETRIGGER, CMD_SEQ_STATE_BUSY ) )
         {
            abcc_asCmdSeq[ i ].bRetryCount++;
            if( abcc_asCmdSeq[ i ].bRetryCount > ABCC_CFG_CMD_SEQ_MAX_NUM_RETRIES )
            {
               ABCC_LOG_WARNING( ABCC_EC_CMD_SEQ_RETRY_LIMIT, 0, "Command sequence retry limit reached\n" );
            }

            if( psMsg == NULL )
            {
               psMsg = ABCC_GetCmdMsgBuffer();
            }

            if( ExecCmdSequence( &abcc_asCmdSeq[ i ], psMsg ) == TRUE )
            {
               /*
               ** Message buffer consumed
               */
               psMsg = NULL;
            }
         }
      }

      if( psMsg != NULL )
      {
         ABCC_ReturnMsgBuffer( &psMsg );
      }
   }
}
#endif
