/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** SPI driver implementation
********************************************************************************
*/

#include "abcc_config.h"
#include "abcc.h"

#if ABCC_CFG_DRV_SPI_ENABLED

#include "abcc_types.h"
#include "abcc_log.h"
#include "abcc_hardware_abstraction.h"
#include "../abcc_timer.h"
#include "../abcc_driver_interface.h"
#include "../abcc_memory.h"
#include "../abcc_handler.h"
#include "abcc_crc32.h"
#include "abcc_hardware_abstraction_spi.h"

#if ( ABCC_CFG_MAX_MSG_SIZE < 16 )
#error "ABCC_CFG_MAX_MSG_SIZE must be at least a 16 bytes"
#endif

#define ABCC_MSG_HEADER_TYPE_SIZEOF 12

/*------------------------------------------------------------------------------
** Since the masking of control registers are endian dependent since
** we operating on the mosi and misu structures that are little endian
**------------------------------------------------------------------------------
*/
#ifdef ABCC_SYS_BIG_ENDIAN
static const UINT16 iSpiCtrlWrPdWalid =   ABP_SPI_CTRL_WRPD_VALID << 8;
static const UINT16 iSpiCtrlCmdCntShift  =  9;
static const UINT16 iSpiCtrlCmdCnt  =     ABP_SPI_CTRL_CMDCNT << 8;
static const UINT16 iSpiCtrl_M =          ABP_SPI_CTRL_M << 8;
static const UINT16 iSpiCtrlLastFrag =    ABP_SPI_CTRL_LAST_FRAG << 8;
static const UINT16 iSpiCtrl_T =          ABP_SPI_CTRL_T << 8;

static const UINT16 iSpiStatusWrMsgFull = ABP_SPI_STATUS_WRMSG_FULL;
static const UINT16 iSpiStatusCmdCnt =    ABP_SPI_STATUS_CMDCNT;
static const UINT16 iSpiStatusCmdCntShift  =  1;
static const UINT16 iSpiStatus_M =        ABP_SPI_STATUS_M;
static const UINT16 iSpiStatusLastFrag =  ABP_SPI_STATUS_LAST_FRAG;
static const UINT16 iSpiStatusNewPd =     ABP_SPI_STATUS_NEW_PD;
#else
static const UINT16 iSpiCtrlWrPdWalid =   ABP_SPI_CTRL_WRPD_VALID;
static const UINT16 iSpiCtrlCmdCntShift  =  1;
static const UINT16 iSpiCtrlCmdCnt  =     ABP_SPI_CTRL_CMDCNT;
static const UINT16 iSpiCtrl_M =          ABP_SPI_CTRL_M;
static const UINT16 iSpiCtrlLastFrag =    ABP_SPI_CTRL_LAST_FRAG;
static const UINT16 iSpiCtrl_T =          ABP_SPI_CTRL_T;

static const UINT16 iSpiStatusWrMsgFull = ABP_SPI_STATUS_WRMSG_FULL << 8;
static const UINT16 iSpiStatusCmdCnt =    ABP_SPI_STATUS_CMDCNT << 8;
static const UINT16 iSpiStatusCmdCntShift  =  9;
static const UINT16 iSpiStatus_M =        ABP_SPI_STATUS_M << 8;
static const UINT16 iSpiStatusLastFrag =  ABP_SPI_STATUS_LAST_FRAG << 8;
static const UINT16 iSpiStatusNewPd =     ABP_SPI_STATUS_NEW_PD << 8;
#endif

/*------------------------------------------------------------------------------
** Help defines.
** Note: They are in words and not bytes.
**------------------------------------------------------------------------------
*/
#define NUM_BYTES_2_WORDS(x) ( ( (x) + 1 ) >> 1 )

#define SPI_DEFAULT_PD_LEN 0
#define CRC_WORD_LEN_IN_WORDS 2
#define SPI_FRAME_SIZE_EXCLUDING_DATA (7)

#if ABCC_CFG_SPI_MSG_FRAG_LEN > ABCC_CFG_MAX_MSG_SIZE
#error  "SPI fragmentation length cannot exceed max msg size"
#endif
#define MAX_PAYLOAD_WORD_LEN ( ( NUM_BYTES_2_WORDS( ABCC_CFG_SPI_MSG_FRAG_LEN ) ) + ( NUM_BYTES_2_WORDS( ABCC_CFG_MAX_PROCESS_DATA_SIZE ) ) + ( CRC_WORD_LEN_IN_WORDS ) )

