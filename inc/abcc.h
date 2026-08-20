/*******************************************************************************
** Copyright 2013-present HMS Industrial Networks AB.
** Licensed under the MIT License.
********************************************************************************
** File Description:
** ABCC driver API used by the application.
********************************************************************************
** Services provided by ABCC driver:
**    ABCC_GpioReset()               - Clear a digital output for SYNC timing
**                                     measurements.
**    ABCC_GpioSet()                 - Set a digital output for SYNC timing
**                                     measurements.
**    ABCC_HwInit()                  - Initiate local hardware before driver
**                                     startup.
**    ABCC_StartDriver()             - Make the driver ready for use.
**    ABCC_ShutdownDriver()          - Close the driver.
**    ABCC_WaitForFwUpdate()         - Initiate grace period to allow for a
**                                     firmware update.
**    ABCC_isReadyForCommunication() - Check if the driver is ready to
**                                     communicate.
**    ABCC_ISR()                     - ABCC interrupt service routine.
**    ABCC_RunTimerSystem()          - Handle timers for the ABCC driver.
**    ABCC_GetUptimeMs()             - Get ABCC driver uptime.
**    ABCC_HWReset                   - Reset ABCC.
**    ABCC_HWReleaseReset            - Release the reset on ABCC.
**    ABCC_RunDriver()               - Main routine to be called cyclically
**                                     during polling.
**    ABCC_UserInitComplete()        - End of user specific setup sequence.
**    ABCC_SendCmdMsg()              - Send a command message to the ABCC.
**    ABCC_SendRespMsg()             - Send a response message to the ABCC.
**    ABCC_GetCmdQueueSize()         - Get the number of entries left in the
**                                     command queue.
**    ABCC_StartServerRespSegmentationSession() - Send a segmented response
**                                                message to ABCC.
**    ABCC_SendRemapRespMsg()        - Send remap response message.
**    ABCC_GetAppStatus()            - Get current application status.
**    ABCC_SetAppStatus()            - Set the application status.
**    ABCC_GetCmdMsgBuffer()         - Allocate the command message buffer.
**    ABCC_ReturnMsgBuffer()         - Free the message buffer.
**    ABCC_TakeMsgBufferOwnership()  - Take the ownership of the message
**                                     buffer.
**    ABCC_SetMsgFragSize()          - Set the new SPI message fragment size.
**    ABCC_ReadModuleId()            - Read module ID.
**    ABCC_ModuleDetect()            - Detect if a module is present.
**    ABCC_ModCap()                  - Read the module capability.
**    ABCC_LedStatus()               - Read the LED status.
**    ABCC_IsFirstCommandPending()   - Check if first message command to
**                                     the ABCC is awaiting a response.
**    ABCC_AnbState()                - Read the Anybus state.
**    ABCC_IsSupervised()            - Get state of the SUP bit.
**    ABCC_FirmwareVersion()         - Get ABCC firmware version.
**    ABCC_NetworkType()             - Get network type.
**    ABCC_ModuleType()              - Get module type.
**    ABCC_NetFormat()               - Get network endianess.
**    ABCC_ParameterSupport()        - Check if network supports
**                                     parameter access.
**    ABCC_GetOpmode()               - Get operating mode.
**
** Services to be implemented by the user:
**    ABCC_CbfEvent()                - Events received. Called from ISR.
**    ABCC_CbfSyncIsr()              - Callback for sync event.
**    ABCC_CbfUserInitReq()          - User specific setup made by the
**                                     application.
**    ABCC_CbfHandleCommandMessage() - Callback for processing received
**                                     ABCC commands.
**    ABCC_CbfUpdateWriteProcessData() - Callback for updating the
**                                       write process data buffer.
**    ABCC_CbfNewReadPd()            - Process newly received
**                                     read process data.
**    ABCC_CbfWdTimeout()            - Communication lost.
**    ABCC_CbfWdTimeoutRecovered()   - Communication restored.
**    ABCC_CbfAdiMappingReq()        - Retrieve the ADI mapping information.
**    ABCC_CbfDriverError()          - Callback for error notifications.
**    ABCC_CbfAnbStateChanged()      - The Anybus state has changed.
**    ABCC_CbfRemapDone()            - Acknowledge of remap has been sent
**                                     to the ABCC.
** Event related functions:
**    ABCC_TriggerRdPdUpdate()       - Trigger the RdPd read.
**    ABCC_TriggerReceiveMessage()   - Trigger the message read.
**    ABCC_TriggerWrPdUpdate()       - Trigger the WrPd update.
**    ABCC_TriggerAnbStatusUpdate()  - Check for Anybus status change.
**    ABCC_TriggerTransmitMessage()  - Check sending queue.
**
** Message support functions:
**    ABCC_GetAttribute()            - Fill "Get Attribute" message.
**    ABCC_SetByteAttribute()        - Fill "Set Attribute" message.
**    ABCC_SetMsgHeader()            - Set message header fields.
**    ABCC_VerifyMessage()           - Check if E(rror) bit is set.
**    ABCC_GetNewSourceId()          - Return an incrementing source id.
**    ABCC_GetDataTypeSize()         - Return size of ABCC data type.
**    ABCC_GetDataTypeSizeInBits()   - Return size of ABCC data type in bits.
**    ABCC_GetMessageChannelSize()   - Get size of message channel.
**    ABCC_GetMaxMessageSize()       - Get maximum data size of message channel.
********************************************************************************
*/
#ifndef ABCC_H_
#define ABCC_H_

