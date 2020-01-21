// No doxygen 'group' header because this file is included by both user & kernel implementations

//------------------------------------------------------------------------------
// Copyright (c) 2013 Cohda Wireless Pty Ltd
//------------------------------------------------------------------------------

#ifndef __LINUX__COHDA__LLC__LLC_API_H__
#define __LINUX__COHDA__LLC__LLC_API_H__

//------------------------------------------------------------------------------
// Included headers
//------------------------------------------------------------------------------
#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/string.h> // for memcpy

// Limits of integral types
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#else
#include <stdint.h>
#include <string.h> // For memcpy()
#endif

//------------------------------------------------------------------------------
// Macros & Constants
//------------------------------------------------------------------------------

/// Number of WISPA Tx Gain settings available on WISPA-ITS
#define WISPA_ITS_GAIN_CNT (32)
/// Number of WISPA Tx Gain settings available on WISPA-TC2
#define WISPA_TC2_GAIN_CNT (9)

/// Set the number of WISPA Tx gain settings for the TC2 (ITS doesn't need 32)
#define WISPA_TX_GAIN_CNT (WISPA_TC2_GAIN_CNT)

/// MKx magic value
#define MKX_API_MAGIC (0xC0DA)

/// The size of the Address Matching Table
#define AMS_TABLE_COUNT 8

/// The number of channels that Rx RSSI Calibration is performed on (168-184)
#define RSSI_CAL_CHANNEL_CNT (((184 - 168)/2) + 1)

/// Rename of inline
#ifndef INLINE
#define INLINE __inline
#endif

//------------------------------------------------------------------------------
// Type definitions
//------------------------------------------------------------------------------

/**
 * @section llc_remote Remote LLC Module
 *
 * LLCRemote implements a mechanism to allow the MKx Modem to be used as a
 * remote MAC to a Linux Host machine.
 *
 * @verbatim
                       Source provided -> +----------------+
   +-----------------+                    |  llc debug app |
   |   Stack / Apps  |                    +-------*--------+
   +---*-------*-----+                            |                  User Space
 ------|-------|----------------------------------|---------------------------
       | ioctl | socket(s)                        | socket         Kernel Space
     +-*-------*---+                              |
     |  simTD API  | (optional binary)            |
     +------*------+                              |
            | API (MKx_* functions)               |
     +------*------+                              |
     |  LLCRemote  +------------------------------+
     |             |<- Source code provided
     +--*-------*--+
        | USB   | Ethernet (MKxIF_* structures)
    +---*-+ +---*----+
    | USB | | TCP/IP |
    +-----+ +--------+                                         Client side (uP)
 -----------------------------------------------------------------------------
 +---------------------+ +---------------------+              Server Side (SDR)
 |        WMAC         | |   C2X Security      |
 +---------------------+ +---------------------+
 |     802.11p MAC     |
 +---------------------+
 |     802.11p PHY     |
 +---------------------+
 @endverbatim
 *
 * @subsection llc_remote_design LLCRemote MAC Design
 *
 * The LLCRemote module communicates between Server and Client via two USB bulk
 * endpoints or two UDP sockets.
 */

/// Types for the LLCRemote message transfers
typedef enum
{
  /// A null packet (to indicate no message data)
  MKXIF_NONE        = 0,
  /// A transmit packet (message data is @ref tMKxTxPacket)
  MKXIF_TXPACKET    = 1,
  /// A received packet (message data is @ref tMKxRxPacket)
  MKXIF_RXPACKET    = 2,
  /// New UTC Time (message data is @ref tMKxSetTSF)
  MKXIF_SET_TSF     = 3,
  /// Transmitted packet event (message data is @ref tMKxTxEventData)
  MKXIF_TXEVENT     = 4,
  /// Radio config for Radio A (message data is @ref tMKxRadioConfig)
  MKXIF_RADIOACFG   = 5,
  /// Radio config for Radio B (message data is @ref tMKxRadioConfig)
  MKXIF_RADIOBCFG   = 6,
  /// Radio A statistics (message data is @ref tMKxRadioStats)
  MKXIF_RADIOASTATS = 7,
  /// Radio B statistics (message data is @ref tMKxRadioStats)
  MKXIF_RADIOBSTATS = 8,
  /// Flush a single queue or all queues (message data is @ref tMKxFlushQueue)
  MKXIF_FLUSHQ      = 9,
  /// A generic debug container.
  MKXIF_DEBUG       = 10,
  /// C2XSEC message (message data is @ref tMKxC2XSec)
  MKXIF_C2XSEC      = 11,
  /// Temperature config message (message data is @ref tMKxTempConfig)
  MKXIF_TEMPCFG     = 12,
  /// Temperature measurement message (message data is @ref tMKxTemp)
  MKXIF_TEMP        = 13,
  /// Power detector config message (message data is @ref tMKxPowerDetConfig)
  MKXIF_POWERDETCFG = 14,
  /// Read the current UTC Time (message data is @ref tMKxGetTSF)
  MKXIF_GET_TSF     = 15,
  /// RSSI Cal config message (message data is @ref tMKxRSSICalConfig)
  MKXIF_RSSICALCFG  = 16,
  /// Invalid message type, used for array dimensioning
  MKXIF_COUNT       = 17,
  /// Invalid message type, used for bounds checking
  MKXIF_MAX = MKXIF_COUNT - 1
} eMKxIFMsgType;
/// @copydoc eMKxIFMsgType
typedef uint16_t tMKxIFMsgType;

/// LLCRemote message (LLC managed header)
typedef struct MKxIFMsg
{
  /// Message type
  tMKxIFMsgType Type;
  /// Length of the message, including the header itself
  uint16_t Len;
  /// Sequence number
  uint16_t Seq;
  /// Return value
  int16_t Ret;
  /// Message data
  uint8_t Data[];
} __attribute__ ((packed)) tMKxIFMsg;

/**
 * @section llc_api MKx API
 *
 * This section provides an overview of the MKx WAVE MAC usage, in order to clarify its
 * functionality.
 *
 * @subsection general_usage General usage in a WSM/Proprietary Protocol System (user-space implementation)
 *
 * Typical usage would be:
 * - Load the MKx LLC kernel module
 * - Open the MKx interface using the MKx_Init() function.
 * - Enable notifications by setting the pMKx->API.Callbacks.NotifInd() callback
 * - Enable packet reception by setting the pMKx->API.Callbacks.RxAlloc()
 *    pMKx->API.Callbacks.RxInd() and callbacks
 * - Enable transmit confirmations by setting the pMKx->API.Callbacks.TxCnf() callback
 * - Set the Radio A (CCH & SCH-A) parameters using the MKx_Config() function.
 * - Set the Radio B (CCH & SCH-B) parameters using the MKx_Config() function.
 * - Packets can be transmitted using the TxReq() function and the success/failure
 *    of the frame is indicated via the TxCnf() callback
 * - Packets received on either radio will be allocated with the RxAlloc()
 *    callback and delivered via the RxInd() callback
 * - When done, the MKx interface can be gracefully closed with MKx_Exit()
 *
 * @subsection channel_measurements Channel Measurements
 * - Statistics are updates are notified via the NotifInd() callback every 50ms
 * - Counters can be read directly from the MKx handle or using the MKx_GetStats() helper function
 *   - Channel busy ratio is provided in the per-channel statistics.
 *     This is the ratio of channel busy (virtual carrier sense is asserted)
 *     time to channel idle time.
 *     It is an 8-bit unsigned value, where 100% channel utilisation is indicated by a value of 255.
 *   - Average idle period power is provided in the per-channel statistics.
 *     This is the average RSSI recorded whilst the channel isn't busy (virtual carrier sense is not asserted).
 *
 * @subsection dual_channel_operation  Dual channel operation
 * When operating in a dual-radio configuration, it is possible to configure the MAC channel
 * access function to consider the state of the other radio channel before making transmit
 * decisions. The WMAC allows the following configuration options for the channel access
 * function when operating in a dual-radio system.
 *
 * - No consideration of other radio. In this case, the radio will transmit without regard to
 * the state of the other radio channel. The system will behave effectively as two
 * independent radio systems.
 * - Tx inhibit. In this mode, the MAC will prevent this radio from transmitting while the
 * other radio is transmitting. In this case, when the other radio is transmitting, the local
 * radio behaves as if the local channel is busy.
 * - Rx inhibit. In this mode, the MAC will prevent this radio from transmitting while the
 * other radio is actively receiving a frame. In this case, when the other radio is receiving,
 * the local radio behaves as if the local channel is busy. This prevents transmissions from
 * this radio from corrupting the reception of a frame on the other radio, tuned to a nearby
 * radio channel (in particular when shared or co-located antennas are in use).
 * - TxRx inhibit. In this mode, the MAC will prevent this radio from transmitting while
 * the other radio is either transmitting or receiving.
 *
 * In all cases, the transmission inhibit occurs at the MAC channel-access level, so packets will
 * not be dropped when transmission is inhibited, they will simply be deferred.
 *
 */

/// Forward declaration of the MKx Handle
struct MKx;

/// MKx MLME interface return codes
typedef enum
{
  /// Success return code
  MKXSTATUS_SUCCESS = 0,
  // -1 to -255 reserved for @c errno values (see <errno.h>)
  /// Unspecified failure return code (catch-all)
  MKXSTATUS_FAILURE_INTERNAL_ERROR              = -256,
  /// Failure due to invalid MKx Handle
  MKXSTATUS_FAILURE_INVALID_HANDLE              = -257,
  /// Failure due to invalid RadioConfig
  MKXSTATUS_FAILURE_INVALID_CONFIG              = -258,
  /// Failure due to invalid parameter setting
  MKXSTATUS_FAILURE_INVALID_PARAM               = -260,
  /// Auto-cal requested when radio is running auto-cal
  MKXSTATUS_FAILURE_AUTOCAL_REJECT_SIMULTANEOUS = -261,
  /// Auto-cal requested but radio is not configured
  MKXSTATUS_FAILURE_AUTOCAL_REJECT_UNCONFIGURED = -262,
  /// Radio config failed (likely to be a hardware fault) maximum
  MKXSTATUS_FAILURE_RADIOCONFIG_MAX             = -513,
  /// Radio config failed (likely to be a hardware fault) minimum
  MKXSTATUS_FAILURE_RADIOCONFIG_MIN             = -768,
  /// Packet failed by exceeding Time To Live
  MKXSTATUS_TX_FAIL_TTL                         = -769,
  /// Packet failed by exceeding Max Retry count
  MKXSTATUS_TX_FAIL_RETRIES                     = -770,
  /// Packet failed because queue was full
  MKXSTATUS_TX_FAIL_QUEUEFULL                   = -771,
  /// Packet failed because requested radio is not present
  MKXSTATUS_TX_FAIL_RADIO_NOT_PRESENT           = -772,
  /// Packet failed because the frame was malformed
  MKXSTATUS_TX_FAIL_MALFORMED                   = -773,
  /// Packet failed because requested radio is not present
  MKXSTATUS_TX_FAIL_RADIO_UNCONFIGURED          = -774,
  /// Packet failed because it was too long
  MKXSTATUS_TX_FAIL_PACKET_TOO_LONG             = -775,
  /// Security message failed due to security accelerator not being present
  MKXSTATUS_SECURITY_ACCELERATOR_NOT_PRESENT    = -1024,
  /// Security message failed due to security FIFO being full
  MKXSTATUS_SECURITY_FIFO_FULL                  = -1025,
  // Reserved
  MKXSTATUS_RESERVED                            = 0xC0DA
} eMKxStatus;
/// @copydoc eMKxStatus
typedef int tMKxStatus;

