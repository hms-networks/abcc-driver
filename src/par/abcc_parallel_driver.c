/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Parallel (PARI) driver implementation.
********************************************************************************
*/

#include "abcc_config.h"

#if ABCC_CFG_DRV_PARALLEL_ENABLED

#include "abcc_types.h"
#include "abcc.h"
#include "abcc_log.h"
#include "abcc_hardware_abstraction.h"
#include "../abcc_timer.h"
#include "../abcc_driver_interface.h"
#include "../abcc_memory.h"
#include "abp.h"
#include "abcc_hardware_abstraction_parallel.h"
#include "../abcc_handler.h"
#include "abcc_port.h"

#if ( ABCC_CFG_MAX_MSG_SIZE < 16 )
#error "ABCC_CFG_MAX_MSG_SIZE must be at least a 16 bytes"
#endif

/*
** If the internal and external memory bus have different endianess special
** macros are required to convert between bus endian and native endian.
*/
#ifdef ABCC_SYS_BIG_ENDIAN
   #if ABCC_CFG_PAR_EXT_BUS_ENDIAN_DIFF
      #define iLeExtBusTOi( iLeFoo )               (UINT16)( iLeFoo )
      #define iTOiLeExtBus( iFoo )                 (UINT16)( iFoo )
   #else
      #define iLeExtBusTOi( iLeFoo )               ABCC_iEndianSwap( iLeFoo )
      #define iTOiLeExtBus( iFoo )                 ABCC_iEndianSwap( iFoo )
   #endif
#else
   #if ABCC_CFG_PAR_EXT_BUS_ENDIAN_DIFF
      #define iLeExtBusTOi( iLeFoo )               ABCC_iEndianSwap( iLeFoo )
      #define iTOiLeExtBus( iFoo )                 ABCC_iEndianSwap( iFoo )
   #else
      #define iLeExtBusTOi( iLeFoo )               (UINT16)( iLeFoo )
      #define iTOiLeExtBus( iFoo )                 (UINT16)( iFoo )
   #endif
#endif

static ABCC_MsgType par_drv_uReadMessageData;
static  void* par_drv_pbRdPdBuffer;

static UINT16   par_drv_iSizeOfReadPd;
static UINT16   par_drv_iSizeOfWritePd;

static   UINT8    par_drv_bNbrOfCmds;          /* Number of commands supported by the application. */

static const    UINT16   iWRPDFlag     = 0x01;
static const    UINT16   iRDPDFlag     = 0x02;
static const    UINT16   iWRMSGFlag    = 0x04;
static const    UINT16   iRDMSGFlag    = 0x08;
static const    UINT16   iANBRFlag     = 0x10;
static const    UINT16   iAPPRFlag     = 0x20;
static const    UINT16   iAPPRCLRFlag  = 0x40;

#define ABCC_MSG_HEADER_TYPE_SIZEOF 12

#ifdef ABCC_SYS_16_BIT_CHAR
static const UINT16 iWrMsgAdrOffset =        ABP_WRMSG_ADR_OFFSET / 2;
static const UINT16 iRdMsgAdrOffset =        ABP_RDMSG_ADR_OFFSET / 2;
static const UINT16 iModCapAdrOffset =       ABP_MODCAP_ADR_OFFSET / 2;
static const UINT16 iLedStatusAdrOffset =    ABP_LEDSTATUS_ADR_OFFSET / 2;
static const UINT16 iAppStatusAdrOffset =    ABP_APPSTATUS_ADR_OFFSET / 2;
static const UINT16 iAnbStatusAdrOffset =    ABP_ANBSTATUS_ADR_OFFSET / 2;
static const UINT16 iBufCtrlAdrOffset =      ABP_BUFCTRL_ADR_OFFSET / 2;
static const UINT16 iMsgHdrEndAdrOffset =    ABCC_MSG_HEADER_TYPE_SIZEOF / 2;
static const UINT16 iIntMaskAdrOffset =      ABP_INTMASK_ADR_OFFSET / 2;
static const UINT16 iIntStatusAdrOffset =    ABP_INTSTATUS_ADR_OFFSET / 2;
#else
static const UINT16 iWrMsgAdrOffset =        ABP_WRMSG_ADR_OFFSET;
static const UINT16 iRdMsgAdrOffset =        ABP_RDMSG_ADR_OFFSET;
static const UINT16 iModCapAdrOffset =       ABP_MODCAP_ADR_OFFSET;
static const UINT16 iLedStatusAdrOffset =    ABP_LEDSTATUS_ADR_OFFSET;
static const UINT16 iAppStatusAdrOffset =    ABP_APPSTATUS_ADR_OFFSET;
static const UINT16 iAnbStatusAdrOffset =    ABP_ANBSTATUS_ADR_OFFSET;
static const UINT16 iBufCtrlAdrOffset =      ABP_BUFCTRL_ADR_OFFSET;
static const UINT16 iMsgHdrEndAdrOffset =    ABCC_MSG_HEADER_TYPE_SIZEOF;
static const UINT16 iIntMaskAdrOffset =      ABP_INTMASK_ADR_OFFSET;
static const UINT16 iIntStatusAdrOffset =    ABP_INTSTATUS_ADR_OFFSET;

