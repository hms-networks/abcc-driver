/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implements command sequenser.
**
********************************************************************************
*/

#ifndef ABCC_CMD_SEQ_H_
#define ABCC_CMD_SEQ_H_
#include "abcc_config.h"
#include "abp.h"

/*------------------------------------------------------------------------------
** Initiate command sequencer.
**------------------------------------------------------------------------------
** Arguments:
**       None
**
** Returns:
**       None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CmdSequencerInit( void );

/*------------------------------------------------------------------------------
** Cyclic execution of command sequencer. Checks for command sequences that for
** some reason needs re-triggering (ex. previous lack of resources)
**------------------------------------------------------------------------------
** Arguments:
**       None
**
** Returns:
**       None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CmdSequencerExec( void );

#endif