#include "abcc_config.h"
#include "abcc_port.h"
#include "abcc_types.h"
#include "abp.h"
#include "abcc_application_data_interface.h"
#include "abcc_error_codes.h"

#include "abcc_log.h"
#include "abcc_message.h"

/*------------------------------------------------------------------------------
** Bit definitions of ABCC events.
** These bit definitions are used in the bit field
** forwarded to the ABCC_CbfEvent() callback.
**------------------------------------------------------------------------------
*/
#define ABCC_ISR_EVENT_RDPD       0x01
#define ABCC_ISR_EVENT_RDMSG      0x02
#define ABCC_ISR_EVENT_WRMSG      0x04
#define ABCC_ISR_EVENT_STATUS     0x08

/*------------------------------------------------------------------------------
** ABCC firmware version structure.
**------------------------------------------------------------------------------
*/
typedef struct ABCC_FwVersion
{
   UINT8 bMajor;
   UINT8 bMinor;
   UINT8 bBuild;
}
ABCC_FwVersionType;

/*------------------------------------------------------------------------------
** ABCC_CommunicationStateType:
**
** ABCC_NOT_READY_FOR_COMMUNICATION: Nothing is wrong, but it
**                                   is not yet possible to
**                                   communicate with the ABCC.
** ABCC_READY_FOR_COMMUNICATION:     Possible to communicate with ABCC
** ABCC_STARTUP_TIMEOUT:             ABCC module did not start within the
**                                   expected time.
** ABCC_FW_UPDATE:                   The ABCC is assumed to be performing a
**                                   firmware update since it's not responding
**                                   within the expected time.
**------------------------------------------------------------------------------
*/
typedef enum ABCC_CommunicationState
{
   ABCC_NOT_READY_FOR_COMMUNICATION = 0,
   ABCC_READY_FOR_COMMUNICATION = 1,
   ABCC_STARTUP_TIMEOUT = 2,
#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
   ABCC_ASSUME_FW_UPDATE = 3
#endif
}
ABCC_CommunicationStateType;


/*------------------------------------------------------------------------------
** Used for storing the data format of the network.
** NET_UNKNOWN means that the ABCC has not yet responded
** to our command to read the network data format.
**------------------------------------------------------------------------------
*/
typedef enum ABCC_NetFormatType
{
   NET_LITTLEENDIAN,
   NET_BIGENDIAN,
   NET_UNKNOWN
}
ABCC_NetFormatType;

/*------------------------------------------------------------------------------
** Type to indicate if parameter support is available.
**------------------------------------------------------------------------------
*/
typedef enum ABCC_ParameterSupportType
{
   NOT_PARAMETER_SUPPORT,
   PARAMETER_SUPPORT,
   PARAMETER_UNKNOWN
}
ABCC_ParameterSupportType;