#define INSERT_SPI_CTRL_CMDCNT( ctrl, cmdcnt ) ctrl = ( ( ctrl ) & ~iSpiCtrlCmdCnt ) | ( ( cmdcnt ) << iSpiCtrlCmdCntShift )
#define EXTRACT_SPI_STATUS_CMDCNT( status ) ( ( ( status ) & iSpiStatusCmdCnt ) >> iSpiStatusCmdCntShift )
#define SPI_BASE_FRAME_WORD_LEN  5 /* Frame length excluding MSG and PD data */

/*------------------------------------------------------------------------------
** SPI MOSI structure (7044 - ABCC40).
**------------------------------------------------------------------------------
*/

typedef struct drv_SpiMosiFrameType
{
   UINT16 iSpiControl;
   UINT16 iMsgLen;
   UINT16 iPdLen;
   UINT16 iIntMaskAppStatus;
   UINT16 iData[ MAX_PAYLOAD_WORD_LEN ];
   UINT16   dummy;
} drv_SpiMosiFrameType;

/*------------------------------------------------------------------------------
** SPI MISO structure
**------------------------------------------------------------------------------
*/
typedef struct
{
   UINT16 iReserved;
   UINT16 iLedStat;
   UINT16 iSpiStatusAnbStatus;
   UINT16 iNetTime[ 2 ];
   UINT16 iData[ MAX_PAYLOAD_WORD_LEN ];
} drv_SpiMisoFrameType;

/*------------------------------------------------------------------------------
** Message fragmentation info types.
**------------------------------------------------------------------------------
*/
typedef struct
{
  ABP_MsgType*       psWriteMsg;          /* Pointer to the write message. */
  UINT16*            puCurrPtr;           /* Pointer to the current fragment. */
  INT16              iNumWordsLeft;       /* Number of words left to send. */
  UINT16             iCurrFragLength;     /* Current fragmention block length. */
} drv_SpiWriteMsgFragInfoType;

typedef struct
{
  ABP_MsgType* psReadMsg;                 /* Pointer to the receive message buffer. */
  UINT16*            puCurrPtr;           /* Pointer to the current position in receive buffer. */
  UINT16             iNumWordsReceived;   /* Number of words received. */
} drv_SpiReadMsgFragInfoType;

/*------------------------------------------------------------------------------
** Internal SPI states.
**------------------------------------------------------------------------------
*/
typedef enum
{
   SM_SPI_INIT = 0,
   SM_SPI_RDY_TO_SEND_MOSI,
   SM_SPI_WAITING_FOR_MISO
} drv_SpiStateType;

/*------------------------------------------------------------------------------
** MISO privates.
**------------------------------------------------------------------------------
*/
static drv_SpiMisoFrameType         spi_drv_sMisoFrame;           /* Place holder for the MISO frame. */
static drv_SpiReadMsgFragInfoType   spi_drv_sReadFragInfo;        /* Read message info. */
static BOOL                         spi_drv_fNewMisoReceived;     /* MISO received flag. */
static UINT8                        spi_drv_bAnbStatus;           /* Latest received anb status. */
static UINT16                       spi_drv_iLedStatus;            /* Latest received anb status. */
static UINT8                        spi_drv_bAnbCmdCnt;            /* Latest correct received cmd count */
static UINT8*                       spi_drv_bpRdPd;                /* Pointer to latest correctly received RdPd */

static ABP_MsgType*                 spi_drv_psReadMessage;        /* Pointer to the read message. */

/*------------------------------------------------------------------------------
** MOSI privates.
**------------------------------------------------------------------------------
*/
static drv_SpiMosiFrameType         spi_drv_sMosiFrame;           /* Place holder for the MISO frame. */
static drv_SpiWriteMsgFragInfoType  spi_drv_sWriteFragInfo;       /* Write message info. */
static UINT8                        spi_drv_bNbrOfCmds;           /* Number of commands support by the application. */
static UINT8                        spi_drv_bNextAppStatus;       /* Appstatus to be sent in next MOSI frame */
static UINT8                        spi_drv_bNextIntMask;         /* Intmask to be sent in next MOSI frame */

