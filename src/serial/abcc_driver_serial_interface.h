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
** Function pointer invoked by the serial driver upon completion of a read
** remap response.
**
** In serial operating mode, the read remap operation uniquely determines the
** Rx frame size at the point of transmission. Consequently, the driver cannot
** calculate the new Read Process Data (RdPd) size until this specific message
** is fully prepared.
**
** This callback notifies the upper layer that the remap response is ready.
** The handler must update the RdPd size via the driver interface before
** returning, ensuring the correct frame length is applied to the subsequent
** communication cycle.
**------------------------------------------------------------------------------
** Arguments:
**       psMsg:   Pointer to the completed read remap response message.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvCbfReadRemapDone )( const ABP_MsgType* const psMsg );

/*------------------------------------------------------------------------------
** Initialize the driver to default values.
** Must be called before the driver is used.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerInit( UINT8 bOpmode );

/*------------------------------------------------------------------------------
** Call in the interrupt context to acknowledge received interrupts.
**
** Remarks:
**       The ISR routine will clear all pending interrupts.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Acknowledged interrupts.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16  ABCC_DrvSerISR( void );

/*------------------------------------------------------------------------------
** Process the serial driver's transmit state machine.
**
** Prepares and sends the next ping telegram, including write message
** fragments, process data, and control bits.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerRunDriverTx( void );

/*------------------------------------------------------------------------------
** Process the serial driver's receive state machine.
**
** Validates the received pong telegram (CRC, T-bit toggle), extracts status,
** process data, and any read message fragments. Upon successful acknowledgment
** of a previously sent write message, ownership of that message buffer is
** returned to the caller.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Pointer to the acknowledged write message, or NULL if no write
**       message completed during this cycle.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_DrvSerRunDriverRx( void );

/*------------------------------------------------------------------------------
** Write a message to the driver.
**------------------------------------------------------------------------------
** Arguments:
**       psWriteMsg:    Pointer to message.
**
** Returns:
**       True:          Message was successfully written and can be deallocated
**                      immediately.
**       False:         Message was not yet written and cannot be deallocated.
**                      The psWriteMsg pointer is owned by the driver until the
**                      message is written and the pointer is returned in the
**                      driver execution response.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerWriteMessage( ABP_MsgType* psWriteMsg );

/*------------------------------------------------------------------------------
** Write current process data.
** The data is copied before returning from the method.
**------------------------------------------------------------------------------
** Arguments:
**       pxProcessData: Pointer to process data to be sent.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerWriteProcessData( void* pxProcessData );

/*------------------------------------------------------------------------------
** Check if the driver is in the correct state for writing process data to the
** ABCC.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       True:          Driver is in correct state to send WrPd.
**       False:         Driver is not in correct state to send WrPd.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsReadyForWrPd( void );

/*------------------------------------------------------------------------------
** Check if the driver is ready to send a new write message.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       True:          Driver is ready to send a new write message.
**       False:         Driver is not ready to send a new write message.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsReadyForWriteMessage( void );

/*------------------------------------------------------------------------------
** The host application checks if the ABCC is ready to receive a new command
** message.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       True:          OK to send new command.
**       False:         NOK to send new command.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsReadyForCmd( void );

/*------------------------------------------------------------------------------
** Set the number of simultaneous commands
** that is supported by the application.
**------------------------------------------------------------------------------
** Arguments:
**       bNbrOfCmds:    Number of commands that the application is ready to
**                      receive. 
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetNbrOfCmds( UINT8 bNbrOfCmds );

/*------------------------------------------------------------------------------
** Set the current application status.
** Note! This is not supported by serial protocol.
**------------------------------------------------------------------------------
** Arguments:
**       eAppStatus:    Current application status.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetAppStatus( ABP_AppStatusType eAppStatus );

/*------------------------------------------------------------------------------
** Set the current process data size.
**------------------------------------------------------------------------------
** Arguments:
**       iReadPdSize:   Size of read process data (bytes).
**       iWritePdSize:  Size of write process data (bytes).
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetPdSize( const UINT16 iReadPdSize, const UINT16 iWritePdSize );

/*------------------------------------------------------------------------------
** Set the receiver buffer, to be used for the next read message.
**------------------------------------------------------------------------------
** Arguments:
**       psReadMsg:     Pointer where next read message will be put.
**                      psReadMsg is not allowed to contain a NULL value.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg );

/*------------------------------------------------------------------------------
** Set interrupt mask.
** Note! This is not supported by serial protocol.
**------------------------------------------------------------------------------
** Arguments:
**       iIntMask:      Interrupt mask.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_DrvSerSetIntMask( const UINT16 iIntMask );

/*------------------------------------------------------------------------------
** Get WrpdBuffer for the user to update.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Pointer to WrPd buffer.
**------------------------------------------------------------------------------
*/
EXTFUNC void* ABCC_DrvSerGetWrPdBuffer( void );

/*------------------------------------------------------------------------------
** Get module capability.
** Note! This is not supported by serial protocol.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_DrvSerGetModCap( void );

/*------------------------------------------------------------------------------
** Get LED status.
** Note! This is not supported by serial protocol.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_DrvSerGetLedStatus( void );

/*------------------------------------------------------------------------------
** Get the Anybus interrupt status.
** Note! This is not supported by serial protocol.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       0.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_DrvSerGetIntStatus( void );

/*------------------------------------------------------------------------------
** Get the Anybus state.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       The Anybus state.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_DrvSerGetAnybusState( void );

/*------------------------------------------------------------------------------
** Get pointer to valid read process data.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       A pointer to the read process data; or NULL if no process data to read
**       was available.
**------------------------------------------------------------------------------
*/
EXTFUNC void* ABCC_DrvSerReadProcessData( void );

/*------------------------------------------------------------------------------
** Get pointer to the read message.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       A pointer to the read message; or NULL if no message is available.
**       The pointer, if not NULL, will point to the buffer previously set by
**       calling ABCC_DrvSetMsgReceiverBuffer().
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_DrvSerReadMessage( void );

/*------------------------------------------------------------------------------
**  Return supervision bit in status register.
**------------------------------------------------------------------------------
** Arguments:
**          -
**
** Returns:
**          TRUE: The device is supervised by another network device.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_DrvSerIsSupervised( void );

/*------------------------------------------------------------------------------
**  Return Anybus status register.
**------------------------------------------------------------------------------
** Arguments:
**          -
**
** Returns:
**          Anybus status register
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_DrvSerGetAnbStatus( void );

#endif  /* ABCC_CFG_DRV_SERIAL_ENABLED */

#endif  /* inclusion lock */
