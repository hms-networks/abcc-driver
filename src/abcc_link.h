/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** Provides message handling services including flow control message resource
** handling and message destination mapping.
********************************************************************************
*/

#ifndef ABCC_LINK_H_
#define ABCC_LINK_H_

#include "abcc_config.h"
#include "abcc_types.h"

typedef void (*ABCC_LinkNotifyIndType)( void );

/*------------------------------------------------------------------------------
** Must be called before the driver is used.
**------------------------------------------------------------------------------
** Arguments:
**          None.
**
** Returns:
**          None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_LinkInit( void );

/*------------------------------------------------------------------------------
** This method drives the internal receive state machine.
**------------------------------------------------------------------------------
** Arguments: -
**
** Returns:
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_LinkRunDriverRx( void );

/*------------------------------------------------------------------------------
** Write message to the driver.
**
**------------------------------------------------------------------------------
** Arguments:
**          psWriteMsg:    Pointer to message.
**
** Returns:
**          ABCC_ErrorCodeType
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_LinkWriteMessage( ABP_MsgType* psWriteMsg );

/*------------------------------------------------------------------------------
** Provides number of queue entries left in the command queue.
**------------------------------------------------------------------------------
** Arguments:
**          -
**
** Returns:
**          Number of queue entries left in the command queue.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_LinkGetNumCmdQueueEntries( void );

/*------------------------------------------------------------------------------
** Write message to the driver. ABCC_ErrorCodeType is returned.
** Note that if the message was sent successfully before 
** returning from the function, no callback is invoked.
**------------------------------------------------------------------------------
** Arguments:
**          psWriteMsg:    Pointer to message.
**          pnHandler:     Function to call when message is sent.
**
** Returns:
**          ABCC_ErrorCodeType
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_LinkWrMsgWithNotification( ABP_MsgType* psWriteMsg,
                                                           ABCC_LinkNotifyIndType pnHandler );

/*------------------------------------------------------------------------------
** Allocates a message buffer.
**------------------------------------------------------------------------------
** Arguments:
**          ppsBuffer:     Required size (in bytes) of the buffer.
**
** Returns:
**          None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_LinkFree( ABP_MsgType** ppsBuffer );

/*------------------------------------------------------------------------------
** Maps the source id to a message handle function.
** Typically used when sending a command with a given source id knowing what
** function must handle the response ( ABCC_LinkGetMsgHandler() ).
**------------------------------------------------------------------------------
** Arguments:
**          bSrcId:        Key to cross reference table.
**          pnMsgHandler:  Pointer to handle function for message using key
**                         source id.
**
** Returns:
**          ABCC_ErrorCodeType
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_LinkMapMsgHandler( UINT8 bSrcId, ABCC_MsgHandlerFuncType  pnMsgHandler );

/*------------------------------------------------------------------------------
** Get function that is mapped to a given source id ( ABCC_LinkMapMsgHandler() ).
**------------------------------------------------------------------------------
** Arguments:
**          bSrcId:        Given source id.
**
** Returns:
**          Function to call. NULL if no cross reference is found.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_MsgHandlerFuncType ABCC_LinkGetMsgHandler( UINT8 bSrcId );

/*------------------------------------------------------------------------------
** Returns TRUE if the source id has a valid map in the message handler table.
**------------------------------------------------------------------------------
** Arguments:
**          bSrcId:  Given source id.
**
** Returns:
**          TRUE Used.
**          FALSE Not Used.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_LinkIsSrcIdUsed( UINT8 bSrcId );

/*------------------------------------------------------------------------------
** Receive read message if available.
**------------------------------------------------------------------------------
** Arguments:
**          None.
**
** Returns:
**          Pointer to received message. NULL if no message is found.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_LinkReadMessage( void );

/*------------------------------------------------------------------------------
** Check queues for write messages.
**------------------------------------------------------------------------------
** Arguments:
**          None.
**
** Returns:
**          None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_LinkCheckSendMessage( void );

#endif  /* inclusion lock */
