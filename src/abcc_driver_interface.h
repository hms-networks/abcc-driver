/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Defines the generic driver interface implemented by each specific driver.
********************************************************************************
*/

#ifndef PHY_DRV_IF_H_
#define PHY_DRV_IF_H_

#include "abcc_config.h"
#include "abcc_types.h"
#include "abcc_log.h"
#include "abp.h"

/*------------------------------------------------------------------------------
** Reads an amount of bytes from the ABCC memory.
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
#if ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
#define ABCC_DrvParallelRead( iMemOffset, pxData, iLength )                    \
        ABCC_PORT_MemCpy( (pxData), (void*)( ABCC_CFG_PARALLEL_BASE_ADR + (iMemOffset) ), (iLength) )
#else
#define ABCC_DrvParallelRead( iMemOffset, pxData, iLength )                    \
        ABCC_HAL_ParallelRead( iMemOffset, pxData, iLength )
#endif

/*------------------------------------------------------------------------------
** Reads 16 bits from the ABCC memory.
**------------------------------------------------------------------------------
** Arguments:
**    iMemOffset - Offset from ABCC base address.
**                 8 bit char platforms  : iMemOffset in octets
**                 16 bit char platforms : iMemOffset in 16 bit words
**
** Returns:
**    Read UINT8
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
#define ABCC_DrvRead16( iMemOffset )                                           \
        *(volatile UINT16*)( ABCC_CFG_PARALLEL_BASE_ADR + (iMemOffset) )
#else
#define ABCC_DrvRead16( iMemOffset ) ABCC_HAL_ParallelRead16( iMemOffset )
#endif

/*------------------------------------------------------------------------------
** Writes an amount of words to the ABCC memory.
** In case of a memory mapped system this function does not need not be
** implemented.
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
#if ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
#define ABCC_DrvParallelWrite( iMemOffset, pxData, iLength )                   \
        ABCC_PORT_MemCpy( (void*)( ABCC_CFG_PARALLEL_BASE_ADR + (iMemOffset) ), (pxData), (iLength) )
#else
#define ABCC_DrvParallelWrite( iMemOffset, pxData, iLength )                   \
        ABCC_HAL_ParallelWrite( iMemOffset, pxData, iLength )
#endif

/*------------------------------------------------------------------------------
** Write 16 bits to the ABCC memory.
**------------------------------------------------------------------------------
** Arguments:
**    iMemOffset - Offset from ABCC base address.
**                 8 bit char platforms  : iMemOffset in octets
**                 16 bit char platforms : iMemOffset in 16 bit words
**    pbData     - Data to be written to ABCC
**
** Returns:
**    None
**
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
#define ABCC_DrvWrite16( iMemOffset, pbData )                                  \
        *(volatile UINT16*)( ABCC_CFG_PARALLEL_BASE_ADR + (iMemOffset) ) = pbData
#else
#define ABCC_DrvWrite16( iMemOffset, pbData )                                  \
        ABCC_HAL_ParallelWrite16( iMemOffset, pbData )
#endif

/*------------------------------------------------------------------------------
** Get the address to the received read process data for event parallel
** operating mode.
** For a non memory mapped system the hardware abstraction layer need to provide
** a buffer where the read process data can be stored.
** No implementation is needed for a memory mapped system since the macro
** provides the information.
**------------------------------------------------------------------------------
** Argument:
**    None
**
** Returns:
**    Address to RdPdBuffer.
**
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_DRV_PARALLEL_ENABLED
#if ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
#ifdef ABCC_SYS_16_BIT_CHAR
#define ABCC_DrvParallelGetRdPdBuffer()                                        \
        (void*)( ABCC_CFG_PARALLEL_BASE_ADR + ( ABP_RDPD_ADR_OFFSET >> 1 ) )
#else
#define ABCC_DrvParallelGetRdPdBuffer()                                        \
        (void*)( ABCC_CFG_PARALLEL_BASE_ADR + ABP_RDPD_ADR_OFFSET )
#endif
#else
#define ABCC_DrvParallelGetRdPdBuffer()                                        \
        ABCC_HAL_ParallelGetRdPdBuffer()
#endif
#endif

/*------------------------------------------------------------------------------
** Get the address to store the write process data for event parallel operating
** mode.
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
#if ABCC_CFG_DRV_PARALLEL_ENABLED
#if ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
#ifdef ABCC_SYS_16_BIT_CHAR
#define ABCC_DrvParallelGetWrPdBuffer()                                        \
        (void*)( ABCC_CFG_PARALLEL_BASE_ADR + ( ABP_WRPD_ADR_OFFSET >> 1 ) )
#else
#define ABCC_DrvParallelGetWrPdBuffer()                                        \
        (void*)( ABCC_CFG_PARALLEL_BASE_ADR + ABP_WRPD_ADR_OFFSET )
#endif
#else
#define ABCC_DrvParallelGetWrPdBuffer()   ABCC_HAL_ParallelGetWrPdBuffer()
#endif
#endif

/*------------------------------------------------------------------------------
** Run the driver.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvRun )( void );

/*------------------------------------------------------------------------------
** Initializes the driver to default values.
** Must be called before the driver is used.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvInit )( UINT8 bOpmode );

/*------------------------------------------------------------------------------
** Calls in the interrupt context to acknowledge received interrupts.The ISR
** routine will clear all pending interrupts.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Acknowledged interrupts.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ( *pnABCC_DrvISR )( void );

/*------------------------------------------------------------------------------
** Drives the internal send process.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvRunDriverTx )( void );

/*------------------------------------------------------------------------------
** Drives the internal receive process.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Pointer to successfully sent write message.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ( *pnABCC_DrvRunDriverRx )( void );

/*------------------------------------------------------------------------------
** Copy message to the ABCC40 interface if applicable.
** The actual write trigger is done by pnABCC_DrvPrepareWriteMessage. This
** function is only implemented for the parallel operating modes. Check function
** pointer for NULL before calling.
** Note! It is only allowed to call this functions if the driver is ready to
** handle a new message. Use the following functions to ensure this:
** ABCC_DrvParIsReadyForWriteMessage()
** ABCC_DrvParIsReadyForCmd()
**------------------------------------------------------------------------------
** Arguments:
**    psWriteMsg   - Pointer to message.
**
** Returns:
**    -
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvPrepareWriteMessage ) ( ABP_MsgType* psWriteMsg );

/*------------------------------------------------------------------------------
** Writes a message to the driver.
**------------------------------------------------------------------------------
** Arguments:
**    psWriteMsg   - Pointer to message.
**
** Returns:
**    True         - Message was successfully written and can be deallocated
**                   immediately.
**    False        - Message was not yet written and cannot be deallocated.
**                   The psWriteMsg pointer is owned by the driver until the
**                   message is written and the pointer is returned in the
**                   driver execution response.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ( *pnABCC_DrvWriteMessage ) ( ABP_MsgType* psWriteMsg );

/*------------------------------------------------------------------------------
** Writes current process data.
** The data is copied before returning from the method.
**------------------------------------------------------------------------------
** Arguments:
**    pbProcessData - Pointer to process data to be sent.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvWriteProcessData )( void* pbProcessData );

/*------------------------------------------------------------------------------
** Checks if the driver is in the correct state for writing process data to the
** anybus
**------------------------------------------------------------------------------
** Arguments:
**       None
**
** Returns:
**       True        - Driver is in correct state to send WrPd
**       False:      - Driver is not in correct state to send Wrpd
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ( *pnABCC_DrvISReadyForWrPd )( void );

/*------------------------------------------------------------------------------
** Checks if the driver is ready to send a new write message.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    True        - Driver is ready to send a new write message.
**    False       - Driver is not ready to send a new write message.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ( *pnABCC_DrvISReadyForWriteMessage )( void );

/*------------------------------------------------------------------------------
** The host application checks if the Anybus is ready to receive a new command
** message.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    True         - OK to send new command.
**    False        - NOK to send new command.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ( *pnABCC_DrvISReadyForCmd )( void );

/*------------------------------------------------------------------------------
** Sets the number of simultaneous commands that is supported by the
** application.
**------------------------------------------------------------------------------
** Arguments:
**    bNbrOfCmds  - Number of commands that the application is ready to receive.
**
** Returns:
**       None
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvSetNbrOfCmds )( UINT8 bNbrOfCmds );

/*------------------------------------------------------------------------------
**  Sets the current application status.
**  Note! This information is not supported by all protocols.
**------------------------------------------------------------------------------
** Arguments:
**    eAppStatus   - Current application status.
**
** Returns:
**    None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvSetAppStatus )( ABP_AppStatusType eAppStatus );

/*------------------------------------------------------------------------------
** Sets the current process data size.
**------------------------------------------------------------------------------
** Arguments:
**    iReadPdSize  - Size of read process data (bytes)
**    iWritePdSize - Size of write process data (bytes)
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvSetPdSize )( const UINT16 iReadPdSize,
                                       const UINT16 iWritePdSize );

/*------------------------------------------------------------------------------
** Sets Interrupt mask according to h_aci.h.
**------------------------------------------------------------------------------
** Arguments:
**    iIntMask     - Interrupt mask set according to h_aci.h.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *pnABCC_DrvSetIntMask )( const UINT16 iIntMask );

/*------------------------------------------------------------------------------
** Get WrpdBuffer for the user to update.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    Pointer to WrPd buffer.
**------------------------------------------------------------------------------
*/
EXTFUNC void* ( *pnABCC_DrvGetWrPdBuffer )( void );