/*------------------------------------------------------------------------------
** General privates.
**------------------------------------------------------------------------------
*/
static UINT16                       spi_drv_iPdOffset;            /* Offset in payload where the PD is located. */
static UINT16                       spi_drv_iCrcOffset;           /* Offset in payload where the CRC is located. */

static UINT16                       spi_drv_iPdSize;              /* PD size in spiFrame. */
static UINT16                       spi_drv_iWritePdSize;         /* Current write PD size. */
static UINT16                       spi_drv_iReadPdSize;          /* Current read PD size. */

static UINT16                       spi_drv_iSpiFrameSize;        /* Current Spi frame size. */
static BOOL                         spi_drv_fRetransmit;          /* Indicate retransmission. */

static drv_SpiStateType             spi_drv_eState;               /* SPI driver state. */
static ABCC_TimerHandle             xWdTmoHandle;
static BOOL                         fWdTmo;                       /* Current wd timeout status */

static UINT16                       spi_drv_iMsgLen;              /* Message length ( in words ) */

static UINT16                       drv_iCrcErrorCount;           /* CRC error counter */

static void spi_drv_DataReceived( void );
static void spi_drv_ResetReadFragInfo( void );
static void spi_drv_ResetWriteFragInfo( void );

static void DrvSpiSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg );

/*------------------------------------------------------------------------------
**  Handles preparation and transmission of the MOSI frame.
**  Depending on the physical implementation of the SPI transaction this method
**  could be blocking until the MISO is received.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Driver status.
**------------------------------------------------------------------------------
*/
void ABCC_DrvSpiRunDriverTx( void )
{
   UINT32 lCrc;
   BOOL   fHandleWriteMsg = FALSE;
   UINT16 iRdyForCmd;

   ABCC_PORT_UseCritical();

   if( spi_drv_eState ==  SM_SPI_RDY_TO_SEND_MOSI )
   {
      spi_drv_eState = SM_SPI_WAITING_FOR_MISO;

      if( !spi_drv_fRetransmit )
      {
         /*
         ** Everything is OK. Reset retransmission and toggle the T bit.
         */
         spi_drv_sMosiFrame.iSpiControl ^= iSpiCtrl_T;
      }

      spi_drv_fRetransmit = FALSE;

      /*---------------------------------------------------------------------------
      ** Write message handling.
      **---------------------------------------------------------------------------
      */
      ABCC_PORT_EnterCritical();
      if( spi_drv_sWriteFragInfo.psWriteMsg != NULL )
      {
         fHandleWriteMsg = TRUE;
      }
      ABCC_PORT_ExitCritical();

      if( fHandleWriteMsg )
      {
         if( !spi_drv_sWriteFragInfo.puCurrPtr )
         {
            ABCC_LOG_FATAL( ABCC_EC_UNEXPECTED_NULL_PTR,
               (UINT32)spi_drv_sWriteFragInfo.puCurrPtr,
               "Unexpected NULL pointer\n" );
         }

         /*
         ** Write the message to be sent.
         */
         spi_drv_sMosiFrame.iSpiControl |= iSpiCtrl_M;

         if( spi_drv_sWriteFragInfo.iNumWordsLeft <= spi_drv_iMsgLen )
         {
            spi_drv_sMosiFrame.iSpiControl |= iSpiCtrlLastFrag;
            spi_drv_sWriteFragInfo.iCurrFragLength = spi_drv_sWriteFragInfo.iNumWordsLeft;
         }
         else
         {
            /*
            ** This is not the last fragment.
            */
            spi_drv_sMosiFrame.iSpiControl &= ~iSpiCtrlLastFrag;
            spi_drv_sWriteFragInfo.iCurrFragLength = spi_drv_iMsgLen;
         }

         /*
         ** Copy the message into the MOSI frame buffer.
         */
         ABCC_PORT_MemCpy( (void*)spi_drv_sMosiFrame.iData,
                           (void*)spi_drv_sWriteFragInfo.puCurrPtr,
                           spi_drv_sWriteFragInfo.iCurrFragLength << 1 );
      }
      else
      {
         /*
         ** There is no message fragment to be sent.
         */
         spi_drv_sMosiFrame.iSpiControl &= ~iSpiCtrl_M;
         spi_drv_sMosiFrame.iSpiControl &= ~iSpiCtrlLastFrag;
      }

      iRdyForCmd = 0;
      if( spi_drv_bNbrOfCmds > 3 )
      {
         iRdyForCmd = 3;
      }
      else
      {
         iRdyForCmd =  spi_drv_bNbrOfCmds & 0x3;
      }
      INSERT_SPI_CTRL_CMDCNT( spi_drv_sMosiFrame.iSpiControl, iRdyForCmd );

      ABCC_SetLowAddrOct( spi_drv_sMosiFrame.iIntMaskAppStatus, spi_drv_bNextAppStatus );
      ABCC_SetHighAddrOct( spi_drv_sMosiFrame.iIntMaskAppStatus, spi_drv_bNextIntMask );
      spi_drv_bpRdPd = NULL;

      /*
      ** Apply the CRC checksum.
      */
      lCrc = CRC_Crc32( (UINT8*)&spi_drv_sMosiFrame, spi_drv_iSpiFrameSize*2 - 6 );
      lCrc = lTOlBe( lCrc );

      ABCC_PORT_MemCpy( &spi_drv_sMosiFrame.iData[ spi_drv_iCrcOffset ],
                        &lCrc,
                        ABP_UINT32_SIZEOF );

      /*
      ** Send the MOSI frame.
      */
      ABCC_LOG_DEBUG_SPI_HEXDUMP_MOSI( (UINT16*)&spi_drv_sMosiFrame, spi_drv_iSpiFrameSize );
      ABCC_HAL_SpiSendReceive( &spi_drv_sMosiFrame, &spi_drv_sMisoFrame, spi_drv_iSpiFrameSize << 1 );
   }
   else if( spi_drv_eState == SM_SPI_INIT )
   {
      ABCC_TimerStart( xWdTmoHandle, ABCC_CFG_WD_TIMEOUT_MS );
      spi_drv_eState = SM_SPI_RDY_TO_SEND_MOSI;
   }
}

