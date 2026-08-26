/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Defines the serial driver interface.
********************************************************************************
*/
#ifndef PHY_DRV_SER_IF_H_
#define PHY_DRV_SER_IF_H_

#include "abcc_config.h"
#include "abcc_types.h"
#include "abp.h"

#if ABCC_CFG_DRV_SERIAL_ENABLED

/*------------------------------------------------------------------------------
** Function pointer used by low level serial driver to indicate that a read
** remap is ready and the RdPd size can be updated.
** The reason for this special callback is that the read remap in serial
** operating mode is a unique case where the position for Rx frame size
** update only can be decided on a lower level.
** It is required that the new RdPd size is passed to the serial driver
** before returning from this function.
**------------------------------------------------------------------------------
** Arguments:
**    psMsg:         Pointer to sent read remap response message.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvCbfReadRemapDone )( const ABP_MsgType* const psMsg );

/*------------------------------------------------------------------------------
** Initializes the driver to default values.
** Must be called before the driver is used.
**------------------------------------------------------------------------------
** Arguments:
**    bOpmode:       Operating mode.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerInit( UINT8 bOpmode );


/*------------------------------------------------------------------------------
** Calls in the interrupt context to acknowledge received interrupts.
**
** Remarks:
**    Filler function since interrupt operation is not supported by
**    the serial driver.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16  ABCC_DrvSerISR( void );


/*------------------------------------------------------------------------------
** Drives the internal send process.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerRunDriverTx( void );

/*------------------------------------------------------------------------------
** Drives the internal receive process.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Pointer to successfully sent write message.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_DrvSerRunDriverRx( void );

/*------------------------------------------------------------------------------
** Writes a message to the driver.
**------------------------------------------------------------------------------
** Arguments:
**    psWriteMsg:    Pointer to message.
**
** Returns:
**    True:          Message was successfully written and can be deallocated
**                   immediately.
**    False:         Message was not yet written and cannot be deallocated.
**                   The psWriteMsg pointer is owned by the driver until the
**                   message is written and the pointer is returned in the
**                   driver execution response.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerWriteMessage( ABP_MsgType* psWriteMsg );

/*------------------------------------------------------------------------------
** Writes current process data.
** The data is copied before returning from the method.
**------------------------------------------------------------------------------
** Arguments:
**    pxProcessData: Pointer to process data to be sent.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerWriteProcessData( void* pxProcessData );

/*------------------------------------------------------------------------------
** Checks if the driver is in the correct state for writing process data to the
** Anybus.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    True:          Driver is in correct state to send WrPd
**    False:         Driver is not in correct state to send WrPd
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsReadyForWrPd( void );

/*------------------------------------------------------------------------------
** Checks if the driver is ready to send a new write message.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    True:          Driver is ready to send a new write message.
**    False:         Driver is not ready to send a new write message.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsReadyForWriteMessage( void );

/*------------------------------------------------------------------------------
** The host application checks if the Anybus is ready to receive a new command
** message.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    True:          OK to send new command.
**    False:         NOK to send new command.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsReadyForCmd( void );


/*------------------------------------------------------------------------------
** Sets the number of simultaneous commands that is supported by the
** application.
**------------------------------------------------------------------------------
** Arguments:
**    bNbrOfCmds:    Number of commands that the application is ready to
**                   receive.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetNbrOfCmds( UINT8 bNbrOfCmds );

/*------------------------------------------------------------------------------
**  Sets the current application status.
**  Note! This information is not supported by all protocols.
**------------------------------------------------------------------------------
** Arguments:
**    eAppStatus:    Current application status.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetAppStatus( ABP_AppStatusType eAppStatus );

/*------------------------------------------------------------------------------
** Sets the current process data size.
**------------------------------------------------------------------------------
** Arguments:
**    iReadPdSize:   Size of read process data (bytes)
**    iWritePdSize:  Size of write process data (bytes)
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetPdSize( const UINT16 iReadPdSize, const UINT16 iWritePdSize );

/*------------------------------------------------------------------------------
** Sets the receiver buffer, to be used for the next read message.
**
** Remarks:
**    Not implemented, the serial driver uses the static function
**    DrvSerSetMsgReceiverBuffer() instead.
**------------------------------------------------------------------------------
** Arguments:
**    -
**
** Returns:
**    -
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg );

/*------------------------------------------------------------------------------
** Sets interrupt mask according to abp.h.
**------------------------------------------------------------------------------
** Arguments:
**    iIntMask:      Interrupt mask according to abp.h.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetIntMask( const UINT16 iIntMask );


/*------------------------------------------------------------------------------
** Get WrPdBuffer for the user to update.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Pointer to WrPd buffer.
**------------------------------------------------------------------------------
*/
EXTFUNC void* ABCC_DrvSerGetWrPdBuffer( void );

/*------------------------------------------------------------------------------
** Read module capability.
**
** Remarks:
**    Filler function since module capability is not supported by
**    the serial driver.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_DrvSerGetModCap( void );


/*------------------------------------------------------------------------------
** Read LED status.
**
** Remarks:
**    Filler function since LED status is not supported by
**    the serial driver.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_DrvSerGetLedStatus( void );

/*------------------------------------------------------------------------------
** Gets the Anybus interrupt status.
**
** Remarks:
**    Filler function since interrupt status is not supported by
**    the serial driver.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_DrvSerGetIntStatus( void );

/*------------------------------------------------------------------------------
** Gets the Anybus state.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    The Anybus state.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_DrvSerGetAnybusState( void );

/*------------------------------------------------------------------------------
** Reads the read process data.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    A pointer to the read process data or NULL if no process data to read
**    was available.
**------------------------------------------------------------------------------
*/
EXTFUNC void* ABCC_DrvSerReadProcessData( void );

/*------------------------------------------------------------------------------
** Reads the read message.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    A pointer to the read message or NULL if no message is available.
**    The pointer, if not NULL, will point to the driver's internally
**    allocated read-message buffer.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_DrvSerReadMessage( void );

/*------------------------------------------------------------------------------
**  Returns supervision bit in status register.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    TRUE: The device is supervised by another network device.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsSupervised( void );

/*------------------------------------------------------------------------------
**  Returns Anybus status register.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Anybus status register
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_DrvSerGetAnbStatus( void );

#endif  /* ABCC_CFG_DRV_SERIAL_ENABLED */

#endif  /* inclusion lock */