#endif

void ABCC_DrvParInit( UINT8 bOpmode )
{
   (void)bOpmode;

   par_drv_iSizeOfReadPd  = 0;
   par_drv_iSizeOfWritePd = 0;
   par_drv_bNbrOfCmds     = 0;
   par_drv_pbRdPdBuffer   = ABCC_DrvParallelGetRdPdBuffer();
}


void ABCC_DrvParSetIntMask( const UINT16 iIntMask )
{
   ABCC_DrvWrite16( iIntMaskAdrOffset, iTOiLeExtBus( iIntMask ) );
}

#if ABCC_CFG_INT_ENABLED
UINT16 ABCC_DrvParISR( void )
{
   UINT16 iIntStatus;
   UINT16 iIntToHandle;

   /*---------------------------------------------------------------------------
   ** Read the interrupt status register and acknowledge all interrupts.
   ** Read interrupt status until all enabled interrupts are acknowledged.
   ** This will make sure that edge triggered interrupt always will trigger
   ** even if a new event has occurred between the int status read the
   ** acknowledge.
   **---------------------------------------------------------------------------
   */
   iIntStatus = ABCC_DrvRead16( iIntStatusAdrOffset );
   ABCC_DrvWrite16( iIntStatusAdrOffset, iIntStatus );
   iIntStatus = ( iLeExtBusTOi( iIntStatus ) ) & ABCC_iInterruptEnableMask;
   iIntToHandle = iIntStatus;

   while( iIntStatus != 0 )
   {
    iIntStatus = ABCC_DrvRead16( iIntStatusAdrOffset );
    ABCC_DrvWrite16( iIntStatusAdrOffset, iIntStatus );
    iIntStatus = ( iLeExtBusTOi( iIntStatus ) ) & ABCC_iInterruptEnableMask;
    iIntToHandle |= iIntStatus;
   }

   return( iIntToHandle );
}
#else
UINT16 ABCC_DrvParISR( void )
{
   ABCC_LOG_WARNING( ABCC_EC_INTERNAL_ERROR,
      0,
      "ABCC_DrvParISR() called when interrupts are disabled\n" );
   return( 0 );
}
#endif

ABP_MsgType* ABCC_DrvParRunDriverRx( void )
{
   /*
   ** Always NULL for the parallel interface.
   */
   return( NULL );
}


void ABCC_DrvParPrepareWriteMessage( ABP_MsgType* psWriteMsg )
{
   if( !psWriteMsg )
   {
      ABCC_LOG_FATAL( ABCC_EC_UNEXPECTED_NULL_PTR, 0, "Unexpected NULL pointer\n" );
   }
#ifdef MSG_TIMING
   /*Toggle led for timing measurement*/
   GPIO_OUT0  = 0;
#endif
   ABCC_DrvParallelWrite( iWrMsgAdrOffset, psWriteMsg, ABCC_MSG_HEADER_TYPE_SIZEOF +  ABCC_GetMsgDataSize( psWriteMsg ) );
}

BOOL ABCC_DrvParWriteMessage( ABP_MsgType* psWriteMsg )
{
   UINT16 iBufControlWriteFlags;
   ABCC_MsgType uMsg;
   iBufControlWriteFlags = 0;
   if( !psWriteMsg )
   {
      ABCC_LOG_FATAL( ABCC_EC_UNEXPECTED_NULL_PTR, 0, "Unexpected NULL pointer\n" );
   }
   uMsg.psMsg = psWriteMsg;

   iBufControlWriteFlags |= iWRMSGFlag;

   /*
   ** Determine if command messages (instead of response messages) can be sent.
   */
   if( !( ABCC_GetLowAddrOct( uMsg.psMsg16->sHeader.iCmdReserved ) & ABP_MSG_HEADER_C_BIT ) )
   {
      /*
      ** A command message has been received by the host application and a
      ** response message (not a command message) will be sent back to the
      ** Anybus. The number of commands the host application can receive
      ** shall be increased by one, as a previous received command is now
      ** handled.
      */
      par_drv_bNbrOfCmds++;

      /*
      ** When a change of the number of commands which the host application
      ** can receive is made from 0 to 1, it means that we can set the APPRF
      ** flag again to indicate for the Anybus that the host is now ready to
      ** receive a new command.
      */
      if( par_drv_bNbrOfCmds == 1 )
      {
         iBufControlWriteFlags |= iAPPRFlag;
      }
   }
   /*
   ** Update the buffer control register.
   */

   ABCC_DrvWrite16( iBufCtrlAdrOffset, iTOiLeExtBus( iBufControlWriteFlags ) );
#ifdef MSG_TIMING
   /*Toggle led for timing measurement*/
   GPIO_OUT0  = 1;
#endif
   return( TRUE );
}

