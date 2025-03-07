/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** ABCC command sequencer API used by the the application.
********************************************************************************
*/
#ifndef ABCC_CMD_SEQ_IF_H_
#define ABCC_CMD_SEQ_IF_H_

#include "abcc_config.h"
#include "abcc_port.h"
#include "abcc_types.h"
#include "abp.h"
#include "abcc_error_codes.h"

/*
** Defines used to build the command sequence.
** The debug option can be enabled in abcc_driver_config.h or in abcc_platform_cfg.h
** Example:
** static const ABCC_CmdSeqType ExampleSequence[] =
** {
**    ABCC_CMD_SEQ( CmdBuilder1,      RespHandler1 ),
**    ABCC_CMD_SEQ( CmdBuilder2,      NULL         ),
**    ABCC_CMD_SEQ( CmdBuilder3,      RespHandler3 ),
**    ABCC_CMD_SEQ_END()
** };
**
*/
#if ABCC_CFG_DEBUG_CMD_SEQ_ENABLED
#define ABCC_CMD_SEQ( cmd, resp ) { cmd, resp, #cmd, #resp }
#else
#define ABCC_CMD_SEQ( cmd, resp ) { cmd, resp }
#endif

#if ABCC_CFG_DEBUG_CMD_SEQ_ENABLED
#define ABCC_CMD_SEQ_END()    { NULL, NULL, NULL, NULL }
#else
#define ABCC_CMD_SEQ_END()    { NULL, NULL }
#endif

/*
** Handle type to reference individual running command sequences.
*/
typedef void *ABCC_CmdSeqHandle;

/*
** Response types for command callback
*/
typedef enum ABCC_CmdSeqCmdStatus
{
   ABCC_CMDSEQ_CMD_SEND,
   ABCC_CMDSEQ_CMD_SKIP,
   ABCC_CMDSEQ_CMD_ABORT
}
ABCC_CmdSeqCmdStatusType;

/*
** Response types for response callback
*/
typedef enum ABCC_CmdSeqRespStatus
{
   ABCC_CMDSEQ_RESP_EXEC_NEXT,
   ABCC_CMDSEQ_RESP_EXEC_CURRENT,
   ABCC_CMDSEQ_RESP_ABORT
}
ABCC_CmdSeqRespStatusType;

/*
** Result code for completion callback
*/
typedef enum ABCC_CmdSeqResult
{
   ABCC_CMDSEQ_RESULT_COMPLETED,
   ABCC_CMDSEQ_RESULT_ABORT_INT,
   ABCC_CMDSEQ_RESULT_ABORT_EXT
}
ABCC_CmdSeqResultType;

/*------------------------------------------------------------------------------
** Type for command callback used by command sequencer.
** See also description of ABCC_CmdSeqAdd() and ABCC_CmdSeqAbort().
**------------------------------------------------------------------------------
** Arguments:
**    psCmdMsg   - Pointer to command buffer
**    pxUserData - Pointer to user-defined data. This is given in the
**                 ABCC_CmdSeqAdd() call.
**
** Returns:
**    ABCC_CmdSeqCmdStatusType:
**    ABCC_CMDSEQ_CMD_SEND  - Send the command to ABCC
**    ABCC_CMDSEQ_CMD_SKIP  - Skip and move to next command in sequence
**    ABCC_CMDSEQ_CMD_ABORT - Abort whole sequence
**------------------------------------------------------------------------------
*/
typedef ABCC_CmdSeqCmdStatusType (*ABCC_CmdSeqCmdHandler)( ABP_MsgType* psCmdMsg, void* pxUserData );

/*------------------------------------------------------------------------------
** Type for response callback used by command sequencer.
** See also description of ABCC_CmdSeqAdd() and ABCC_CmdSeqAbort().
**------------------------------------------------------------------------------
** Arguments:
**    psRespMsg  - Pointer to response buffer
**    pxUserData - Pointer to user-defined data. This is given in the
**                 ABCC_CmdSeqAdd() call.
**
** Returns:
**    ABCC_CmdSeqRespStatusType:
**    ABCC_CMDSEQ_RESP_EXEC_NEXT    - Move to next command in sequence
**    ABCC_CMDSEQ_RESP_EXEC_CURRENT - Execute current command again.
**    ABCC_CMDSEQ_RESP_ABORT        - Abort whole sequence
**------------------------------------------------------------------------------
*/
typedef ABCC_CmdSeqRespStatusType (*ABCC_CmdSeqRespHandler)( ABP_MsgType* psRespMsg, void* pxUserData );