/*------------------------------------------------------------------------------
** Read module capabillity
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Module capability.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ( *pnABCC_DrvGetModCap )( void );

/*------------------------------------------------------------------------------
** Read module capability
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    Module capability.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ( *pnABCC_DrvGetLedStatus )( void );

/*------------------------------------------------------------------------------
** Gets the Anybus interrupt status. The pnABCC_DrvISR() function will clear all
** pending interrupts. This function must be called before pnABCC_DrvISR() or it
** will always return 0.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    The Anybus interrupt status.
**------------------------------------------------------------------------------
*/
/*EXTFUNC UINT16 ( *pnABCC_DrvGetIntStatus )( void );*/

/*------------------------------------------------------------------------------
** Gets the Anybus state.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    The Anybus state
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ( *pnABCC_DrvGetAnybusState )( void );

/*------------------------------------------------------------------------------
** Reads the read process data.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    A pointer to the read process data; or NULL if no process data to read
**    was available.
**------------------------------------------------------------------------------
*/
EXTFUNC void* ( *pnABCC_DrvReadProcessData )( void );

/*------------------------------------------------------------------------------
** Reads the read message.
**------------------------------------------------------------------------------
** Arguments:
**    None.
**
** Returns:
**    A pointer to the read message; or NULL if no message is available.
**    The pointer, if not NULL, will point to the buffer previously set by
**    calling pnABCC_DrvSetMsgReceiverBuffer().
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ( *pnABCC_DrvReadMessage )( void );

/*------------------------------------------------------------------------------
** Returns supervision bit in status register.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    TRUE: The device is supervised by another network device.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ( *pnABCC_DrvIsSupervised )( void );

/*------------------------------------------------------------------------------
**  Returns anybus status register.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    Anybus status register
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ( *pnABCC_DrvGetAnbStatus )( void );

#endif  /* inclusion lock */