void ABCC_DrvParWriteProcessData( void* pxProcessData )
{

   if( par_drv_iSizeOfWritePd )
   {
      /*
      ** Write process data.
      */
#if !( ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED )
      ABCC_HAL_ParallelWrite( ABP_WRPD_ADR_OFFSET,
                              pxProcessData,
                              par_drv_iSizeOfWritePd );
#else
      (void)pxProcessData;
#endif

      /*
      ** Update the buffer control register.
      */
      ABCC_DrvWrite16( iBufCtrlAdrOffset, iTOiLeExtBus( iWRPDFlag ) );
#ifdef PD_TIMING
      /*Toggle led for timing measurement*/
      GPIO_OUT0  = 1;
#endif
   }
}


BOOL ABCC_DrvParIsReadyForWriteMessage( void )
{
   UINT16 iBufControl;
   iBufControl = ABCC_DrvRead16( iBufCtrlAdrOffset );

   return( !( iLeExtBusTOi( iBufControl ) & iWRMSGFlag ) );
}


BOOL ABCC_DrvParIsReadyForCmd( void )
{
   UINT16 iBufControl;
   iBufControl = ABCC_DrvRead16( iBufCtrlAdrOffset );
   iBufControl = iLeExtBusTOi( iBufControl );
   return( !( iBufControl & iWRMSGFlag ) && ( iBufControl & iANBRFlag ) );
}


void ABCC_DrvParSetNbrOfCmds( UINT8 bNbrOfCmds )
{
   par_drv_bNbrOfCmds = bNbrOfCmds;

   /*
   ** Acknowledge that we are ready to accept the first command message.
   */
   ABCC_DrvWrite16( iBufCtrlAdrOffset, iTOiLeExtBus( iAPPRFlag ) );
}


void ABCC_DrvParSetAppStatus( ABP_AppStatusType eAppStatus )
{
   ABCC_DrvWrite16( iAppStatusAdrOffset, iTOiLeExtBus( (UINT16)eAppStatus ) );
}


void ABCC_DrvParSetPdSize( const UINT16 iReadPdSize, const UINT16 iWritePdSize )
{
   par_drv_iSizeOfReadPd = iReadPdSize;
   par_drv_iSizeOfWritePd = iWritePdSize;
   (void)par_drv_iSizeOfReadPd;
}


static void DrvParSetMsgReceiverBuffer( ABP_MsgType* const psReadMsg )
{
   par_drv_uReadMessageData.psMsg = psReadMsg;
}


UINT16 ABCC_DrvParGetIntStatus( void )
{
   UINT16 iIntStatus;

   iIntStatus = ABCC_DrvRead16( iIntStatusAdrOffset );

   return( iLeExtBusTOi( iIntStatus ) );
}

UINT8 ABCC_DrvParGetAnybusState( void )
{
   UINT16 iAnbStatus;

   /*
   ** Reading out the Anybus status.
   */
   iAnbStatus = ABCC_DrvRead16( iAnbStatusAdrOffset );

   /*
   ** The Anybus state is stored in bits 0-2 of the read register.
   */
   return( (UINT8)( iLeExtBusTOi( iAnbStatus ) & 0x07 ) );
}


void* ABCC_DrvParReadProcessData( void )
{
   UINT16 iBufctrl;

   /*
   ** Check if the Anybus has updated the read process data.
   */
   iBufctrl = ABCC_DrvRead16( iBufCtrlAdrOffset );

   if( iLeExtBusTOi( iBufctrl ) & iRDPDFlag  )
   {
      /*
      ** The RDPD flag must be set before we try to read the process data.
      ** Otherwise the buffers won't be switched and we won't have any process
      ** data available.
      */
      ABCC_DrvWrite16( iBufCtrlAdrOffset, iTOiLeExtBus( iRDPDFlag ) );

      /*
      ** We have process data to read.
      */
#if !( ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED )
      ABCC_HAL_ParallelRead( ABP_RDPD_ADR_OFFSET,
                             par_drv_pbRdPdBuffer,
                             par_drv_iSizeOfReadPd );
#endif

      return( par_drv_pbRdPdBuffer );
   }
   else
   {
      return( NULL );
   }
}