/*------------------------------------------------------------------------------
**  Handle the reception of the MISO frame.
**------------------------------------------------------------------------------
** Arguments:
**       psResp:  Pointer to the response message.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
ABP_MsgType* ABCC_DrvSpiRunDriverRx( void )
{
   UINT32 lRecievedCrc;
   UINT32 lCalculatedCrc;
   ABP_MsgType* psWriteMsg = NULL;

   if( spi_drv_eState == SM_SPI_WAITING_FOR_MISO )
   {
      if( !spi_drv_fNewMisoReceived )
      {
         /*
         ** Nothing has happened. No MISO was received.
         */
         return( psWriteMsg );
      }
      else
      {
         spi_drv_fNewMisoReceived = FALSE;
      }

      ABCC_LOG_DEBUG_SPI_HEXDUMP_MISO( (UINT16*)&spi_drv_sMisoFrame, spi_drv_iSpiFrameSize );

      lCalculatedCrc = CRC_Crc32( (UINT8*)&spi_drv_sMisoFrame, spi_drv_iSpiFrameSize*2 - 4 );
      lCalculatedCrc = lTOlBe( lCalculatedCrc );

      ABCC_PORT_MemCpy( &lRecievedCrc,
                        &spi_drv_sMisoFrame.iData[ spi_drv_iCrcOffset ],
                        ABP_UINT32_SIZEOF );

      if( lCalculatedCrc != lRecievedCrc )
      {
         /*
         ** We will request a retransmit if the data is corrupt.
         */
         drv_iCrcErrorCount++;
         ABCC_LOG_WARNING( ABCC_EC_CHECKSUM_MISMATCH,
            drv_iCrcErrorCount,
            "CRC check failed for received message (error count: %" PRIu16 ")\n",
            drv_iCrcErrorCount );
         spi_drv_fRetransmit = TRUE;
         spi_drv_eState = SM_SPI_RDY_TO_SEND_MOSI;
         return( NULL );
      }

      /*
      ** Restart watchdog
      */
      if( fWdTmo )
      {
        ABCC_CbfWdTimeoutRecovered();
      }

      ABCC_TimerStop( xWdTmoHandle );
      fWdTmo = FALSE;
      ABCC_TimerStart( xWdTmoHandle, ABCC_CFG_WD_TIMEOUT_MS );

      /*
      ** Save the current anybus status.
      */
      spi_drv_bAnbStatus =  ABCC_GetLowAddrOct( spi_drv_sMisoFrame.iSpiStatusAnbStatus );
      spi_drv_iLedStatus  = iLeTOi( spi_drv_sMisoFrame.iLedStat );

      spi_drv_bAnbCmdCnt = (UINT8)EXTRACT_SPI_STATUS_CMDCNT( spi_drv_sMisoFrame.iSpiStatusAnbStatus  );

      if( spi_drv_sMisoFrame.iSpiStatusAnbStatus & iSpiStatusNewPd )
      {
         /*
         ** Report the new process data.
         */
         spi_drv_bpRdPd = (UINT8*)&spi_drv_sMisoFrame.iData[ spi_drv_iPdOffset ];
      }

      /*---------------------------------------------------------------------------
      ** Write message handling.
      **---------------------------------------------------------------------------
      */
      if( spi_drv_sWriteFragInfo.iCurrFragLength != 0 )
      {
         /*
         ** Write the message to be sent.
         */
         if( !( spi_drv_sMisoFrame.iSpiStatusAnbStatus & iSpiStatusWrMsgFull ) )
         {
            /*
            ** Write message was received.
            ** Update the write fragmentation information.
            */
            spi_drv_sWriteFragInfo.puCurrPtr         += spi_drv_sWriteFragInfo.iCurrFragLength;
            spi_drv_sWriteFragInfo.iNumWordsLeft     -= spi_drv_sWriteFragInfo.iCurrFragLength;
            spi_drv_sWriteFragInfo.iCurrFragLength    = 0;

            if( spi_drv_sWriteFragInfo.iNumWordsLeft <= 0 )
            {
               psWriteMsg = (ABP_MsgType*)spi_drv_sWriteFragInfo.psWriteMsg;

               spi_drv_ResetWriteFragInfo();

               if( ( ABCC_GetLowAddrOct( ((ABP_MsgType16*)psWriteMsg)->sHeader.iCmdReserved ) & ABP_MSG_HEADER_C_BIT ) == 0 )
               {
                  spi_drv_bNbrOfCmds++;
               }
            }
         }
      }

      /*---------------------------------------------------------------------------
      ** Read message handling
      ** --------------------------------------------------------------------------
      */
      if( spi_drv_sMisoFrame.iSpiStatusAnbStatus & iSpiStatus_M )
      {
         /*
         ** Read message was received.
         ** Update the read fragmentation information.
         */

         if( spi_drv_sReadFragInfo.puCurrPtr == 0 )
         {
            DrvSpiSetMsgReceiverBuffer( ABCC_MemAlloc() );

            if( spi_drv_sReadFragInfo.puCurrPtr == 0 )
            {
               ABCC_LOG_WARNING( ABCC_EC_OUT_OF_MSG_BUFFERS, 0, "Out of message buffers when attempting to read a message\n" );
               return( NULL );
            }
         }

         if( ( ( spi_drv_sReadFragInfo.iNumWordsReceived + spi_drv_iMsgLen ) << 1 ) <=
             ( ABCC_CFG_MAX_MSG_SIZE + ABCC_MSG_HEADER_TYPE_SIZEOF ) )
         {
            /*
            ** Message fits in buffer so read it.
            */
            ABCC_PORT_MemCpy( spi_drv_sReadFragInfo.puCurrPtr,
                              spi_drv_sMisoFrame.iData,
                              spi_drv_iMsgLen << 1 );

            spi_drv_sReadFragInfo.puCurrPtr += spi_drv_iMsgLen;
            spi_drv_sReadFragInfo.iNumWordsReceived += spi_drv_iMsgLen;
         }

         if( spi_drv_sMisoFrame.iSpiStatusAnbStatus & iSpiStatusLastFrag )
         {
            /*
            ** Last fragment of the read message. Return the message.
            ** Update the application flow control.
            */
            if( ABCC_GetLowAddrOct( ((ABP_MsgType16*)spi_drv_sReadFragInfo.psReadMsg)->sHeader.iCmdReserved ) & ABP_MSG_HEADER_C_BIT )
            {
               spi_drv_bNbrOfCmds--;
            }

            spi_drv_psReadMessage = spi_drv_sReadFragInfo.psReadMsg;
            spi_drv_ResetReadFragInfo();
         }
      }

      /*
      ** Clear the valid pd for the next frame.
      */
      spi_drv_sMosiFrame.iSpiControl &= ~iSpiCtrlWrPdWalid;
      spi_drv_eState = SM_SPI_RDY_TO_SEND_MOSI;
   }
   else if( spi_drv_eState == SM_SPI_INIT )
   {
      spi_drv_eState = SM_SPI_RDY_TO_SEND_MOSI;
   }

   return( psWriteMsg );
} /* end of spi_drv_HandleMiso() */

