/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** ABCC driver API used by the the application.
********************************************************************************
*/

#include "abcc_software_port.h"
#include "abcc_config.h"

#ifndef ABCC_PORT_H_
#define ABCC_PORT_H_

/*------------------------------------------------------------------------------
** Macro is used by driver for debug prints such as events or error debug
** information. If not defined the driver will be silent.
** Note! Don't use this macro directly for printing, use ABCC_LOG_*()
**       instead as it may add additional information based on the
**       configuration.
** Note! The compiler need to be C99 compliant to support VA_ARGS in macro.
**------------------------------------------------------------------------------
** Arguments:
**    printf style with a format string followed by variable number of
**    arguments.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_printf
#define ABCC_PORT_printf()
#endif

/*------------------------------------------------------------------------------
** vprintf style function, see standard C documentation for details.
**------------------------------------------------------------------------------
** Arguments:
**    vprintf style function, see standard C documentation for details.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_vprintf
#if ABCC_CFG_LOG_STRINGS_ENABLED
#error "ABCC_PORT_vprintf() must be ported if ABCC_CFG_LOG_STRINGS_ENABLED is 1."
#endif
#endif

/*------------------------------------------------------------------------------
** Critical section implementation guidance.
**
** Critical sections are used when there is a risk of resource conflicts or race
** conditions between ABCC interrupt handler context and the application thread.
**
** Six macros are used to implement the critical sections, three of these are
** specific to the timer functionality of the driver:
** ABCC_PORT_UseCritical()
** ABCC_PORT_EnterCritical()
** ABCC_PORT_ExitCritical()
** ABCC_PORT_TIMER_UseCritical()
** ABCC_PORT_TIMER_EnterCritical()
** ABCC_PORT_TIMER_ExitCritical()
**
** Depending on the configuration of the driver there are different requirements
** on the critical section implementation. Please choose the most suitable
** implementation from the numbered list below. The first statement that is true
** will choose the requirement.
**
** 1. The first three macros need to be implemented if any of the statements
**    below are true.
**       - Any message handling is done within interrupt context.
**
**       Requirements:
**       - The implementation must support that a critical section is entered
**       from interrupt context. ABCC_PORT_UseCritical() should be used for any
**       declarations needed in advance by ABCC_PORT_EnterCritical().
**       - When entering the critical section the required interrupts i.e.
**       any interrupt that may lead to driver access, must be disabled. When
**       leaving the critical section the interrupt configuration must be
**       restored to the previous state.
**
** 2. ABCC_PORT_EnterCritical() and ABCC_PORT_ExitCritical() need to be
**    implemented if any of the statements below are true.
**       - The application is accessing the ABCC driver message interface from
**       different processes or threads without protecting the message
**       interface on a higher level (semaphores or similar).
**
**       Requirement:
**         - When entering the critical section the required interrupts i.e. any
**         interrupt that may lead to driver access, must be disabled. When
**         leaving the critical section the interrupts must be enabled again.
**
** 3. If none of the above is true no implementation is required.
**
** If the application is calling ABCC_RunTimerSystem() from a timer interrupt,
** the last three timer-specific macros will be defined. If left undefined by
** the application in abcc_software_port.h, these will assume the same
'' definition as the corresponding three macros specified above:
**    Requirements:
**    - ABCC_PORT_TIMER_UseCritical() should be used for any declarations needed
**    in advance by ABCC_PORT_TIMER_EnterCritical().
**    - When entering the critical section the required interrupts i.e.
**    any interrupt that may lead to driver access, specifically the timer
**    system, must be disabled. When leaving the critical section the interrupt
**    configuration must be restored to the previous state.
**
**------------------------------------------------------------------------------
*/

