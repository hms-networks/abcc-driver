/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implements the ABCC timer service.
********************************************************************************
*/

#ifndef ABCC_TIMER_H_
#define ABCC_TIMER_H_

#include "abcc_config.h"
#include "abcc_types.h"
#include "abcc_hardware_abstraction.h"

#define ABCC_TIMER_NO_HANDLE ( 0xff )

/*
** Timeout callback function type.
*/
typedef void (*ABCC_TimerTimeoutCallbackType)( void );

/*
** Type for identifying a timer.
*/
typedef UINT8 ABCC_TimerHandle;

/*------------------------------------------------------------------------------
** Must be called before the timer can be used.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TimerInit( void );

/*------------------------------------------------------------------------------
** Allocates a timer resource and returns a handle.
**------------------------------------------------------------------------------
** Arguments:
**    pnHandleTimeout  -  Function to call if timeout.
**
** Returns:
**    ABCC_TimerHandle - ( Used as identifier when using timer functions. )
**                       TIMER_NO_HANDLE is returned if no timer was available.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_TimerHandle ABCC_TimerCreate( ABCC_TimerTimeoutCallbackType pnHandleTimeout );

/*------------------------------------------------------------------------------
** Starts the timer.
** When the timeout expires, the registered callback function is called.
** Note!! This function depends on ABCC_TimerTick() being called
** on a regular basis.
**------------------------------------------------------------------------------
** Arguments:
**    xHandle    - Identifier of the timer to be started.
**    lTimeoutMs - Timeout in ms.
**
** Returns:
**    TRUE  - Timer expired before the restart.
**    FALSE - Timer did not expire before the restart.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_TimerStart( ABCC_TimerHandle xHandle,
                              UINT32 lTimeoutMs );

/*------------------------------------------------------------------------------
** Stop timer.
**------------------------------------------------------------------------------
** Arguments:
**    xHandle - Identifier of the timer to be stopped.
**
** Returns:
**    TRUE  - Timer stopped OK.
**    FALSE - Timeout is already reached.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_TimerStop( ABCC_TimerHandle xHandle );

/*------------------------------------------------------------------------------
** Provides delta time since last timer tick call.
** Typically called from timer interrupt.
**------------------------------------------------------------------------------
** Arguments:
**    iDeltaTimeMs - Time in ms since last timerTick call.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TimerTick( const INT16 iDeltaTimeMs );

/*------------------------------------------------------------------------------
** Disable tick action.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TimerDisable( void );

/*------------------------------------------------------------------------------
** Get number of ticks counter by the timer system since start.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Total number of ticks counted by the timer system.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT64 ABCC_TimerGetUptimeMs( void );

#endif  /* inclusion lock */
