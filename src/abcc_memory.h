/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Memory management of the driver
********************************************************************************
*/

#ifndef ABCC_MEM_H_
#define ABCC_MEM_H_

#include "abcc_config.h"
#include "abcc_types.h"

/*------------------------------------------------------------------------------
** Buffer status used to keep track of a memory buffer's current state
**------------------------------------------------------------------------------
*/
typedef enum ABCC_MemBufferStatusType
{
   ABCC_MEM_BUFSTAT_FREE = 0,
   ABCC_MEM_BUFSTAT_ALLOCATED = 1,
   ABCC_MEM_BUFSTAT_IN_APPL_HANDLER = 2,
   ABCC_MEM_BUFSTAT_SENT = 3,
   ABCC_MEM_BUFSTAT_OWNED = 4,

   ABCC_MEM_BUFSTAT_UNKNOWN = 0x7FFF
}
ABCC_MemBufferStatusType;

/*------------------------------------------------------------------------------
** Creates a memory pool of buffers with a specific size.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
void ABCC_MemCreatePool( void );

/*------------------------------------------------------------------------------
** Allocates and return pointer to memory of predefined size
** (ABCC_MemCreatePool)
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    Pointer to allocated memory. NULL if pool is empty.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_MemAlloc( void );

/*------------------------------------------------------------------------------
** Return memory to the pool. Note that it is important that the returned memory
** is belonging to the pool from the beginning.
**------------------------------------------------------------------------------
** Arguments:
**    pxItem       - Pointer to the memory to be returned. The pointer is set to
**                   NULL.
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_MemFree( ABP_MsgType** pxItem );

/*------------------------------------------------------------------------------
** Get the currently status of the memory buffer
**------------------------------------------------------------------------------
** Arguments:
**    psMsg - Message buffer to check the status of
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_MemBufferStatusType ABCC_MemGetBufferStatus( ABP_MsgType* psMsg );

/*------------------------------------------------------------------------------
** Set a new status to the memory buffer
**------------------------------------------------------------------------------
** Arguments:
**    psMsg   - Memory buffer to set the status to
**    eStatus - Status to be set to the message buffer
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_MemSetBufferStatus( ABP_MsgType* psMsg,
                                      ABCC_MemBufferStatusType eStatus );

#endif  /* inclusion lock */
