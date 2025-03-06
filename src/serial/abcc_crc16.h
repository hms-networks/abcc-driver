/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** This is the public header file for the CRC calculation routines.
********************************************************************************
*/

#ifndef CRC16_H
#define CRC16_H

#include "abcc_config.h"
#include "abcc_types.h"

/*---------------------------------------------------------------------------
**
** CRC_Crc16()
**
** Calculates a CRC16 checksum on the indicated bytes.
**
**---------------------------------------------------------------------------
**
** Inputs:
**    pbBufferStart            - Where to start calculation
**    iLength                  - The amount of bytes to include
**
** Outputs:
**    Returns                  - The calculated CRC16 checksum
**
** Usage:
**    iCrc = CRC_Crc16( pbStart, 20 );
**
**---------------------------------------------------------------------------
*/

EXTFUNC UINT16 CRC_Crc16( UINT8* pbBufferStart, UINT16 iLength );

#endif  /* inclusion lock */
