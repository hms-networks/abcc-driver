/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implements abcc timer service.
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
** Type for identifying timer
*/
typedef UINT8 ABCC_TimerHandle;

/*------------------------------------------------------------------------------
** void ABCC_TimerInit( void );
** Need to called before the timer is used
**------------------------------------------------------------------------------
** Arguments:
**    None
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TimerInit( void );

/*------------------------------------------------------------------------------
** Allocates a timer resource and returns a handle.
**------------------------------------------------------------------------------
** Arguments:
**    pnHandleTimeout: Function to call if timeout.
** Returns:
**    ABCC_TimerHandle ( Used as identifier when using timer functions. )
**                       TIMER_NO_HANDLE is returned if no timer was available
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_TimerHandle ABCC_TimerCreate( ABCC_TimerTimeoutCallbackType pnHandleTimeout );

/*------------------------------------------------------------------------------
** Start timer.
** When the timeout is reached, the registered callback function is called.
** Note!! This function is dependent on that ABCC_TimerTick() is called
** on regular basis.
**------------------------------------------------------------------------------
** Arguments:
**    xHandle:     Identifier of timer to be started.
**    lTimeoutMs:  Timeout in ms.
**
** Returns:
**    TRUE if the timer had expired before re-start.
**    FALSE Timer had not expired before re-start.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_TimerStart( ABCC_TimerHandle xHandle,
                              UINT32 lTimeoutMs );

/*------------------------------------------------------------------------------
** Stop timer.
**------------------------------------------------------------------------------
** Arguments:
**    xHandle: Identifier of timer to be stopped.
** Returns:
**    FALSE If timeout is already reached
**    TRUE: Timer stopped OK
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_TimerStop( ABCC_TimerHandle xHandle );

/*------------------------------------------------------------------------------
** ABCC_TimerTick(). Provides delta time since last timer tick call.
** Typically called from timer interrupt.
**------------------------------------------------------------------------------
** Arguments:
**    iDeltaTimeMs: Time in ms since last timerTick call
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TimerTick( const INT16 iDeltaTimeMs );

/*------------------------------------------------------------------------------
** ABCC_TimerDisable(). Disable tick action
**------------------------------------------------------------------------------
** Arguments:
**    None
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TimerDisable( void );

/*------------------------------------------------------------------------------
** Get number of ticks counter by the timer system since start.
**------------------------------------------------------------------------------
** Arguments:
**    None
** Returns:
**    Total number of ticks counted by the timer system.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT64 ABCC_TimerGetUptimeMs( void );

#endif  /* inclusion lock */
