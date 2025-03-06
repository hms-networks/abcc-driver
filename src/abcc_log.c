/*******************************************************************************
** Copyright 2024-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** ABCC driver error and debug functions.
********************************************************************************
*/

#if ABCC_CFG_LOG_STRINGS_ENABLED
#include <stdarg.h>
#endif

#include "abcc_config.h"
#include "abcc.h"
#include "abcc_port.h"
#include "abcc_software_port.h"
#include "abcc_types.h"
#include "abp.h"
#include "abcc_log.h"
#include "abcc_handler.h"

/*------------------------------------------------------------------------------
** ANSI color codes for prettier prints
**------------------------------------------------------------------------------
*/
#ifdef ABCC_CFG_LOG_COLORS_ENABLED
   #define ABCC_LOG_ANSI_COLOR_BLACK   "\x1b[30m"
   #define ABCC_LOG_ANSI_COLOR_RED     "\x1b[31m"
   #define ABCC_LOG_ANSI_COLOR_GREEN   "\x1b[32m"
   #define ABCC_LOG_ANSI_COLOR_YELLOW  "\x1b[33m"
   #define ABCC_LOG_ANSI_COLOR_BLUE    "\x1b[34m"
   #define ABCC_LOG_ANSI_COLOR_MAGENTA "\x1b[35m"
   #define ABCC_LOG_ANSI_COLOR_CYAN    "\x1b[36m"
   #define ABCC_LOG_ANSI_COLOR_WHITE   "\x1b[37m"
   #define ABCC_LOG_ANSI_COLOR_RESET   "\x1b[0m"
#else
   #define ABCC_LOG_ANSI_COLOR_BLACK   ""
   #define ABCC_LOG_ANSI_COLOR_RED     ""
   #define ABCC_LOG_ANSI_COLOR_GREEN   ""
   #define ABCC_LOG_ANSI_COLOR_YELLOW  ""
   #define ABCC_LOG_ANSI_COLOR_BLUE    ""
   #define ABCC_LOG_ANSI_COLOR_MAGENTA ""
   #define ABCC_LOG_ANSI_COLOR_CYAN    ""
   #define ABCC_LOG_ANSI_COLOR_WHITE   ""
   #define ABCC_LOG_ANSI_COLOR_RESET   ""
#endif

/*------------------------------------------------------------------------------
** The indentation for new lines differs depending on the configuration.
**------------------------------------------------------------------------------
*/
#define ABCC_LOG_SEVERITY_LENGTH 10
#define ABCC_LOG_MAX_FILE_LINE_LENGTH 37
#define ABCC_LOG_TIMESTAMP_LENGTH 15

#if ABCC_CFG_LOG_TIMESTAMPS_ENABLED && ABCC_CFG_LOG_FILE_LINE_ENABLED
#define ABCC_LOG_INDENTATION ( ABCC_LOG_SEVERITY_LENGTH + ABCC_LOG_MAX_FILE_LINE_LENGTH + ABCC_LOG_TIMESTAMP_LENGTH )
#elif ABCC_CFG_LOG_TIMESTAMPS_ENABLED
#define ABCC_LOG_INDENTATION ( ABCC_LOG_SEVERITY_LENGTH + ABCC_LOG_TIMESTAMP_LENGTH )
#elif ABCC_CFG_LOG_FILE_LINE_ENABLED
#define ABCC_LOG_INDENTATION ( ABCC_LOG_SEVERITY_LENGTH + ABCC_LOG_MAX_FILE_LINE_LENGTH )
#else
#define ABCC_LOG_INDENTATION ( ABCC_LOG_SEVERITY_LENGTH )
#endif

#ifdef ABCC_SYS_16_BIT_CHAR
#define ABCC_GetMsgCmdField( psMsg )   ( ABCC_GetLowAddrOct( (psMsg)->sHeader.iCmdReserved ) )
#else
#define ABCC_GetMsgCmdField( psMsg )   ( (psMsg)->sHeader.bCmd )
#endif