/// MKx Radio
typedef enum
{
  /// Selection of Radio A of the MKX
  MKX_RADIO_A = 0,
  /// Selection of Radio B of the MKX
  MKX_RADIO_B = 1,
  // ...
  /// Used for array dimensioning
  MKX_RADIO_COUNT = 2,
  /// Used for bounds checking
  MKX_RADIO_MAX = MKX_RADIO_COUNT - 1
} eMKxRadio;
/// @copydoc eMKxRadio
typedef int8_t tMKxRadio;

/// MKx Channel
typedef enum
{
  /// Indicates Channel Config 0 is selected
  MKX_CHANNEL_0 = 0,
  /// Indicates Channel Config 1 is selected
  MKX_CHANNEL_1 = 1,
  // ...
  /// Used for array dimensioning
  MKX_CHANNEL_COUNT = 2,
  /// Used for bounds checking
  MKX_CHANNEL_MAX = MKX_CHANNEL_COUNT - 1

} eMKxChannel;
/// @copydoc eMKxChannel
typedef int8_t tMKxChannel;

/// MKx Bandwidth
typedef enum
{
  /// Indicates 10 MHz
  MKXBW_10MHz = 10,
  /// Indicates 20 MHz
  MKXBW_20MHz = 20
} eMKxBandwidth;
/// @copydoc eMKxBandwidth
typedef int8_t tMKxBandwidth;

/// The channel's centre frequency [MHz]
typedef uint16_t tMKxChannelFreq;

/**
 * MKx dual radio transmit control
 * Controls transmit behaviour according to activity on the
 * other radio (inactive in single radio configurations)
 */
typedef enum
{
  /// Do not constrain transmissions
  MKX_TXC_NONE,
  /// Prevent transmissions when other radio is transmitting
  MKX_TXC_TX,
  /// Prevent transmissions when other radio is receiving
  MKX_TXC_RX,
  /// Prevent transmissions when other radio is transmitting or receiving
  MKX_TXC_TXRX,
  /// Default behaviour
  MKX_TXC_DEFAULT = MKX_TXC_TX
} eMKxDualTxControl;
/// @copydoc eMKxDualTxControl
typedef uint8_t tMKxDualTxControl;

/**
 * MKx Modulation and Coding scheme
 */
typedef enum
{
  /// Rate 1/2 BPSK
  MKXMCS_R12BPSK = 0xB,
  /// Rate 3/4 BPSK
  MKXMCS_R34BPSK = 0xF,
  /// Rate 1/2 QPSK
  MKXMCS_R12QPSK = 0xA,
  /// Rate 3/4 QPSK
  MKXMCS_R34QPSK = 0xE,
  /// Rate 1/2 16QAM
  MKXMCS_R12QAM16 = 0x9,
  /// Rate 3/4 16QAM
  MKXMCS_R34QAM16 = 0xD,
  /// Rate 2/3 64QAM
  MKXMCS_R23QAM64 = 0x8,
  /// Rate 3/4 64QAM
  MKXMCS_R34QAM64 = 0xC,
  /// Use default data rate
  MKXMCS_DEFAULT = 0x0,
  /// Use transmit rate control (currently unused)
  MKXMCS_TRC = 0x1
} eMKxMCS;
/// @copydoc eMKxMCS
typedef uint8_t tMKxMCS;

/// Tx & Rx power of frame, in 0.5dBm units.
typedef enum
{
  /// Selects the PHY maximum transmit power
  MKX_POWER_TX_MAX      = INT16_MAX,
  /// Selects the PHY minimum transmit power
  MKX_POWER_TX_MIN      = INT16_MIN,
  /// Selects the PHY default transmit power level
  MKX_POWER_TX_DEFAULT  = MKX_POWER_TX_MIN + 1,
  /// Indicates when the Rx power reported is invalid as antenna is disabled
  MKX_POWER_RX_DISABLED = INT16_MIN
} eMKxPower;
/// @copydoc eMKxPower
typedef int16_t tMKxPower;

/**
 * MKx Antenna Selection
 */
typedef enum
{
  /// Transmit packet on neither antenna (dummy transmit)
  MKX_ANT_NONE = 0,
  /// Transmit packet on antenna 1
  MKX_ANT_1 = 1,
  /// Transmit packet on antenna 2 (when available).
  MKX_ANT_2 = 2,
  /// Transmit packet on both antenna
  MKX_ANT_1AND2 = MKX_ANT_1 | MKX_ANT_2,
  /// Selects the default (ChanConfig) transmit antenna setting
  MKX_ANT_DEFAULT = 4
} eMKxAntenna;
/// Number of antennas that are present for the MKX
#define MKX_ANT_COUNT 2
/// @copydoc eMKxAntenna
typedef uint8_t tMKxAntenna;

/**
 * MKx TSF
 * Indicates absolute 802.11 MAC time in microseconds
 */
typedef uint64_t tMKxTSF;

/**
 * MKx Rate sets
 * Each bit indicates if corresponding MCS rate is supported
 */
typedef enum
{
  /// Rate 1/2 BPSK rate mask
  MKX_RATE12BPSK_MASK = 0x01,
  /// Rate 3/4 BPSK rate mask
  MKX_RATE34BPSK_MASK = 0x02,
  /// Rate 1/2 QPSK rate mask
  MKX_RATE12QPSK_MASK = 0x04,
  /// Rate 3/4 QPSK rate mask
  MKX_RATE34QPSK_MASK = 0x08,
  /// Rate 1/2 16QAM rate mask
  MKX_RATE12QAM16_MASK = 0x10,
  /// Rate 2/3 64QAM rate mask
  MKX_RATE23QAM64_MASK = 0x20,
  /// Rate 3/4 16QAM rate mask
  MKX_RATE34QAM16_MASK = 0x40
} eMKxRate;
/// @copydoc eMKxRate
typedef uint8_t tMKxRate;

/**
 * MKx 802.11 service class specification.
 */
typedef enum
{
  /// Packet should be (was) transmitted using normal ACK policy
  MKX_QOS_ACK = 0x00,
  /// Packet should be (was) transmitted without Acknowledgement.
  MKX_QOS_NOACK = 0x01
} eMKxService;
/// @copydoc eMKxService
typedef uint8_t tMKxService;

/**
 * MKx Additional MAC layer tx control
 * These bits signal to the radio that special Tx behavior is required
 */
typedef enum
{
  /// Do not modify the sequence number field
  MKX_DISABLE_MAC_HEADER_UPDATES_SEQCTRL    = 0x01,
  /// Do not modify the duration ID field
  MKX_DISABLE_MAC_HEADER_UPDATES_DURATIONID = 0x02,
  /// Do not modify the Ack Policy field
  MKX_DISABLE_MAC_HEADER_UPDATES_ACKPOLICY  = 0x04,
  /// Do not modify the Retry field and set Max retries to zero
  MKX_DISABLE_MAC_HEADER_UPDATES_RETRY      = 0x08,
  /// Force the use of RTS/CTS with this packet
  MKX_FORCE_RTSCTS                          = 0x10
} eMKxTxCtrlFlags;
/// @copydoc eMKxTxCtrlFlags
typedef uint8_t tMKxTxCtrlFlags;

/**
 * MKx Transmit Descriptor. This header is used to control how the data packet
 * is transmitted by the LLC. This is the header used on all transmitted
 * packets.
 */
typedef struct MKxTxPacketData
{
  /// Indicate the radio that should be used (Radio A or Radio B)
  tMKxRadio RadioID;
  /// Indicate the channel config for the selected radio
  tMKxChannel ChannelID;
  /// Indicate the antennas upon which packet should be transmitted
  /// (may specify default)
  tMKxAntenna TxAntenna;
  /// Indicate the MCS to be used (may specify default)
  tMKxMCS MCS;
  /// Indicate the power to be used (may specify default)
  tMKxPower TxPower;
  /// Additional control over the transmitter behaviour (must be set to zero
  /// for normal operation)
  tMKxTxCtrlFlags TxCtrlFlags;
  // Reserved (for 64 bit alignment and internal processing)
  uint8_t Reserved0;
  /// Indicate the expiry time as an absolute MAC time in microseconds
  /// (0 means never)
  tMKxTSF Expiry;
  /// Length of the frame (802.11 Header + Body, not including FCS)
  uint16_t TxFrameLength;
  // Reserved (for 32 bit alignment and internal processing)
  uint16_t Reserved1;
  /// Frame (802.11 Header + Body, not including FCS)
  uint8_t TxFrame[];
} __attribute__((__packed__)) tMKxTxPacketData;

/**
 * MKx Transmit Packet format.
 */
typedef struct MKxTxPacket
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Tx Packet control and frame data
  tMKxTxPacketData TxPacketData;
} __attribute__((__packed__)) tMKxTxPacket;

/**
 * Transmit Event Data. This is the structure of the data field for
 * MKxIFMsg messages of type TxEvent.
 */
typedef struct MKxTxEventData
{
  /// Transmit status (transmitted/retired), @ref eMKxStatus
  int16_t TxStatus;
  /// 802.11 MAC sequence number of the transmitted frame
  uint16_t MACSequenceNumber;
  /// The TSF when the packet was transmitted or retired
  tMKxTSF TxTime;
  /// Delay (VDSP ticks) between end of Tx Data frame and start of Rx Ack frame
  /// Note VDSP Clock runs at 300MHz
  uint32_t AckResponseDelay;
  /// Delay (VDSP ticks) between end of Tx RTS frame and start of Rx CTS frame
  /// Note VDSP Clock runs at 300MHz
  uint32_t CTSResponseDelay;
  /// Time (us) between the arrival of the packet at the MAC and its Tx
  uint32_t MACDwellTime;
  /// Short packet retry counter
  uint8_t NumShortRetries;
  /// Long packet retry counter
  uint8_t NumLongRetries;
  /// Destination address of the transmitted frame
  uint8_t DestAddress[6];
} __attribute__((__packed__)) tMKxTxEventData;

/**
 * MKx Transmit Event format.
 */
typedef struct MKxTxEvent
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Tx Event Data
  tMKxTxEventData TxEventData;
} __attribute__((__packed__)) tMKxTxEvent;

