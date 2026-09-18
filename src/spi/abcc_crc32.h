/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** This is the public header file for the CRC calculation routine.
********************************************************************************
*/

#ifndef CRC32_H
#define CRC32_H

#include "abcc_config.h"
#include "abcc_types.h"

/*------------------------------------------------------------------------------
** CRC_Crc32()
**
** Calculates CRC32 checksum for ABCC40 SPI frames.
**
** Expects input sizes divisible by two (16-bit words). Odd-sized buffers
** are incompatible with this optimized algorithm.
**------------------------------------------------------------------------------
** Inputs:
**    pbBuffer                 - Where to start the calculation.
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
