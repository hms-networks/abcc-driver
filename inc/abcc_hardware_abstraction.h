/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** Defines system specific interface.
********************************************************************************
** Services:
**    ABCC_HAL_AbccInterruptEnable   - Enable hardware interrupt on GPIO.
**    ABCC_HAL_AbccInterruptDisable  - Disable hardware interrupt on GPIO.
**    ABCC_HAL_SyncInterruptEnable   - Enable sync interrupt on GPIO.
**    ABCC_HAL_SyncInterruptDisable  - Disable sync interrupt on GPIO.
**    ABCC_HAL_HWReset               - Puts Anybus HW into reset.
**    ABCC_HAL_HWReleaseReset        - Pulls Anybus HW out of reset.
**    ABCC_HAL_IsAbccInterruptActive - Poll ABCC interrupt pin.
**    ABCC_HAL_ReadModuleId          - Read Module Identification pins from the
**                                     ABCC interface.
**    ABCC_HAL_SetOpmode             - Sets ABCC Operating Mode pins from the
**                                     ABCC interface.
**    ABCC_HAL_GetOpmode             - Reads ABCC Operating Mode from hardware.
**    ABCC_HAL_ModuleDetect          - Detects if a module is present by reading
**                                     the Module Detection pins.
**    ABCC_HAL_HwInit                - Initialize local hardware before driver
**                                     startup.
**    ABCC_HAL_GpioReset             - Clear a digital output for SYNC timing
**                                     measurements.
**    ABCC_HAL_GpioSet               - Set a digital output for SYNC timing
**                                     measurements.
**    ABCC_HAL_Init                  - Hardware or system dependent
**                                     initialization.
**    ABCC_HAL_Close                 - Close or free all resources allocated in
**                                     ABCC_HAL_Init.
********************************************************************************
*/
#ifndef ABCC_HAL_
#define ABCC_HAL_
#include "abcc_config.h"
#include "abcc_types.h"