/*------------------------------------------------------------------------------
** Callback from the physical layer to indicate that a MISO frame was received.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void spi_drv_DataReceived( void )
{
   spi_drv_fNewMisoReceived = TRUE;
}

/*------------------------------------------------------------------------------
** Reset the read fragmentation information.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void spi_drv_ResetReadFragInfo( void )
{
   spi_drv_sReadFragInfo.iNumWordsReceived = 0;
   spi_drv_sReadFragInfo.psReadMsg = NULL;
   spi_drv_sReadFragInfo.puCurrPtr = NULL;
}

/*------------------------------------------------------------------------------
** Reset the write fragmentation information.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void spi_drv_ResetWriteFragInfo( void )
{
   ABCC_PORT_UseCritical();

   spi_drv_sWriteFragInfo.iNumWordsLeft = 0;
   spi_drv_sWriteFragInfo.iCurrFragLength = 0;
   spi_drv_sWriteFragInfo.puCurrPtr = NULL;

   ABCC_PORT_EnterCritical();
   spi_drv_sWriteFragInfo.psWriteMsg = NULL;
   ABCC_PORT_ExitCritical();
}

/*------------------------------------------------------------------------------
** Watchdog timeouthandler
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
static void drv_WdTimeoutHandler( void )
{
   fWdTmo = TRUE;
   ABCC_CbfWdTimeout();
}

void ABCC_DrvSpiInit( UINT8 bOpmode )
{
   UINT16 i;
   (void)bOpmode;

   spi_drv_sMosiFrame.iSpiControl = 0;
   for( i = 0; i < MAX_PAYLOAD_WORD_LEN; i++ )
   {
      spi_drv_sMosiFrame.iData[ i ] = 0;
      spi_drv_sMisoFrame.iData[ i ] = 0;
   }

   spi_drv_ResetReadFragInfo();
   spi_drv_fNewMisoReceived = FALSE;
   spi_drv_bAnbStatus = 0;
   spi_drv_psReadMessage = 0;
   spi_drv_ResetWriteFragInfo();
   spi_drv_sMosiFrame.iIntMaskAppStatus = 0;
   spi_drv_bNbrOfCmds = 0;
   spi_drv_eState = SM_SPI_INIT;
   spi_drv_iPdSize = SPI_DEFAULT_PD_LEN;
   spi_drv_iWritePdSize = SPI_DEFAULT_PD_LEN;
   spi_drv_iReadPdSize = SPI_DEFAULT_PD_LEN;
   spi_drv_iPdOffset = NUM_BYTES_2_WORDS( ABCC_CFG_SPI_MSG_FRAG_LEN );
   spi_drv_iCrcOffset = NUM_BYTES_2_WORDS( ABCC_CFG_SPI_MSG_FRAG_LEN ) + SPI_DEFAULT_PD_LEN;
   spi_drv_iSpiFrameSize = SPI_FRAME_SIZE_EXCLUDING_DATA + spi_drv_iCrcOffset;
   spi_drv_fRetransmit = FALSE;
   spi_drv_iMsgLen = 0;

   spi_drv_iMsgLen = NUM_BYTES_2_WORDS( ABCC_CFG_SPI_MSG_FRAG_LEN );
   spi_drv_sMosiFrame.iMsgLen = iTOiLe( spi_drv_iMsgLen );

   spi_drv_sMosiFrame.iPdLen = iTOiLe( spi_drv_iPdSize );
   spi_drv_bNextAppStatus = 0;
   spi_drv_bNextIntMask = 0;
   spi_drv_bpRdPd = NULL;
   spi_drv_bAnbCmdCnt = 0;
   xWdTmoHandle = ABCC_TimerCreate( drv_WdTimeoutHandler );
   fWdTmo = FALSE;

   /*
   ** Register the MISO indicator for the physical SPI driver.
   */
   ABCC_HAL_SpiRegDataReceived( spi_drv_DataReceived );