/**
 * MKx Meta Data type - contains per frame receive meta-data
 *
 * The frequency offset estimate is the measured offset with respect to the
 * local oscillator frequency, which is accurate to +/- 10ppm.
 */
typedef struct MKxRxMeta
{
  /// Estimated frequency offset of rx frame in Hz (with respect to local freq)
  int32_t FreqOffset;
} __attribute__((__packed__)) tMKxRxMeta;

/**
 * MKx Receive descriptor and frame.
 * This header is used to pass receive packet meta-information from
 * the LLC to upper-layers. This header is prepended to all received packets.
 * If only a single receive  power measure is required, then simply take the
 * maximum power of Antenna A and B.
 */
typedef struct MKxRxPacketData
{
  /// Indicate the radio that should be used (Radio A or Radio B)
  tMKxRadio RadioID;
  /// Indicate the channel config for the selected radio
  tMKxChannel ChannelID;
  /// Indicate the data rate that was used
  tMKxMCS MCS;
  // Indicates FCS passed for received frame (1=Pass, 0=Fail)
  uint8_t FCSPass;
  /// Indicate the received power on Antenna A
  tMKxPower RxPowerA;
  /// Indicate the received power on Antenna B
  tMKxPower RxPowerB;
  /// Indicate the receiver noise on Antenna A
  tMKxPower RxNoiseA;
  /// Indicate the receiver noise on Antenna B
  tMKxPower RxNoiseB;
  /// Per Frame Receive Meta Data
  tMKxRxMeta RxMeta;
  /// MAC Rx Timestamp, local MAC TSF time at which packet was received
  tMKxTSF RxTSF;
  /// Length of the Frame (802.11 Header + Body, including FCS)
  uint16_t RxFrameLength;
  // Reserved (for 32 bit alignment)
  uint16_t Reserved1;
  /// Frame (802.11 Header + Body, including FCS)
  uint8_t RxFrame[];
} __attribute__((__packed__)) tMKxRxPacketData;

/**
 * MKx receive packet format.
 */
typedef struct MKxRxPacket
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Rx Packet control and frame data
  tMKxRxPacketData RxPacketData;
} __attribute__((__packed__)) tMKxRxPacket;

/// MKx SetTSF command type
typedef enum MKxSetTSFCmd {
  /// UTC time provided corresponds to the UTC time at 1PPS event
  UTC_AT_1PPS = 1,
  /// UTC time provided corresponds to the TSF timestamp provided
  UTC_AT_TSF = 2
} eMKxSetTSFCmd;
/// @copydoc eMKxSetTSFCmd
typedef uint8_t tMKxSetTSFCmd;

/**
 * Set TSF data
 * Data for setting the time synchronisation function (TSF) to UTC time.
 * The TSF can be set to
 * - UTC time at GPS 1PPS (obtained from NMEA data)
 * - UTC time at TSF Timestamp (through the use of timing advertisements)
 */
 typedef struct MKxSetTSFData
{
  /// Selects the UTC to with 1PPS or TSF Timestamp
  tMKxSetTSFCmd Cmd;
  // Reserved (for 32 bit alignment)
  uint8_t Reserved0;
  uint8_t Reserved1;
  uint8_t Reserved2;
  /// UTC Time at either previous 1PPS event or at TSF Timestamp
  tMKxTSF UTC;
  /// TSF Timestamp at UTC
  tMKxTSF TSF;
} __attribute__((__packed__)) tMKxSetTSFData;

/*
 * MKx Set TSF message format
 */
typedef struct MKxSetTSF
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// SetTSF Message Data
  tMKxSetTSFData SetTSFData;
} __attribute__((__packed__)) tMKxSetTSF;

/*
 * MKx Get TSF message format
 */
typedef struct MKxGetTSF
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// GetTSF Message Data (current TSF value)
  tMKxTSF TSF;
} __attribute__((__packed__)) tMKxGetTSF;

/// Transmit queues (in priority order, where lowest is highest priority)
typedef enum
{
  MKX_TXQ_NON_QOS = 0, ///< Non QoS (for WSAs etc.)
  MKX_TXQ_AC_VO = 1,   ///< Voice
  MKX_TXQ_AC_VI = 2,   ///< Video
  MKX_TXQ_AC_BE = 3,   ///< Best effort
  MKX_TXQ_AC_BK = 4,   ///< Background
  /// For array dimensioning
  MKX_TXQ_COUNT,
  /// For bounds checking
  MKX_TXQ_MAX = MKX_TXQ_COUNT - 1
} eMKxTxQueue;
/// @copydoc eMKxTxQueue
typedef uint8_t tMKxTxQueue;

/*
 * MKx FlushQueue message format
 */
typedef struct MKxFlushQueue
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// Indicate the radio that should be used (Radio A or Radio B)
  tMKxRadio RadioID;
  /// Indicate the channel for the selected radio
  tMKxChannel ChannelID;
  /// Queue selection to be flush (MKX_TXQ_COUNT for all)
  tMKxTxQueue TxQueue;
} __attribute__((__packed__)) tMKxFlushQueue;

/**
 * MKx Rate Set. See @ref eMKxRate for bitmask for enabled rates
 */
typedef uint8_t tMKxRateSet[8];

/// Address matching control bits
/// (bit 0) = ResponseEnable
/// (bit 1) = BufferEnableCtrl
/// (bit 2) = BufferEnableBadFCS
/// (bit 3) = LastEntry
/// (bit 4) = BufferDuplicate
typedef enum
{
  /// ResponseEnable -- Respond with ACK when a DATA frame is matched.
  MKX_ADDRMATCH_RESPONSE_ENABLE = (1 << 0),
  /// BufferEnableCtrl -- Buffer control frames that match.
  MKX_ADDRMATCH_ENABLE_CTRL = (1 << 1),
  /// BufferEnableBadFCS -- Buffer frames even if FCS error was detected.
  MKX_ADDRMATCH_ENABLE_BAD_FCS = (1 << 2),
  /// LastEntry -- Indicates this is the last entry in the table.
  MKX_ADDRMATCH_LAST_ENTRY = (1 << 3),
  /// BufferDuplicate -- Buffer duplicate frames
  MKX_ADDRMATCH_DUPLICATE = (1 << 4)
} eMKxAddressMatchingCtrl;

/**
 * @brief Receive frame address matching structure
 *
 * General operation of the MKx on receive frame:
 * - bitwise AND of 'Mask' and the incoming frame's DA (DA not modified)
 * - equality check between 'Addr' and the masked DA
 * - If equal: continue
 *  - If 'ResponseEnable' is set: Send 'ACK'
 *  - If 'BufferEnableCtrl' is set: Copy into internal buffer
 *         & deliver via RxInd() if FCS check passes
 *  - If 'BufferEnableBadFCS' is set: Deliver via RxInd() even if FCS check fails
 *
 * To receive broadcast frames:
 * - Addr = 0XFFFFFFFFFFFFULL
 * - Mask = 0XFFFFFFFFFFFFULL
 * - MatchCtrl = 0x0000
 * To receive anonymous IEEE1609 heartbeat (multicast) frames:
 * - Addr = 0X000000000000ULL
 * - Mask = 0XFFFFFFFFFFFFULL
 * - MatchCtrl = 0x0000
 * To receive valid unicast frames for 01:23:45:67:89:AB (our MAC address)
 * - Addr = 0XAB8967452301ULL
 * - Mask = 0XFFFFFFFFFFFFULL
 * - MatchCtrl = 0x0001
 * To monitor the channel in promiscuous mode (including failed FCS frames,
 * and all duplicates):
 * - Addr = 0X000000000000ULL
 * - Mask = 0X000000000000ULL
 * - MatchCtrl = 0x0016
 */
typedef struct MKxAddressMatching
{
#ifndef LLC_NO_BITFIELDS
  /// 48 bit mask to apply to DA before comparing with Addr field
  uint64_t Mask:48;
  uint64_t :0; // Align to 64 bit boundary

  /// 48 bit MAC address to match after masking
  uint64_t Addr:48;
  /// Bitmask see @ref eMKxAddressMatchingCtrl
  uint64_t MatchCtrl:8;
  uint64_t :0; // Align to 64 bit boundary
#else
  uint8_t Mask[6];
  uint16_t Reserved0;
  uint8_t Addr[6];
  uint16_t MatchCtrl;
#endif
} tMKxAddressMatching;

/// MKx transmit queue configuration
typedef struct MKxTxQConfig
{
  /// Arbitration inter-frame-spacing (values of 0 to 16)
  uint8_t AIFS;
  /// Contention window min
  uint8_t CWMIN;
  /// Contention window max
  uint16_t CWMAX;
  /// TXOP duration limit [ms]
  uint16_t TXOP;
} __attribute__((__packed__)) tMKxTxQConfig;


/// PHY specific config
typedef struct MKxChanConfigPHY
{
  /// Channel centre frequency (in MHz) that should be used e.g. 5000 + (5*172)
  tMKxChannelFreq ChannelFreq;
  /// Indicate if channel is 10 MHz or 20 MHz
  tMKxBandwidth Bandwidth;
  /// Default Transmit antenna configuration
  /// (can be overridden in @ref tMKxTxPacket)
  /// Antenna selection used for transmission of ACK/CTS
  tMKxAntenna TxAntenna;
  /// Receive antenna configuration
  tMKxAntenna RxAntenna;
  /// Indicate the default data rate that should be used
  tMKxMCS DefaultMCS;
  /// Indicate the default transmit power that should be used
  /// Power setting used for Transmission of ACK/CTS
  tMKxPower DefaultTxPower;
} __attribute__((__packed__)) tMKxChanConfigPHY;

/// MAC specific config
typedef struct MKxChanConfigMAC
{
  /// Dual Radio transmit control (inactive in single radio configurations)
  tMKxDualTxControl DualTxControl;
  /// The RSSI power detection threshold for carrier sense [dBm]
  int8_t CSThreshold;
  /// Slot time/duration, per 802.11-2012
  uint16_t SlotTime;
  /// Distributed interframe space, per 802.11-2012
  uint16_t DIFSTime;
  /// Short interframe space, per 802.11-2012
  uint16_t SIFSTime;
  /// Duration to wait after an erroneously received frame,
  /// before beginning slot periods
  /// @note this should be set to EIFS - DIFS
  uint16_t EIFSTime;
  /// Per queue configuration
  tMKxTxQConfig TxQueue[MKX_TXQ_COUNT];
  /// Address matching filters: DA, broadcast, unicast & multicast
  tMKxAddressMatching AMSTable[AMS_TABLE_COUNT];
  /// Retry limit for short unicast transmissions
  uint16_t ShortRetryLimit;
  /// Retry limit for long unicast transmissions
  uint16_t LongRetryLimit;
  /// Threshold at which RTS/CTS is used for unicast packets (bytes).
  uint16_t RTSCTSThreshold;
} __attribute__((__packed__)) tMKxChanConfigMAC;