/*------------------------------------------------------------------------------
** This function is used to measure sync timings.
** ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED is used when measuring the
** output processing time and ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
** is used to measure the input processing time.
**------------------------------------------------------------------------------
** Arguments:
**       None.
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED || ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED )
EXTFUNC void ABCC_GpioReset( void );
#endif

/*------------------------------------------------------------------------------
** This function is used to measure sync timings.
** ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED is used when measuring the
** output processing time and ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED
** is used to measure the input processing time.
**------------------------------------------------------------------------------
** Arguments:
**       None.
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_SYNC_MEASUREMENT_OP_ENABLED || ABCC_CFG_SYNC_MEASUREMENT_IP_ENABLED )
EXTFUNC void ABCC_GpioSet( void );
#endif

/*------------------------------------------------------------------------------
** This function will initiate the hardware required to communicate with the
** ABCC. It shall be called once during the power up initialization.
** The driver can be restarted without calling this interface again.
**------------------------------------------------------------------------------
** Arguments:
**       None.
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_HwInit( void );

/*------------------------------------------------------------------------------
** This function will initiate the driver, enable interrupt, and set the
** operation mode. If a firmware update is pending, a delay (lMaxStartupTimeMs)
** can be defined describing how long the driver is to wait for the startup
** interrupt. lMaxStartupTimeMs set to Zero (0) makes the driver use the
** ABCC_CFG_STARTUP_TIME_MS time.
**
** When this function has been called, the timer system can be started,
** see ABCC_RunTimerSystem().
**
** Note! This function will NOT release the reset of the ABCC.
** To release reset, ABCC_HwReleaseReset() has to be called by the application.
**------------------------------------------------------------------------------
** Arguments:
**       lMaxStartupTimeMs     - Max startup time for ABCC.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_StartDriver( UINT32 lMaxStartupTimeMs );

/*------------------------------------------------------------------------------
** Stops the driver and puts it into SHUTDOWN state. The ABCC will be reset.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_ShutdownDriver( void );

#if ABCC_CFG_DRV_ASSUME_FW_UPDATE_ENABLED
/*------------------------------------------------------------------------------
** Initiates a grace period to allow the ABCC hardware to complete a
** firmware update.
**
** This function does not block execution. Instead, it configures the
** driver state machine to enter a waiting mode (ABCC_DRV_WAIT_COMMUNICATION_RDY)
** and starts a timer. Use this function only when initial communication
** attempts fail, suggesting the device is currently performing a firmware
** update.
**
**  Call this function if any of the following conditions are met:
**  - ABCC_isReadyForCommunication() returned ABCC_ASSUME_FW_UPDATE
**    (interrupts enabled).
**  - Initial communication was attempted but triggered a watchdog timeout
**    (interrupts disabled).
**
** This routine can only be initiated successfully ONCE per driver start.
** Subsequent calls will return FALSE. This restriction prevents the system
** from entering an infinite wait loop if the firmware update fails
** or is blocked by another issue.
**------------------------------------------------------------------------------
** Arguments:
**       lTimeoutMs - Maximum duration (in milliseconds) to allow for the
**                    firmware update process to finish.
**
** Returns:
**       TRUE       - If the grace period was successfully started (first call).
**       FALSE      - If a grace period has already been started during this
**                    driver instance (subsequent calls).
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_WaitForFwUpdate( UINT32 lTimeoutMs );
#endif

/*------------------------------------------------------------------------------
** This function shall be polled after ABCC_StartDriver() has been executed
** until ABCC_READY_FOR_COMMUNICATION is returned. This indicates that the ABCC
** is ready for communication and the ABCC setup sequence is started.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       ABCC_CommunicationStateType.
**       ( see description of ABCC_CommunicationStateType )
**
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_CommunicationStateType ABCC_isReadyForCommunication( void );

/*------------------------------------------------------------------------------
** Handles and acknowledges ABCC events received via the hardware interrupt
** pin (IRQ_N) on the ABCC application interface. This function must be called
** from within the ABCC interrupt service routine (ISR).
**
** Two configuration masks determine how events are processed.
**
** ABCC_CFG_INT_ENABLE_MASK_X: specifies which events are allowed
** to generate an interrupt.
**
** ABCC_CFG_HANDLE_INT_IN_ISR_MASK: specifies which of those enabled events
** are handled directly by the ISR, versus which are forwarded to the
** application via ABCC_CbfEvent().
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ( *ABCC_ISR )( void );

/*------------------------------------------------------------------------------
** This function is responsible for handling all timers for the ABCC driver.
**
** It is recommended to call this function on a regular basis from a timer
** interrupt. Without this function, no timeout and watchdog functionality will
** work. This function can be called after ABCC_StartDriver() has been called.
**------------------------------------------------------------------------------
** Arguments:
**       iDeltaTimeMs - Milliseconds since last call.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_RunTimerSystem( const INT16 iDeltaTimeMs );

/*------------------------------------------------------------------------------
** This function returns the uptime of the driver.
**
** Note! ABCC_RunTimerSystem() must be called regularly for this function to
** return a proper time.
**
** Note! The uptime counter will reset if the driver is restarted.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Uptime of the driver in milliseconds.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT64 ABCC_GetUptimeMs( void );

/*------------------------------------------------------------------------------
** Shuts down the ABCC driver and triggers a hardware reset by driving the
** reset pin low.
**
** Note! This function only asserts the reset pin. It is the caller's
** responsibility to ensure that the reset pulse is held low for a
** sufficient duration before releasing it.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_HWReset( void );

/*------------------------------------------------------------------------------
** De-asserts the ABCC reset pin, allowing the hardware to exit the reset state
** and begin normal operation.
**
** Note! Ensure that the reset pin was held low for the minimum required
** duration before calling this function.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_HWReleaseReset( void );

/*------------------------------------------------------------------------------
** This function drives the ABCC driver sending and receiving mechanism.
**
** The driver must be ready for communication before this function is called
** (ABCC_isReadyForCommunication() must return ABCC_READY_FOR_COMMUNICATION).
** This function may be called cyclically or be based on events from the ABCC.
** If all events are handled in the interrupt context then there is no need
** to call this function.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_RunDriver( void );

/*------------------------------------------------------------------------------
** This function should be called by the application when the last response from
** the user specific setup has been received. This will end the ABCC setup
** sequence and ABCC_SETUP_COMPLETE will be sent. The user specific setup is a
** part of the ABCC setup sequence and is initiated by the driver by calling
** the ABCC_CbfUserInitReq() function.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_UserInitComplete( void );

/*------------------------------------------------------------------------------
** Sends a command message to the ABCC.
**
** A message buffer needs to be obtained first by e.g. calling
** ABCC_GetCmdMsgBuffer(). Previously received response buffers
** may be reused for this purpose.
**
** The driver uses the SourceId field (part of the message header)
** to route the corresponding response to the correct response handler.
** To obtain a unique SourceId, call ABCC_GetNewSourceId().
**
** Example of using ABCC_CbfMessageReceived() as the response handler:
**
** eResp = ABCC_SendCmdMsg( psMsg, ABCC_CbfMessageReceived );
**------------------------------------------------------------------------------
** Arguments:
**       psCmdMsg     - Pointer to the command message.
**       pnMsgHandler - Pointer to the function that will handle the response
**                      message.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_SendCmdMsg( ABP_MsgType* psCmdMsg,
                                            ABCC_MsgHandlerFuncType pnMsgHandler );

/*------------------------------------------------------------------------------
** Sends a response message to the ABCC.
**
** Note! The buffer from a received command message may be reused for the
** response. Alternatively, a new buffer can be allocated by calling
** ABCC_GetCmdMsgBuffer().
**------------------------------------------------------------------------------
** Arguments:
**       psMsgResp - Pointer to the message.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_SendRespMsg( ABP_MsgType* psMsgResp );

/*------------------------------------------------------------------------------
** Retrieves the number of entries left in the command queue.
**
** Note! When sending a message the returned status must always be checked to
** verify that the message has in fact been sent.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Number of entries left in the command queue.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_GetCmdQueueSize( void );

/*------------------------------------------------------------------------------
** Sends a segmented response message to the ABCC.
**
** This function supports flexible data sourcing strategies:
**
** - Direct buffer: the provided payload buffer (`pxData`) contains the entire message data.
** - Callback stream: a callback function (`pnNext`) supplies data blocks on demand as the
**   transmission progresses.
** - Hybrid: a combination where `pxData` provides the initial block, followed by
**   subsequent blocks via the callback.
**
** Using the callback method is particularly beneficial when data resides in
** scattered memory locations, as it eliminates the need to allocate a
** contiguous buffer for the entire message, thereby saving memory.
**------------------------------------------------------------------------------
** Arguments:
**       psReqMsgHeader - Pointer to request message header.
**       bRspCmdExt0    - Value of Command Extension 0 to use in
**                        response segments. If the command response does not
**                        define specific values, set this to match the
**                        Command Extension 0 from `psReqMsgHeader`.
**       pxData         - Pointer to first message payload block.
**       lSize          - Size (in bytes) of first payload block.
**       pnNext         - Callback function to request the next data block.
**                        NULL if all data is supplied in pxData.
**       pnDone         - Callback function to indicate that the entire message
**                        is sent.
**       pxObject       - User defined object. Forwarded as parameter to the
**                        `pnNext` and `pnDone` callback functions.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_StartServerRespSegmentationSession( const ABP_MsgHeaderType* psReqMsgHeader,
                                                                    UINT8 bRspCmdExt0,
                                                                    const void* pxData,
                                                                    UINT32 lSize,
                                                                    ABCC_SegMsgHandlerNextBlockFuncType pnNext,
                                                                    ABCC_SegMsgHandlerDoneFuncType pnDone,
                                                                    const void* pxObject );

/*------------------------------------------------------------------------------
** Sends a remap response to the ABCC. When the response is sent, the new
** process data sizes will be set and the function ABCC_CbfRemapDone() will be
** called to indicate the change.
**------------------------------------------------------------------------------
** Arguments:
**       psMsgResp       - Pointer to the response message.
**       iNewReadPdSize  - RdPd size when the remap is done.
**       iNewWritePdSize - WrPd size when the remap is done.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_REMAP_SUPPORT_ENABLED
EXTFUNC ABCC_ErrorCodeType ABCC_SendRemapRespMsg( ABP_MsgType* psMsgResp,
                                                  UINT16 iNewReadPdSize,
                                                  const UINT16 iNewWritePdSize );
#endif

/*------------------------------------------------------------------------------
** Get the current application status.
** Note! This information is only supported in SPI and parallel operating mode.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       The application status of the ABCC.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_AppStatusType ABCC_GetAppStatus( void );

/*------------------------------------------------------------------------------
** Sets the current application status.
** Note! This information is only supported in SPI and parallel operating mode.
** When used for other operating modes, the call has no effect.
**------------------------------------------------------------------------------
** Arguments:
**       eAppStatus        - Current application status.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetAppStatus( ABP_AppStatusType eAppStatus  );

/*------------------------------------------------------------------------------
** Retrieves a message buffer. This function MUST be used when allocating
** message buffers. The size of the buffer is controlled by
** ABCC_CFG_MAX_MSG_SIZE.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       ABP_MsgType* - Pointer to the message buffer.
**                      NULL is returned if no resource is available.
**------------------------------------------------------------------------------
*/
EXTFUNC ABP_MsgType* ABCC_GetCmdMsgBuffer( void );

