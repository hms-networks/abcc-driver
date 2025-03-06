/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Implements the ABCC segmentation protocol.
********************************************************************************
*/

#ifndef ABCC_SEG_H_
#define ABCC_SEG_H_

#include "abcc_config.h"
#include "abcc_types.h"
#include "abp.h"

/*------------------------------------------------------------------------------
** Init internal variables
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SegmentationInit( void );

/*------------------------------------------------------------------------------
** Checks if a received command message is related to an ongoing segmentation
** session. If that is the case the response is handled by this function.
** Note that the segmentation session is started by the user by calling
** ABCC_StartServerRespSegmentationSession() declared in abcc.h.
**------------------------------------------------------------------------------
** Arguments:
**       psMsg - Pointer to abcc command message
**
** Returns:
**       TRUE  - Message related to a segmentation session and handled by this
**               function.
**       FALSE - Message is not related to a segmentation session.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_HandleSegmentAck( ABP_MsgType* psMsg );

#endif  /* inclusion lock */