/*------------------------------------------------------------------------------
** Please read the general description above about the critical sections
** implementation for implementation guidance.
**
** If any preparation is needed before calling "ABCC_PORT_EnterCritical()" or
** "ABCC_PORT_ExitCritical()" this macro is used to add HW specific necessities.
** This could for example be a local variable to store the current interrupt
** status.
**
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_UseCritical
#define ABCC_PORT_UseCritical()
#endif

/*------------------------------------------------------------------------------
** Please read the general description above about the critical sections
** implementation for implementation guidance.
**
** If required the macro temporary disables interrupts
** to avoid conflict. Note that all interrupts that could lead to a driver
** access must be disabled.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_EnterCritical
#define ABCC_PORT_EnterCritical()
#endif

/*------------------------------------------------------------------------------
** Please read the general description above about the critical sections
** implementation for implementation guidance.
**
** Restore interrupts to the state when "ABCC_PORT_EnterCritical()"
** was called.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_ExitCritical
#define ABCC_PORT_ExitCritical()
#endif

/*------------------------------------------------------------------------------
** Please read the general description above about the critical sections
** implementation for implementation guidance.
**
** If any preparation is needed before calling "ABCC_PORT_TIMER_EnterCritical()"
** or "ABCC_PORT_TIMER_ExitCritical()" this macro is used to add HW specific
** necessities. This could for example be a local variable to store the current
** interrupt status.
**
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_TIMER_UseCritical
#define ABCC_PORT_TIMER_UseCritical() ABCC_PORT_UseCritical()
#endif

/*------------------------------------------------------------------------------
** Please read the general description above about the critical sections
** implementation for implementation guidance.
**
** If required the macro temporarily disables interrupts
** to avoid conflict. Note that timer interrupts that could lead to a driver
** access must be disabled.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_TIMER_EnterCritical
#define ABCC_PORT_TIMER_EnterCritical() ABCC_PORT_EnterCritical()
#endif

/*------------------------------------------------------------------------------
** Please read the general description above about the critical sections
** implementation for implementation guidance.
**
** Restore interrupts to the state when "ABCC_PORT_TIMER_EnterCritical()"
** was called.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_TIMER_ExitCritical
#define ABCC_PORT_TIMER_ExitCritical() ABCC_PORT_ExitCritical()
#endif

/*------------------------------------------------------------------------------
** Copy a number of octets, from the source pointer to the destination pointer.
**
** Define ABCC_PORT_MemCpy in abcc_software_port.h to override default
** implementation.
**
** This function can be modified to use performance enhancing platform specific
** instructions. The default implementation is memcpy().
** Note that for a 16 bit char platform this function only supports an even
** number of octets.
**------------------------------------------------------------------------------
** Arguments:
**    pbDest         - Pointer to the destination.
**    pbSource       - Pointer to source data.
**    iNbrOfOctets   - The number of octets that shall be copied.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_MemCpy
#ifdef ABCC_SYS_16_BIT_CHAR
#include <string.h>
#define ABCC_PORT_MemCpy( pbDest, pbSource, iNbrOfOctets )                     \
        memcpy( pbDest, pbSource, ( (iNbrOfOctets) + 1 ) >> 1 );
#else
#if ABCC_CFG_PAR_EXT_BUS_ENDIAN_DIFF
/*
** Use special memcpy implementation if the external and internal bus has
** different endianess. The copy must be byte oriented.
*/
void* ABCC_CopyImpl( void *pbDest, const void *pbSource, int iNbrOfOctets );

#define ABCC_PORT_MemCpy ABCC_CopyImpl
#else
#include <string.h>
#define ABCC_PORT_MemCpy memcpy
#endif
#endif
#endif

