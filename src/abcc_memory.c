/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Memory allocation implementation.
********************************************************************************
*/

#include "abcc_config.h"
#include "abcc_types.h"
#include "abcc.h"
#include "abcc_memory.h"
#include "abcc_hardware_abstraction.h"
#include "abcc_port.h"
#include "abcc_log.h"

/*
** Set default value for maximum number of resources
*/
#ifndef ABCC_CFG_MAX_NUM_MSG_RESOURCES
#define ABCC_CFG_MAX_NUM_MSG_RESOURCES          ( ABCC_CFG_MAX_NUM_APPL_CMDS + ABCC_CFG_MAX_NUM_ABCC_CMDS )
#endif

/*
** Magic cookie
*/
#define ABCC_MEM_MAGIC_COOKIE  0x5CC5

/*------------------------------------------------------------------------------
** Structure for defining size of memory message allocation
**
** The magic cookie field is used to evaluate if the buffer status field
** is broken. The buffer status could be broken if the user writes outside the
** bounds of the message data area.
**------------------------------------------------------------------------------
*/
typedef struct
{
   ABP_MsgHeaderType16 sHeader;
   UINT32   alData[ ( ABCC_CFG_MAX_MSG_SIZE + 3 ) >> 2 ];
   UINT16   iMagicCookie;
   UINT16   iBufferStatus;
}
PACKED_STRUCT ABCC_MemAllocType;

/*------------------------------------------------------------------------------
** Union used for casting between memory ABCC_MemAllocType and ABP_MsgType.
**------------------------------------------------------------------------------
*/
typedef union
{
   ABCC_MemAllocType* psAllocMsg;
   ABP_MsgType* psMsg;
}
ABCC_MemAllocUnion;

/*------------------------------------------------------------------------------
** Memory pool structure
**
** ------------------
** iNumFreeMsg = 3  |
** ------------------
** Msg 0 pointer    |---|
** ------------------   |
** Msg 1 pointer    |---+--|
** ------------------   |  |
** Msg 2 pointer    |---+--+--|
** ------------------   |  |  |
** Msg 0            |<--|  |  |
** ------------------      |  |
** Msg 1            |<-----|  |
** ------------------         |
** Msg 2            |<--------|
** ------------------
**------------------------------------------------------------------------------
*/
static UINT16 abcc_iNumFreeMsg;
static ABCC_MemAllocUnion abcc_uFreeMsgStack[ ABCC_CFG_MAX_NUM_MSG_RESOURCES ];
static ABCC_MemAllocType  abcc_asMsgPool[ ABCC_CFG_MAX_NUM_MSG_RESOURCES ];

void ABCC_MemCreatePool( void )
{
   UINT16 i;

   abcc_iNumFreeMsg = ABCC_CFG_MAX_NUM_MSG_RESOURCES;

   for( i = 0; i < ABCC_CFG_MAX_NUM_MSG_RESOURCES; i++ )
   {
      abcc_uFreeMsgStack[ i ].psAllocMsg = &abcc_asMsgPool[ i ];
      abcc_asMsgPool[ i ].iMagicCookie = ABCC_MEM_MAGIC_COOKIE;
      abcc_asMsgPool[ i ].iBufferStatus = ABCC_MEM_BUFSTAT_FREE;
   }
}

ABP_MsgType* ABCC_MemAlloc( void )
{
   ABP_MsgType* pxItem = NULL;
   ABCC_PORT_UseCritical();
   ABCC_PORT_EnterCritical();
   if( abcc_iNumFreeMsg > 0 )
   {
      abcc_iNumFreeMsg--;
      pxItem = abcc_uFreeMsgStack[ abcc_iNumFreeMsg ].psMsg;
      ( (ABCC_MemAllocType*)pxItem )->iBufferStatus = ABCC_MEM_BUFSTAT_ALLOCATED;
   }

   ABCC_PORT_ExitCritical();

   ABCC_LOG_DEBUG_MEM( "Mem: Buffer allocated: 0x%p\n", (void*)pxItem );

   return( pxItem );
}

void ABCC_MemFree( ABP_MsgType** pxItem )
{
   ABCC_MemAllocType* const psBuf = (ABCC_MemAllocType*)(*pxItem);
   ABCC_PORT_UseCritical();

   ABCC_LOG_DEBUG_MEM( "Mem: Buffer returned:  0x%p\n", (void*)*pxItem );

   if( psBuf->iMagicCookie != ABCC_MEM_MAGIC_COOKIE )
   {
      ABCC_LOG_FATAL( ABCC_EC_MSG_BUFFER_CORRUPTED,
         (UINT32)psBuf,
         "Message buffer corrupted: 0x%p\n",
         (void*)psBuf );
   }

   if( psBuf->iBufferStatus == ABCC_MEM_BUFSTAT_FREE )
   {
      ABCC_LOG_FATAL( ABCC_EC_MSG_BUFFER_ALREADY_FREED,
         (UINT32)psBuf,
         "Message buffer already freed: 0x%p\n",
         (void*)psBuf );
   }

   ABCC_PORT_EnterCritical();

   abcc_uFreeMsgStack[ abcc_iNumFreeMsg ].psAllocMsg = psBuf;
   abcc_iNumFreeMsg++;
   psBuf->iBufferStatus = ABCC_MEM_BUFSTAT_FREE;
   *pxItem = NULL;

   ABCC_PORT_ExitCritical();
}

ABCC_MemBufferStatusType ABCC_MemGetBufferStatus( ABP_MsgType* psMsg )
{
   const ABCC_MemAllocType* const psBuf = (ABCC_MemAllocType*)psMsg;

   if( psBuf->iMagicCookie != ABCC_MEM_MAGIC_COOKIE )
   {
      ABCC_LOG_FATAL( ABCC_EC_MSG_BUFFER_CORRUPTED,
         (UINT32)psBuf,
         "Message buffer corrupted: 0x%p\n",
         (void*)psBuf );
   }

   return( (ABCC_MemBufferStatusType)psBuf->iBufferStatus );
}

void ABCC_MemSetBufferStatus( ABP_MsgType* psMsg,
                              ABCC_MemBufferStatusType eStatus )
{
   ABCC_MemAllocType* const psBuf = (ABCC_MemAllocType*)psMsg;

   if( psBuf->iMagicCookie != ABCC_MEM_MAGIC_COOKIE )
   {
      ABCC_LOG_FATAL( ABCC_EC_MSG_BUFFER_CORRUPTED,
         (UINT32)psBuf,
         "Message buffer corrupted: 0x%p\n",
         (void*)psBuf );
   }

   psBuf->iBufferStatus = eStatus;
}
