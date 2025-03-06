/*******************************************************************************
** Copyright 2024-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** This file provides the logging functionality for the ABCC driver.
********************************************************************************
*/

#ifndef ABCC_DEBUG_ERR_H_
#define ABCC_DEBUG_ERR_H_

#include <inttypes.h>
#include <stdarg.h>
#include "abcc_config.h"
#include "abcc_types.h"
#include "abp.h"
#include "abcc_hardware_abstraction.h"
#include "abcc_error_codes.h"
#include "abcc_port.h"

/*------------------------------------------------------------------------------
** Defines to set the desired level of logging functionality to be enabled.
**------------------------------------------------------------------------------
*/
#define ABCC_LOG_SEVERITY_DISABLED 0
#define ABCC_LOG_SEVERITY_FATAL_ENABLED 1
#define ABCC_LOG_SEVERITY_ERROR_ENABLED 2
#define ABCC_LOG_SEVERITY_WARNING_ENABLED 3
#define ABCC_LOG_SEVERITY_INFO_ENABLED 4
#define ABCC_LOG_SEVERITY_DEBUG_ENABLED 5

/*------------------------------------------------------------------------------
** The different severity levels that can be logged.
**------------------------------------------------------------------------------
*/
typedef enum
{
   ABCC_LOG_SEVERITY_FATAL = 0,
   ABCC_LOG_SEVERITY_ERROR,
   ABCC_LOG_SEVERITY_WARNING,
   ABCC_LOG_SEVERITY_INFO,
   ABCC_LOG_SEVERITY_DEBUG,
   ABCC_LOG_SEVERITY_UNKNOWN // Do not use, only for sanity check.
}
ABCC_LogSeverityType;

