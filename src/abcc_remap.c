/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implementation of remap function
********************************************************************************
*/

#include "abcc_config.h"

#if ABCC_CFG_REMAP_SUPPORT_ENABLED

#include "abcc_types.h"
#include "abp.h"
#include "abcc.h"
#include "abcc_handler.h"
#include "abcc_link.h"

static UINT16   abcc_iNewPdReadSize    = 0;
static UINT16   abcc_iNewPdWriteSize   = 0;

static void abcc_RemapRespMsgSent( void )
{
   ABCC_SetPdSize( abcc_iNewPdReadSize, abcc_iNewPdWriteSize );
   ABCC_CbfRemapDone();
}

ABCC_ErrorCodeType ABCC_SendRemapRespMsg( ABP_MsgType* psMsgResp, UINT16 iNewReadPdSize, const UINT16 iNewWritePdSize )
{
   ABCC_ErrorCodeType eResult;
   abcc_iNewPdReadSize = iNewReadPdSize;
   abcc_iNewPdWriteSize = iNewWritePdSize;

   /*
   ** When ack is sent abcc_RemapRespMsgSent will be called.
   */
   eResult = ABCC_LinkWrMsgWithNotification( psMsgResp, abcc_RemapRespMsgSent );

   return( eResult );
}

#endif
