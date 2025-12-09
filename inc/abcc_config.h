/*******************************************************************************
** Copyright 2015-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Configuration parameters of the driver.
**
** The following user definitions are used to control and configure the driver.
** The default options specified in this file can be overridden by the user
** in abcc_driver_config.h and abcc_types.h.
********************************************************************************
*/

#ifndef ABCC_CFG_H_
#define ABCC_CFG_H_

#include "abcc_driver_config.h"

/*------------------------------------------------------------------------------
** #define ABCC_SYS_BIG_ENDIAN
**
** Defined in abcc_types.h.
**
** Define if an big endian system is used as host. If not defined little endian
** is assumed.
**------------------------------------------------------------------------------
*/

/*------------------------------------------------------------------------------
** #define ABCC_SYS_16_BIT_CHAR
**
** Defined in abcc_types.h.
**
** Define if a 16 bit char system is used as host. If not defined 8 bit char
** system is assumed.
**------------------------------------------------------------------------------
*/

/*------------------------------------------------------------------------------
** #define ABCC_CFG_PAR_EXT_BUS_ENDIAN_DIFF   1 - Enable / 0 - Disable
**
** Defined in abcc_types.h.
**
** Define as 1 if the internal and external memory bus of the host application
** processor have different endianess. The default behavior is 'no endian swap'.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_PAR_EXT_BUS_ENDIAN_DIFF
    #define ABCC_CFG_PAR_EXT_BUS_ENDIAN_DIFF 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_DRV_SPI_ENABLED        1 - Enable / 0 - Disable
** #define ABCC_CFG_DRV_PARALLEL_ENABLED   1 - Enable / 0 - Disable
** #define ABCC_CFG_DRV_SERIAL_ENABLED     1 - Enable / 0 - Disable
**
** At least one of the low-level drivers must be enabled.
**
** These identifiers enables/disables the inclusion of the corresponding
** low-level application interface driver and handshaking code.
**
** These are related to, but are not the same as, the Operating Mode setting.
**
** It is possible to include support for multiple low-level drivers. Each
** enabled driver increases ROM and RAM consumption.
**
** Depending on the value of the Module Identification pins in the application
** connector different operating modes can be selected automatically at startup.
** See ABCC_CFG_ABCC_OP_MODE configuration further down.
**------------------------------------------------------------------------------
** Summary of the capabilities of the individual drivers, and the associated
** Operating Modes:
**
** "ABCC_CFG_DRV_SPI_ENABLED"
**
** - This driver provides support for full-duplex, cycle-based data transfers
**   via the SPI application interface on the ABCC.
**
** - Extended ABCC40 Process Data (4096 bytes) and Message Data (1536 bytes)
**   sizes are supported.
**
** - Synchronous operation is supported.
**
** - This driver is mandatory to include if the "ABP_OP_MODE_SPI" Operation
**   Mode is to be used.
**
** "ABCC_CFG_DRV_PARALLEL_ENABLED"
**
** - This driver provides support for event-based handshaking and data
**   transfers via the parallel memory interface on the ABCC.
**
** - Extended ABCC40 Process Data (4096 bytes) and Message Data (1536 bytes)
**   sizes are supported.
**
** - Synchronous operation is supported.
**
** - This driver should be the default one if the parallel application
**   interface of an ABCC is to be used. It supports both 8-bit and 16-bit
**   data bus width, i.e. the "ABP_OP_MODE_8_BIT_PARALLEL" and
**   "ABP_OP_MODE_16_BIT_PARALLEL" Operation Modes can both be used with this
**   driver.
**
** - This driver is mandatory to include if 16-bit parallel operation
**   ("ABP_OP_MODE_16_BIT_PARALLEL") is to be used.
**
** "ABCC_CFG_DRV_SERIAL_ENABLED"
**
** - Includes the low-level driver for the 'half-duplex' serial handshake
**   method.
**
** - This handshake method is supported by ABCC40, but is not recommended
**   as the primary one for the ABCC40 series.
**
** - Synchronous operation is not supported.
**
** - This driver is mandatory to include if any of the "ABP_OP_MODE_SERIAL_..."
**   Operation Modes are to be used.
**------------------------------------------------------------------------------
** Compatibility between the different Operating Modes and the low-level
** drivers:
**
** "ABP_OP_MODE_SPI"
**
** - Requires that the "ABCC_CFG_DRV_SPI_ENABLED" low-level driver is included.
**
** "ABP_OP_MODE_16_BIT_PARALLEL"
**
** - Requires that the "ABCC_CFG_DRV_PARALLEL_ENABLED" low-level driver is
**   included.
**
** "ABP_OP_MODE_8_BIT_PARALLEL"
**
** - Requires that "ABCC_CFG_DRV_PARALLEL_ENABLED" low-level driver is included.
**
** "ABP_OP_MODE_SERIAL_..."
**
**  - Requires that the "ABCC_CFG_DRV_SERIAL_ENABLED" low-level driver is
**    included.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_DRV_SPI_ENABLED
    #define ABCC_CFG_DRV_SPI_ENABLED 0
#endif
#ifndef ABCC_CFG_DRV_PARALLEL_ENABLED
    #define ABCC_CFG_DRV_PARALLEL_ENABLED 0
#endif
#ifndef ABCC_CFG_DRV_SERIAL_ENABLED
    #define ABCC_CFG_DRV_SERIAL_ENABLED 0
#endif

#if ( ABCC_CFG_DRV_SPI_ENABLED == 0 ) && ( ABCC_CFG_DRV_PARALLEL_ENABLED == 0 ) && ( ABCC_CFG_DRV_SERIAL_ENABLED == 0 )
    #error "At least one of the low-level drivers must be enabled."
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_OP_MODE_GETTABLE         1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver to retrieve the operating mode from external hardware.
** If 1 the ABCC_HAL_GetOpmode() function must be implemented in the hardware
** abstraction layer.
**
** If this is not 1 ABCC_CFG_ABCC_OP_MODE_X described above must be defined.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_OP_MODE_GETTABLE
    #define ABCC_CFG_OP_MODE_GETTABLE 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_OP_MODE_SETTABLE         1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver to control the operating mode set to the ABCC host
** connector. Else it is assumed the operating mode signals of the host
** connector is fixed or controlled by external hardware.
** If 1 the ABCC_HAL_SetOpmode() function must be implemented in the hardware
** abstraction layer.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_OP_MODE_SETTABLE
    #define ABCC_CFG_OP_MODE_SETTABLE 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_ABCC_OP_MODE
**
** If the operating mode is fixed (hardware strapped) ABCC_CFG_ABCC_OP_MODE must
** be defined to the fixed operating mode unless it's gettable (configured by
** ABCC_CFG_OP_MODE_GETTABLE).
**
** Available operating modes are available in abp.h as "ABP_OP_MODE_*".
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_ABCC_OP_MODE
    #if( !ABCC_CFG_OP_MODE_GETTABLE )
        #error "ABCC_CFG_ABCC_OP_MODE must be defined if ABCC_CFG_OP_MODE_GETTABLE is disabled."
    #endif
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_SPI_MSG_FRAG_LEN                   ( 32 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Length of SPI message fragment in bytes per SPI transaction.
** If the message fragment length is shorter than the largest message to be
** transmitted the sending or receiving of a message may be fragmented and
** take several SPI transactions to be completed. Each SPI transaction will have
** a message field of this length regardless if a message is present or not.
** If messages are important the fragment length should be set to the largest
** message to avoid fragmentation. If IO data are important the message fragment
** length should be set to a smaller value to speed up the SPI transaction.
** For high message performance a fragment length up to 1524 octets is
** supported. The message header is 12 octets, so 16 or 32 octets will be enough
** to support small messages without fragmentation.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_SPI_MSG_FRAG_LEN
    #define ABCC_CFG_SPI_MSG_FRAG_LEN ( 32 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_SPI_CRC_REDUCED_TABLE_ENABLED  1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** There are two versions of the CRC algorithm implemented where one uses a full
** CRC table and one uses a reduced CRC table. The full table is typically
** faster but will consume more memory (1024 bytes vs 64 bytes). Configure this
** based on performance needs vs available memory.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_SPI_CRC_REDUCED_TABLE_ENABLED
   #define ABCC_SPI_CRC_REDUCED_TABLE_ENABLED 1
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED  1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for memory mapped ABCC interface. If memory
** direct access is chosen the user will have access directly to the ABCC
** process data memory i.e. no internal copy is required.
** If 0 the following functions must be implemented in the hardware abstraction
** layer:
**    ABCC_HAL_ParallelRead()
**    ABCC_HAL_ParallelRead16()
**    ABCC_HAL_ParallelWrite()
**    ABCC_HAL_ParallelWrite16()
**    ABCC_HAL_ParallelGetRdPdBuffer()
**    ABCC_HAL_ParallelGetWrPdBuffer()
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
    #if ABCC_CFG_DRV_PARALLEL_ENABLED
        #error "ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED must be defined to 1 or \
0 if ABCC_CFG_DRV_PARALLEL_ENABLED is enabled."
    #endif
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_PARALLEL_BASE_ADR             ( 0x00000000 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Define the base address of the ABCC if a memory mapped interface is used.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_PARALLEL_BASE_ADR
    #if ABCC_CFG_DRV_PARALLEL_ENABLED && ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED
        #error "ABCC_CFG_PARALLEL_BASE_ADR must be defined if \
ABCC_CFG_DRV_PARALLEL_ENABLED and ABCC_CFG_MEMORY_MAPPED_ACCESS_ENABLED are enabled."
    #endif
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_SERIAL_TMO_19_2                      ( 350 )
** #define ABCC_CFG_SERIAL_TMO_57_6                      ( 120 )
** #define ABCC_CFG_SERIAL_TMO_115_2                     ( 60 )
** #define ABCC_CFG_SERIAL_TMO_625                       ( 20 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Sets the baudrate-dependent telegram-cyle timeout time, in milliseconds,
** for the asynchronous serial application interface. This time is used in
** "abcc_serial_driver.c" for each serial telegram cycle.
**
** For an implementation where UART operations have low latency, like with a
** normal microcontroller with an integrated UART, there is usually no need
** to change those values. For an implementation that might add extra latency
** to the UART operations, like if a USB UART or similar is used, it might be
** necessary to increase those values in order to compensate for any constant
** or baudrate-dependent latency that this interface may add.
**
** For the details about the conditions that this timeout applies to, check:
**    Chapter 4.6.4 - Transmission Errors
**    Anybus CompactCom 30
**    SOFTWARE DESIGN GUIDE
**    HMSI-168-97
**    revision 3.1.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_SERIAL_TMO_19_2
    #define ABCC_CFG_SERIAL_TMO_19_2 ( 350 )
#endif
#ifndef ABCC_CFG_SERIAL_TMO_57_6
    #define ABCC_CFG_SERIAL_TMO_57_6 ( 120 )
#endif
#ifndef ABCC_CFG_SERIAL_TMO_115_2
    #define ABCC_CFG_SERIAL_TMO_115_2 ( 60 )
#endif
#ifndef ABCC_CFG_SERIAL_TMO_625
    #define ABCC_CFG_SERIAL_TMO_625 ( 20 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MODULE_ID_PINS_CONN       1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver to retrieve the module identification from external
** hardware. If 1 the ABCC_HAL_ReadModuleId() function must be implemented
** in the hardware abstraction layer. If 0 module identification
** ABP_MODULE_ID_ACTIVE_ABCC40 is be assumed.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MODULE_ID_PINS_CONN
    #define ABCC_CFG_MODULE_ID_PINS_CONN 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MOD_DETECT_PINS_CONN     1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Set to 1 if the module detect pins on the ABCC host connector are
** in use. If 1 the ABCC_HAL_ModuleDetect() function in the hardware abstraction
** layer must be implemented.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MOD_DETECT_PINS_CONN
    #define ABCC_CFG_MOD_DETECT_PINS_CONN 1
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MAX_NUM_APPL_CMDS     ( 2 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Number of commands that could be sent without receiving a response.
** At least 2 buffers are required by the driver.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MAX_NUM_APPL_CMDS
    #define ABCC_CFG_MAX_NUM_APPL_CMDS ( 2 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MAX_NUM_ABCC_CMDS     ( 2 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Number of commands that could be received without sending a response.
** At least 2 buffers are required by the driver.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MAX_NUM_ABCC_CMDS
    #define ABCC_CFG_MAX_NUM_ABCC_CMDS ( 2 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MAX_MSG_SIZE                       ( 1524 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Size of largest message in bytes that will be used.
**
** Note! ABCC40 supports 1524 byte messages. ABCC_CFG_MAX_MSG_SIZE should be set
** to largest size that will be sent or received. If this size is not known it
** recommended to set the maximum supported size.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MAX_MSG_SIZE
    #define ABCC_CFG_MAX_MSG_SIZE ( 1524 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MAX_PROCESS_DATA_SIZE              ( 512 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Size of max process data in bytes that will be used in either direction.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MAX_PROCESS_DATA_SIZE
    #define ABCC_CFG_MAX_PROCESS_DATA_SIZE ( 512 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_SYNC_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for sync.
** If 1 the ABCC_CbfSyncIsr() must be implemented by the application.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_SYNC_ENABLED
    #define ABCC_CFG_SYNC_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support to enable and disable sync interrupt using
** the sync signal from the ABCC.
** If 1 ABCC_HAL_SyncInterruptEnable() and ABCC_HAL_SyncInterruptDisable()
** must be implemented by the application and ABCC_CbfSyncIsr() must be called
** from the sync interrupt handler.
** If 0 the ABCC interrupt sync event will be used as sync source and
** ABCC_CbfSyncIsr() will be called by the driver.
** The define is only valid if ABCC_CFG_SYNC_ENABLED is 1.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED
    #define ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support to read the status of the interrupt pin.
** This function will enable the driver to use the interrupt pin to check if an
** event has occurred even if the interrupt is disabled
** (ABCC_CFG_INT_ENABLED = 0). Presently this is used only to detect the
** startup interrupt.
**
** If 1 the user must implement the ABCC_HAL_IsAbccInterruptActive() function
** in the hardware abstraction layer.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED
    #define ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_INT_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for ABCC interrupt (IRQ_N pin in the host
** connector). If 1 the user must implement the following functions in the
** hardware abstraction layer:
**
**    ABCC_HAL_AbccInterruptEnable()
**    ABCC_HAL_AbccInterruptDisable()
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_INT_ENABLED
    #define ABCC_CFG_INT_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_INT_ENABLE_MASK_PAR                 ( ABP_INTMASK_RDPDIEN )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Defines what ABCC interrupts shall be enabled in parallel operating mode.
** This is a bit mask built up by the ABP_INTMASK_X definitions in abp.h.
** If an event is not notified via the ABCC interrupt it must be polled by
** ABCC_RunDriver(). If not defined in abcc_driver_config.h the default mask
** is 0.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_INT_ENABLE_MASK_PAR
    #define ABCC_CFG_INT_ENABLE_MASK_PAR ( 0 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_INT_ENABLE_MASK_SPI              ( 0 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Defines what ABCC interrupts shall be enabled in SPI operating mode.
** The mask is composed of ABP_INTMASK_X definitions in abp.h.
** If an event is not notified via the ABCC interrupt it must be polled using
** ABCC_RunDriver().
** If not defined in abcc_driver_config.h the default mask is 0.
**
** Note! There are currently no support in the driver to handle interrupt
** driven SPI based on ABCC events.
**------------------------------------------------------------------------------
*/

