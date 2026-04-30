/*
 * PROJECT: LoRaWAN 2.4 GHz Node Implementation
 * AUTHOR: Maxmilián Babič
 * DATE: 2026
 * DESCRIPTION:
 *   Custom implementation of LoRaWAN uplink transmission on the 2.4 GHz band
 *   using the Semtech SX1280 transceiver and an ESP32-S3 microcontroller.
 *
 * HARDWARE:
 *   - MCU:     LilyGO T3 S3 (ESP32-S3 + SX1280)
 *   - Sensor:  DHT11 on GPIO 15 (temperature & humidity)
 *   - Display: SSD1306 OLED 128x64 (I2C, SDA=GPIO 18, SCL=GPIO 17)
 *   - FEM:     External Front-End Module controlled via TX_EN/RX_EN
 *
 * CONFIGURATION:
 *   - Frequency:        2479 MHz (above standard Wi-Fi channels 1-13)
 *   - Spreading Factor: SF7
 *   - Bandwidth:        812.5 kHz (LORA_BW_0800)
 *   - Coding Rate:      4/5
 *   - Activation:       ABP (Activation By Personalization)
 *   - LoRaWAN spec:     1.0.2
 *
 * NOTE:
 *   This file uses placeholder LoRaWAN session keys. Before flashing,
 *   replace DEVADDR, NWK_S_KEY and APP_S_KEY with the values issued by
 *   your LoRaWAN network server (e.g. The Things Stack Sandbox).
 *   See README.md for details.
 *
 * REPOSITORY:
 *   https://github.com/diamondminer857/LoRaWAN-2G4-Node
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#include "sx1280-hal.h"
#include "config.h"
#include "radio.h"

// -------------------- PERIPHERAL SETTINGS --------------------

// OLED Display Settings
#define OLED_SDA 18
#define OLED_SCL 17
#define SCREEN_WIDTH  128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// DHT11 Sensor Settings
constexpr int8_t DHT_PIN = 15;
#define DHTTYPE DHT11
DHT dht(DHT_PIN, DHTTYPE);

// -------------------- SX1280 PINS ON LILYGO T3 S3 --------------------

constexpr int8_t LORA_MOSI = 6;
constexpr int8_t LORA_MISO = 3;
constexpr int8_t LORA_SCLK = 5;
constexpr int8_t LORA_NSS  = 7;
constexpr int8_t LORA_BUSY = 36;
constexpr int8_t LORA_DIO1 = 9;
constexpr int8_t LORA_DIO2 = -1;
constexpr int8_t LORA_DIO3 = -1;
constexpr int8_t LORA_RST  = 8;

// -------------------- FEM CONTROL PINS --------------------
// Front-End Module (PA + LNA) control:
//   TX_EN=1, RX_EN=0  -> PA active (transmit through amplifier)
//   TX_EN=0, RX_EN=1  -> LNA active (receive through amplifier)
//   TX_EN=0, RX_EN=0  -> idle (bypass)
constexpr int8_t LORA_TX_EN = 10;
constexpr int8_t LORA_RX_EN = 21;

// -------------------- RADIO CALLBACKS --------------------

static void onTxDone(void)                  { Serial.println("IRQ: TX done"); }
static void onRxDone(void)                  { Serial.println("IRQ: RX done"); }
static void onRxSync(void)                  { Serial.println("IRQ: RX SyncWord"); }
static void onRxHeader(void)                { Serial.println("IRQ: RX Header"); }
static void onTxTimeout(void)               { Serial.println("IRQ: TX timeout"); }
static void onRxTimeout(void)               { Serial.println("IRQ: RX timeout"); }
static void onRxError(IrqErrorCode_t err)   { Serial.print("IRQ: RX error code: "); Serial.println((int)err); }
static void onRangingDone(IrqRangingCode_t code) { Serial.print("IRQ: Ranging done code: "); Serial.println((int)code); }
static void onCadDone(bool cadFlag)         { Serial.print("IRQ: CAD done: "); Serial.println(cadFlag); }

RadioCallbacks_t radioCallbacks = {
    .txDone         = onTxDone,
    .rxDone         = onRxDone,
    .rxSyncWordDone = onRxSync,
    .rxHeaderDone   = onRxHeader,
    .txTimeout      = onTxTimeout,
    .rxTimeout      = onRxTimeout,
    .rxError        = onRxError,
    .rangingDone    = onRangingDone,
    .cadDone        = onCadDone
};

// GLOBAL RADIO OBJECT
SX1280Hal radio(LORA_MOSI, LORA_MISO, LORA_SCLK, LORA_NSS, LORA_BUSY,
                LORA_DIO1, LORA_DIO2, LORA_DIO3, LORA_RST, &radioCallbacks);

// -------------------- LORA SETTINGS --------------------

// Default channel: 2479 MHz (above Wi-Fi channels 1-13).
// Other valid LoRaWAN 2.4 GHz default channels: 2403 MHz, 2425 MHz.
const uint32_t LORA_FREQ_HZ = 2479000000UL;

// -------------------- LoRaWAN ABP KEYS --------------------
//
// IMPORTANT: Replace the placeholder values below with the keys issued
// by your LoRaWAN network server (e.g. The Things Stack Sandbox console)
// when registering the device with ABP activation.
//
// Notes on byte order:
//   - DEVADDR is stored in LSB-first order. If TTSS shows "26 0B E6 4B",
//     write it here as { 0x4B, 0xE6, 0x0B, 0x26 }.
//   - NWK_S_KEY and APP_S_KEY are stored in MSB order, exactly as shown
//     in the TTSS console.

static const uint8_t DEVADDR[4] = {
    // LSB order
    0x00, 0x00, 0x00, 0x00
};

static const uint8_t NWK_S_KEY[16] = {
    // MSB order
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t APP_S_KEY[16] = {
    // MSB order
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

// Direction: 0 = uplink, 1 = downlink
static const uint8_t LORAWAN_DIR_UP = 0x00;

// Global counter for uplink frames (FCntUp)
static uint32_t g_uplinkCounter = 0;


// -------------------- CRYPTO (mbedTLS) --------------------
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h"
#include "mbedtls/cmac.h"

// AES-128 in ECB mode
static void aes128_encrypt(const uint8_t key[16], const uint8_t input[16], uint8_t output[16])
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, 128);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, input, output);
    mbedtls_aes_free(&ctx);
}

// AES-CMAC
static void aes128_cmac(const uint8_t key[16], const uint8_t *msg, size_t len, uint8_t out[16])
{
    const mbedtls_cipher_info_t *cipher_info =
        mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);
    mbedtls_cipher_cmac(cipher_info, key, 128, msg, len, out);
}

// FRMPayload encryption (uplink) using AES-128 CTR with AppSKey
static void lorawan_encrypt_frmpayload(uint8_t *buf, uint8_t len,
                                       const uint8_t appSKey[16],
                                       const uint8_t devAddr[4],
                                       uint32_t fCnt, uint8_t direction)
{
    uint8_t A[16], S[16];
    uint8_t ctr = 1;
    uint8_t remaining = len;
    uint8_t *p = buf;

    while (remaining > 0) {
        memset(A, 0, sizeof(A));
        A[0]  = 0x01;
        A[5]  = direction;
        A[6]  = devAddr[0]; A[7]  = devAddr[1];
        A[8]  = devAddr[2]; A[9]  = devAddr[3];
        A[10] = (uint8_t)(fCnt & 0xFF);
        A[11] = (uint8_t)((fCnt >> 8) & 0xFF);
        A[12] = (uint8_t)((fCnt >> 16) & 0xFF);
        A[13] = (uint8_t)((fCnt >> 24) & 0xFF);
        A[15] = ctr;

        aes128_encrypt(appSKey, A, S);

        uint8_t blockLen = (remaining >= 16) ? 16 : remaining;
        for (uint8_t i = 0; i < blockLen; i++) { p[i] ^= S[i]; }
        remaining -= blockLen;
        p += blockLen;
        ctr++;
    }
}

// MIC computation (LoRaWAN 1.0.x) - returns the lowest 32 bits of AES-CMAC
static uint32_t lorawan_compute_mic(const uint8_t *nwkSKey,
                                    const uint8_t devAddr[4],
                                    uint32_t fCnt, uint8_t direction,
                                    const uint8_t *payload, uint8_t len)
{
    uint8_t B0[16];
    memset(B0, 0, sizeof(B0));
    B0[0]  = 0x49;
    B0[5]  = direction;
    B0[6]  = devAddr[0]; B0[7]  = devAddr[1];
    B0[8]  = devAddr[2]; B0[9]  = devAddr[3];
    B0[10] = (uint8_t)(fCnt & 0xFF);
    B0[11] = (uint8_t)((fCnt >> 8) & 0xFF);
    B0[12] = (uint8_t)((fCnt >> 16) & 0xFF);
    B0[13] = (uint8_t)((fCnt >> 24) & 0xFF);
    B0[15] = len;

    uint8_t cmac[16];
    uint8_t buf[16 + 64];
    memcpy(buf, B0, 16);
    memcpy(buf + 16, payload, len);

    aes128_cmac(nwkSKey, buf, 16 + len, cmac);
    return  (uint32_t)cmac[0]
         | ((uint32_t)cmac[1] << 8)
         | ((uint32_t)cmac[2] << 16)
         | ((uint32_t)cmac[3] << 24);
}

// -------------------- LORAWAN PACKET BUILDER --------------------

static uint8_t build_lorawan_uplink(uint8_t *outBuf, uint8_t maxLen,
                                    const uint8_t *appPayload, uint8_t appLen,
                                    uint32_t fCnt)
{
    uint8_t idx = 0;

    // MHDR: MType = 010 (UnconfirmedDataUp), Major = 00 -> 0x40
    const uint8_t MHDR = 0x40;
    outBuf[idx++] = MHDR;

    // FHDR
    // DevAddr (LSB first)
    outBuf[idx++] = DEVADDR[0];
    outBuf[idx++] = DEVADDR[1];
    outBuf[idx++] = DEVADDR[2];
    outBuf[idx++] = DEVADDR[3];

    // FCtrl
    outBuf[idx++] = 0x00;

    // FCnt (lowest 16 bits)
    outBuf[idx++] = (uint8_t)(fCnt & 0xFF);
    outBuf[idx++] = (uint8_t)((fCnt >> 8) & 0xFF);

    // FPort
    outBuf[idx++] = 1;

    // FRMPayload
    uint8_t *frmPtr = outBuf + idx;
    memcpy(frmPtr, appPayload, appLen);
    lorawan_encrypt_frmpayload(frmPtr, appLen, APP_S_KEY, DEVADDR, fCnt, LORAWAN_DIR_UP);

    uint8_t lenNoMic = idx + appLen;

    // MIC over MHDR..FRMPayload
    uint32_t mic = lorawan_compute_mic(NWK_S_KEY, DEVADDR, fCnt, LORAWAN_DIR_UP,
                                       outBuf, lenNoMic);
    outBuf[lenNoMic + 0] = (uint8_t)(mic & 0xFF);
    outBuf[lenNoMic + 1] = (uint8_t)((mic >> 8) & 0xFF);
    outBuf[lenNoMic + 2] = (uint8_t)((mic >> 16) & 0xFF);
    outBuf[lenNoMic + 3] = (uint8_t)((mic >> 24) & 0xFF);

    return lenNoMic + 4;
}

// -------------------- DEBUG OUTPUT --------------------

void printStatusLog(float t, float h, uint8_t *payload, uint8_t len, uint32_t counter)
{
    Serial.println("\n------------------------------------------------");
    Serial.println("[LoRaWAN UPLINK]");
    Serial.println("------------------------------------------------");

    Serial.printf("Uptime:      %lu s\n", millis() / 1000);
    Serial.printf("Packet ID:   %u\n", counter);

    Serial.println("- SENSORS -");
    Serial.printf("Temperature: %.1f C\n", t);
    Serial.printf("Humidity:    %.1f %%\n", h);

    Serial.println("- RADIO -");
    Serial.printf("Freq:        %.1f MHz\n", LORA_FREQ_HZ / 1000000.0);
    Serial.print("Payload HEX: ");
    for (int i = 0; i < len; i++) { Serial.printf("%02X ", payload[i]); }
    Serial.println();
    Serial.println("------------------------------------------------\n");
}

// -------------------- LORA RADIO SETUP --------------------

void setupLora()
{
    Serial.println("Setting up SX1280...");
    radio.SetPacketType(PACKET_TYPE_LORA);
    radio.SetRfFrequency(LORA_FREQ_HZ);

    ModulationParams_t modParams;
    memset(&modParams, 0, sizeof(modParams));
    modParams.PacketType = PACKET_TYPE_LORA;
    modParams.Params.LoRa.SpreadingFactor = LORA_SF7;
    modParams.Params.LoRa.Bandwidth       = LORA_BW_0800;
    modParams.Params.LoRa.CodingRate      = LORA_CR_4_5;
    radio.SetModulationParams(&modParams);

    PacketParams_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.PacketType                 = PACKET_TYPE_LORA;
    pkt.Params.LoRa.PreambleLength = 12;
    pkt.Params.LoRa.HeaderType     = LORA_PACKET_VARIABLE_LENGTH;
    pkt.Params.LoRa.PayloadLength  = 16;
    pkt.Params.LoRa.InvertIQ       = LORA_IQ_NORMAL;
    radio.SetPacketParams(&pkt);

    // LoRaWAN public sync word (0x34)
    radio.WriteRegister(REG_LR_SYNCWORDBASEADDRESS1, 0x34);
    radio.SetBufferBaseAddresses(0x00, 0x00);

    // SX1280 raw output is kept low (~4 dBm) because the external FEM
    // amplifies the signal to roughly +20 dBm at the antenna port.
    // Using higher SX1280 output may damage the FEM input stage.
    radio.SetTxParams(4, RADIO_RAMP_20_US);

    Serial.println("SX1280 LoRa configured.");
}

// -------------------- FEM CONTROL HELPERS --------------------

static void fem_set_tx_mode()
{
    digitalWrite(LORA_RX_EN, LOW);
    digitalWrite(LORA_TX_EN, HIGH);
}

static void fem_set_idle_mode()
{
    digitalWrite(LORA_TX_EN, LOW);
    digitalWrite(LORA_RX_EN, LOW);
}

// -------------------- HELPER FUNCTIONS (UI & SENSORS) --------------------

void initPeripherals()
{
    // OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("OLED Init Failed");
    } else {
        display.clearDisplay();
        display.setTextColor(WHITE);
        display.setTextSize(1);
        display.setCursor(0, 0);
        display.println("LoRaWAN Node 2.4G");
        display.println("Initializing...");
        display.display();
    }
    // DHT11
    dht.begin();
}

void updateDisplay(float t, float h, bool sending)
{
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.println("- LoRaWAN 2.4GHz -");

    display.setTextSize(2);
    display.setCursor(0, 20);
    display.print(t, 1); display.print(" C");

    display.setTextSize(1);
    display.setCursor(0, 45);
    display.print("Humidity: "); display.print(h, 0); display.print(" %");

    display.setCursor(90, 55);
    display.setTextSize(1);
    if (sending) {
        display.setTextColor(BLACK, WHITE);
        display.print(" TX ");
    } else {
        display.setTextColor(WHITE, BLACK);
        display.print(" TX ");
    }
    display.setTextColor(WHITE);
    display.display();
}

// -------------------- ARDUINO SETUP/LOOP --------------------

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println("\n=== SX1280 2.4 GHz Node ===");

    // 1. Initialize FEM control pins
    Serial.println("Initializing FEM control pins...");
    pinMode(LORA_TX_EN, OUTPUT);
    pinMode(LORA_RX_EN, OUTPUT);
    fem_set_idle_mode();

    // 2. Initialize sensors and display
    initPeripherals();

    // 3. Initialize the radio
    Serial.println("Resetting SX1280...");
    radio.Reset();
    RadioStatus_t st = radio.GetStatus();
    Serial.print("Initial RadioStatus.Value = ");
    Serial.println(st.Value);

    setupLora();
}

void loop()
{
    static uint32_t lastTx = 0;
    uint32_t now = millis();

    // Transmit every 15 seconds
    if (now - lastTx > 15000) {
        lastTx = now;

        // --- 1. Read sensor data ---
        float t = dht.readTemperature();
        float h = dht.readHumidity();

        if (isnan(t) || isnan(h)) {
            Serial.println("DHT Sensor Error!");
            t = 0.0;
            h = 0.0;
        }

        // --- 2. Update display (TX active) ---
        updateDisplay(t, h, true);

        // --- 3. Build 4-byte payload ---
        // Temperature and humidity are scaled by 100 and stored as int16_t (MSB first).
        int16_t t_int = (int16_t)(t * 100);
        int16_t h_int = (int16_t)(h * 100);

        uint8_t payloadData[4];
        payloadData[0] = highByte(t_int);
        payloadData[1] = lowByte(t_int);
        payloadData[2] = highByte(h_int);
        payloadData[3] = lowByte(h_int);

        // --- 4. Build the LoRaWAN frame ---
        Serial.print("Building Uplink FCnt: ");
        Serial.println(g_uplinkCounter);

        uint8_t lorawanBuf[64];
        uint8_t phyLen = build_lorawan_uplink(
            lorawanBuf, sizeof(lorawanBuf),
            payloadData, 4,
            g_uplinkCounter
        );
        g_uplinkCounter++;

        printStatusLog(t, h, lorawanBuf, phyLen, g_uplinkCounter - 1);

        // --- 5. Transmit ---
        Serial.print("Sending PHYPayload len: ");
        Serial.println(phyLen);

        PacketParams_t pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.PacketType                 = PACKET_TYPE_LORA;
        pkt.Params.LoRa.PreambleLength = 12;
        pkt.Params.LoRa.HeaderType     = LORA_PACKET_VARIABLE_LENGTH;
        pkt.Params.LoRa.PayloadLength  = phyLen;
        pkt.Params.LoRa.Crc            = LORA_CRC_ON;
        pkt.Params.LoRa.InvertIQ       = LORA_IQ_NORMAL;
        radio.SetPacketParams(&pkt);

        radio.WriteBuffer(0x00, lorawanBuf, phyLen);

        // Enable PA (TX mode) before starting transmission
        Serial.println("FEM: activating PA for TX");
        fem_set_tx_mode();

        // Timeout 2000 ms
        TickTime_t timeout;
        timeout.PeriodBase      = RADIO_TICK_SIZE_1000_US;
        timeout.PeriodBaseCount = 2000;
        radio.SetTx(timeout);

        // Wait for TX to complete - SF7 packet is ~15 ms, 200 ms is plenty
        Serial.println("Waiting for TX to complete...");
        delay(200);

        // Explicitly set radio to standby before switching the FEM
        radio.SetStandby(STDBY_RC);
        delay(50);

        // Return FEM to idle state after TX
        Serial.println("FEM: TX complete, returning to idle");
        fem_set_idle_mode();

        updateDisplay(t, h, false);
    }
}
