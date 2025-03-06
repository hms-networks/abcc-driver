/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Interface for driver internal interface to the abcc_handler
********************************************************************************
*/

#ifndef ABCC_HANDLER_H_
#define ABCC_HANDLER_H_

#include "abcc_config.h"
#include "abp.h"

/*
** Set default interrupt mask if not defined in abcc_driver_config.h
*/
#if ABCC_CFG_DRV_PARALLEL_ENABLED
#ifndef ABCC_CFG_INT_ENABLE_MASK_PAR
#define ABCC_CFG_INT_ENABLE_MASK_PAR          ( 0 )
#endif
#endif

#if ABCC_CFG_DRV_SPI_ENABLED
#ifndef ABCC_CFG_INT_ENABLE_MASK_SPI
#define ABCC_CFG_INT_ENABLE_MASK_SPI          ( 0 )
#endif
#endif

typedef union
{
   ABP_MsgType*    psMsg;
   ABP_MsgType16*  psMsg16;
} ABCC_MsgType;

/*
** Type for ABCC main states
*/
typedef enum ABCC_MainStateType
{
   ABCC_DRV_INIT,
   ABCC_DRV_SHUTDOWN,
   ABCC_DRV_ERROR,
   ABCC_DRV_WAIT_COMMUNICATION_RDY,
   ABCC_DRV_SETUP,
   ABCC_DRV_RUNNING
} ABCC_MainStateType;

#if ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
/*------------------------------------------------------------------------------
** Flag used for sync measurement
**------------------------------------------------------------------------------
*/
EXTVAR BOOL fAbccUserSyncMeasurementIp;
#endif

/*
** The interrupt mask that has been set to the ABCC at start up.
*/
EXTVAR UINT16 ABCC_iInterruptEnableMask;

/*------------------------------------------------------------------------------
** ABCC_SetPdSize()
** Sets the new process data sizes.
**------------------------------------------------------------------------------
** Arguments:
**       iReadPdSize       - Size of the read process data (in bytes), used from
**                           this point on.
**       iWritePdSize      - Size of the write process data (in bytes), used from
**                           this point on.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetPdSize( const UINT16 iReadPdSize, const UINT16 iWritePdSize );


/*------------------------------------------------------------------------------
** The anybus is ready for communication. This function shall be called either
** due to power up interrupt or initial handshake timeout
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetReadyForCommunication( void );

/*------------------------------------------------------------------------------
** Set main state machine into error state. This will stop ABCC_ISR()
** and ABCC_RunDriver to perform any action towards application or anybus
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetError( ABCC_LogSeverityType eSeverity,
   ABCC_ErrorCodeType eErrorCode,
   UINT32 lAdditionalInfo );

/*------------------------------------------------------------------------------
** Gets currents state
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Current state ( ABCC_MainStateType )
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_MainStateType ABCC_GetMainState( void );

/*------------------------------------------------------------------------------
** Checks if update write process data is requested.
** If requested, the update is performed.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None:
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CheckWrPdUpdate( void );

#endif  /* inclusion lock */
