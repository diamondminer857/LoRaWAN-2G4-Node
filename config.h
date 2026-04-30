/**********************************************************************
* COPYRIGHT 2019 MULTI-TECH SYSTEMS, INC.
*
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of MULTI-TECH SYSTEMS, INC. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
******************************************************************************
*/
/*
 * PROJECT: LoRaWAN 2.4 GHz Node Implementation
 * FILE: config.h
 * AUTHOR: Maxmilián Babič
 * DATE: December 2025
 * DESCRIPTION:
 * Configuration structures and definitions.
 * Defines the data structures for holding network settings, session keys,
 * and application configuration.
 */

#pragma once

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include "sx1280.h"

#ifndef __MTS_LORAWAN_CONFIG__
#define __MTS_LORAWAN_CONFIG__

// ==================== CONSTANTS & MACROS ====================
typedef struct {
    int16_t fd;
    char name[33];
    uint32_t size;
} file_record;

const uint8_t CURRENT_CONFIG_VERSION = 8;


#define SETTINGS_ADDR   0x0000      // configuration is 1024 bytes (0x000-0x3FF)
#define PROTECTED_ADDR  0x0400      // protected configuration is 256 bytes (0x400-0x4FF)
#define SESSION_ADDR    0x0500      // session is 512 bytes (0x500-0x6FF)
#define USER_ADDR       0x0800      // user space is 6*1024 bytes (0x800 - 0x1FFF)

#define MULTICAST_SESSIONS 3
#define EUI_LENGTH 8
#define KEY_LENGTH 16
#define PASSPHRASE_LENGTH 128

#define PROTECTED_RFU_SIZE 223

// ==================== SETTINGS STRUCTURES ====================

// Protected settings (EUI, Keys)
typedef struct {
    uint8_t FrequencyBand;
    uint8_t DeviceEUI[EUI_LENGTH];
    uint8_t AppEUI[EUI_LENGTH];
    uint8_t AppKey[KEY_LENGTH];
    uint8_t RFU[PROTECTED_RFU_SIZE];
} ProtectedSettings_t;

// DON'T CHANGE THIS UNLESS YOU REALLY WANT TO
#define CONFIG_RFU_SIZE 352

// General Network Settings
typedef struct {
    uint8_t ConfigVersion;

    uint32_t SerialBaudRate;
    uint32_t DebugBaudRate;
    uint8_t StartUpMode;

    /* Network Settings */
    uint8_t AppEUI[EUI_LENGTH];
    uint8_t NetworkEUIPassphrase[PASSPHRASE_LENGTH];
    uint8_t AppKey[KEY_LENGTH];
    uint8_t NetworkKeyPassphrase[PASSPHRASE_LENGTH];

    uint32_t NetworkAddress;
    uint8_t NetworkSessionKey[KEY_LENGTH];
    uint8_t DataSessionKey[KEY_LENGTH];

    uint8_t JoinMode;
    uint8_t JoinRetries;

    /* Radio Settings */
    uint8_t ForwardErrorCorrection_deprecated;
    uint8_t ACKAttempts;
    bool EnableEncryption;
    bool EnableCRC;

    bool EnableADR;

    bool EnableEcho;
    bool EnableVerbose;

    uint32_t TxFrequency;
    uint8_t TxDataRate;
    bool TxInverted_deprecated;
    uint8_t TxPower;
    bool TxWait;
    uint8_t FrequencySubBand;

    uint32_t RxFrequency;
    uint8_t RxDataRate;
    bool RxInverted_deprecated;
    uint8_t RxOutput;

    /* Serial Settings */
    uint32_t WakeInterval;
    uint16_t WakeTimeout;
    uint32_t WakeDelay;

    uint8_t PublicNetwork;
    uint8_t LinkCheckCount;
    uint8_t LinkCheckThreshold;

    uint8_t LogLevel;
    uint8_t JoinByteOrder_deprecated;

    uint8_t WakePin;
    uint8_t WakeMode;

    uint8_t PreserveSessionOverReset;

    uint8_t JoinDelay;
    uint8_t RxDelay;
    uint8_t Port;

    uint8_t Class;
    int8_t AntennaGain;

    bool FlowControl;
    uint8_t Repeat;

    bool SerialClearOnError;
    uint8_t Rx2Datarate;
    uint8_t JoinRx1DatarateOffset;
    uint8_t JoinRx2DatarateIndex;

    uint32_t Channels[16];
    uint8_t ChannelRanges[16];

    uint32_t JoinRx2Frequency;

    uint8_t MaxEIRP;
    uint8_t UlDwellTime;
    uint8_t DlDwellTime;

    uint8_t padding;

    uint32_t DlChannels[16];

    uint8_t LastPlan;

    int8_t lbtThreshold;
    uint16_t lbtTimeUs;

    uint32_t MulticastAddress[MULTICAST_SESSIONS];
    uint8_t MulticastNetSessionKey[MULTICAST_SESSIONS][KEY_LENGTH];
    uint8_t MulticastAppSessionKey[MULTICAST_SESSIONS][KEY_LENGTH];

    uint8_t AutoSleep;

    uint8_t PingPeriodicity;

    int32_t TxFrequencyOffset;

    uint16_t AdrAckLimit;
    uint16_t AdrAckDelay;

    uint8_t RFU[CONFIG_RFU_SIZE];
} NetworkSettings_t;