/// LLC (WMAC) specific config
typedef struct MKxChanConfigLLC
{
  /// Duration of this channel interval, in microseconds. Zero means forever.
  /// Also sets the interval between stats messages sent.
  uint32_t IntervalDuration;
  /// Duration of guard interval upon entering this channel, in microseconds
  uint32_t GuardDuration;
} __attribute__((__packed__)) tMKxChanConfigLLC;

/**
 * MKx channel configuration
 */
typedef struct MKxChanConfig
{
  /// PHY specific config
  struct MKxChanConfigPHY PHY;
  /// MAC specific config
  struct MKxChanConfigMAC MAC;
  /// LLC (WMAC) specific config
  struct MKxChanConfigLLC LLC;
} __attribute__((__packed__)) tMKxChanConfig;

typedef enum
{
  /// Radio is off
  MKX_MODE_OFF = 0,
  /// Radio is using channel config 0 configuration only
  MKX_MODE_CHANNEL_0 = 1,
  /// Radio is enabled to use channel config 1 configuration only
  MKX_MODE_CHANNEL_1 = 2,
  /// Radio is enabled to channel switch between config 0 & config 1 configs
  MKX_MODE_SWITCHED = 3
} eRadioMode;
/// @copydoc eRadioMode
typedef uint16_t tMKxRadioMode;

/// MKx per radio configuration
typedef struct MKxRadioConfigData
{
  /// Operation mode of the radio
  tMKxRadioMode Mode;
  /// Reserved (for 32 bit alignment)
  uint16_t Reserved0;
  /// Channel Configurations for this radio
  tMKxChanConfig ChanConfig[MKX_CHANNEL_COUNT];
} __attribute__((__packed__)) tMKxRadioConfigData;

/**
 * MKx configuration message format.
 */
typedef struct MKxRadioConfig
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Radio configuration data
  tMKxRadioConfigData RadioConfigData;
} __attribute__((__packed__)) tMKxRadioConfig;

/// Tx Queue stats counters
typedef struct MKxTxQueueStats
{
  /// Number of frames submitted via MKx_TxReq() to the current queue
  uint32_t    TxReqCount;
  /// Number of frames successfully transmitted (excluding retries)
  uint32_t    TxCnfCount;
  /// Number of frames unsuccessfully transmitted (excluding retries)
  uint32_t    TxErrCount;
  /// Number of packets transmitted on the channel (including retries)
  uint32_t    TxValid;
  /// Number of internal collisions experienced
  uint32_t    InternalCollisions;
  /// Number of packets in the queue
  uint32_t    TxPending;
} tMKxTxQueueStats;

/// Channel stats counters
typedef struct MKxChannelStats
{
  /// Number of frames submitted via MKx_TxReq()
  uint32_t    TxReq;
  /// Number of Tx frames discarded by the MKx
  uint32_t    TxFail;
  /// Number of frames successfully transmitted (excluding retries)
  uint32_t    TxCnf;
  /// Number of frames unsuccessfully transmitted (excluding retries)
  uint32_t    TxErr;
  /// Number of packets transmitted on the channel (including retries)
  uint32_t    TxValid;
  /// Number of frames delivered via MKx_RxInd()
  uint32_t    RxInd;
  /// Number of Rx frames discarded by the MKx
  uint32_t    RxFail;
  /// Total number of duplicate (unicast) packets received on the channel
  uint32_t    RxDup;
  /// Per queue statistics
  tMKxTxQueueStats TxQueue[MKX_TXQ_COUNT];
  /// Medium busy time.  Number of us that the medium is declared busy over
  /// the last measurement period.  Medium is declared busy during Tx, Rx and
  /// Nav events.
  uint32_t MediumBusyTime;
  /// Proportion of time which the radio is considered busy over the last
  /// measurement period. (255 = 100%)
  uint8_t ChannelBusyRatio;
  /// Average idle period power [dBm]
  int8_t AverageIdlePower;
} tMKxChannelStats;

/// Radio level stats counters
typedef struct MKxRadioStatsData
{
  /// Per channel context statistics
  tMKxChannelStats Chan[MKX_CHANNEL_COUNT];
  /// TSF timer value at the end of the last measurement period [us]
  tMKxTSF TSF;
} __attribute__((__packed__)) tMKxRadioStatsData;

/**
 * MKx Radio stats format
 */
typedef struct MKxRadioStats
{
  /// Interface Message Header
  tMKxIFMsg Hdr;
  /// Radio Stats Data
  tMKxRadioStatsData RadioStatsData;
} __attribute__((__packed__)) tMKxRadioStats;

/**
 * C2X Security API
 *
 * See SAF5x00 Security user manual v0.5 (24 Jan 2017)
 */

/// Security command SW return codes
typedef enum MKxC2XSecSW
{
  /// The function completed successfully.
  MKXC2XSEC_SW_NO_ERROR                 = 0x9000,
  /// The CLA value is not supported by the applet.
  MKXC2XSEC_SW_CLA_NOT_SUPPORTED        = 0x6E00,
  /// The INS value is not supported by the applet.
  MKXC2XSEC_SW_INS_NOT_SUPPORTED        = 0x6D00,
  /// The value of parameter P1 or P2 is invalid.
  MKXC2XSEC_SW_INCORRECT_P1P2           = 0x6A86,
  /// The value of parameter Lc or Le is invalid.
  MKXC2XSEC_SW_WRONG_LENGTH             = 0x6700,
  /// The data field of the command contains wrong data.
  MKXC2XSEC_SW_WRONG_DATA               = 0x6A80,
  /// No more memory available.
  MKXC2XSEC_SW_FILE_FULL                = 0x6A84,
  /// Internal execution error and the result is that the NVRAM is unchanged.
  MKXC2XSEC_SW_EXE_ERR_NVRAM_UNCHANGED  = 0x6400,
  /// Internal execution error and the result is that the NVRAM is changed.
  MKXC2XSEC_SW_EXE_ERR_NVRAM_CHANGED    = 0x6500,
  /// An exception occurred of which no precise diagnosis is available. This
  /// error code should also be used in case security intrusion is detected.
  MKXC2XSEC_SW_NO_PRECISE_DIAGNOSIS     = 0x6F00,
  /// Command not supported (e.g. by this device variant or configuration).
  MKXC2XSEC_SW_CONDITIONS_NOT_SATISFIED = 0x6985
} eMKxC2XSecSW;

/// Security verification results
typedef enum MKxC2XSecVerifyResult
{
  MKXC2XSEC_VERIFY_SUCCESS = 0u,
  MKXC2XSEC_VERIFY_FAILURE = 1u
} eMKxC2XSecVerifyResult;
/// Security verification result
typedef uint8_t tMKxC2XSecVerRes;

/// ECDSA Security Curve Identifiers
typedef enum MKxC2XSecCurveId
{
  MKXC2XSEC_CID_NIST256          = 0u, /// NIST curve param
  MKXC2XSEC_CID_BRAINPOOL_P256R1 = 1u, /// Brainpool curve P256r1 param
  MKXC2XSEC_CID_BRAINPOOL_P256T1 = 2u, /// Brainpool curve P256t1 param

  MKXC2XSEC_CID_COUNT
} eMKxC2XSecCurveId;
/// Public key curve id
typedef uint8_t tMKxC2XSecCId;

/// Public key signature
typedef uint8_t tMKxC2XSecSig[32];
/// Public key hash
typedef uint8_t tMKxC2XSecHash[32];
/// Public key coordinate
typedef uint8_t tMKxC2XSecCoord[32];

/// Public key pair
typedef struct MKxC2XSecPair
{
  /// X coordinate for elliptical signature
  tMKxC2XSecCoord X;
  /// Y coordinate for elliptical signature
  tMKxC2XSecCoord Y;
}__attribute__((__packed__)) tMKxC2XSecPair;

/// Compressed public key
typedef struct MKxC2XSecCompPubKey
{
  /// X coordinate for elliptical signature
  tMKxC2XSecCoord X;
  /// The least significant _bit_ of the Y coordinate
  uint8_t Ybit;
}__attribute__((__packed__)) tMKxC2XSecCompPubKey;

/// Verify Signature of Hash
/// See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 4.1.1
typedef struct MKxC2XSecVSoH
{
  /// Public key of the entity that created the signature
  tMKxC2XSecPair PubKey;
  /// Hash protected by signature
  tMKxC2XSecHash E;
  /// The signature over the hash, to be verified (R)
  tMKxC2XSecSig R;
  /// The signature over the hash, to be verified (S)
  tMKxC2XSecSig S;
  /// Identifies ECC curve used to verify the ECC public key
  tMKxC2XSecCId CurveId;
}__attribute__((__packed__)) tMKxC2XSecVSoH;

/// Decompress Public Key
/// See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 4.1.2
typedef struct MKxC2XSecDPK
{
  /// ECC Public key to decompressed
  tMKxC2XSecCompPubKey MKxC2XSecCompPubKey;
  /// Identifies ECC curve used to decompress ECC public key
  tMKxC2XSecCId CurveId;
}__attribute__((__packed__)) tMKxC2XSecDPK;

/// Recontruct ECC Public Key
/// See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 4.1.3
typedef struct MKxC2XSecREPK
{
  /// Hash value used in derivation of ECC public key
  tMKxC2XSecHash hvij;
  /// Public reconstruction value used in derivation of ECC public key
  tMKxC2XSecPair RVij;
  /// Public key of Pseudonym CA used in derivation of the ECC public key
  tMKxC2XSecPair Spca;
  /// Type of ECC curve used to reconstruct the ECC public key
  tMKxC2XSecCId CurveId;
}__attribute__((__packed__)) tMKxC2XSecREPK;

/// Security command payload structure
typedef union MKxC2XSecCmdPL
{
  /// Verify Signature of Hash command message
  tMKxC2XSecVSoH VerifySigOfHash;
  /// Decompress Public Key command message
  tMKxC2XSecDPK DecompEccPubKey;
  /// Reconstruct ECC Public Key command message
  tMKxC2XSecREPK ReconEccPubKey;
  /// Raw payload data
  uint8_t Data[0];
}__attribute__((__packed__)) tMKxC2XSecCmdPL;

/**
 * C2X security command message
 * +------+------+------+------+------+---...---+------+
 * | CLA  | INS  | USN0 | USN1 |  LC  | Payload |  LE  |
 * +------+------+------+------+------+---...---+------+
 *    1      1       1      1      1      'LC'      1
 *
 *    See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 3 Fig 5
 *
 * The Payload is a variable size however LE is always present. To illustrate
 * this the payload is represented by a zero length array in the structure
 * below. Although the offset can be accessed by .Payload[0] it occupies no
 * space in the structure. This is useful when determining if the length of the
 * packet is correct or the maximum size required to store a command.
 * For example:
 *
 *   uint16_t VSoHRepLength = sizeof(tMKxC2XSecCmd) + sizeof(tMKxC2XSecVSoH);
 *   uint16_t MaxRspLength = MKXC2XSEC_CMD_MAX_SIZE;
 *
 * LC provides the byte array index to determine LE.
 * For example:
 *
 *   tMKxC2XSecCmd *pSecCmd = (tMKxC2XSecCmd *)pData;
 *   uint8_t LC = pSecCmd->LC;
 *   uint8_t LE = pSecCmd->Payload[0].data[LC];
 *
 */