void ABCC_LogHandler(
   ABCC_LogSeverityType eSeverity,
   ABCC_ErrorCodeType eErrorCode,
   UINT32 lAdditionalInfo
#if ABCC_CFG_LOG_FILE_LINE_ENABLED
   , const char *pcFilename,
   int xLine
#endif
#if ABCC_CFG_LOG_STRINGS_ENABLED
   , const char* pcFormat,
   ...
#endif
)
{
   const char* apcSeverityToString[] =
   {
      ".."ABCC_LOG_ANSI_COLOR_RED    "|FATAL| "ABCC_LOG_ANSI_COLOR_RESET,
      ".."ABCC_LOG_ANSI_COLOR_RED    "|ERROR| "ABCC_LOG_ANSI_COLOR_RESET,
      ""ABCC_LOG_ANSI_COLOR_YELLOW "|WARNING| "ABCC_LOG_ANSI_COLOR_RESET,
      "..."ABCC_LOG_ANSI_COLOR_CYAN   "|INFO| "ABCC_LOG_ANSI_COLOR_RESET,
      ".."ABCC_LOG_ANSI_COLOR_MAGENTA"|DEBUG| "ABCC_LOG_ANSI_COLOR_RESET,
      ""ABCC_LOG_ANSI_COLOR_RED    "|UNKNOWN| "ABCC_LOG_ANSI_COLOR_RESET
   };

   if( ( eSeverity < 0 ) || ( eSeverity >= ABCC_LOG_SEVERITY_UNKNOWN ) )
   {
      eSeverity = ABCC_LOG_SEVERITY_UNKNOWN;
   }

#if ABCC_CFG_LOG_TIMESTAMPS_ENABLED
   UINT64 llUptime = ABCC_GetUptimeMs();
   ABCC_PORT_printf( ABCC_LOG_ANSI_COLOR_GREEN"%02u:%02u:%02u.%03u - "ABCC_LOG_ANSI_COLOR_RESET,
      (UINT16)( llUptime / 1000 / 60 / 60 % 24 ),
      (UINT8)( llUptime / 1000 / 60 % 60 ),
      (UINT8)( llUptime / 1000 % 60 ),
      (UINT16)( llUptime % 1000 ) );
#endif

#if ABCC_CFG_LOG_FILE_LINE_ENABLED
   const char* pcPos;
   const char* pcBasename;
   int xLen;
   int xPad;

   /*
   ** Extract base name from file
   */
   for( pcPos = pcBasename = pcFilename; *pcPos != '\0'; pcPos++ )
   {
      if( *pcPos == '/' || *pcPos == '\\' )
      {
         pcBasename = pcPos + 1;
      }
   }

   xLen = ABCC_PORT_printf( "%s:%d", pcBasename, xLine );
   xPad = ABCC_LOG_MAX_FILE_LINE_LENGTH - xLen;
   xPad = xPad < 0 ? 0 : xPad;

   while( xPad-- )
   {
      ABCC_PORT_printf( "." );
   }
#endif

   ABCC_PORT_printf( "%s", apcSeverityToString[ eSeverity ] );

#if ABCC_CFG_LOG_STRINGS_ENABLED
   va_list argp;
   va_start( argp, pcFormat );
   ABCC_PORT_vprintf( pcFormat, argp );
   va_end( argp );
#else
   if( eSeverity <= ABCC_LOG_SEVERITY_WARNING )
   {
      ABCC_PORT_printf( "Error code: %d, Additional info: %" PRIu32 "\n", eErrorCode, lAdditionalInfo );
   }
   else
   {
      ABCC_PORT_printf( "\n" );
   }
#endif

   if( eSeverity <= ABCC_LOG_SEVERITY_ERROR )
   {
      ABCC_LogError( eSeverity, eErrorCode, lAdditionalInfo );
   }
}

void ABCC_LogError( ABCC_LogSeverityType eSeverity, ABCC_ErrorCodeType eErrorCode, UINT32 lAdditionalInfo )
{
   ABCC_SetError( eSeverity, eErrorCode, lAdditionalInfo );
}

#if ABCC_CFG_DEBUG_MESSAGING_ENABLED
void ABCC_LogMsg( ABP_MsgType* psMsg )
{
   UINT16 i;
   UINT16 iDataSize;
   UINT8 bData;

   iDataSize = ABCC_GetMsgDataSize( psMsg );

   ABCC_PORT_printf( "%*sMsgBuf:0x%p Size:0x%04x SrcId:  0x%02x DestObj:0x%02x\n%*sInst:  0x%04x     Cmd: 0x%02x   CmdExt0:0x%02x CmdExt1:0x%02x\n",
                     ABCC_LOG_INDENTATION, "",
                     (void*)psMsg,
                     ABCC_GetMsgDataSize( psMsg ),
                     ABCC_GetMsgSourceId( psMsg ),
                     ABCC_GetMsgDestObj( psMsg ),
                     ABCC_LOG_INDENTATION, "",
                     ABCC_GetMsgInstance( psMsg ),
                     ABCC_GetMsgCmdField( psMsg ),
                     ABCC_GetMsgCmdExt0( psMsg ),
                     ABCC_GetMsgCmdExt1( psMsg ) );

   ABCC_PORT_printf( "%*sData:  [ ", ABCC_LOG_INDENTATION, "" );
   for( i = 0; i < iDataSize; i++ )
   {
      if( ( i % 16 ) == 15 )
      {
         ABCC_PORT_printf( ("\n  ") );
      }

      ABCC_GetMsgData8( psMsg, &bData, i );
      ABCC_PORT_printf( "0x%02x ", bData );
   }

   ABCC_PORT_printf( ( "]\n" ) );
}