/*------------------------------------------------------------------------------
** Returns the message buffer to the driver's message pool.
** Note! This function may only be used in combination with
** ABCC_TakeMsgBufferOwnership().
**------------------------------------------------------------------------------
** Arguments:
**       ppsBuffer - Pointer to the message buffer that was freed.
**                   The buffer pointer will be set to NULL.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_ReturnMsgBuffer( ABP_MsgType** ppsBuffer );

/*------------------------------------------------------------------------------
** Takes the ownership of the message buffer. The driver will not free this
** buffer when returning from e.g. a response callback. It is the user's
** responsibility to free this buffer when it is not needed anymore by using
** ABCC_ReturnMsgBuffer().
**------------------------------------------------------------------------------
** Arguments:
**       psMsg     - Pointer to the message buffer to take ownership of.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TakeMsgBufferOwnership( ABP_MsgType* psMsg );

#if ABCC_CFG_SPI_DYNAMIC_MSG_FRAG_LEN
/*------------------------------------------------------------------------------
** ABCC_SetMsgFragSize()
** Sets the new message fragment size (used for SPI, only).
**------------------------------------------------------------------------------
** Arguments:
**       iReqMsgFragSize    - Requested message fragment size (in bytes).
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_SetMsgFragSize( const UINT16 iReqMsgFragSize );
#endif // ABCC_CFG_SPI_DYNAMIC_MSG_FRAG_LEN

/*------------------------------------------------------------------------------
** Reads the module ID.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       ModuleId.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_ReadModuleId( void );

/*------------------------------------------------------------------------------
** Detects the presence of an ABCC module by checking the Module Detect pins
** on the host connector.
**
** If the Module Detect pins are physically unconnected, set
** ABCC_CFG_MOD_DETECT_PINS_CONN to 0 in your configuration. Then this
** function will bypass the hardware check and always return TRUE.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       TRUE     - Module detected.
**       FALSE    - No module detected.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_ModuleDetect( void );

/*------------------------------------------------------------------------------
** Reads the module capability. This function is only supported by the ABCC40
** parallel operating mode.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Module capability.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_DRV_PARALLEL_ENABLED
EXTFUNC UINT16 ABCC_ModCap( void );
#endif

/*------------------------------------------------------------------------------
** Reads the LED status. Only supported in SPI and parallel operating mode.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       LED status according to the ABCC Software Design Guide.
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_DRV_PARALLEL_ENABLED || ABCC_CFG_DRV_SPI_ENABLED )
EXTFUNC UINT16 ABCC_LedStatus( void );
#endif

/*------------------------------------------------------------------------------
** Checks whether the initial application-to-ABCC command is currently awaiting
** a response.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       TRUE    - The first command has been sent, but a response
**                 has not yet been received.
**       FALSE   - Either no command has been sent, or the response to the 
**                 first command has already been received.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_IsFirstCommandPending( void );

/*------------------------------------------------------------------------------
** Read the current Anybus state.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Anybus state.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_AnbState( void );

/*------------------------------------------------------------------------------
** Return the current status of the supervision bit.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       TRUE    - Is supervised by another network device.
**       FALSE   - Not supervised.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_IsSupervised( void );

/*------------------------------------------------------------------------------
** Retrieve the ABCC firmware version.
**
** This function will return a valid value after the parameter has been
** retrieved from the ABCC during SETUP state. If called earlier,
** the function will return 0xFF for each firmware version field which
** indicates that the version is unknown.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       ABCC firmware version.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_FwVersionType ABCC_FirmwareVersion( void );

/*------------------------------------------------------------------------------
** Retrieves the network type.
**
** This function will return a valid value after the parameter has been
** retrieved from the ABCC during SETUP state. If called earlier,
** the function will return 0xFFFF which indicates that the network is
** unknown. The different network types can be found in abp.h.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Network type (0xFFFF if the network is unknown).
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_NetworkType( void );

/*------------------------------------------------------------------------------
** Retrieves the module type.
**
** This function will return a valid value after the parameter has been
** retrieved from the ABCC during SETUP state. If called earlier,
** the function will return 0xFFFF which indicates that the module type is
** unknown. The different module types can be found in abp.h.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Module type (0x04XX for ABCC modules).
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_ModuleType( void );

/*------------------------------------------------------------------------------
** Retrieves the network format.
**
** This function will return a valid value after the parameter has been
** retrieved from the ABCC during SETUP state. If called earlier,
** the function will return NET_UNKNOWN.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Network format type (NET_LITTLEENDIAN, NET_BIGENDIAN).
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_NetFormatType ABCC_NetFormat( void );

/*------------------------------------------------------------------------------
** Retrieves the parameter support.
**
** This function will return a valid value after the parameter has been
** retrieved from the ABCC during SETUP state. If called earlier,
** PARAMETER_UNKNOWN will be returned.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       ABCC_ParameterSupportType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ParameterSupportType ABCC_ParameterSupport( void );

/*------------------------------------------------------------------------------
** Reads the ABCC hardware operating mode by calling ABCC_HAL_GetOpmode().
**
** If the operating mode is fixed or otherwise controlled by the application,
** this function may be safely ignored.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       The ABCC40 4 bit operating mode according to abp.h.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_GetOpmode( void );

/*******************************************************************************
** Callback Functions.
** These functions must be implemented by the application. The context of the
** callback may differ depending on the implementation.
**
** If, for example, the read process data is chosen to be interrupt driven and
** the message handling chosen to be polled ( see ABCC_CFG_INT_ENABLE_MASK_X and
** ABCC_CFG_HANDLE_INT_IN_ISR_MASK in inc/abcc_config.h ), ABCC_CbfNewReadPd()
** will be called from interrupt context and ABCC_CbfHandleCommandMessage() will
** be called from the same context as ABCC_RunDriver().
********************************************************************************
*/