typedef struct MKxC2XSecCmd
{
  /// CLA - Class Byte.
  uint8_t CLA;
  /// INS - Instruction Byte.
  uint8_t INS;
  /// USN0,USN1 - 2 Byte USN field.P1 and P2 fields are reused for USN
  uint8_t USN[2];
  /// LC - Length in Bytes of the payload. (0x01 ... 0xFF)
  uint8_t LC;
  /// Payload - Command Payload.
  tMKxC2XSecCmdPL Payload[0];
  /// LE - Expected Length of response. (0x01 ... 0xFF)
  uint8_t LE;
} __attribute__((__packed__)) tMKxC2XSecCmd;

/// Wrapper to ensure tMKxC2XSecCmd.Payload is 64-bit aligned (payload aligned),
/// Data[0] and End[0] provide useful offsets into the structure without adding
/// to the overall size.
typedef struct MKxC2XSecCmdPA
{
  /// Padding bytes to ensure tMKxC2XSecCmd.Payload is 64-bit aligned
  uint8_t Padding[3];
  /// Command message data
  uint8_t Data[0];
  /// Message structure, now with aligned .Payload member
  tMKxC2XSecCmd Cmd;
  /// End of structure
  uint8_t End[0];
} __attribute__((__packed__)) tMKxC2XSecCmdPA;

/// Result of Verify Signature of Hash command
/// See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 4.1.1
typedef struct MKxC2XSecResRsp
{
  /// The result of the verification
  tMKxC2XSecVerRes VerResult;
}__attribute__((__packed__)) tMKxC2XSecResRsp;

/// Public Key response structure
/// See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 4.1.2 and 4.1.3
typedef struct MKxC2XSecPubKeyRsp
{
  /// Public Key response
  tMKxC2XSecPair PubKey;
}__attribute__((__packed__)) tMKxC2XSecPubKeyRsp;

/// Security command response payload structure
typedef union MKxC2XSecRspPL
{
  /// Verify Signature of Hash command response
  tMKxC2XSecResRsp ResRsp;
  /// Decompress Public Key and Reconstruct Public Key commands response
  tMKxC2XSecPubKeyRsp PubKeyRsp;
  /// Raw payload data
  uint8_t Data[0];
}__attribute__((__packed__)) tMKxC2XSecRspPL;

/**
 * C2X security response message
 * +------+------+---...---+------+------+
 * | USN0 | USN1 | Payload |  SW1 |  SW2 |
 * +------+------+---...---+------+------+
 *    1      1      'LE'       1      1
 *
 *    See SAF5x00 Security user manual v0.5 (24 Jan 2017) Sec 3 Fig 6
 *
 * When SW indicated no error the Payload is a variable size defined by LE,
 * otherwise the Payload is empty.
 *
 */
typedef struct MKxC2XSecRsp
{
  /// USN0, USN1
  uint8_t USN[2];
  /// Payload - Response Payload.
  tMKxC2XSecRspPL Payload[0];
  /// SW1,SW2 indicate the response code.
  /// Refer to C2X Security API document for valid error codes
  uint8_t SW[2];
} __attribute__((__packed__)) tMKxC2XSecRsp;

/// Wrapper to ensure tMKxC2XSecRsp.Payload is 64-bit aligned (payload aligned),
/// Data[0] and End[0] provide useful offsets into the structure without adding
/// to the overall size.
typedef struct MKxC2XSecRspPA
{
  /// Padding bytes to ensure tMKxC2XSecRsp.Payload is 64-bit aligned
  uint8_t Padding[6];
  /// Raw response message data
  uint8_t Data[0];
  /// Message structure with aligned .Payload member
  tMKxC2XSecRsp Rsp;
  /// End of structure
  uint8_t End[0];
} __attribute__((__packed__)) tMKxC2XSecRspPA;

/**
 * C2X security message
 */
typedef union MKxC2XSecAPDU
{
  /// Command APDU (payload aligned)
  tMKxC2XSecCmdPA C;
  /// Response APDU  (payload aligned)
  tMKxC2XSecRspPA R;
} __attribute__((__packed__)) tMKxC2XSecAPDU;

/**
 * C2X security request/indication
 */
typedef struct MKxC2XSec
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// C2X Security API APDU
  tMKxC2XSecAPDU APDU;
} __attribute__((__packed__)) tMKxC2XSec;

/// Maximum size of a Security Command
#define MKXC2XSEC_CMD_MAX_SIZE \
  (sizeof(tMKxC2XSecCmd) + sizeof(tMKxC2XSecCmdPL))

/// Maximum size of a payload aligned Security Command
#define MKXC2XSEC_CMD_PA_MAX_SIZE \
  (sizeof(tMKxC2XSecCmdPA) + sizeof(tMKxC2XSecCmdPL))

/// Maximum size of a Security Command Response
#define MKXC2XSEC_RSP_MAX_SIZE \
  (sizeof(tMKxC2XSecRsp) + sizeof(tMKxC2XSecRspPL))

/// Maximum size of a payload aligned Security Command Response
#define MKXC2XSEC_RSP_PA_MAX_SIZE \
  (sizeof(tMKxC2XSecRspPA) + sizeof(tMKxC2XSecRspPL))

/// Maximum size of a payload aligned Security Command or Response
#define MKXC2XSEC_MAX_SIZE (sizeof(tMKxIFMsg) \
  + ((MKXC2XSEC_CMD_PA_MAX_SIZE > MKXC2XSEC_RSP_PA_MAX_SIZE) ? \
     MKXC2XSEC_CMD_PA_MAX_SIZE : MKXC2XSEC_RSP_PA_MAX_SIZE))

/// MKx temperature sensor source
typedef enum MKxTempSource {
  /// No I2C sensors present, temperatures set via the MKXIF_TEMP command
  MKX_TEMP_SOURCE_MANUAL = 0,
  /// Single I2C sensor, acting for both PAAnt1 and PAAnt2 temperature settings
  MKX_TEMP_SOURCE_PA1_ONLY = 1,
  /// Dual I2C sensors, one for each PA (PAAnt1, PAAnt2)
  MKX_TEMP_SOURCE_BOTH = 2
} eMKxTempSource;
/// @copydoc eMKxTempSource
typedef uint8_t tMKxTempSource;

/**
 * RSSI Calibration Config
 * Used to configure the RSSI compensation behaviour
 *
 * RSSI compensation can be enabled or disabled.
 * The configuration can be supplied or read back
 *
 */

/// MKx RSSI calibration operating mode
typedef enum MKxRSSICalMode {
  /// No compensation
  MKX_RSSI_CAL_OFF = 0,
  /// RSSI compensation enabled
  MKX_RSSI_CAL_ON = 1,
  /// Get RSSI compensation
  MKX_RSSI_CAL_GET = 2
} eMKxRSSICalMode;
/// @copydoc eMKxRSSICalMode
typedef uint8_t tMKxRSSICalMode;

/**
 * RSSI Calibration config data
 * Config data for the RSSI compensation, includes
 * - mode switch, so the RSSI compensation can be disabled/changed.
 * - RSSI calibration data, where measured RSSI is adjusted by the
 *   compensation formula: FreqDependentIntercept + Slope*CurrentTemperature
 */
typedef struct MKxRSSICalConfigData
{
  /// Operating mode
  tMKxRSSICalMode RSSICalMode;

  /// Reserved for 32 bit alignment
  uint8_t Reserved0;
  uint8_t Reserved1;
  uint8_t Reserved2;

  /// Slope for temperature compensation (S15Q16 format)
  int32_t Slope;

  /// Frequency dependent zero temperature intercept (S15Q16 format)
  int32_t FreqDependentIntercept[RSSI_CAL_CHANNEL_CNT];
}__attribute__((__packed__)) tMKxRSSICalConfigData;

/**
 * RSSI Cal config request/indication (see @ref tMKxRSSICalConfigData)
 */
typedef struct MKxRSSICalConfig
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// RSSI Calibration Config Data
  tMKxRSSICalConfigData RSSICalConfigData;
} __attribute__((__packed__)) tMKxRSSICalConfig;

/**
 * Temperature Config Data
 * Used to configure the temperature sensing, temperature power compensation and
 * Tx power limiting at maximum temperature.
 * For temperature sensing configuration, SensorSource =
 *   MKX_TEMP_SOURCE_MANUAL, temperatures set via the MKXIF_TEMP message
 *   MKX_TEMP_SOURCE_PA1_ONLY, temperatures set via single I2C temp sensor
 *                             (connected to I2CAddrPAAnt1)
 *   MKX_TEMP_SOURCE_BOTH, temperatures set via PAAnt1 & PAAnt2 I2C temp sensors
 * Note
 * I2CAddrPAAnt1 value is don't care when SensorSource = MKX_TEMP_SOURCE_MANUAL
 * I2CAddrPAAnt2 value is don't care when SensorSource = MKX_TEMP_SOURCE_MANUAL
 * or MKX_TEMP_SOURCE_PA1_ONLY
 *
 * Power compensation = PowerCalTempFactor * CurrentTemp + PowerCalTempOffset
 * Power compensation is in dB and is added to each requested power in order to
 * correct for change in output powers due to temperature.
 *
 * The S15Q16 fixed point format (used for both PowerCalTempFactor and
 * PowerCalTempOffset parameters) scales the values such that
 * 1.0 = 2^16 = 65536.
 *
 * The power limit parameters are used limit the temperature compensated TxPower
 * when the measured temperature > PowerLimitMaxTemp.
 */
typedef struct MKxTempConfigData
{
  /// Number of I2C temperature sensors connected to the SAF5x00
  tMKxTempSource SensorSource;
  /// I2C Address for the PA Ant1 I2C temperature sensor
  uint8_t I2CAddrPAAnt1;
  /// I2C Address for the PA Ant2 I2C temperature sensor
  uint8_t I2CAddrPAAnt2;
  /// Power calibration factor for temperature compensation (S15Q16 format)
  int32_t PowerCalTempFactorAnt1;
  /// Power calibration offset (dB) for temperature compensation (S15Q16 format)
  int32_t PowerCalTempOffsetAnt1;
  /// Power calibration factor for temperature compensation (S15Q16 format)
  int32_t PowerCalTempFactorAnt2;
  /// Power calibration offset (dB) for temperature compensation (S15Q16 format)
  int32_t PowerCalTempOffsetAnt2;
  /// Maximum temp (degC) for when tx power is limited to PowerLimitMaxPower
  int16_t PowerLimitMaxTemp;
  /// Set maximum power (in 0.5 dBm units) when maximum temperature is reached
  tMKxPower PowerLimitMaxPower;
} __attribute__((__packed__)) tMKxTempConfigData;

