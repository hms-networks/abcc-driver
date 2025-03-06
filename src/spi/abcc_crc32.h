/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** This is the public header file for the CRC calculation routines.
********************************************************************************
*/

#ifndef CRC32_H
#define CRC32_H

#include "abcc_config.h"
#include "abcc_types.h"

/*------------------------------------------------------------------------------
** CRC_Crc32()
**
** Calculates a CRC32 checksum on the indicated octets for SPI use.
**------------------------------------------------------------------------------
** Inputs:
**    pbBufferStart            - Where to start the calculation.
**    xLength                  - The amount of octets to include.
**
** Outputs:
**    Returns                  - The calculated CRC32 checksum for the SPI.
**
** Usage:
**    iCrc = CRC_Crc32( pbStart, 20 );
**------------------------------------------------------------------------------
*/
EXTFUNC UINT32 CRC_Crc32( UINT8* pbBuffer, size_t xLength );

#endif  /* inclusion lock */