#if ABCC_CFG_INT_ENABLED
/*------------------------------------------------------------------------------
** This function is called from ABCC_ISR() when events specified in
** ABCC_CFG_INT_ENABLE_MASK_X have occurred. The function is passed a bit field
** of ABCC_ISR_EVENT_X definitions with the currently active events that
** have not already been handled by the ISR itself. Which interrupt is
** handled by the ISR is defined in the ABCC_CFG_HANDLE_INT_IN_ISR_MASK.
**
** This function is always called from interrupt context.
**------------------------------------------------------------------------------
** Arguments:
**       iEvents - Bit coded ISR events ABCC_ISR_EVENT_X.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CbfEvent( UINT16 iEvents );
#endif

/*------------------------------------------------------------------------------
** If sync is supported, this function will be invoked at the sync event.
** The function is executed in interrupt context. If the separate hardware
** sync pin in the ABCC application interface is used, this function
** shall be called from the interrupt handler. If the ABCC interrupt is used
** to indicate a sync event, the driver will call this function.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_SYNC_ENABLED
EXTFUNC void ABCC_CbfSyncIsr( void );
#endif


/*------------------------------------------------------------------------------
** This function needs to be implemented by the application. The function is
** called to trigger a user specific setup during the ABCC setup state. The
** ABCC driver will remain in SETUP state until ABCC_UserInitComplete()
** is called by the application. If no user specific setup is required,
** ABCC_UserInitComplete() must be called inside this function, otherwise setup
** complete will never be sent.
**
** This function call will be invoked in the same context as the read message
** handling. Regarding callback context, see comment for callback section above.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CbfUserInitReq( void );

/*------------------------------------------------------------------------------
** A message has been received from the ABCC. This is the receive function for
** all received commands from the ABCC. It may also be used as a response
** handler if passed on as an argument to the ABCC_SendCmdMsg() function.
** Regarding callback context, see comment for callback section above.
**------------------------------------------------------------------------------
** Arguments:
**       psReceivedMsg       - Pointer to received message.
**
** Returns:
**       None
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CbfHandleCommandMessage( ABP_MsgType* psReceivedMsg );

/*------------------------------------------------------------------------------
** This function needs to be implemented by the application. The function
** updates the current write process data. The data must be copied into the
** buffer before returning from the function.
** The data will only be sent to the ABCC if the return value is TRUE.
** Regarding callback context, see comment for callback section above.
**------------------------------------------------------------------------------
** Arguments:
**       pxWritePd - Pointer to the process data to be sent.
**
** Returns:
**       TRUE  - If the process data has been changed since last call.
**       FALSE - Process data not changed.
**------------------------------------------------------------------------------
*/
EXTFUNC BOOL ABCC_CbfUpdateWriteProcessData( void* pxWritePd );