/**
 * Temperature Config request/indication (see @ref tMKxTempConfigData)
 */
typedef struct MKxTempConfig
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// Temperature Config Data
  tMKxTempConfigData TempConfigData;
} __attribute__((__packed__)) tMKxTempConfig;


/**
 * Temperature measurement data
 */
typedef struct MKxTempData
{
  /// Temperature setting in degrees C for PA Ant1, when no I2C sensors present
  int8_t TempPAAnt1;
  /// Temperature setting in degrees C for PA Ant2, when no I2C sensors present
  int8_t TempPAAnt2;
} __attribute__((__packed__)) tMKxTempData;

/**
 * Temperature measurement message
 * Used to indicate (or manually set) the two temperatures used for tx power
 * compensation.
 * @note This message is only accepted by the SAF5x00 when SensorSource in
 * MKxTempConfig is set to MKX_TEMP_SOURCE_MANUAL.
 */
typedef struct MKxTemp
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// Temperature measurement data
  tMKxTempData TempData;
} __attribute__((__packed__)) tMKxTemp;

/// MKx power calibration mode selection
typedef enum MKxPowerCalMode {
  /// No tx power calibration
  MKX_POWER_CAL_OFF = 0,
  /// Use only the temperature for tx power calibration
  MKX_POWER_CAL_TEMP_ONLY = 1,
  /// Use the tx power detector for calibration.  Note this mode uses
  /// temperature based power calibration until 1st valid TxPowerDet read.
  MKX_POWER_CAL_POWERDET = 2
} eMKxPowerCalMode;
/// @copydoc eMKxPowerCalMode
typedef uint8_t tMKxPowerCalMode;

/**
 * Data structure that defines a calibration point and line in the Tx power
 * detector value vs actual transmit power relationship.
 * The rate parameter is in dBm per PowerDetValue
 */
typedef struct MKxPowerDetCalPoint
{
  /// Power detector calibration point, power detector value
  int32_t PowerDet;
  /// Power detector calibration point, power in dBm value (S15Q16 format)
  int32_t PowerConstant;
  /// Power detector calibration rate dBm/PowerDet from defined point (S15Q16)
  int32_t PowerRate;
} __attribute__((__packed__)) tMKxPowerDetCalPoint;

/**
 * Data structure that defines a temperature offset adjustment line in the Tx
 * power offset vs temperature relationship.
 * The rate parameter is in dBm per degree Celsius.
 */
typedef struct MKxPowerDetTempCalPoint
{
  /// Power detector calibration temperature
  /// Temperature where the power detector curves have been calculated at
  int32_t CalTemp;
  /// Power offset rate dBm/deg C (S15Q16)
  int32_t TempOffsetRate;
} __attribute__((__packed__)) tMKxPowerDetTempCalPoint;

/**
 * Power detector calibration data
 * Used to configure the tx power detector calibration by specifying the
 * TxPowerDet relationship to actual/measured power.  Specification is in the
 * form of two lines (specified as two points and two lines).
 * All TxPowerDet values below the 1st calibration point are ignored.
 */
 typedef struct MKxPowerDetCalData
{
  /// Power detector 1st calibration point/line (Ant1)
  tMKxPowerDetCalPoint CalPoint1Ant1;
  /// Power detector 2nd calibration point/line (Ant1)
  tMKxPowerDetCalPoint CalPoint2Ant1;
  /// Power detector 1st calibration point/line (Ant2)
  tMKxPowerDetCalPoint CalPoint1Ant2;
  /// Power detector 2nd calibration point/line (Ant2)
  tMKxPowerDetCalPoint CalPoint2Ant2;
  /// Power detector temperature calibration point/line
  tMKxPowerDetTempCalPoint TempCalPoint;
} __attribute__((__packed__)) tMKxPowerDetCalData;

/**
 * Power detector config data
 * Config data for the power detector, includes
 * - mode switch, so the power calibration can be disabled/changed.
 * - power detector calibration data for both antennas
 * - power detector read period
 */
 typedef struct MKxPowerDetConfigData
{
  /// Selects the power calibration mode (Temperature or TxPowerDet)
  tMKxPowerCalMode PowerCalMode;
  // Reserved (for 32 bit alignment)
  uint8_t Reserved0;
  uint8_t Reserved1;
  uint8_t Reserved2;
  /// Power detector calibration data for both antennas
  tMKxPowerDetCalData Cal;
  /// Period between enabling the tx power detector reads, in us
  uint32_t PowerDetReadPeriod;
} __attribute__((__packed__)) tMKxPowerDetConfigData;

/**
 * Power detector config request/indication (see @ref tMKxPowerDetConfigData)
 */
typedef struct MKxPowerDetConfig
{
  /// Interface Message Header (reserved area for LLC usage)
  tMKxIFMsg Hdr;
  /// Power detector Config Data
  tMKxPowerDetConfigData PowerDetConfigData;
} __attribute__((__packed__)) tMKxPowerDetConfig;

//------------------------------------------------------------------------------
// Function Types
//------------------------------------------------------------------------------

/**
 * @brief Request the configuration of a particular radio channel
 * @param pMKx MKx handle
 * @param Radio the selected radio
 * @param pConfig Pointer to the new configuration to apply
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * @code
 * // Get the current/default config
 * tMKxRadioConfig Cfg = {0,};
 * memcpy(&Cfg, &(pMKx->Config.Radio[MKX_RADIO_A]), sizeof(Cfg));
 * // Update the values that we want to change
 * Cfg.Mode = MKX_MODE_SWITCHED
 * Cfg.Chan[MKX_CHANNEL_0].PHY.ChannelFreq = 5000 + (5 * 178)
 * Cfg.Chan[MKX_CHANNEL_1].PHY.ChannelFreq = 5000 + (5 * 182)
 * ...
 * // Apply the configuration
 * Res = MKx_Config(pMKx, MKX_RADIO_A, &Cfg);
 * @endcode
 */
typedef tMKxStatus (*fMKx_Config) (struct MKx *pMKx,
                                   tMKxRadio Radio,
                                   tMKxRadioConfig *pConfig);

/**
 * @brief Request the transmission of an 802.11 frame
 * @param pMKx MKx handle
 * @param pTxPkt The packet pointer (including tx header)
 * @param pPriv Pointer to provide when invoking the @ref fMKx_TxCnf callback
 * @return MKXSTATUS_SUCCESS if the transmit request was accepted
 *
 * @note The following MAC header parameters are manipulated by the MAC layer:
 * - Duration field is overwritten (for data frames only)
 * - Sequence number is incremented and overwritten (note MAC layer maintains
 * sequence numbers for each individual QoS queue. This is performed for all
 * frame types except for control frame types).
 * In addition, unicast frames are determined by the destination address top
 * byte LSB being set to 0.
 * - For unicast QoS frames, the ACK policy is determined by the QoSControl
 * setting.
 * - For unicast non-QoS frames, the ACK policy is always true.
 * - For multi-cast QoS frames, the ACK policy of the QoSControl field is
 * cleared by the MAC layer to be  always false
 *
 * @note When applicable, RTS, CTS, and Ack control frames are created by the
 * MAC layer and are not made available via the LLC interface.
 *
 * @note The buffer must lie in DMA accessible memory and there is usually some
 * relation between pTxPkt and pPriv. A possible stack implementation is
 * shown below:
 * @code
 * Len = <802.11 Frame length> + sizeof(struct MKxTxDescriptor);
 * pSkb = alloc_skb(Len, GFP_DMA);
 * pTxPkt = pSkb->data;
 * pTxPkt->Length = <802.11 Frame length>
 *  ...
 * Res = MKx_TxReq(pMKx, pTxPkt, pSkb);
 * @endcode
 */
typedef tMKxStatus (*fMKx_TxReq) (struct MKx *pMKx,
                                  tMKxTxPacket *pTxPkt,
                                  void *pPriv);

/**
 * @brief Transmit notification callback
 * @param pMKx MKx handle
 * @param pTxPkt As provided in the @ref fMKx_TxReq call
 * @param pTxEvent A pointer to the event data generated for the TxReq
 * @param pPriv As provided in the @ref fMKx_TxReq call
 * @return MKXSTATUS_SUCCESS if the 'confirm' was successfully handled.
 *         Other values are logged for debug purposes.
 *
 * A callback invoked by the LLC to notify the stack that the provided transmit
 * packet was either successfully transmitted or failed to be
 * queued/transmitted. The status can be determined from first inspecting
 * pTxEvent->Hdr.Ret - if this indicates success (MKXSTATUS_SUCCESS) then the
 * TxReq was successfully sent to the radio. The success / failure of the radio
 * transmitting the packet can then be observed by inspecting
 * pTxEvent->TxEventData.TxStatus. i.e.:
 *
 * @code
 * {
 *   tMKxStatus Result = (pTxEvent->Hdr.Ret == MKXSTATUS_SUCCESS ?
 *                        pTxEvent->TxEventData.TxStatus :
 *                        pTxEvent->Hdr.Ret);
 * }
 * @endcode
 *
 * @note: The pTxEvent should not be modified and will be freed after return
 * from this callback.
 *
 * Continuing the example from @ref fMKx_TxReq...
 * @code
 * {
 *   ...
 *   free_skb(pPriv);
 *   return MKXSTATUS_SUCCESS;
 * }
 * @endcode
 */
typedef tMKxStatus (*fMKx_TxCnf) (struct MKx *pMKx,
                                  tMKxTxPacket *pTxPkt,
                                  const tMKxTxEvent *pTxEvent,
                                  void *pPriv);

/**
 * @brief Flush all pending transmit packets
 * @param pMKx MKx handle
 * @param RadioID The specific radio (MKX_RADIO_A or MKX_RADIO_B)
 * @param ChannelID The specific channel (MKX_CHANNEL_0 or MKX_CHANNEL_1)
 * @param TxQueue The specific queue (MKX_TXQ_COUNT for all)
 * @return MKXSTATUS_SUCCESS if the flush request was accepted
 *
 */
typedef tMKxStatus (*fMKx_TxFlush) (struct MKx *pMKx,
                                    tMKxRadio RadioID,
                                    tMKxChannel ChannelID,
                                    tMKxTxQueue TxQueue);

