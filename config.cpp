/*
 * PROJECT: LoRaWAN 2.4 GHz Node Implementation
 * FILE: config.cpp
 * AUTHOR: Maxmilián Babič
 * DATE: December 2025
 * DESCRIPTION:
 * Configuration manager implementation.
 * Handles default settings for the device (frequencies, data rates, etc.).
 * NOTE: Active LoRaWAN keys and specific radio settings are overridden 
 * in the main .ino file for this specific implementation.
 */

#include "config.h"

// OTAA Keys placeholders (Not used in ABP mode, kept for structure compatibility)
uint8_t devEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appEUI[8] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
uint8_t appKey[16] = { 
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

ConfigManager::ConfigManager() {}
ConfigManager::~ConfigManager() {}

void ConfigManager::DefaultProtected(DeviceConfig_t& dc)
{
    ProtectedSettings_t& p = dc.provisioning;
    p.FrequencyBand = 0; // 0 = custom

    memcpy(p.DeviceEUI, devEUI, EUI_LENGTH);
    memcpy(p.AppEUI,    appEUI, EUI_LENGTH);
    memcpy(p.AppKey,    appKey, KEY_LENGTH);

    memset(p.RFU, 0, sizeof(p.RFU));
}

void ConfigManager::DefaultSettings(DeviceConfig_t& dc)
{
    NetworkSettings_t& s = dc.settings;

    memset(&s, 0, sizeof(s));

    s.ConfigVersion   = CURRENT_CONFIG_VERSION;

    // UART Baudrates (Informational)
    s.SerialBaudRate  = 115200;
    s.DebugBaudRate   = 115200;

    // LoRaWAN Basic Settings
    s.JoinMode        = ABP;          // Changed to ABP to match main logic
    s.JoinRetries     = 3;

    s.EnableADR       = false;        // ADR disabled for manual SF7 control
    s.EnableCRC       = true;
    s.EnableEncryption = true;

    // 2.4 GHz LoRaWAN Settings (Matching the successful test)
    s.TxFrequency     = 2479000000;   // 2.479 GHz (Clean channel)
    s.RxFrequency     = 2479000000;

    s.TxDataRate      = DR0;          // Placeholder
    s.RxDataRate      = DR0;
    s.TxPower         = 10;           // 10 dBm (Matching .ino setup)

    s.Port            = 1;            // Default FPort
    s.PublicNetwork   = 1;            // Public LoRaWAN
}

void ConfigManager::DefaultSession(DeviceConfig_t& dc)
{
    NetworkSession_t& sess = dc.session;
    memset(&sess, 0, sizeof(sess));

    sess.Joined = false;
    sess.UplinkCounter   = 0;
    sess.DownlinkCounter = 0;
}

void ConfigManager::Default(DeviceConfig_t& dc)
{
    // Clear the entire structure
    memset(&dc, 0, sizeof(dc));

    DefaultProtected(dc);
    DefaultSettings(dc);
    DefaultSession(dc);

    // Application Settings
    dc.app_settings.DutyCycleEnabled = false;
    dc.app_settings.TxInterval       = 15000;       // 15 s (Matching loop delay)
    dc.app_settings.Bandwidth        = LORA_BW_0800; // 812.5 kHz (SX1280)
}

void ConfigManager::Load(DeviceConfig_t& dc)
{
    // No EEPROM/Flash storage implemented yet - force defaults
    Default(dc);
}

void ConfigManager::Sleep()  {}
void ConfigManager::Wakeup() {}