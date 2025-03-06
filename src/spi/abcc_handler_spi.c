/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** This file implements the ABCC_DunDriver() and ABCC_ISR() routine for SPI
** operating mode.
********************************************************************************
*/

#include "abcc_config.h"

#if ABCC_CFG_DRV_SPI_ENABLED

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
void ABCC_SpiRunDriver( void )
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

   ABCC_CheckWrPdUpdate();
   ABCC_LinkCheckSendMessage();

   /*
   ** Send MOSI frame
   */
   pnABCC_DrvRunDriverTx();

#if ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
   /*
   ** We have now finished sending data to the Anybus and thus we end the
   ** sync measurement.
   */
   if( fAbccUserSyncMeasurementIp )
   {
      ABCC_HAL_GpioReset();
      fAbccUserSyncMeasurementIp = FALSE;
   }
#endif

   /*
   ** Handle received MISO frame
   */
   ABCC_LinkRunDriverRx();

   ABCC_TriggerRdPdUpdate();
   ABCC_TriggerAnbStatusUpdate();
   ABCC_TriggerReceiveMessage();
#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
   ABCC_CmdSequencerExec();
#endif
}

#if ABCC_CFG_INT_ENABLED
void ABCC_SpiISR()
{
   ABCC_MainStateType eMainState;

   eMainState = ABCC_GetMainState();

   if( eMainState < ABCC_DRV_WAIT_COMMUNICATION_RDY )
   {
      return;
   }

   if( eMainState == ABCC_DRV_WAIT_COMMUNICATION_RDY )
   {
      ABCC_SetReadyForCommunication();
      return;
   }

   ABCC_CbfEvent( 0 );
}
#else
void ABCC_SpiISR()
{
   ABCC_LOG_WARNING( ABCC_EC_INTERNAL_ERROR,
      0,
      "ABCC_SpiISR() called when ABCC_CFG_INT_ENABLED is 0\n" );
}
#endif
#endif /* ABCC_CFG_DRV_SPI_ENABLED */