/**
 * @brief Callback invoked by the LLC to allocate a receive packet buffer
 * @param pMKx MKx handle
 * @param BufLen Maximum length of the receive packet
 * @param ppBuf Pointer to a to-be-allocated buffer for the receive packet.
 *              In the case of an error: *ppBuf == NULL
 * @param ppPriv Pointer to provide when invoking any callback associated with
 *               this receive packet. Usually the provided contents of ppBuf
 *               and ppPriv have some association
 * @return MKXSTATUS_SUCCESS if the receive packet allocation request was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 * A callback invoked by the LLC in an interrupt context to request the
 * stack to allocate a receive packet buffer.
 *
 * @note The buffer must lie in DMA accessible memory.
 * A possible implementation is shown below:
 * @code
 * *ppPriv = alloc_skb(BufLen, GFP_DMA|GFP_ATOMIC);
 * *ppBuf = (*ppPriv)->data;
 * @endcode
 *
 */
typedef tMKxStatus (*fMKx_RxAlloc) (struct MKx *pMKx,
                                    int BufLen,
                                    uint8_t **ppBuf,
                                    void **ppPriv);

/**
 * @brief A callback invoked by the LLC to deliver a receive packet buffer to
 *        the stack
 * @param pMKx MKx handle
 * @param pRxPkt Pointer to the receive packet.
 *            (same as @c *ppBuf provided in @ref fMKx_RxAlloc)
 * @param pPriv Private packet pointer
 *             (same as provided in @ref fMKx_RxAlloc)
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 */
typedef tMKxStatus (*fMKx_RxInd) (struct MKx *pMKx,
                                  tMKxRxPacket *pRxPkt,
                                  void *pPriv);

/// Signalled notifications via MKx_NotifInd()
typedef enum
{
  // Useful masks
  MKX_NOTIF_MASK_ERROR       = 0x8000000, ///< Error
  MKX_NOTIF_MASK_UTC         = 0x4000000, ///< UTC boundary (PPS)
  MKX_NOTIF_MASK_STATS       = 0x2000000, ///< Statistics updated
  MKX_NOTIF_MASK_ACTIVE      = 0x1000000, ///< Radio channel active
  MKX_NOTIF_MASK_RADIOA      = 0x0000010, ///< Specific to radio A
  MKX_NOTIF_MASK_RADIOB      = 0x0000020, ///< Specific to radio B
  MKX_NOTIF_MASK_CHANNEL0    = 0x0000001, ///< Specific to channel 0
  MKX_NOTIF_MASK_CHANNEL1    = 0x0000002, ///< Specific to channel 1
  MKX_NOTIF_MASK_TEMPCFG     = 0x0000040, ///< Temperature configuration
  MKX_NOTIF_MASK_TEMP        = 0x0000080, ///< Temperature measurement
  MKX_NOTIF_MASK_POWERDETCFG = 0x0000100, ///< Power detector configuration
  MKX_NOTIF_MASK_RSSICALCFG  = 0x0000200, ///< RSSI calibration configuration
  /// No notification
  MKX_NOTIF_NONE          = 0x0000000,
  /// Active: Radio A, Channel 0
  MKX_NOTIF_ACTIVE_A0     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Active: Radio A, Channel 1
  MKX_NOTIF_ACTIVE_A1     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// Active: Radio B, Channel 0
  MKX_NOTIF_ACTIVE_B0     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Active: Radio B, Channel 1
  MKX_NOTIF_ACTIVE_B1     = MKX_NOTIF_MASK_ACTIVE | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// Stats updated: Radio A, Channel 0
  MKX_NOTIF_STATS_A0      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Stats updated: Radio A, Channel 1
  MKX_NOTIF_STATS_A1      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOA |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// Stats updated: Radio B, Channel 0
  MKX_NOTIF_STATS_B0      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL0,
  /// Stats updated: Radio B, Channel 1
  MKX_NOTIF_STATS_B1      = MKX_NOTIF_MASK_STATS  | MKX_NOTIF_MASK_RADIOB |
                            MKX_NOTIF_MASK_CHANNEL1,
  /// UTC second boundary
  MKX_NOTIF_UTC           = MKX_NOTIF_MASK_UTC,
  /// Temperature configuration update
  MKX_NOTIF_TEMPCFG       = MKX_NOTIF_MASK_TEMPCFG,
  /// Temperature measurement update
  MKX_NOTIF_TEMP          = MKX_NOTIF_MASK_TEMP,
  /// Power detector configuration update
  MKX_NOTIF_POWERDETCFG   = MKX_NOTIF_MASK_POWERDETCFG,
  /// RSSI calibration configuration update
  MKX_NOTIF_RSSICALCFG    = MKX_NOTIF_MASK_RSSICALCFG,
  /// Error
  MKX_NOTIF_ERROR         = MKX_NOTIF_MASK_ERROR
} eMKxNotif;
/// @copydoc eMKxNotif
typedef uint32_t tMKxNotif;

/**
 * @brief MKx notification callback
 * @param pMKx MKx handle
 * @param Notif The notification
 * @return MKXSTATUS_SUCCESS if the 'notif' was successfully handled.
 *         Other values are logged for debug purposes.
 *
 * Notification that the
 *  - Radio has encountered a UTC boundary
 *  - Channel is now active
 *  - Radio/Channel has experienced an error
 *  - Temperature update
 */
typedef tMKxStatus (*fMKx_NotifInd) (struct MKx *pMKx,
                                     tMKxNotif Notif);

/**
 * @brief Request a change to the temperature configuration
 * @param pMKx MKx handle
 * @param pCfg Pointer to the new configuration to apply
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * @code
 * // Get the current/default config
 * tMKxTempConfig Cfg = {0,};
 * memcpy(&(Cfg.TempConfigData), &(pMKx->Config.Temp), sizeof(tMKxTempConfigData));
 * // Update the values that we want to change
 * Cfg.TempConfigData.SensorSource = MKX_TEMP_SOURCE_MANUAL;
 * ...
 * // Apply the configuration
 * Res = MKx_TempCfg(pMKx, &Cfg);
 * @endcode
 */
typedef tMKxStatus (*fMKx_TempCfg) (struct MKx *pMKx,
                                    tMKxTempConfig *pCfg);

/**
 * @brief Request a change to the power detector configuration
 * @param pMKx MKx handle
 * @param pCfg Pointer to the new configuration to apply
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * @code
 * // Get the current/default config
 * tMKxPowerDetConfig Cfg = {0,};
 * memcpy(&(Cfg.PowerDetConfigData), &(MKx->Config.PowerDet),
 *        sizeof(tMKxPowerDetConfigData));
 * // Update the values that we want to change
 * Cfg.PowerDetConfigData.PowerCalMode = MKX_POWER_CAL_POWERDET;
 * ...
 * // Apply the configuration
 * Res = MKx_PowerDetCfg(pMKx, &Cfg);
 * @endcode
 */
typedef tMKxStatus (*fMKx_PowerDetCfg) (struct MKx *pMKx,
                                        tMKxPowerDetConfig *pCfg);

/**
 * @brief Request a change to the RSSI calibration configuration
 * @param pMKx MKx handle
 * @param pCfg Pointer to the new configuration to apply
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * @code
 * // Get the current/default config
 * tMKxRSSICalConfig Cfg = {0,};
 * memcpy(&(Cfg.RSSICalConfigData), &(MKx->Config.RSSICal),
 *        sizeof(tMKxRSSICalConfigData));
 * // Update the values that we want to change
 * Cfg.RSSICalConfigData.RSSICalMode = MKX_RSSI_CAL_ON;
 * ...
 * // Apply the configuration
 * Res = MKx_RSSICalCfg(pMKx, &Cfg);
 * @endcode
 */
typedef tMKxStatus (*fMKx_RSSICalCfg) (struct MKx *pMKx,
                                       tMKxRSSICalConfig *pCfg);

/**
 * @brief Update the PA temperature measurement
 * @param pMKx MKx handle
 * @param pTemp Pointer to the new measurements to apply
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * @code
 * // Get the current/default temperature
 * tMKxTemp Temp = {0,};
 * memcpy(&(Temp.TempData), &(pMKx->State.Temp), sizeof(Temp));
 * // Update both the temperatures
 * Temp.TempData.TempPAAnt1 = 42;
 * Temp.TempData.TempPAAnt2 = 67;
 * ...
 * // Deliver the measurements
 * Res = MKx_Temp(pMKx, &Temp);
 * @endcode
 */
typedef tMKxStatus (*fMKx_Temp) (struct MKx *pMKx,
                                 tMKxTemp *pTemp);

/**
 * @brief Request to get the underlying MKx TSF.
 * @param pMKx MKx handle
 * @return MKXSTATUS_SUCCESS if the request was accepted
 *
 * TSF is returned via fMKx_GetTSFInd callback.
 */
typedef tMKxStatus (*fMKx_GetTSFReq) (struct MKx *pMKx);

/**
 * @brief A callback invoked by the LLC to deliver the current TSF to the stack
 * @param pMKx MKx handle
 * @param TSF The TSF counter value
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 */
typedef tMKxStatus (*fMKx_GetTSFInd) (struct MKx *pMKx, tMKxTSF TSF);

/**
 * @brief Set the MKx TSF to UTC time, using either 1PPS event or TSF timestamp
 * @param pMKx MKx handle
 * @param pSetTSF Pointer to data structure containing UTC set
 * @return MKXSTATUS_SUCCESS (0) or a negative error code @sa eMKxStatus
 *
 * When pSetTSF->SetTSFData.Cmd == UTC_AT_1PPS,
 * - Set MKx TSF using UTC input, which is the time that corresponded to the
 *   last 1PPS event.
 *
 * When pSetTSF->SetTSFData.Cmd == UTC_AT_TSF,
 * - Set MKx TSF using UTC input, which is the UTC time that corresponds to the
 *   TSF timestamp input.
 */
typedef tMKxStatus (*fMKx_SetTSF) (struct MKx *pMKx, tMKxSetTSF *pSetTSF);

/**
 * @brief A function invoked by the stack to deliver a C2X APDU buffer to the SAF5100
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the buffer was sent successful.
 *         Other values may be logged by the MKx for debug purposes.
 *
 * @note This function blocks until the buffer is sent on-the-wire
 */
typedef tMKxStatus (*fC2XSec_CommandReq) (struct MKx *pMKx,
                                          tMKxC2XSec *pMsg);
/**
 * @brief A callback invoked by the LLC to deliver a C2X APDU buffer to the stack
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 * @note pBuf must be handled (or copied) in the callback
 */
typedef tMKxStatus (*fC2XSec_ReponseInd) (struct MKx *pMKx,
                                          tMKxC2XSec *pMsg);



/**
 * @brief A function invoked by the stack to deliver a debug buffer to the MKx
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the buffer was sent successful.
 *         Other values may be logged by the MKx for debug purposes.
 *
 * @note This function blocks until the buffer is sent on-the-wire
 */
typedef tMKxStatus (*fMKx_DebugReq) (struct MKx *pMKx,
                                     struct MKxIFMsg *pMsg);
/**
 * @brief A callback invoked by the LLC to deliver a debug buffer to the stack
 * @param pMKx MKx handle
 * @param pMsg Pointer to the buffer.
 * @return MKXSTATUS_SUCCESS if the receive packet allocation delivery was
 *         successful. Other values may be logged by the MKx for debug purposes.
 *
 * @note pBuf must be handled (or copied) in the callback
 */