/*------------------------------------------------------------------------------
** There are a set of macros available to log different severity levels. One
** macro for each severity level is available. Depending on the severity the
** macros takes a different set of arguments. All macros accepts a format string
** followed by a variadic number of arguments as the last arguments.
** The macros are:
** - ABCC_LOG_FATAL( eErrorCode, lAdditionalInfo, ... )
** - ABCC_LOG_ERROR( eErrorCode, lAdditionalInfo, ... )
** - ABCC_LOG_WARNING( eErrorCode, lAdditionalInfo, ... )
** - ABCC_LOG_INFO( ... )
** - ABCC_LOG_DEBUG( ... )
**
**   Example usage:
**   ABCC_LOG_FATAL( ABCC_EC_INTERNAL_ERROR, 1, "Fatal event %d", 1 );
**   ABCC_LOG_ERROR( ABCC_EC_INTERNAL_ERROR, 1, "Error event %d", 2 );
**   ABCC_LOG_WARNING( ABCC_EC_INTERNAL_ERROR, 1, "Warning event %d", 3 );
**   ABCC_LOG_INFO( "Info event %d", 4 );
**   ABCC_LOG_DEBUG( "Debug event %d", 5 );
**
** See ABCC_CFG_LOG_* in abcc_config.h for details on how to enable logging
** for different severities and how to customize log prints.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_FATAL_ENABLED
#define ABCC_LOG_FATAL( eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandlerWrapper( ABCC_LOG_SEVERITY_FATAL, eErrorCode, lAdditionalInfo, __VA_ARGS__ )
#else
#define ABCC_LOG_FATAL( eErrorCode, lAdditionalInfo, ... ) ABCC_LogError( ABCC_LOG_SEVERITY_FATAL, eErrorCode, lAdditionalInfo )
#endif

#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_ERROR_ENABLED
#define ABCC_LOG_ERROR( eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandlerWrapper( ABCC_LOG_SEVERITY_ERROR, eErrorCode, lAdditionalInfo, __VA_ARGS__ )
#else
#define ABCC_LOG_ERROR( eErrorCode, lAdditionalInfo, ... ) ABCC_LogError( ABCC_LOG_SEVERITY_ERROR, eErrorCode, lAdditionalInfo )
#endif

#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_WARNING_ENABLED
#define ABCC_LOG_WARNING( eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandlerWrapper( ABCC_LOG_SEVERITY_WARNING, eErrorCode, lAdditionalInfo, __VA_ARGS__ )
#else
#define ABCC_LOG_WARNING( ... )
#endif

#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_INFO_ENABLED
#define ABCC_LOG_INFO( ... ) \
   ABCC_LogHandlerWrapper( ABCC_LOG_SEVERITY_INFO, ABCC_EC_NO_ERROR, 0, __VA_ARGS__ )
#else
#define ABCC_LOG_INFO( ... )
#endif

#if ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_DEBUG_ENABLED
#define ABCC_LOG_DEBUG( ... ) \
   ABCC_LogHandlerWrapper( ABCC_LOG_SEVERITY_DEBUG, ABCC_EC_NO_ERROR, 0, __VA_ARGS__ )
#else
#define ABCC_LOG_DEBUG( ... )
#endif

#if ABCC_CFG_DEBUG_CMD_SEQ_ENABLED
#define ABCC_LOG_DEBUG_CMD_SEQ( ... ) ABCC_LOG_DEBUG( __VA_ARGS__ )
#else
#define ABCC_LOG_DEBUG_CMD_SEQ( ... )
#endif

#if ABCC_CFG_DEBUG_MESSAGING_ENABLED && ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_DEBUG_ENABLED
#define ABCC_LOG_DEBUG_MSG_CONTENT( psMsg, ... )  \
do                                                \
{                                                 \
   ABCC_LOG_DEBUG( __VA_ARGS__ );                 \
   ABCC_LogMsg( psMsg );                          \
} while( 0 )
#define ABCC_LOG_DEBUG_MSG_EVENT( psMsg, ... ) \
do                                             \
{                                              \
   ABCC_LOG_DEBUG( __VA_ARGS__ );              \
   ABCC_LogMsgEvent( psMsg );                  \
} while( 0 )
#define ABCC_LOG_DEBUG_MSG_GENERAL( ... ) ABCC_LOG_DEBUG( __VA_ARGS__ )
#else
#define ABCC_LOG_DEBUG_MSG_CONTENT( psMsg, ... )
#define ABCC_LOG_DEBUG_MSG_EVENT( psMsg, ... )
#define ABCC_LOG_DEBUG_MSG_GENERAL( ... )
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_MSG_ENABLED && ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_DEBUG_ENABLED
#define ABCC_LOG_DEBUG_HEXDUMP_MSG_TX( psMsg )                 \
do                                                             \
{                                                              \
   ABCC_LOG_DEBUG( "" );                                       \
   ABCC_LogHexdumpMsg( psMsg, TRUE );                          \
} while( 0 )
#define ABCC_LOG_DEBUG_HEXDUMP_MSG_RX( psMsg )                 \
do                                                             \
{                                                              \
   ABCC_LOG_DEBUG( "" );                                       \
   ABCC_LogHexdumpMsg( psMsg, FALSE );                         \
} while( 0 )
#else
#define ABCC_LOG_DEBUG_HEXDUMP_MSG_TX( psMsg )
#define ABCC_LOG_DEBUG_HEXDUMP_MSG_RX( psMsg )
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_UART_ENABLED && ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_DEBUG_ENABLED
#define ABCC_LOG_DEBUG_UART_HEXDUMP_TX( pbData, iSizeInBytes ) \
do                                                             \
{                                                              \
   ABCC_LOG_DEBUG( "" );                                       \
   ABCC_LogHexdumpUart( pbData, iSizeInBytes, TRUE );          \
} while( 0 )
#define ABCC_LOG_DEBUG_UART_HEXDUMP_RX( pbData, iSizeInBytes ) \
do                                                             \
{                                                              \
   ABCC_LOG_DEBUG( "" );                                       \
   ABCC_LogHexdumpUart( pbData, iSizeInBytes, FALSE );         \
} while( 0 )
#else
#define ABCC_LOG_DEBUG_UART_HEXDUMP_TX( pbData, iSizeInBytes )
#define ABCC_LOG_DEBUG_UART_HEXDUMP_RX( pbData, iSizeInBytes )
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_SPI_ENABLED && ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_DEBUG_ENABLED
#define ABCC_LOG_DEBUG_SPI_HEXDUMP_MOSI( pbData, iSizeInWords ) \
do                                                              \
{                                                               \
   ABCC_LOG_DEBUG( "" );                                        \
   ABCC_LogHexdumpSpi( pbData, iSizeInWords, TRUE );            \
} while( 0 )
#define ABCC_LOG_DEBUG_SPI_HEXDUMP_MISO( pbData, iSizeInWords ) \
do                                                              \
{                                                               \
   ABCC_LOG_DEBUG( "" );                                        \
   ABCC_LogHexdumpSpi( pbData, iSizeInWords, FALSE );           \
} while( 0 )
#else
#define ABCC_LOG_DEBUG_SPI_HEXDUMP_MOSI( pbData, iSizeInBytes )
#define ABCC_LOG_DEBUG_SPI_HEXDUMP_MISO( pbData, iSizeInBytes )
#endif

#if ABCC_LOG_DEBUG_MEMORY_ENABLED && ABCC_CFG_LOG_SEVERITY >= ABCC_LOG_SEVERITY_DEBUG_ENABLED
#define ABCC_LOG_DEBUG_MEM( ... ) ABCC_LOG_DEBUG( __VA_ARGS__ )
#else
#define ABCC_LOG_DEBUG_MEM( ... )
#endif

/*------------------------------------------------------------------------------
** The functions/macros below are intended for internal use only. They are used
** by the public macros above.
**------------------------------------------------------------------------------
*/
#if ( defined(__GNUC__) && __GNUC__ >= 3 ) || defined(__clang__)
#define ABCC_PRINTF_ATTRIBUTES( x, y ) __attribute__ ((format ( printf, x, y )))
#else
#define ABCC_PRINTF_ATTRIBUTES( x, y )
#endif