/*------------------------------------------------------------------------------
** This function needs to be implemented by the application. The function is
** called when new process data has been received. The process data needs to
** be copied to the application ADI:s before returning from the function. Note
** that the data may not be changed since last time.
** Regarding callback context, see comment for callback section above.
**------------------------------------------------------------------------------
** Arguments:
**       pxReadPd - Pointer to the received process data.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CbfNewReadPd( void* pxReadPd );

/*------------------------------------------------------------------------------
** This function needs to be implemented by the application. The function is
** called when communication with the ABCC module has been lost. The watchdog
** timeout is defined by ABCC_CFG_WD_TIMEOUT_MS.
** Note! No watch functionality is provided for parallel 8/16 bit operation
** mode.
** This function is invoked in the same context as ABCC_RunTimerSystem().
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_DRV_SPI_ENABLED || ABCC_CFG_DRV_SERIAL_ENABLED )
EXTFUNC void ABCC_CbfWdTimeout( void );
#endif

/*------------------------------------------------------------------------------
** This function needs to be implemented by the application. The function
** indicates that we recently had an ABCC watchdog timeout, but now the
** communication is working again.
** This function will be invoked from same context as the receive handling.
** (see comment for callback section above).
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
#if ( ABCC_CFG_DRV_SPI_ENABLED || ABCC_CFG_DRV_SERIAL_ENABLED )
EXTFUNC void ABCC_CbfWdTimeoutRecovered( void );
#endif

/*------------------------------------------------------------------------------
** This function needs to be implemented by the application. The function is
** called when the driver is about to start the automatic process data mapping.
** If no automatic configuration is desired then the pointers are set to NULL.
** Otherwise the pointers are set to point at the structures containing mapping
** information. The mapping structures are defined in
** abcc_application_data_interface.h. This function will be invoked in the same
** context as the read message handling. (See comment for callback section
** above)
**------------------------------------------------------------------------------
** Arguments:
**       ppsAdiEntry   - Pointer to the requested
**                       configuration structure pointer.
**       ppsDefaultMap - Pointer to default mapping table.
**
** Returns:
**       Number of ADI:s in the psAdiEntry table.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_CbfAdiMappingReq( const AD_AdiEntryType** const ppsAdiEntry,
                                      const AD_MapType**      const ppsDefaultMap );

/*------------------------------------------------------------------------------
** This function needs to be implemented by the application.
** The context of the call is depending on where the error has occured.
**
** If the severity level is ABCC_LOG_SEVERITY_FATAL, the driver will get stuck
** in an infinite loop if this function returns.
**------------------------------------------------------------------------------
** Arguments:
**       eSeverity  - Severity of the event (see ABCC_LogSeverityType).
**       iErrorCode - Error code.
**       lAddInfo   - Depending on error, different additional information
**                    can be added.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CbfDriverError( ABCC_LogSeverityType eSeverity,
                                  ABCC_ErrorCodeType  iErrorCode,
                                  UINT32  lAddInfo );

/*------------------------------------------------------------------------------
** This callback is invoked if the Anybus changes state.
** See ABP_AnbStateType in abp.h for more information.
**
** Regarding callback context, see comment for callback section above.
**------------------------------------------------------------------------------
** Arguments:
**       eNewAnbState   - New Anybus state.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_CbfAnbStateChanged( ABP_AnbStateType eNewAnbState );

/*******************************************************************************
** REMAP related functions.
********************************************************************************
*/

