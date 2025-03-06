/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Timer implementation.
********************************************************************************
*/

#include "abcc_timer.h"
#include "abcc_log.h"
#include "abcc.h"
#include "abcc_port.h"

/*
** Timer resource structure
*/
typedef struct ABCC_TimerTimeoutType
{
   BOOL  fActive;
   BOOL  fTmoOccured;
   INT32 lTimeLeft;
   ABCC_TimerTimeoutCallbackType pnHandleTimeout;
}
ABCC_TimerTimeoutType;
#define MAX_NUM_TIMERS 3
static ABCC_TimerTimeoutType sTimer[ MAX_NUM_TIMERS ];
static BOOL fTimerEnabled = FALSE;
static UINT64 llTotalTicks = 0;

void ABCC_TimerInit( void )
{
   ABCC_TimerHandle xHandle;

   for( xHandle = 0; xHandle < MAX_NUM_TIMERS; xHandle++ )
   {
      sTimer[ xHandle ].pnHandleTimeout = NULL;
   }
   fTimerEnabled = TRUE;

   llTotalTicks = 0;
}

ABCC_TimerHandle ABCC_TimerCreate( ABCC_TimerTimeoutCallbackType pnHandleTimeout )
{
   ABCC_TimerHandle xHandle = ABCC_TIMER_NO_HANDLE;
   ABCC_PORT_TIMER_UseCritical();

   ABCC_PORT_TIMER_EnterCritical();

   for( xHandle = 0; xHandle < MAX_NUM_TIMERS; xHandle++ )
   {
      if( sTimer[ xHandle ].pnHandleTimeout == NULL )
      {
         sTimer[ xHandle ].fActive = FALSE;
         sTimer[ xHandle ].fTmoOccured = FALSE;
         sTimer[ xHandle ].pnHandleTimeout = pnHandleTimeout;
         break;
      }
   }
   ABCC_PORT_TIMER_ExitCritical();

   if( xHandle >=  MAX_NUM_TIMERS )
   {
      xHandle = ABCC_TIMER_NO_HANDLE;
   }
   return( xHandle );
}

BOOL ABCC_TimerStart( ABCC_TimerHandle xHandle,
                      UINT32 lTimeoutMs )
{
   BOOL fTmo;
   ABCC_PORT_TIMER_UseCritical();

   if( !sTimer[ xHandle ].pnHandleTimeout )
   {
      ABCC_LOG_ERROR( ABCC_EC_UNEXPECTED_NULL_PTR,
         0,
         "Timer timeout handle not registered\n" );
      return( FALSE );
   }

   ABCC_PORT_TIMER_EnterCritical();
   fTmo = sTimer[ xHandle ].fTmoOccured;
   sTimer[ xHandle ].lTimeLeft = (INT32)lTimeoutMs;
   sTimer[ xHandle ].fTmoOccured = FALSE;
   sTimer[ xHandle ].fActive = TRUE;

   ABCC_PORT_TIMER_ExitCritical();
   return( fTmo );
}

BOOL ABCC_TimerStop( ABCC_TimerHandle xHandle )
{
   BOOL fTmo;
   ABCC_PORT_TIMER_UseCritical();

   ABCC_PORT_TIMER_EnterCritical();
   fTmo = sTimer[ xHandle ].fTmoOccured;

   sTimer[ xHandle ].fActive = FALSE;
   sTimer[ xHandle ].fTmoOccured = FALSE;

   ABCC_PORT_TIMER_ExitCritical();
   return( fTmo );
}

void ABCC_TimerTick(const INT16 iDeltaTimeMs)
{
   ABCC_TimerHandle xHandle;
   ABCC_PORT_TIMER_UseCritical();

   if( !fTimerEnabled )
   {
      return;
   }

   ABCC_PORT_TIMER_EnterCritical();

   for( xHandle = 0; xHandle < MAX_NUM_TIMERS; xHandle++ )
   {
       if( ( sTimer[ xHandle ].pnHandleTimeout != NULL ) &&
             ( sTimer[ xHandle ].fActive == TRUE ) )
       {
          sTimer[ xHandle ].lTimeLeft -= (INT32)iDeltaTimeMs;
          if( sTimer[ xHandle ].lTimeLeft <= 0 )
          {
             sTimer[ xHandle ].fTmoOccured = TRUE;
             sTimer[ xHandle ].fActive = FALSE;
             sTimer[ xHandle ].pnHandleTimeout();
          }
       }
   }

   llTotalTicks += iDeltaTimeMs;

   ABCC_PORT_TIMER_ExitCritical();
}

void ABCC_TimerDisable( void )
{
   fTimerEnabled = FALSE;
}

UINT64 ABCC_TimerGetUptimeMs( void )
{
   ABCC_PORT_TIMER_UseCritical();

   UINT64 llUptime;

   ABCC_PORT_TIMER_EnterCritical();

   llUptime = llTotalTicks;

   ABCC_PORT_TIMER_ExitCritical();

   return( llUptime );
}