#if ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
   /*
   ** Initialise sync measurement flag
   */
   fAbccUserSyncMeasurementIp = FALSE;
#endif
}

BOOL ABCC_DrvSpiWriteMessage( ABP_MsgType* psWriteMsg )
{
   ABCC_PORT_UseCritical();

   if( !psWriteMsg )
   {
      ABCC_LOG_FATAL( ABCC_EC_UNEXPECTED_NULL_PTR,
         0,
         "Unexpected NULL pointer\n" );
   }

   ABCC_PORT_EnterCritical();
   if( spi_drv_sWriteFragInfo.psWriteMsg != NULL )
   {
      ABCC_PORT_ExitCritical();
      ABCC_LOG_WARNING( ABCC_EC_ASSERT_FAILED, 0, "Write message not NULL\n" );
      return( FALSE );
   }
#if ( __GNUC__ >= 9 )
   #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
   /*
   ** psWriteMsg is 32 bit aligned so it is safe to ignore this warning
   */
#endif
   spi_drv_sWriteFragInfo.puCurrPtr = (UINT16*)psWriteMsg;
#if ( __GNUC__ >= 9 )
   #pragma GCC diagnostic pop
#endif
   spi_drv_sWriteFragInfo.iNumWordsLeft = NUM_BYTES_2_WORDS( iLeTOi( psWriteMsg->sHeader.iDataSize ) + ABCC_MSG_HEADER_TYPE_SIZEOF );
   spi_drv_sWriteFragInfo.psWriteMsg = psWriteMsg;
   ABCC_PORT_ExitCritical();

   /*
   ** The SPI driver still owns the buffer.
   */
   return( FALSE );
}