/*------------------------------------------------------------------------------
** Copy a native formatted string to a packed string
**
** Define ABCC_PORT_StrCpyToPacked in abcc_software_port.h to override default
** implementation.
**
** This can typically be handled by a standard memcpy (with destination octet
** offset considerations) but for 16 bit char platforms special handling is
** needed. Example implementation for 16 bit char platform is implemented in
** abcc_copy.c.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Pointer to the destination.
**    iDestOctetOffset  - Octet offset to the destination where the copy will
**                        begin.
**    pxSrc             - Pointer to source data.
**    iNbrOfChars       - The number of bytes that shall be copied.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_StrCpyToPacked
#ifdef ABCC_SYS_16_BIT_CHAR
void ABCC_StrCpyToPackedImpl( void* pxDest, UINT16 iDestOctetOffset,
                               const void* pxSrc, UINT16 iNbrOfChars );
#define ABCC_PORT_StrCpyToPacked ABCC_StrCpyToPackedImpl
#else
#define ABCC_PORT_StrCpyToPacked( pxDest, iDestOctetOffset, pxSrc,             \
                                  iNbrOfChars )                                \
        ABCC_PORT_MemCpy( (void*)&( (UINT8*)(pxDest) )[ iDestOctetOffset ],    \
                           pxSrc, iNbrOfChars )
#endif
#endif

/*------------------------------------------------------------------------------
** Copy a packed string to a native formatted string
**
** Define ABCC_PORT_StrCpyToNative in abcc_software_port.h to override default
** implementation.
**
** This can typically be handled by a standard memcpy (with source octet
** offset considerations) but for 16 bit char platforms special handling is
** needed. Example implementation for 16 bit char platform is implemented in
** abcc_copy.c.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Pointer to the destination.
**    pxSrc             - Pointer to source data.
**    iSrcOctetOffset   - Octet offset to the source where the copy will begin.
**    iNbrOfChars       - The number of bytes that shall be copied.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_StrCpyToNative
#ifdef ABCC_SYS_16_BIT_CHAR
void ABCC_StrCpyToNativeImpl( void* pxDest, const void* pxSrc,
                              UINT16 iSrcOctetOffset, UINT16 iNbrOfChars );
#define ABCC_PORT_StrCpyToNative ABCC_StrCpyToNativeImpl
#else
#define ABCC_PORT_StrCpyToNative( pxDest, pxSrc, iSrcOctetOffset, iNbrOfChars )\
        ABCC_PORT_MemCpy( (void*)(pxDest),                                     \
                          (void*)&( (UINT8*)(pxSrc) )[ iSrcOctetOffset ],      \
                          iNbrOfChars )
#endif
#endif

/*------------------------------------------------------------------------------
** Copy a number of octets from a source to a destination.
**
** Define ABCC_PORT_CopyOctets in abcc_software_port.h to override default
** implementation.
**
** This can typically be handled by a standard memcpy (with source/dest octet
** offset considerations) but for 16 bit char platforms special handling is
** needed. Example implementation for 16 bit char platform is implemented in
** abcc_copy.c.
**
** For a 16 bit char platform octet alignment support (the octet offset is odd)
** need to be considered when porting this macro.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Base pointer to the destination.
**    iDestOctetOffset  - Octet offset to the destination where the copy will
**                        begin.
**    pxSrc             - Base pointer to source data.
**    iSrcOctetOffset   - Octet offset to the source where the copy will begin.
**    iNumOctets        - Number of octets to copy.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_CopyOctets
#ifdef ABCC_SYS_16_BIT_CHAR
void ABCC_CopyOctetsImpl( void* pxDest, UINT16 iDestOctetOffset,
                          const void* pxSrc, UINT16 iSrcOctetOffset,
                          UINT16 iNumOctets );
#define ABCC_PORT_CopyOctets ABCC_CopyOctetsImpl
#else
#define ABCC_PORT_CopyOctets( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset, \
                              iNumOctets )                                      \
        ABCC_PORT_MemCpy( (void*)&( (UINT8*)(pxDest) )[ iDestOctetOffset ],     \
                          (void*)&( (UINT8*)(pxSrc) )[ iSrcOctetOffset ],       \
                          iNumOctets )
#endif
#endif

/*------------------------------------------------------------------------------
** Copy 8 bits from a source to a destination.
**
** Define ABCC_PORT_Copy8 in abcc_software_port.h to override default
** implementation.
**
** This can typically be handled by a standard value assignment
** (with source/dest octet offset considerations).
**
** For a 16 bit char platform octet alignment support (the octet offset is odd)
** need to be considered when porting this macro.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Base pointer to the destination.
**    iDestOctetOffset  - Octet offset to the destination where the copy will
**                        begin.
**    pxSrc             - Base pointer to source data.
**    iSrcOctetOffset   - Octet offset to the source where the copy will begin.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_Copy8
#ifdef ABCC_SYS_16_BIT_CHAR
#define ABCC_PORT_Copy8( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset )    \
        ABCC_CopyOctetsImpl( pxDest, iDestOctetOffset, pxSrc,                  \
                             iSrcOctetOffset, 1 )
#else
#define ABCC_PORT_Copy8( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset )    \
        ( (UINT8*)(pxDest) )[ iDestOctetOffset ] =                             \
        ( (UINT8*)(pxSrc) )[ iSrcOctetOffset ]
#endif
#endif

/*------------------------------------------------------------------------------
** Copy 16 bits from a source to a destination.
**
** Define ABCC_PORT_Copy16 in abcc_software_port.h to override default
** implementation.
**
** Octet alignment support (the octet offset is odd) need to be considered
** when porting this macro.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Base pointer to the destination.
**    iDestOctetOffset  - Octet offset to the destination where the copy will
**                        begin.
**    pxSrc             - Base pointer to source data.
**    iSrcOctetOffset   - Octet offset to the source where the copy will begin.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_Copy16
#define ABCC_PORT_Copy16( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset )   \
        ABCC_PORT_CopyOctets( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset, 2 )
#endif

/*------------------------------------------------------------------------------
** Copy 32 bits from a source to a destination.
**
** Define ABCC_PORT_Copy32 in abcc_software_port.h to override default
** implementation.
**
** Octet alignment support (the octet offset is odd) need to be considered
** when porting this macro.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Base pointer to the destination.
**    iDestOctetOffset  - Octet offset to the destination where the copy will
**                        begin.
**    pxSrc             - Base pointer to source data.
**    iSrcOctetOffset   - Octet offset to the source where the copy will begin.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#ifndef ABCC_PORT_Copy32
#define ABCC_PORT_Copy32( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset )   \
        ABCC_PORT_CopyOctets( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset, 4 )
#endif

/*------------------------------------------------------------------------------
** Copy 64 bits from a source to a destination.
**
** Define ABCC_PORT_Copy64 in abcc_software_port.h to override default
** implementation.
**
** Octet alignment support (the octet offset is odd) need to be considered
** when porting this macro.
**------------------------------------------------------------------------------
** Arguments:
**    pxDest            - Base pointer to the destination.
**    iDestOctetOffset  - Octet offset to the destination where the copy will
**                        begin.
**    pxSrc             - Base pointer to source data.
**    iSrcOctetOffset   - Octet offset to the source where the copy will begin.
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED || ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED )
#ifndef ABCC_PORT_Copy64
#define ABCC_PORT_Copy64( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset )   \
        ABCC_PORT_CopyOctets( pxDest, iDestOctetOffset, pxSrc, iSrcOctetOffset, 8 )
#endif
#endif

/*------------------------------------------------------------------------------
** Functions for copying native UINT8 arrays to and from packed octet strings.
** There should be no need to override these.
**------------------------------------------------------------------------------
*/
#define ABCC_PORT_Uint8CpyToNative( pxDest, pxSrc, iSrcOctetOffset, iNbrOfOctets )\
        ABCC_PORT_StrCpyToNative( pxDest, pxSrc, iSrcOctetOffset, iNbrOfOctets )

#define ABCC_PORT_Uint8CpyToPacked( pxDest, iDestOctetOffset, pxSrc, iNbrOfOctets ) \
        ABCC_PORT_StrCpyToPacked( pxDest, iDestOctetOffset, pxSrc, iNbrOfOctets )

#endif  /* inclusion lock */