/*------------------------------------------------------------------------------
** This callback is invoked when a REMAP response is successfully sent to the
** ABCC.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
#if ABCC_CFG_REMAP_SUPPORT_ENABLED
EXTFUNC void ABCC_CbfRemapDone( void );
#endif

/*******************************************************************************
** Event related functions.
********************************************************************************
*/

/*------------------------------------------------------------------------------
** This function is called to trigger a RdPd read. If the read process data is
** available then a call to the function ABCC_CbfNewReadPd() will be triggered.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TriggerRdPdUpdate( void );

/*------------------------------------------------------------------------------
** This function is called to trigger a message receive read. If a read message
** is available then the corresponding message handler will be called.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TriggerReceiveMessage( void );

/*------------------------------------------------------------------------------
** This function indicates that new process data from the application is
** available and will be sent to the ABCC.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void (*ABCC_TriggerWrPdUpdate)( void );

/*------------------------------------------------------------------------------
** Check if current Anybus status has changed.
** If the status is changed ABCC_CbfAnbStateChanged() will be invoked.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TriggerAnbStatusUpdate( void );

/*------------------------------------------------------------------------------
** Check if there are any messages to send.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_TriggerTransmitMessage( void );

/*******************************************************************************
** Message support functions.
********************************************************************************
*/