void ABCC_DrvSpiWriteProcessData( void* pxProcessData )
{
   (void)pxProcessData;
   if( spi_drv_eState == SM_SPI_RDY_TO_SEND_MOSI )
   {
      spi_drv_sMosiFrame.iSpiControl |= iSpiCtrlWrPdWalid;
   }
   else
   {
      ABCC_LOG_WARNING( ABCC_EC_SPI_OP_NOT_ALLOWED_DURING_SPI_TRANSACTION,
         (UINT32)spi_drv_eState,
         "Write process data operation not allowed in this state (state: %d)\n",
         spi_drv_eState );
   }
}

BOOL ABCC_DrvSpiIsReadyForWriteMessage( void )
{
   BOOL fRdyForWrMsg = FALSE;

   if( spi_drv_sWriteFragInfo.psWriteMsg == NULL )
   {
      fRdyForWrMsg = TRUE;
   }
   return( fRdyForWrMsg );
}

BOOL ABCC_DrvSpiIsReadyForCmd( void )
{
   return( pnABCC_DrvISReadyForWriteMessage() && ( spi_drv_bAnbCmdCnt > 0 ) );
}

void ABCC_DrvSpiSetNbrOfCmds( UINT8 bNbrOfCmds )
{
   spi_drv_bNbrOfCmds = bNbrOfCmds;
}

void ABCC_DrvSpiSetAppStatus( ABP_AppStatusType eAppStatus )
{
   spi_drv_bNextAppStatus = eAppStatus;
}

void ABCC_DrvSpiSetPdSize( const UINT16  iReadPdSize, const UINT16  iWritePdSize )
{
   if( spi_drv_eState == SM_SPI_RDY_TO_SEND_MOSI )
   {
      /*
      ** Use the largest PD data size since the PD cannot be fragmented.
      */
      spi_drv_iWritePdSize = NUM_BYTES_2_WORDS( iWritePdSize );
      spi_drv_iReadPdSize = NUM_BYTES_2_WORDS( iReadPdSize );

      spi_drv_iPdSize = spi_drv_iWritePdSize;
      if( spi_drv_iReadPdSize > spi_drv_iWritePdSize )
      {
         spi_drv_iPdSize = spi_drv_iReadPdSize;
      }

      /*
      ** Update the CRC position and the total frame size since the process data
      ** size might have changed.
      */
      spi_drv_iCrcOffset = spi_drv_iPdOffset + spi_drv_iPdSize;
      spi_drv_iSpiFrameSize = SPI_FRAME_SIZE_EXCLUDING_DATA + spi_drv_iCrcOffset;
      spi_drv_sMosiFrame.iPdLen = iTOiLe( spi_drv_iPdSize );
   }
   else
   {
      ABCC_LOG_WARNING( ABCC_EC_SPI_OP_NOT_ALLOWED_DURING_SPI_TRANSACTION,
         (UINT32)spi_drv_eState, "Set PD size operation not allowed in this state (state: %d)\n", spi_drv_eState );
   }
}

