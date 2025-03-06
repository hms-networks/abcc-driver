/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
*/
#include "abcc_config.h"

#if ABCC_CFG_DRV_SERIAL_ENABLED

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


/*------------------------------------------------------------------------------
** pnABCC_DrvRun()
**------------------------------------------------------------------------------
*/
void ABCC_SerRunDriver( void )
{
   ABCC_MainStateType eMainState;

   eMainState = ABCC_GetMainState();

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

   ABCC_LinkRunDriverRx();
   ABCC_TriggerRdPdUpdate();
   ABCC_TriggerAnbStatusUpdate();
   ABCC_TriggerReceiveMessage();
#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
   ABCC_CmdSequencerExec();
#endif
   ABCC_CheckWrPdUpdate();
   ABCC_LinkCheckSendMessage();
   pnABCC_DrvRunDriverTx();
}
#endif /* End of #if ABCC_CFG_DRV_SERIAL_ENABLED */
