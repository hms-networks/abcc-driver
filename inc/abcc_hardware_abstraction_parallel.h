/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Target dependent interface for parallel 8 / 16 operation mode.
********************************************************************************
*/

#ifndef ABCC_HAL_PAR_
#define ABCC_HAL_PAR_
#include "abcc_config.h"
#include "abcc_types.h"
#include "abp.h"

/*------------------------------------------------------------------------------
** Reads an amount of bytes from the ABCC memory. Implementation is not needed
** for a memory mapped system.
** This function/macro will be used by the driver when reading process data or
** message data from the ABCC memory.
**------------------------------------------------------------------------------
** Arguments:
**    iMemOffset  - Memory offset to start writing to.
**                  8 bit char platforms  : iMemOffset in octets
**                  16 bit char platforms : iMemOffset in 16 bit words
**    pxData      - Pointer to the data to be written.
**    iLength     - The amount of data to write in octets.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if !ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
EXTFUNC void ABCC_HAL_ParallelRead( UINT16 iMemOffset, void* pxData, UINT16 iLength );
#endif

/*------------------------------------------------------------------------------
** Reads 16 bits from the ABCC memory.
**------------------------------------------------------------------------------
** Arguments:
**    iMemOffset - Offset from ABCC base address
**                 8 bit char platforms  : iMemOffset in octets
**                 16 bit char platforms : iMemOffset in 16 bit words
**
** Returns:
**    Read UINT16
**------------------------------------------------------------------------------
*/
#if !ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
EXTFUNC UINT16 ABCC_HAL_ParallelRead16( UINT16 iMemOffset );
#endif

/*------------------------------------------------------------------------------
** Writes an amount of bytes to the ABCC memory
** This function will be used by the driver when writing process data or message
** data to the ABCC memory.
**------------------------------------------------------------------------------
** Arguments:
**    iMemOffset  - Memory offset to start writing to.
**                  8 bit char platforms  : iMemOffset in octets
**                  16 bit char platforms : iMemOffset in 16 bit words
**    pxData      - Pointer to the data to be written.
**    iLength     - The amount of data to write in octets.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if !ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
EXTFUNC void ABCC_HAL_ParallelWrite( UINT16 iMemOffset,
                                     void* pxData,
                                     UINT16 iLength );
#endif

/*------------------------------------------------------------------------------
** Writes 16 bits to the ABCC memory.
**------------------------------------------------------------------------------
** Arguments:
**    iMemOffset - Offset from ABCC base address.
**                 8 bit char platforms  : iMemOffset in octets
**                 16 bit char platforms : iMemOffset in 16 bit words
**    iData      - Data to be written to ABCC
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if !ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
EXTFUNC void ABCC_HAL_ParallelWrite16( UINT16 iMemOffset, UINT16 iData );
#endif

/*------------------------------------------------------------------------------
** Get the address to the received read process data.
** For a non memory mapped system the hardware abstraction layer need to provide
** a buffer where the read process data can be stored.
**------------------------------------------------------------------------------
** Argument:
**    None
**
** Returns:
**    Address to RdPdBuffer.
**
**------------------------------------------------------------------------------
*/
#if !ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
   EXTFUNC void* ABCC_HAL_ParallelGetRdPdBuffer( void );
#endif

/*------------------------------------------------------------------------------
** Get the address to store the write process data.
** For a non memory mapped system the hardware abstraction layer need to provide
** a buffer where the write process data can be stored.
** No implementation is needed for a memory mapped system since the macro
** provides the information.
**------------------------------------------------------------------------------
** Argument:
**    None
**
** Returns:
**    Address to WrPdBuffer
**
**------------------------------------------------------------------------------
*/
#if !ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
   EXTFUNC void* ABCC_HAL_ParallelGetWrPdBuffer( void );
#endif

#endif  /* inclusion lock */