static void DrvSpiSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg )
{
   if( spi_drv_sReadFragInfo.puCurrPtr == NULL )
   {
      spi_drv_sReadFragInfo.psReadMsg = psReadMsg;
#if ( __GNUC__ >= 9 )
   #pragma GCC diagnostic push
   #pragma GCC diagnostic ignored "-Waddress-of-packed-member"
   /*
   ** psReadMsg is 32 bit aligned so it is safe to ignore this warning
   */
#endif
      spi_drv_sReadFragInfo.puCurrPtr = (UINT16*)psReadMsg;
#if ( __GNUC__ >= 9 )
   #pragma GCC diagnostic pop
#endif
      spi_drv_sReadFragInfo.iNumWordsReceived = 0;
   }
   else
   {
      ABCC_LOG_WARNING( ABCC_EC_SPI_OP_NOT_ALLOWED_DURING_SPI_TRANSACTION,
         (UINT32)spi_drv_sReadFragInfo.puCurrPtr,
         "Set message receiver buffer operation not allowed in this state (state: %d)\n",
         spi_drv_eState );
   }
}

UINT16 ABCC_DrvSpiGetIntStatus( void )
{
   ABCC_LOG_FATAL( ABCC_EC_INTSTATUS_NOT_SUPPORTED_BY_DRV_IMPL,
      0,
      "Interrupt status not supported by SPI driver\n" );

   return( 0xFFFF );
}

UINT8 ABCC_DrvSpiGetAnybusState( void )
{
   return( spi_drv_bAnbStatus & 0x7 );
}

void* ABCC_DrvSpiReadProcessData( void )
{
   UINT8* pxRdPd = NULL;

   if(  spi_drv_eState == SM_SPI_RDY_TO_SEND_MOSI )
   {
      pxRdPd = spi_drv_bpRdPd;
   }
   return( pxRdPd );
}

ABP_MsgType* ABCC_DrvSpiReadMessage( void )
{
   ABP_MsgType* psRdMsg = NULL;

   if( spi_drv_eState == SM_SPI_RDY_TO_SEND_MOSI )
   {
      if( spi_drv_psReadMessage != NULL )
      {
         psRdMsg = spi_drv_psReadMessage;
         spi_drv_psReadMessage = NULL;
      }
   }
   return( psRdMsg );
}

void ABCC_DrvSpiSetIntMask( const UINT16 iIntMask )
{
   spi_drv_bNextIntMask = (UINT8)iIntMask;
}

void* ABCC_DrvSpiGetWrPdBuffer( void )
{
   return( &spi_drv_sMosiFrame.iData[ spi_drv_iPdOffset ] );
}

UINT16 ABCC_DrvSpiGetModCap( void )
{
   ABCC_LOG_WARNING( ABCC_EC_MODCAP_NOT_SUPPORTED_BY_DRV_IMPL,
      0,
      "Module capability not supported by SPI driver\n" );
   return( 0 );
}

UINT16 ABCC_DrvSpiGetLedStatus( void )
{
   return( spi_drv_iLedStatus );
}

BOOL ABCC_DrvSpiIsReadyForWrPd( void )
{
   if( spi_drv_eState == SM_SPI_RDY_TO_SEND_MOSI )
   {
      return( TRUE );
   }
   return( FALSE );
}

BOOL ABCC_DrvSpiIsSupervised( void )
{
   /*
   ** The Anybus supervision bis is stored in bit 3
   */
   return( ( spi_drv_bAnbStatus  >> 3 ) & 1 );
}

UINT8 ABCC_DrvSpiGetAnbStatus( void )
{
   return( (UINT8)spi_drv_bAnbStatus & 0xf );
}

#endif