ABP_MsgType* ABCC_DrvParReadMessage( void )
{
   UINT16 iBufctrl;
   UINT16 iMsgSize;

   iBufctrl = ABCC_DrvRead16( iBufCtrlAdrOffset );

   if( iLeExtBusTOi( iBufctrl ) & iRDMSGFlag  )
   {
      DrvParSetMsgReceiverBuffer( ABCC_MemAlloc() );

      if( par_drv_uReadMessageData.psMsg == NULL )
      {
         ABCC_LOG_WARNING( ABCC_EC_OUT_OF_MSG_BUFFERS,
            0,
            "Out of message buffers when attempting to read a message\n" );
         return( NULL );
      }

      /*
      ** We have message data to read. First read the header and check the size
      ** of data area.
      */
      ABCC_DrvParallelRead( iRdMsgAdrOffset,
                            par_drv_uReadMessageData.psMsg16,
                            ABCC_MSG_HEADER_TYPE_SIZEOF );

      iMsgSize = iLeTOi( par_drv_uReadMessageData.psMsg16->sHeader.iDataSize );

      if( ( iMsgSize <= ABCC_CFG_MAX_MSG_SIZE ) &&
          ( iMsgSize != 0 ) )
      {
         /*
         ** There is data and it fits in buffer size, so the message is read.
         */
         ABCC_DrvParallelRead( iRdMsgAdrOffset + iMsgHdrEndAdrOffset,
                               par_drv_uReadMessageData.psMsg16->aiData,
                               iMsgSize );
      }

      /*
      ** Determine if command messages (instead of response messages) can be read.
      */
      if( ABCC_GetLowAddrOct( par_drv_uReadMessageData.psMsg16->sHeader.iCmdReserved ) & ABP_MSG_HEADER_C_BIT )
      {
         /*
         ** A command messages has been sent by the Anybus and it has been read
         ** by the host application. The number of commands allowed by the host
         ** application must be decreased by one.
         */
         par_drv_bNbrOfCmds--;

         /*
         ** Indicates that the host application is not ready to receive a new
         ** command from the Anybus. Writing to this register must only be done
         ** when the RDMSG bit is set to 1. A check is not required however,
         ** since the RDMSG bit is set to 1 a few lines higher up in the code.
         */
         if( par_drv_bNbrOfCmds == 0 )
         {
            /*
            ** Update the buffer control register.
            */
            ABCC_DrvWrite16( iBufCtrlAdrOffset, iTOiLeExtBus( iAPPRCLRFlag ) );
         }
      }

      ABCC_DrvWrite16( iBufCtrlAdrOffset, iTOiLeExtBus( iRDMSGFlag ) );

      return( par_drv_uReadMessageData.psMsg );
   }
   else
   {
      return( NULL );
   }
}

void* ABCC_DrvParGetWrPdBuffer( void )
{
   return( ABCC_DrvParallelGetWrPdBuffer() );
}

UINT16 ABCC_DrvParGetModCap( void )
{
   UINT16 iModCap;
   iModCap = ABCC_DrvRead16( iModCapAdrOffset );
   return( iLeExtBusTOi( iModCap ) );
}


UINT16 ABCC_DrvParGetLedStatus( void )
{
   UINT16 iLedStatus;
   iLedStatus = ABCC_DrvRead16( iLedStatusAdrOffset );
   return( iLeExtBusTOi( iLedStatus ) );
}

BOOL ABCC_DrvParIsReadyForWrPd( void )
{
   return( TRUE );
}


BOOL ABCC_DrvParIsSupervised( void )
{
   UINT16 iAnbStatus;
   /*
   ** Reading out the Anybus status.
   */
   iAnbStatus = ABCC_DrvRead16( iAnbStatusAdrOffset );
   iAnbStatus = iLeExtBusTOi( iAnbStatus );

   /*
   ** The Anybus supervision bis is stored in bit 3
   */
   return( ( iAnbStatus >> 3 ) & 1 );

}

UINT8 ABCC_DrvParGetAnbStatus( void )
{
   UINT16 iAnbStatus;
   /*
   ** Reading out the Anybus status.
   */
   iAnbStatus = ABCC_DrvRead16( iAnbStatusAdrOffset );
   iAnbStatus = iLeExtBusTOi( iAnbStatus );
   return( (UINT8)iAnbStatus & 0xf );

}
#endif