// DON'T CHANGE THIS UNLESS YOU REALLY WANT TO
#define SESSION_RFU_SIZE 240

typedef struct {
    bool Joined;
    uint8_t Rx1DatarateOffset;
    uint8_t Rx2Datarate;
    uint8_t ChannelMask500k;

    uint32_t NetworkAddress;

    uint8_t NetworkKey[KEY_LENGTH];
    uint8_t DataKey[KEY_LENGTH];

    uint64_t ChannelMask;

    uint32_t Channels[16];
    uint8_t ChannelRanges[16];

    uint32_t UplinkCounter;

    uint8_t Rx1Delay;
    uint8_t Datarate;
    uint8_t TxPower;
    uint8_t Repeat;

    uint32_t Rx2Frequency;

    uint32_t DownlinkCounter;

    uint8_t MaxDutyCycle;
    uint8_t AdrAckCounter;
    uint8_t LinkFailCount;
    uint8_t FrequencySubBand;

    uint32_t NetworkId;

    bool ServerAckRequested;
    uint8_t DeviceClass;

    uint8_t CommandBufferIndex;

    uint8_t CommandBuffer[15];

    uint8_t UlDwellTime;
    uint8_t DlDwellTime;

    uint32_t DlChannels[16];

    uint8_t MaxEIRP;

    uint32_t MulticastCounters[MULTICAST_SESSIONS];

    uint8_t LastPlan;

    uint32_t BeaconFrequency;
    bool BeaconFreqHop;
    uint32_t PingSlotFrequency;
    uint8_t PingSlotDatarateIndex;
    bool PingSlotFreqHop;

    uint8_t RFU[SESSION_RFU_SIZE];
} NetworkSession_t;

typedef struct {
    bool DutyCycleEnabled;
    uint32_t TxInterval;
    RadioLoRaBandwidths_t Bandwidth;
} ApplicationSettings_t;

typedef struct {
    ProtectedSettings_t provisioning;
    NetworkSettings_t settings;
    NetworkSession_t session;
    ApplicationSettings_t app_settings;
} DeviceConfig_t;


//default otaa
extern uint8_t devEUI[8];
extern uint8_t appEUI[8];
extern uint8_t appKey[16];

class ConfigManager {

public:
    static const uint8_t EuiLength = EUI_LENGTH;
    static const uint8_t KeyLength = KEY_LENGTH;
    static const uint8_t PassPhraseLength = PASSPHRASE_LENGTH;

    enum JoinMode {
        MANUAL,
        OTA,
        AUTO_OTA,
        PEER_TO_PEER
    };

    enum Mode {
        COMMAND_MODE,
        SERIAL_MODE
    };

    enum RX_Output {
        HEXADECIMAL,
        BINARY
    };

    enum DataRates {
        DR0, DR1, DR2, DR3, DR4, DR5, DR6, DR7,
        DR8, DR9, DR10, DR11, DR12, DR13, DR14, DR15
    };

    enum FrequencySubBands {
        FSB_ALL,
        FSB_1,
        FSB_2,
        FSB_3,
        FSB_4,
        FSB_5,
        FSB_6,
        FSB_7,
        FSB_8
    };

    ConfigManager();
    ~ConfigManager();

    void Load(DeviceConfig_t& dc);
    void Default(DeviceConfig_t& dc);
    void DefaultSettings(DeviceConfig_t& dc);
    void DefaultSession(DeviceConfig_t& dc);
    void DefaultProtected(DeviceConfig_t& dc);

    void Sleep();
    void Wakeup();
};

#endif // __MTS_LORAWAN_CONFIG__