typedef tMKxStatus (*fMKx_DebugInd) (struct MKx *pMKx,
                                     struct MKxIFMsg *pMsg);

//------------------------------------------------------------------------------
// Handle Structures
//------------------------------------------------------------------------------

/// MKx LLC status information (including statistics)
typedef struct MKxState
{
  /// Statistics (read only)
  tMKxRadioStats Stats[MKX_RADIO_COUNT];
  /// Temperature measurements (read only)
  tMKxTempData Temp;
} tMKxState;

/// Global MKx MKx API functions
typedef struct MKxFunctions
{
  fMKx_Config Config;
  fMKx_TxReq TxReq;
  fMKx_GetTSFReq GetTSFReq;
  fMKx_SetTSF SetTSF;
  fMKx_TxFlush TxFlush;
  fMKx_TempCfg TempCfg;
  fMKx_Temp Temp;
  fMKx_PowerDetCfg PowerDetCfg;
  fMKx_RSSICalCfg RSSICalCfg;
  fMKx_DebugReq DebugReq;
  fC2XSec_CommandReq C2XSecCmd;
} tMKxFunctions;

/// Global MKx MKx API callbacks (set by the stack)
typedef struct MKxCallbacks
{
  fMKx_TxCnf TxCnf;
  fMKx_RxAlloc RxAlloc;
  fMKx_RxInd RxInd;
  fMKx_NotifInd NotifInd;
  fMKx_DebugInd DebugInd;
  fMKx_GetTSFInd GetTSFInd;
  fC2XSec_ReponseInd C2XSecRsp;
} tMKxCallbacks;

/// MKx API functions and callbacks
typedef struct MKxAPI
{
  /// Stack -> SDR
  tMKxFunctions Functions;
  /// SDR -> Stack
  tMKxCallbacks Callbacks;
} tMKxAPI;

/// MKx LLC configuration
typedef struct MKxConfig
{
  /// Radio configuration (read only)
  tMKxRadioConfigData Radio[MKX_RADIO_COUNT];
  /// Temperature configuration
  tMKxTempConfigData Temp;
  /// Power detector configuration
  tMKxPowerDetConfigData PowerDet;
  /// RSSI calibration configuration
  tMKxRSSICalConfigData RSSICal;
} tMKxConfig;

/// MKx LLC handle
typedef struct MKx
{
  /// 'Magic' value used as an indicator that the handle is valid
  uint32_t Magic;
  /// Reserved (for 64bit aligment)
  uint32_t Reserved;
  /// Private data (for the stack to store stuff)
  union {
    void *pPriv;
    uint64_t Priv;
  };
  /// State information (read only)
  const tMKxState State;
  /// Configuration (read only)
  const tMKxConfig Config;
  /// MKx API functions and callbacks
  struct MKxAPI API;
} tMKx;

//------------------------------------------------------------------------------
// Functions
//------------------------------------------------------------------------------

/**
 * @brief Initialize the LLC and get a handle
 * @param DevId Device number (0..1)
 * @param ppMKx MKx handle to initilize
 * @return MKXSTATUS_SUCCESS (0) or a negative error code @sa eMKxStatus
 *
 * This function will:
 *  - Optionally reset and download the SDR firmware
 *   - The SDR firmware image may be complied into the driver as a binary object
 *  - Intialize the USB or UDP interface
 */
tMKxStatus MKx_Init (uint8_t DevId, tMKx **ppMKx);

/**
 * @brief De-initialize the LLC
 * @param pMKx MKx handle
 * @return MKXSTATUS_SUCCESS (0) or a negative error code @sa eMKxStatus
 *
 */
tMKxStatus MKx_Exit (tMKx *pMKx);


/**
 * @copydoc fMKx_Config
 */
static INLINE tMKxStatus MKx_Config (tMKx *pMKx,
                                     tMKxRadio Radio,
                                     tMKxRadioConfig *pConfig)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (Radio > MKX_RADIO_MAX)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  if (pConfig == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;

  return pMKx->API.Functions.Config(pMKx, Radio, pConfig);
}

/**
 * @copydoc fMKx_TxReq
 */
static INLINE tMKxStatus MKx_TxReq (tMKx *pMKx,
                                    tMKxTxPacket *pTxPkt,
                                    void *pPriv)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  return pMKx->API.Functions.TxReq(pMKx, pTxPkt, pPriv);
}

/**
 * @copydoc fMKx_TxFlush
 */
static INLINE tMKxStatus MKx_TxFlush (tMKx *pMKx,
                                      tMKxRadio RadioID,
                                      tMKxChannel ChannelID,
                                      tMKxTxQueue TxQueue)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  return pMKx->API.Functions.TxFlush(pMKx, RadioID, ChannelID, TxQueue);
}

/**
 * @copydoc fMKx_GetTSFReq
 */
static INLINE tMKxStatus MKx_GetTSFReq (tMKx *pMKx)
{
  if (pMKx == NULL)
    return 0;
  if (pMKx->Magic != MKX_API_MAGIC)
    return 0;

  return pMKx->API.Functions.GetTSFReq(pMKx);
}

/**
 * @copydoc fMKx_SetTSF
 */
static INLINE tMKxStatus MKx_SetTSF (tMKx *pMKx, tMKxSetTSF *pSetTSF)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  return pMKx->API.Functions.SetTSF(pMKx, pSetTSF);
}

/**
 * @brief Helper function to read the MKx statistics
 * @param pMKx MKx handle
 * @param Radio the selected radio
 * @param pStats Storage to place the radio's statistics in
 */
static INLINE tMKxStatus MKx_GetStats (const struct MKx *pMKx,
                                       tMKxRadio Radio,
                                       tMKxRadioStats *pStats)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (Radio > MKX_RADIO_MAX)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  if (pStats == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;

  memcpy(pStats, &(pMKx->State.Stats[Radio]), sizeof(tMKxRadioStats));

  return MKXSTATUS_SUCCESS;
}

/**
 * @copydoc fMKx_TempCfg
 */
static INLINE tMKxStatus MKx_SetTempCfg (tMKx *pMKx,
                                         tMKxTempConfig *pCfg)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pCfg == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  return pMKx->API.Functions.TempCfg(pMKx, pCfg);
}

/**
 * @brief Helper function to read the MKx temperature configuration
 * @param pMKx MKx handle
 * @param pCfg Storage to place the temperature config in
 */
static INLINE tMKxStatus MKx_GetTempCfg (tMKx *pMKx,
                                         tMKxTempConfig *pCfg)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pCfg == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  // Send a bogus 'MKX_IF_TEMPCFG' message so that the MKx replies with internal defaults
  memset(&(pCfg->TempConfigData), 0x80, sizeof(tMKxTempConfigData));
  (void)pMKx->API.Functions.TempCfg(pMKx, pCfg);
  // Get the latest values from the MKx handle
  memcpy(&(pCfg->TempConfigData), &(pMKx->Config.Temp), sizeof(tMKxTempConfigData));
  return MKXSTATUS_SUCCESS;
}

/**
 * @copydoc fMKx_Temp
 */
static INLINE tMKxStatus MKx_SetTemp (tMKx *pMKx,
                                      tMKxTemp *pTemp)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pTemp == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  return pMKx->API.Functions.Temp(pMKx, pTemp);
}

/**
 * @brief Helper function to read the MKx temperature measurements
 * @param pMKx MKx handle
 * @param pTemp Storage to place the temperature measurements in
 */
static INLINE tMKxStatus MKx_GetTemp (tMKx *pMKx,
                                      tMKxTemp *pTemp)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pTemp == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  // Send a bogus 'MKX_IF_TEMP' message so that the MKx replies with internal measurements
  memset(&(pTemp->TempData), 0x80, sizeof(tMKxTempData));
  (void)pMKx->API.Functions.Temp(pMKx, pTemp);
  // Get the latest values from the MKx handle
  memcpy(&(pTemp->TempData), &(pMKx->State.Temp), sizeof(tMKxTempData));
  return MKXSTATUS_SUCCESS;
}

/**
 * @copydoc fMKx_PowerDetCfg
 */
static INLINE tMKxStatus MKx_SetPowerDetCfg (tMKx *pMKx,
                                             tMKxPowerDetConfig *pCfg)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pCfg == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  return pMKx->API.Functions.PowerDetCfg(pMKx, pCfg);
}

/**
 * @brief Helper function to read the MKx power detector configuration
 * @param pMKx MKx handle
 * @param pCfg Storage to place the power detector config in
 */
static INLINE tMKxStatus MKx_GetPowerDetCfg (tMKx *pMKx,
                                             tMKxPowerDetConfig *pCfg)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pCfg == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  // Send a bogus 'MKX_IF_POWERDETCFG' message so that the MKx replies with internal defaults
  memset(&(pCfg->PowerDetConfigData), 0x80, sizeof(tMKxPowerDetConfigData));
  (void)pMKx->API.Functions.PowerDetCfg(pMKx, pCfg);
  // Get the latest values from the MKx handle
  memcpy(&(pCfg->PowerDetConfigData), &(pMKx->Config.PowerDet),
         sizeof(tMKxPowerDetConfigData));
  return MKXSTATUS_SUCCESS;
}

/**
 * @copydoc fMKx_RSSICalCfg
 */
static INLINE tMKxStatus MKx_SetRSSICalCfg (tMKx *pMKx,
                                             tMKxRSSICalConfig *pCfg)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pCfg == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  return pMKx->API.Functions.RSSICalCfg(pMKx, pCfg);
}

/**
 * @brief Helper function to read the MKx power detector configuration
 * @param pMKx MKx handle
 * @param pCfg Storage to place the power detector config in
 */
static INLINE tMKxStatus MKx_GetRSSICalCfg (tMKx *pMKx,
                                             tMKxRSSICalConfig *pCfg)
{
  if (pMKx == NULL)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;
  if (pMKx->Magic != MKX_API_MAGIC)
    return MKXSTATUS_FAILURE_INVALID_HANDLE;

  if (pCfg == NULL)
    return MKXSTATUS_FAILURE_INVALID_PARAM;
  // Send a bogus 'MKX_IF_RSSICALCFG' message so that the MKx replies with internal defaults
  memset(&(pCfg->RSSICalConfigData), 0x80, sizeof(tMKxRSSICalConfigData));
  (void)pMKx->API.Functions.RSSICalCfg(pMKx, pCfg);
  // Get the latest values from the MKx handle
  memcpy(&(pCfg->RSSICalConfigData), &(pMKx->Config.RSSICal),
         sizeof(tMKxRSSICalConfigData));
  return MKXSTATUS_SUCCESS;
}

#endif // #ifndef __LINUX__COHDA__LLC__LLC_API_H__