/*------------------------------------------------------------------------------
** This function shall enable the interrupt functionality on the
** host controller side for the GPIO connected to the ABCC /IRQ pin
** on the application interface. It is called by the driver
** when the ABCC interrupt shall be enabled.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_INT_ENABLED
EXTFUNC void ABCC_HAL_AbccInterruptEnable( void );
#endif

/*------------------------------------------------------------------------------
** This function shall disable the interrupt functionality on the
** host controller side for the GPIO connected to the ABCC /IRQ pin
** on the application interface. It is called by the driver
** when the ABCC interrupt shall be disabled.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_INT_ENABLED
EXTFUNC void ABCC_HAL_AbccInterruptDisable( void );
#endif

/*------------------------------------------------------------------------------
** This function shall enable the interrupt functionality on the
** host controller side for the GPIO connected to the
** ABCC MI0/SYNC pin on the application interface. It is called
** by the driver when the sync interrupt shall be enabled.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_SYNC_ENABLED && ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED )
EXTFUNC void ABCC_HAL_SyncInterruptEnable( void );
#endif

/*------------------------------------------------------------------------------
** This function shall disable the interrupt functionality on the
** host controller side for the GPIO connected to the
** ABCC MI0/SYNC pin on the application interface. It is called
** by the driver when the sync interrupt shall be disabled.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_SYNC_ENABLED && ABCC_CFG_USE_ABCC_SYNC_SIGNAL_ENABLED )
EXTFUNC void ABCC_HAL_SyncInterruptDisable( void );
#endif

/*------------------------------------------------------------------------------
** Reset ABCC. Set the /Reset pin on the ABCC interface to LOW.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_HAL_HWReset( void );

/*------------------------------------------------------------------------------
** Release reset of ABCC. Sets the /Reset pin on the ABCC interface to HIGH.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_HAL_HWReleaseReset( void );

/*------------------------------------------------------------------------------
** This function shall read the interrupt signal from the CompactCom
** and return TRUE if the interrupt pin is LOW (i.e. interrupt is active)
** and return FALSE if the interrupt pin is HIGH (i.e. the interrupt is
** inactive). It is used to poll the interrupt pin of the CompactCom interface
** if interrupts are not enabled.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       TRUE if an interrupt is active, otherwise FALSE.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_POLL_ABCC_IRQ_PIN_ENABLED
EXTFUNC BOOL ABCC_HAL_IsAbccInterruptActive( void );
#endif

/*------------------------------------------------------------------------------
** This function shall read the Module Identification pins MI0/MI1
** on the CompactCom. If the Module Identification pins are not connected,
** the ABCC_CFG_MODULE_ID_PINS_CONN definition must be set to 0
** in abcc_driver_config.h.
**
** Valid return values:
**    00b (0) Active CompactCom 30-series
**    01b (1) Passive CompactCom
**    10b (2) Active CompactCom 40-series
**    11b (3) Customer specific
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    Module identification value
**
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_MODULE_ID_PINS_CONN
EXTFUNC UINT8 ABCC_HAL_ReadModuleId( void );
#endif

/*------------------------------------------------------------------------------
** This function shall set ABCC Operating Mode pins OM0..3 on the
** ABCC interface in case the operating mode is configurable. Otherwise,
** the definition ABCC_CFG_ABCC_OP_MODE_X shall be set in abcc_driver_config.h
** instead of implementing this function. If it is hardware configurable,
** ABCC_CFG_OP_MODE_HW_CONF must be defined.
**------------------------------------------------------------------------------
** Arguments:
**    bOpMode - 1 SPI
**            - 2 Shift Register ( not supported )
**            - 3-6 Reserved
**            - 7 16 bit parallel
**            - 8 8 bit parallel
**            - 9 Serial 19.2 kbit/s
**            - 10 Serial 57.6 kbit/s
**            - 11 Serial 115.2 kbit/s
**            - 12 Serial 625 kbit/s
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_OP_MODE_SETTABLE
EXTFUNC void ABCC_HAL_SetOpmode( UINT8 bOpMode );
#endif

/*------------------------------------------------------------------------------
** This function shall return the operating mode fetched from an external source
** and needs to be implemented if the operating mode is hardware configurable.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    bOpMode - 1 SPI
**            - 2 Shift Register ( not supported )
**            - 3-6 Reserved
**            - 7 16 bit parallel
**            - 8 8 bit parallel
**            - 9 Serial 19.2 kbit/s
**            - 10 Serial 57.6 kbit/s
**            - 11 Serial 115.2 kbit/s
**            - 12 Serial 625 kbit/s
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_OP_MODE_GETTABLE
EXTFUNC UINT8 ABCC_HAL_GetOpmode( void );
#endif

/*------------------------------------------------------------------------------
** This function shall detect if a module is present by reading the
** Module Detection pins MD on the ABCC interface. If the Module Detection pins
** are not connected, the ABCC_CFG_MOD_DETECT_PINS_CONN definition must be set
** to 0 in abcc_driver_config.h instead of implementing this function.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    TRUE  - Module is detected.
**    FALSE - Module is not detected.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_MOD_DETECT_PINS_CONN
EXTFUNC BOOL ABCC_HAL_ModuleDetect( void );
#endif

/*------------------------------------------------------------------------------
** This function is called by the driver from the ABCC_HwInit() interface.
** If there is any hardware or system dependent initialization required
** to be done at the power up initialization, it shall be done here.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    TRUE  - Initialization succeeded.
**    FALSE - Initialization failed.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_HAL_HwInit( void );

/*------------------------------------------------------------------------------
** This function is used to measure sync timings.
** ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED is used when measuring the output
** processing time and ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED is used to measure
** the input processing time. It should reset an output signal that can be
** measured to ascertain aforementioned sync times.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED || ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED )
EXTFUNC void ABCC_HAL_GpioReset( void );
#endif

/*------------------------------------------------------------------------------
** This function is used to measure sync timings.
** ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED is used when measuring the output
** processing time and ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED is used to measure
** the input processing time. It should set an output signal that can be
** measured to ascertain aforementioned sync times.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    None
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED || ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED )
EXTFUNC void ABCC_HAL_GpioSet( void );
#endif

/*------------------------------------------------------------------------------
** This function is called by the driver at the beginning of ABCC_StartDriver().
** If there is any hardware specific tasks required to be done every time the
** driver starts, it shall be done here. Note that ABCC_StartDriver() will also
** be called during restart of the driver.
**------------------------------------------------------------------------------
** Arguments:
**    None
**
** Returns:
**    TRUE  - Initialization succeeded.
**    FALSE - Initialization failed.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_HAL_Init( void );

/*------------------------------------------------------------------------------
** This function is called by the driver at the end of ABCC_ShutDown().
** Any hardware specific tasks that are required to be done every time
** the driver is stopped, shall be done here. Note that the driver
** could be started again by calling ABCC_StartDriver().
**------------------------------------------------------------------------------
** Arguments:
**    None
** Returns:
**    None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_HAL_Close( void );

#endif  /* inclusion lock */