/*------------------------------------------------------------------------------
** Type for sequence-done callback used by command sequencer.
** See also description of ABCC_CmdSeqAdd().
**------------------------------------------------------------------------------
** Arguments:
**    eSeqResult - ABCC_CmdSeqResultType indicating if the sequence ran to
**                 completion or was aborted.
**    pxUserData - Pointer to user-defined data. This is given in the
**                 ABCC_CmdSeqAdd() call.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
typedef void (*ABCC_CmdSeqDoneHandler)( const ABCC_CmdSeqResultType eSeqResult, void* pxUserData );

/*------------------------------------------------------------------------------
** Type used by command sequencer to define command-response callback pairs.
** See also description of ABCC_CmdSeqAdd().
**------------------------------------------------------------------------------
*/
typedef struct ABCC_CmdSeq
{
   ABCC_CmdSeqCmdHandler   pnCmdHandler;
   ABCC_CmdSeqRespHandler  pnRespHandler;
#if ABCC_CFG_DEBUG_CMD_SEQ_ENABLED
   char*                   pcCmdName;
   char*                   pcRespName;
#endif
}
ABCC_CmdSeqType;

#if ABCC_CFG_DRV_CMD_SEQ_ENABLED
/*------------------------------------------------------------------------------
** Add a command sequence.
**
** This is an alternative way of sending commands to the ABCC module. The driver
** provides support for command buffer allocation, resource control and
** sequencing of messages. The user has to provide functions to build messages
** and handle responses (See description of ABCC_CmdSeqCmdHandler,
** ABCC_CmdSeqRespHandler and ABCC_CmdSeqDoneHandler callback functions).
**
** An array of ABCC_CmdSeqType's is provided and defines the command sequence to
** be executed. The last entry in the array is indicated by NULL pointers.
** The next command in the sequence will be executed when the previous command
** has successfully received a response.
**
** If a command sequence response handler exists the response will be passed to
** the application.
**
** If the command sequence response handler is NULL the application will not be
** notified. If the error bit is set the application will be notified by the
** ABCC_CbfDriverError() callback.
**
** If the pnCmdSeqDone function callback exists the application will be
** notified when the whole command sequence has finished.
**
** The number of concurrent command sequences is limited by
** ABCC_CFG_MAX_NUM_CMD_SEQ defined in abcc_driver_config.h.
**
** Example:
** ABCC_CmdSeqAdd( ExampleSequence, CbfDone, psUserData, &ExampleHandle );
**
**------------------------------------------------------------------------------
** Arguments:
**    pasCmdSeq    - Pointer to command sequence to be executed.
**    pnCmdSeqDone - Function pointer for notification when sequence is
**                   done.
**                   Set to NULL to skip notification to application.
**    pxUserData   - Pointer to user-defined data. This pointer will be passed
**                   on to each command + response structure and can be used to
**                   hold data that the command sequence should work with.
**    pxHandle     - Pointer to handle, required if the caller needs to keep
**                   track of different running instances of the same command
**                   sequence. Set to NULL if this is not required.
** Returns:
**    ABCC_EC_NO_ERROR
**    ABCC_EC_NO_RESOURCES
**    ABCC_EC_PARAMETER_NOT_VALID
**    ABCC_EC_INTERNAL_ERROR
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_CmdSeqAdd( const ABCC_CmdSeqType* pasCmdSeq,
                                           const ABCC_CmdSeqDoneHandler pnCmdSeqDone,
                                           void* pxUserData,
                                           ABCC_CmdSeqHandle* pxHandle );

/*------------------------------------------------------------------------------
** Performs immediate termination of the specified command sequence.
**
** Note! This function shall only be used if there is no possibility to use the
** ABORT return value of the command or response handler.
** (see description of ABCC_CmdSeqCmdHandler and ABCC_CmdSeqRespHandler)
**
** Note! Calling this function has some restrictions to perform safe abortion.
** 1. The call has to be done in a context that has the same or lower priority
**    than the ABCC driver main loop context.
** 2. The call has to be done in a context that has the same or lower priority
**    than the ABCC receive message context.
**------------------------------------------------------------------------------
** Arguments:
**   xHandle - Handle for the command sequence to terminate. NULL if all active
**             sequences shall be aborted.
** Returns:
**    ABCC_EC_NO_ERROR
**    ABCC_EC_PARAMETER_NOT_VALID
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_CmdSeqAbort( const ABCC_CmdSeqHandle xHandle );
#endif

#endif  /* inclusion lock */