void ABCC_LogMsgEvent( ABP_MsgType* psMsg )
{
#if !ABCC_CFG_LOG_STRINGS_ENABLED
   ABCC_PORT_printf( "%*s", ABCC_LOG_INDENTATION, "" );
#endif

   ABCC_PORT_printf( "MsgBuf:0x%p SrcId:0x%02x\n",
                     (void*)psMsg,
                     ABCC_GetMsgSourceId( psMsg ) );
}
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_MSG_ENABLED
void ABCC_LogHexdumpMsg( ABP_MsgType* psMsg, BOOL fTx )
{
   UINT16   iIndex;
   UINT16   iSizeInBytes;

#if !ABCC_CFG_LOG_STRINGS_ENABLED
   ABCC_PORT_printf( "%*s", ABCC_LOG_INDENTATION, "" );
#endif

   if( fTx )
   {
      ABCC_PORT_printf( "MSG TX: " );
   }
   else
   {
      ABCC_PORT_printf( "MSG RX: " );
   }

   iSizeInBytes = ABCC_GetMsgDataSize( psMsg );

#ifdef ABCC_SYS_16_BIT_CHAR
   iSizeInBytes += sizeof( ABP_MsgHeaderType16 );

   iIndex = 0;
   while( iSizeInBytes > 0 )
   {
      UINT16 iData;

      iData = iLeTOi( ((UINT16*)psMsg)[ iIndex ] );

      ABCC_PORT_printf( "%02x", (UINT8)(iData & 0xFF) );
      iSizeInBytes--;
      if( iSizeInBytes > 0 )
      {
         ABCC_PORT_printf( "%02x", (UINT8)(( iData >> 8 ) & 0xFF) );
         iSizeInBytes--;
      }

      iIndex++;
   }
#else
   iSizeInBytes += sizeof( ABP_MsgHeaderType );

   for( iIndex = 0; iIndex < iSizeInBytes; iIndex++ )
   {
      ABCC_PORT_printf( "%02x", ((UINT8*)psMsg)[ iIndex ] );
   }
#endif

   ABCC_PORT_printf( "\n" );
}
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_SPI_ENABLED
void ABCC_LogHexdumpSpi( UINT16* piData, UINT16 iSizeInWords, BOOL fMosi )
{
#if !ABCC_CFG_LOG_STRINGS_ENABLED
   ABCC_PORT_printf( "%*s", ABCC_LOG_INDENTATION, "" );
#endif

   if( fMosi )
   {
      ABCC_PORT_printf( "SPI MOSI: " );
   }
   else
   {
      ABCC_PORT_printf( "SPI MISO: " );
   }

   while( iSizeInWords )
   {
      ABCC_PORT_printf( "%04x", iTOiBe( *piData ) );
      piData++;
      iSizeInWords--;
   }

   ABCC_PORT_printf( "\n" );
}
#endif

#if ABCC_CFG_DEBUG_HEXDUMP_UART_ENABLED
void ABCC_LogHexdumpUart( UINT8* pbData, UINT16 iSizeInBytes, BOOL fTx )
{
#if !ABCC_CFG_LOG_STRINGS_ENABLED
   ABCC_PORT_printf( "%*s", ABCC_LOG_INDENTATION, "" );
#endif

   if( fTx )
   {
      ABCC_PORT_printf( "UART TX: " );
   }
   else
   {
      ABCC_PORT_printf( "UART RX: " );
   }

   while( iSizeInBytes )
   {
      ABCC_PORT_printf( "%02x", *pbData );
      pbData++;
      iSizeInBytes--;
   }

   ABCC_PORT_printf( "\n" );
}
#endif
