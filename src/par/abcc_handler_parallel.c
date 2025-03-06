/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** This file implements the ABCC_DunDriver() and ABCC_ISR() routine
** for parallel operating mode.
********************************************************************************
*/

#include "abcc_config.h"

#if ABCC_CFG_DRV_PARALLEL_ENABLED

#include "abcc_types.h"
#include "../abcc_driver_interface.h"
#include "abp.h"
#include "abcc.h"
#include "../abcc_link.h"
#include "abcc_hardware_abstraction.h"
#include "abcc_log.h"
#include "../abcc_handler.h"
#include "../abcc_timer.h"
#include "../abcc_command_sequencer.h"

#if ( ABCC_CFG_INT_ENABLE_MASK_PAR & ABP_INTMASK_SYNCIEN )
#error "Use ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED define in abcc_driver_config.h to choose sync interrupt source. Do not use ABP_INTMASK_SYNCIEN"
#endif

#if ABCC_CFG_INT_ENABLED
/*------------------------------------------------------------------------------
** Translates the ABP Interrupt status register value to the driver's ISR Event
** bit definitions (ABCC_ISR_EVENT_X).
**------------------------------------------------------------------------------
** Arguments:
**    iIntStatus - ABP Interrupt status register value
**
** Returns:
**    Bit field composed of ABCC_ISR_EVENT_X bit definitions
**------------------------------------------------------------------------------
*/
static UINT16 AbpIntStatusToAbccIsrEvent( UINT16 iIntStatus )
{
   UINT16 iAbccIsrEvents;

   iAbccIsrEvents = 0;

   if( iIntStatus & ABP_INTSTATUS_RDPDI )
   {
      iAbccIsrEvents |= ABCC_ISR_EVENT_RDPD;
   }

   if( iIntStatus & ABP_INTSTATUS_RDMSGI )
   {
      iAbccIsrEvents |= ABCC_ISR_EVENT_RDMSG;
   }

   if( ( iIntStatus & ABP_INTSTATUS_WRMSGI ) ||
       ( iIntStatus & ABP_INTSTATUS_ANBRI ) )
   {
      iAbccIsrEvents |= ABCC_ISR_EVENT_WRMSG;
   }

   if( iIntStatus & ABP_INTSTATUS_STATUSI )
   {
      iAbccIsrEvents |= ABCC_ISR_EVENT_STATUS;
   }

   return( iAbccIsrEvents );
}
#endif /* ( ABCC_CFG_INT_ENABLED ) */

/*------------------------------------------------------------------------------
** pnABCC_DrvRun()
**------------------------------------------------------------------------------
*/
void ABCC_ParRunDriver( void )
{
   ABCC_MainStateType eMainState = ABCC_GetMainState();

   if( eMainState < ABCC_DRV_SETUP )
   {
      if( eMainState != ABCC_DRV_ERROR )
      {
         ABCC_LOG_ERROR( ABCC_EC_INCORRECT_STATE,
            (UINT32)eMainState,
            "ABCC_RunDriver() called in incorrect state (%d)\n",
            eMainState );
      }

      return;
   }

   if( ( ABCC_iInterruptEnableMask & ( ABP_INTMASK_WRMSGIEN | ABP_INTMASK_ANBRIEN ) ) == 0 )
   {
      ABCC_LinkCheckSendMessage();
   }

   if( ( ABCC_iInterruptEnableMask & ABP_INTMASK_RDPDIEN ) == 0 )
   {
      ABCC_TriggerRdPdUpdate();
   }

   if( ( ABCC_iInterruptEnableMask & ABP_INTMASK_STATUSIEN ) == 0 )
   {
      ABCC_TriggerAnbStatusUpdate();
   }

   if( ( ABCC_iInterruptEnableMask & ABP_INTMASK_RDMSGIEN ) == 0 )
   {
      ABCC_TriggerReceiveMessage();
   }
#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
   ABCC_CmdSequencerExec();
#endif
}
#if ABCC_CFG_INT_ENABLED
void ABCC_ParISR( void )
{
   UINT16 iIntStatus;
   UINT16 iIntToHandleInISR;
   UINT16 iEventToHandleInCbf;
   ABCC_MainStateType eMainState;

   eMainState = ABCC_GetMainState();

   /*
   ** Let the driver handle the interrupt and clear the interrupt register.
   */
   iIntStatus = pnABCC_DrvISR();

   if( eMainState < ABCC_DRV_WAIT_COMMUNICATION_RDY )
   {
      return;
   }

   if( eMainState == ABCC_DRV_WAIT_COMMUNICATION_RDY )
   {
      ABCC_SetReadyForCommunication();
      return;
   }

   /*
   ** Only handle event defined in ABCC_CFG_HANDLE_INT_IN_ISR_MASK
   ** Special case for sync. If sync is supported and the sync signal
   ** is not used.
   */
#if ( ABCC_CFG_SYNC_ENABLED && !ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED )
   iIntToHandleInISR = iIntStatus & ( ABCC_CFG_HANDLE_INT_IN_ISR_MASK | ABP_INTSTATUS_SYNCI );

   if( iIntToHandleInISR & ABP_INTSTATUS_SYNCI )
   {
      ABCC_CbfSyncIsr();
   }
#else
   iIntToHandleInISR = iIntStatus & ABCC_CFG_HANDLE_INT_IN_ISR_MASK;
#endif


   if( iIntToHandleInISR & ABP_INTSTATUS_RDPDI )
   {
      ABCC_TriggerRdPdUpdate();
   }

   if( iIntToHandleInISR & ABP_INTSTATUS_STATUSI )
   {
      ABCC_TriggerAnbStatusUpdate();
   }

   if( iIntToHandleInISR & ABP_INTSTATUS_RDMSGI )
   {
      ABCC_TriggerReceiveMessage();
   }

   if( ( iIntToHandleInISR & ABP_INTSTATUS_WRMSGI ) ||
       ( iIntToHandleInISR & ABP_INTSTATUS_ANBRI ) )
   {
      /*
      ** Check if there is something in the queue to be sent.
      */
      ABCC_LinkCheckSendMessage();
   }

   if( ( iIntStatus & ~ABCC_CFG_HANDLE_INT_IN_ISR_MASK ) != 0 )
   {
      iEventToHandleInCbf = AbpIntStatusToAbccIsrEvent( iIntStatus & ~ABCC_CFG_HANDLE_INT_IN_ISR_MASK );

      if( iEventToHandleInCbf != 0 )
      {
         ABCC_CbfEvent( iEventToHandleInCbf );
      }
   }
}
#else
void ABCC_ParISR( void )
{
   ABCC_LOG_WARNING( ABCC_EC_INTERNAL_ERROR,
      0,
      "ABCC_ParISR() called when ABCC_CFG_INT_ENABLED is 0\n" );
}
#endif

#endif /* ABCC_CFG_DRV_PARALLEL_ENABLED */
