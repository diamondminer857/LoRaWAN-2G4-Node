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

#ifndef __MTS_LORAWAN_CONFIG__
#define __MTS_LORAWAN_CONFIG__

#include <Arduino.h>
#include <stdint.h>
#include <string.h>
#include "sx1280.h"

// ==================== CONSTANTS & MACROS ====================

typedef struct {
    int16_t fd;
    char name[33];
    uint32_t size;
} file_record;

const uint8_t CURRENT_CONFIG_VERSION = 8;

// Memory Map (Legacy from full stack implementation)
#define SETTINGS_ADDR   0x0000      // 1024 bytes
#define PROTECTED_ADDR  0x0400      // 256 bytes
#define SESSION_ADDR    0x0500      // 512 bytes
#define USER_ADDR       0x0800      // User space

#define MULTICAST_SESSIONS 3
#define EUI_LENGTH 8
#define KEY_LENGTH 16
#define PASSPHRASE_LENGTH 128

#define PROTECTED_RFU_SIZE 223
#define CONFIG_RFU_SIZE 352
#define SESSION_RFU_SIZE 240

// ==================== SETTINGS STRUCTURES ====================

// Protected settings (EUI, Keys)
typedef struct {
    uint8_t FrequencyBand;
    uint8_t DeviceEUI[EUI_LENGTH];
    uint8_t AppEUI[EUI_LENGTH];
    uint8_t AppKey[KEY_LENGTH];
    uint8_t RFU[PROTECTED_RFU_SIZE];
} ProtectedSettings_t;

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

// Session State (Counters, Status)
typedef struct {
    bool Joined;
    uint8_t