/*------------------------------------------------------------------------------
** This function fills an ABCC message with parameters to get an attribute.
**------------------------------------------------------------------------------
** Arguments:
**       psMsg      - Pointer to message buffer.
**       bObject    - Object number.
**       iInstance  - Instance number.
**       bAttribute - Attribute number.
**       bSourceId  - Source identifier
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_GetAttribute( ABP_MsgType* psMsg,
                                UINT8 bObject,
                                UINT16 iInstance,
                                UINT8 bAttribute,
                                UINT8 bSourceId );

/*------------------------------------------------------------------------------
** This function fills an ABCC message with parameters in order to set an
** attribute.
**------------------------------------------------------------------------------
** Arguments:
**       psMsg      - Pointer to message buffer.
**       bObject    - Object number.
**       iInstance  - Instance number.
**       bAttribute - Attribute number.
**       bVal       - Value to set.
**       bSourceId  - Source identifier.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetByteAttribute( ABP_MsgType* psMsg,
                                    UINT8 bObject,
                                    UINT16 iInstance,
                                    UINT8 bAttribute,
                                    UINT8 bVal,
                                    UINT8 bSourceId );

/*------------------------------------------------------------------------------
** This function sets the input arguments to the ABCC message header correctly.
** The data must be copied to the message data buffer separately.
**------------------------------------------------------------------------------
** Arguments:
**       psMsg      - Pointer to message buffer.
**       bObject    - Object number.
**       iInstance  - Instance number.
**       bAttribute - Attribute number.
**       eService   - Message command
**       iDataSize  - Size of the message data in bytes
**       bSourceId  - Source identifier.
**
** Returns:
**       None.
**------------------------------------------------------------------------------
*/
EXTFUNC void ABCC_SetMsgHeader( ABP_MsgType* psMsg,
                                UINT8 bObject,
                                UINT16 iInstance,
                                UINT8 bAttribute,
                                ABP_MsgCmdType eService,
                                UINT16 iDataSize,
                                UINT8 bSourceId );

/*------------------------------------------------------------------------------
** This function verifies an ABCC response message.
**------------------------------------------------------------------------------
** Arguments:
**       psMsg - Pointer to message buffer.
**
** Returns:
**       ABCC_ErrorCodeType.
**------------------------------------------------------------------------------
*/
EXTFUNC ABCC_ErrorCodeType ABCC_VerifyMessage( const ABP_MsgType* psMsg );

/*------------------------------------------------------------------------------
** This function generates a unique source Id for outgoing command messages.
**
** Each call of this function returns a new source identifier that is
** guaranteed to be distinct from IDs currently in use, provided the function
** is invoked whenever a new command message is sent. This approach eliminates
** the need for users to manage fixed source IDs manually, reducing the risk of
** collisions.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       New source Id.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_GetNewSourceId( void );

/*------------------------------------------------------------------------------
** This function returns the size of an ABP data type.
**------------------------------------------------------------------------------
** Arguments:
**       bDataType - Data type number.
**
** Returns:
**       Data type size in bytes.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT8 ABCC_GetDataTypeSize( UINT8 bDataType );

/*------------------------------------------------------------------------------
** This function returns the size of an ABP data type in bits.
**------------------------------------------------------------------------------
** Arguments:
**       bDataType - Data type number.
**
** Returns:
**       Data type size in bits.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_GetDataTypeSizeInBits( UINT8 bDataType );

/*------------------------------------------------------------------------------
** Get size of the message channel in number of octets.
**
** Note that this isn't necessarily the same as the maximum message size as the
** user can configure the driver to use a smaller message size. For that
** purpose, use ABCC_GetMaxMessageSize().
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Size of the message channel
**       (ABP_MAX_MSG_255_DATA_BYTES or ABP_MAX_MSG_DATA_BYTES).
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_GetMessageChannelSize( void );

/*------------------------------------------------------------------------------
** Get maximum data size that can be used in the data portion of a message in
** number of octets.
**
** The useable message channel size is limited by the minimum value of the
** message size supported by the operating mode and ABCC_CFG_MAX_MSG_SIZE.
**------------------------------------------------------------------------------
** Arguments:
**       None.
**
** Returns:
**       Maximum message size.
**------------------------------------------------------------------------------
*/
EXTFUNC UINT16 ABCC_GetMaxMessageSize( void );

#endif  /* inclusion lock */