/*------------------------------------------------------------------------------
** #define ABCC_CFG_HANDLE_INT_IN_ISR_MASK  ( ABP_INTMASK_RDPDIEN )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Defines what interrupt events from the ABCC that should be handled in
** interrupt context. The mask is composed of ABP_INTMASK_X bits.
** Events that are enabled in the interrupt enable mask (ABCC_CFG_INT_ENABLE_MASK_X)
** but not configured to be handled by the ISR will be translated to a bit field
** of ABCC_ISR_EVENT_X definitions and forwarded to the user via the
** ABCC_CbfEvent() callback.
** If not defined in abcc_driver_config.h the default value will be:
** Parallel 16/8: 0 (No events handled by the ISR)
** Other operating modes N/A
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_HANDLE_INT_IN_ISR_MASK
    #define ABCC_CFG_HANDLE_INT_IN_ISR_MASK ( 0 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_WD_TIMEOUT_MS                      ( 1000 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Timeout for ABCC communication watchdog.
** Note! Currently the watchdog functionality is only supported by SPI-,
** serial operating mode.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_WD_TIMEOUT_MS
    #define ABCC_CFG_WD_TIMEOUT_MS ( 1000 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_REMAP_SUPPORT_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver and AD object support for the re-map command.
** If 1 the ABCC_CbfRemapDone() needs to be implemented by the application.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_REMAP_SUPPORT_ENABLED
    #define ABCC_CFG_REMAP_SUPPORT_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_STRUCT_DATA_TYPE_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver and AD object support for structured data data types.
** This define will affect the AD_AdiEntryType
** (abcc_application_data_interface.h) used for defining the user ADI:s.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_STRUCT_DATA_TYPE_ENABLED
    #define ABCC_CFG_STRUCT_DATA_TYPE_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for triggering of callback notifications each
** time an ADI is read or written. This define will affect the AD_AdiEntryType
** (abcc_application_data_interface.h) used for defining the user ADI:s.
** If an ADI is read by the network the callback is invoked before the action.
** If an ADI is written by the network the callback is invoked after the action.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED
    #define ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for triggering of callback used to set the
** Value attribute of an ADI. With this callback enabled it is up to the
** callback to validate the data in the set-command as well as copy the data to
** the correct data structure. Note that the pnSetAdiValue callback function
** enabled by ABCC_CFG_ADI_GET_SET_CALLBACK_ENABLED isn't called from the
** application data object command handler when this define is enabled and
** pnSetAdiValueTransparent isn't NULL. This define will affect the
** AD_AdiEntryType (abcc_application_data_interface.h) used for defining the
** user ADI:s.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED
    #define ABCC_CFG_ADI_TRANS_SET_CALLBACK_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Define if 64 bit integers in the Application Data object shall be supported.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED
    #define ABCC_CFG_64BIT_ADI_SUPPORT_ENABLED 1
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Define if 64 bit floats in the Application Data object shall be supported.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED
    #define ABCC_CFG_DOUBLE_ADI_SUPPORT_ENABLED 1
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_LOG_SEVERITY
**
** Configure the severity level to be logged. The different severity levels are
** defined in abcc_log.h using the format ABCC_LOG_SEVERITY_*_ENABLED.
** All severity levels including and above the configured level will be logged.
** ABCC_LOG_SEVERITY_DISABLED can be used to disable logging completely.
** For the severity level debug some component specific defines have to be
** enabled for full debug output. See ABCC_CFG_DEBUG_*_ENABLED below for
** details.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_LOG_SEVERITY
    #define ABCC_CFG_LOG_SEVERITY ABCC_LOG_SEVERITY_ERROR_ENABLED
#endif

/*------------------------------------------------------------------------------
** Defines for making the log output more verbose.
**
** #define ABCC_CFG_LOG_FILE_LINE_ENABLED   1 - Enable / 0 - Disable
**
** Include file name in the output logs. Makes it easier to locate the source
** of the print but consumes more memory.
**
** #define ABCC_CFG_LOG_COLORS_ENABLED      1 - Enable / 0 - Disable
**
** Enable colored debug logs. The console displaying the logs need to support
** ANSI colors for this to work.
**
** #define ABCC_CFG_LOG_TIMESTAMPS_ENABLED  1 - Enable / 0 - Disable
**
** Enable timestamps to be included in debug logs. The timestamps rely on
** ABCC_RunTimerSystem() being called at a regular interval.
**
** #define ABCC_CFG_LOG_STRINGS_ENABLED     1 - Enable / 0 - Disable
**
** Enable strings to be included in debug logs. This will increase the memory
** footprint of the driver but will aid in debugging. Log level debug and
** info will not be very useful without this define enabled.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_LOG_FILE_LINE_ENABLED
    #define ABCC_CFG_LOG_FILE_LINE_ENABLED 1
#endif
#ifndef ABCC_CFG_LOG_COLORS_ENABLED
    #define ABCC_CFG_LOG_COLORS_ENABLED 0
#endif
#ifndef ABCC_CFG_LOG_TIMESTAMPS_ENABLED
    #define ABCC_CFG_LOG_TIMESTAMPS_ENABLED 1
#endif
#ifndef ABCC_CFG_LOG_STRINGS_ENABLED
    #define ABCC_CFG_LOG_STRINGS_ENABLED 1
#endif

/*------------------------------------------------------------------------------
** Enable component specifc debug logs.
**
** #define ABCC_CFG_DEBUG_HEXDUMP_SPI_ENABLED          1 - Enable / 0 - Disable
** #define ABCC_CFG_DEBUG_HEXDUMP_UART_ENABLED         1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enables raw hexadecimal printouts of messages, SPI frames, and UART
** telegrams. Enabling any of those will also include a corresponding printout
** when a HW reset of the ABCC is made. Enabling the message hexdump will also
** add a printout when the ABCCs state changes.
**
** #define ABCC_CFG_DEBUG_MESSAGING_ENABLED            1 - Enable / 0 - Disable
**
** Enable/disable printout of received and sent messages. Related events such as
** buffer allocation and queue information is also printed.
**
** #define ABCC_CFG_DEBUG_HEXDUMP_MSG_ENABLED          1 - Enable / 0 - Disable
**
** Enable/disable printout of received and sent messages as hexdump. This is an
** alternative to ABCC_CFG_DEBUG_MESSAGING_ENABLED defined above in a less
** readable format but better suited for parsing.
**
** #define ABCC_CFG_DEBUG_CMD_SEQ_ENABLED              1 - Enable / 0 - Disable
**
** Enable/disable printout of command sequencer actions.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_DEBUG_HEXDUMP_SPI_ENABLED
    #define ABCC_CFG_DEBUG_HEXDUMP_SPI_ENABLED 0
#endif
#ifndef ABCC_CFG_DEBUG_HEXDUMP_UART_ENABLED
    #define ABCC_CFG_DEBUG_HEXDUMP_UART_ENABLED 0
#endif
#ifndef ABCC_CFG_DEBUG_MESSAGING_ENABLED
    #define ABCC_CFG_DEBUG_MESSAGING_ENABLED 0
#endif
#ifndef ABCC_CFG_DEBUG_HEXDUMP_MSG_ENABLED
    #define ABCC_CFG_DEBUG_HEXDUMP_MSG_ENABLED 0
#endif
#ifndef ABCC_CFG_DEBUG_CMD_SEQ_ENABLED
    #define ABCC_CFG_DEBUG_CMD_SEQ_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** Enable fetching and clearing of the Fatal log.
**
** This is a debugging aid in case the operating system inside the ABCC has
** trapped a fatal fault. Data about the fault will then be saved to NVS and 
** can be fetched after a restart by sending a GetAttribute for the Fatal log
** instance attribute of the Anybus Object. The default is disabled in order to
** not slow down the startup process.
**
** Default values can be overridden in abcc_driver_config.h
**
** #define ABCC_CFG_DEBUG_GET_FLOG        1 - Enable / 0 - Disable
**
** Enable/disable the inclusion of the extra GetAttribute command during the
** SETUP state, the data in the log is printed via TODO.
**
** #define ABCC_CFG_DEBUG_CLR_FLOG        1 - Enable / 0 - Disable
**
** Enable/disable automatic clearing of the Fatal log, if a log entry exists.
** The main purpose of this is to make multiple occurances of the same fault
** more visible.
**
** WARNING:
** ABCC_CFG_DEBUG_CLR_FLOG should *NOT* be enabled unless the printouts from
** the driver are being logged and saved!
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_DEBUG_GET_FLOG
   #define ABCC_CFG_DEBUG_GET_FLOG 0
   #ifndef ABCC_CFG_DEBUG_CLR_FLOG
      #define ABCC_CFG_DEBUG_CLR_FLOG 0
   #endif
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED        1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** If 1 a size validation is done in ABCC_SeMsgt* and ABCC_GetMsg* macros where
** data is copied to/from message buffers. If the attempted set/get would result
** in a buffer overflow or read out of bounds a fatal event will be logged.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED
    #define ABCC_CFG_MESSAGE_SIZE_CHECK_ENABLED  0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_STARTUP_TIME_MS           ( 1500 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** Sets the startup timeout / delay time.
**
** If the driver has been configured to poll the ABCC interrupt pin
** (ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED) this value will be used as a timeout.
** If the interrupt signal from the ABCC has not been seen within this time it
** is assumed that the ABCC is not present or not responding.
**
** If the driver has not been been configured to poll the ABCC interrupt pin
** the driver will wait for this time to expire before starting the handshake
** operations against the ABCC.
**
** Check the chapter "Initialization and Startup" in the "Anybus CompactCom 40
** Software Design Guide" for more details about the startup stage that this
** delay is applicable to, and which operation modes that supports direct
** startup detection via the ABCCs interrupt signal.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_STARTUP_TIME_MS
    #define ABCC_CFG_STARTUP_TIME_MS ( 1500 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED  1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Assume firmware update on initial communication timeout.
**
** If the application hasn't implemented attribute 5 (ABP_APP_IA_FW_AVAILABLE)
** in the application object to be stored in NVS it may opt to enable this
** define instead. If enabled the driver and the application can assume that
** the ABCC is performing a firmware update in the following cases:
**
** 1. If interrupts are enabled and the ABCC hasn't generated a startup interrupt
**    within 1.5 seconds.
**
** 2. If interrupts are disabled and the initial communication fails after 1.5
**    seconds.
**
** When enabled ABCC_isReadyForCommunication can return ABCC_ASSUME_FW_UPDATE
** if it believes the ABCC is performing a firmware update. It can also be
** assumed that a firmware update is ongoing if the watchdog timeout is triggered
** for the first command sent from the application to the ABCC.
**
** NOTE
** The driver does not support detecting an assumed firmware update for
** parallell operating modes with interrupts disabled.
**
** IMPORTANT
** This should only be enabled after it has been verified that the application
** hardware is working properly (e.g. if the SPI controller hasn't been
** properly configured the initial communication failure is likely casued
** by that).
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
    #define ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED 1
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for measurement of input processing time (used
** for SYNC). This define is used during development by activating it and
** compiling special test versions of the product.
** When ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED is 1 ABCC_HAL_GpioReset() is
** called at the WRPD interrupt. If running in SPI operating mode it is
** instead called when ABCC_SpiRunDriver() has finished sending data to the
** Anybus.
** When ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED is 1 ABCC_GpioSet() needs to be
** called at the Input Capture Point.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
    #define ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
**
** Enable/disable driver support for measurement of output processing time
** (used for SYNC). This define is used during development by activating it and
** compiling special test versions of the product.
** When ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED is 1 ABCC_HAL_GpioSet() is called
** from the RDPDI interrupt.
** When ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED is 1 ABCC_GpioReset() needs to be
** called at the Output Valid Point.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED
    #define ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED 0
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_DRV_CMD_SEQ_ENABLED   1 - Enable / 0 - Disable
**
** Default value below can be overridden in abcc_driver_config.h
** Enables and disables support for the command sequencer.
**
** NOTE: The example application relies on this being enabled and will not
**       build without it.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_DRV_CMD_SEQ_ENABLED
    #define ABCC_CFG_DRV_CMD_SEQ_ENABLED 1
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_MAX_NUM_CMD_SEQ           ( UINT8  1-254 )
**
** Default value below can be overridden in abcc_driver_config.h
** Max number of simultaneous command sequences (see
** abcc_command_sequencer_interface.h). Default is 2.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_MAX_NUM_CMD_SEQ
    #define ABCC_CFG_MAX_NUM_CMD_SEQ ( 2 )
#endif

/*------------------------------------------------------------------------------
** #define ABCC_CFG_CMD_SEQ_MAX_NUM_RETRIES   ( UINT8 0-254 )
**
** Default value below can be overridden in abcc_driver_config.h
**
** When the command sequencer (abcc_command_sequencer_interface.h) sends a
** command, it utilizes ABCC_GetCmdMsgBuffer() to get a buffer resource. The
** command sequencer can handle temporary resource problems and will re-try each
** main loop cycle (ABCC_RunDriver()). When ABCC_CFG_CMD_SEQ_MAX_NUM_RETRIES
** number of retries has been reached, ABCC_EC_OUT_OF_MSG_BUFFERS error will be
** reported.
**
** Default is 0.
**------------------------------------------------------------------------------
*/
#ifndef ABCC_CFG_CMD_SEQ_MAX_NUM_RETRIES
    #define ABCC_CFG_CMD_SEQ_MAX_NUM_RETRIES ( 0 )
#endif

#endif  /* inclusion lock */