#if ( defined(__GNUC__) && __GNUC__ >= 12 ) || ( defined(__clang__) && __clang_major__ >= 9 )
// __FILE_NAME__ returns just the basename of the file and not the complete path
// which saves memory.
#define ABCC_FILE_IDENTIFIER __FILE_NAME__
#else
#define ABCC_FILE_IDENTIFIER __FILE__
#endif

#if ABCC_CFG_LOG_FILE_LINE_ENABLED
EXTFUNC void ABCC_LogHandler( ABCC_LogSeverityType bSeverity,
   ABCC_ErrorCodeType eErrorCode,
   UINT32 lAdditionalInfo,
   const char *acFile,
   int xLine
#if ABCC_CFG_LOG_STRINGS_ENABLED
   , const char* pcFormat, ... ) ABCC_PRINTF_ATTRIBUTES( 6, 7 );
#else
);
#endif
#else
EXTFUNC void ABCC_LogHandler( ABCC_LogSeverityType bSeverity,
   ABCC_ErrorCodeType eErrorCode,
   UINT32 lAdditionalInfo
#if ABCC_CFG_LOG_STRINGS_ENABLED
   , const char* pcFormat, ... ) ABCC_PRINTF_ATTRIBUTES( 4, 5 );
#else
);
#endif
#endif

void ABCC_LogError( ABCC_LogSeverityType eSeverity,
   ABCC_ErrorCodeType eErrorCode,
   UINT32 lAdditionalInfo );

#if ABCC_CFG_LOG_FILE_LINE_ENABLED && ABCC_CFG_LOG_STRINGS_ENABLED
#define ABCC_LogHandlerWrapper( bSeverity, eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandler( bSeverity, eErrorCode, lAdditionalInfo, ABCC_FILE_IDENTIFIER, __LINE__, __VA_ARGS__ )
#elif ABCC_CFG_LOG_FILE_LINE_ENABLED
#define ABCC_LogHandlerWrapper( bSeverity, eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandler( bSeverity, eErrorCode, lAdditionalInfo, ABCC_FILE_IDENTIFIER, __LINE__ )
#elif ABCC_CFG_LOG_STRINGS_ENABLED
#define ABCC_LogHandlerWrapper( bSeverity, eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandler( bSeverity, eErrorCode, lAdditionalInfo, __VA_ARGS__ )
#else
#define ABCC_LogHandlerWrapper( bSeverity, eErrorCode, lAdditionalInfo, ... ) \
   ABCC_LogHandler( bSeverity, eErrorCode, lAdditionalInfo )
#endif

#if ABCC_CFG_DEBUG_MESSAGING_ENABLED
EXTFUNC void ABCC_LogMsg( ABP_MsgType* psMsg );
EXTFUNC void ABCC_LogMsgEvent( ABP_MsgType* psMsg );
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_SPI_ENABLED
EXTFUNC void ABCC_LogHexdumpSpi( UINT16* piData, UINT16 iSizeInWords, BOOL fMosi );
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_UART_ENABLED
EXTFUNC void ABCC_LogHexdumpUart( UINT8* pbData, UINT16 iSizeInBytes, BOOL fTx );
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_MSG_ENABLED
EXTFUNC void ABCC_LogHexdumpMsg( ABP_MsgType* psMsg, BOOL fTx );
#endif

#endif
