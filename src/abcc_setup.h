/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Interface of the ABCC setup handler.
********************************************************************************
*/

#ifndef ABCC_SETUP_H_
#define ABCC_SETUP_H_

#include "abcc_config.h"
#include "abcc_types.h"
#include "abp.h"

/*------------------------------------------------------------------------------
** Initialize internal variables used by the setup state machine.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetupInit( void );

/*------------------------------------------------------------------------------
** Start command sequence to be run during ABCC setup state.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_StartSetup( void );

#endif  /* inclusion lock */
