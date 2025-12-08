/*
 * PROJECT: LoRaWAN 2.4 GHz Node Implementation
 * AUTHOR: Maxmilián Babič
 * DATE: December 2025
 * DESCRIPTION:
 * Custom implementation of LoRaWAN uplink transmission on 2.4 GHz band
 * using SX1280 radio module and ESP32.
 * * HARDWARE:
 * - MCU: LilyGO T3 S3 (ESP32-S3)
 * - Radio: Semtech SX1280
 * * CONFIGURATION:
 * - Frequency: 2479 MHz (Selected to minimize Wi-Fi interference)
 * - Spreading Factor: SF7 (Optimized for low Time-on-Air)
 * - Bandwidth: 812.5 kHz
 * - Activation: ABP (Activation By Personalization)
 * * NOTE: 
 * Requires a LoRaWAN 2.4 GHz Gateway configured to receive SF7 packets.
 * Keys in this file are placeholders. Replace them with real TTN keys.
 */
#include <Arduino.h>
#include "sx1280-hal.h"
#include "config.h"
#include "radio.h"

// -------------------- PINS SX1280 ON LILYGO T3-S3 --------------------
// LoRa(SX1280) SCK   5
// LoRa(SX1280) MISO  3
// LoRa(SX1280) MOSI  6
// LoRa(SX1280) CS    7
// LoRa(SX1280) RESET 8
// LoRa(SX1280) DIO1  9
// LoRa(SX1280) BUSY  36

constexpr int8_t LORA_MOSI = 6;
constexpr int8_t LORA_MISO = 3;
constexpr int8_t LORA_SCLK = 5;
constexpr int8_t LORA_NSS  = 7;
constexpr int8_t LORA_BUSY = 36;
constexpr int8_t LORA_DIO1 = 9;
constexpr int8_t LORA_DIO2 = -1;
constexpr int8_t LORA_DIO3 = -1;
constexpr int8_t LORA_RST  = 8;

//CALLBACKS

static void onTxDone(void)
{
    Serial.println("IRQ: TX done");
}

static void onRxDone(void)
{
    Serial.println("IRQ: RX done");
}

static void onRxSync(void)
{
    Serial.println("IRQ: RX SyncWord");
}

static void onRxHeader(void)
{
    Serial.println("IRQ: RX Header");
}

static void onTxTimeout(void)
{
    Serial.println("IRQ: TX timeout");
}

static void onRxTimeout(void)
{
    Serial.println("IRQ: RX timeout");
}

static void onRxError(IrqErrorCode_t err)
{
    Serial.print("IRQ: RX error, code = ");
    Serial.println((int)err);
}

static void onRangingDone(IrqRangingCode_t code)
{
    Serial.print("IRQ: Ranging done, code = ");
    Serial.println((int)code);
}

static void onCadDone(bool cadFlag)
{
    Serial.print("IRQ: CAD done, flag = ");
    Serial.println(cadFlag ? "true" : "false");
}

RadioCallbacks_t radioCallbacks = {
    .txDone           = onTxDone,
    .rxDone           = onRxDone,
    .rxSyncWordDone   = onRxSync,
    .rxHeaderDone     = onRxHeader,
    .txTimeout        = onTxTimeout,
    .rxTimeout        = onRxTimeout,
    .rxError          = onRxError,
    .rangingDone      = onRangingDone,
    .cadDone          = onCadDone
};

//GLOBAL RADIO

SX1280Hal radio(
    LORA_MOSI,
    LORA_MISO,
    LORA_SCLK,
    LORA_NSS,
    LORA_BUSY,
    LORA_DIO1,
    LORA_DIO2,
    LORA_DIO3,
    LORA_RST,
    &radioCallbacks
);

// -------------------- LORA SETTINGS --------------------
//Frequency setting; either 2403, 2425 or 2479 MHz

const uint32_t LORA_FREQ_HZ = 2479000000UL;

// -------------------- LoRaWAN KEYS --------------------

// Replace these placeholders with your own keys from TTN Console.
// Example: If TTN says "26 3A F6 4A", write here {0x4A, 0xF6, 0x3A, 0x26}

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

// Direction - 0 = uplink, 1 = downlink
static const uint8_t LORAWAN_DIR_UP = 0x00;

// čítač uplinků (FCntUp)
static uint32_t g_uplinkCounter = 0;





// ----- AES / CMAC from mbedTLS -----
#include "mbedtls/aes.h"
#include "mbedtls/cipher.h"
#include "mbedtls/cmac.h"

// AES-128 in ECB
static void aes128_encrypt(const uint8_t key[16],
                           const uint8_t input[16],
                           uint8_t output[16])
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, key, 128);
    mbedtls_aes_crypt_ecb(&ctx, MBEDTLS_AES_ENCRYPT, input, output);
    mbedtls_aes_free(&ctx);
}

// AES-CMAC
static void aes128_cmac(const uint8_t key[16],
                        const uint8_t *msg, size_t len,
                        uint8_t out[16])
{
    const mbedtls_cipher_info_t *cipher_info =
        mbedtls_cipher_info_from_type(MBEDTLS_CIPHER_AES_128_ECB);

    mbedtls_cipher_cmac(cipher_info, key, 128, msg, len, out);
}


// Encryption of FRMPayload (uplink)
static void lorawan_encrypt_frmpayload(uint8_t *buf, uint8_t len,
                                       const uint8_t appSKey[16],
                                       const uint8_t devAddr[4],
                                       uint32_t fCnt,
                                       uint8_t direction)
{
    uint8_t A[16];
    uint8_t S[16];
    uint8_t ctr = 1;

    uint8_t remaining = len;
    uint8_t *p = buf;

    while (remaining > 0) {
        memset(A, 0, sizeof(A));
        A[0] = 0x01;
        A[5] = direction;
        A[6] = devAddr[0];
        A[7] = devAddr[1];
        A[8] = devAddr[2];
        A[9] = devAddr[3];
        A[10] = (uint8_t)(fCnt & 0xFF);
        A[11] = (uint8_t)((fCnt >> 8) & 0xFF);
        A[12] = (uint8_t)((fCnt >> 16) & 0xFF);
        A[13] = (uint8_t)((fCnt >> 24) & 0xFF);
        A[15] = ctr;

        aes128_encrypt(appSKey, A, S);

        uint8_t blockLen = (remaining >= 16) ? 16 : remaining;
        for (uint8_t i = 0; i < blockLen; i++) {
            p[i] ^= S[i];
        }

        remaining -= blockLen;
        p += blockLen;
        ctr++;
    }
}

// MIC (LoRaWAN 1.0.x) – 32bit, returns LSB as uint32_t
static uint32_t lorawan_compute_mic(const uint8_t *nwkSKey,
                                    const uint8_t devAddr[4],
                                    uint32_t fCnt,
                                    uint8_t direction,
                                    const uint8_t *payload,
                                    uint8_t len)
{
    uint8_t B0[16];
    memset(B0, 0, sizeof(B0));

    B0[0]  = 0x49;
    B0[5]  = direction;
    B0[6]  = devAddr[0];
    B0[7]  = devAddr[1];
    B0[8]  = devAddr[2];
    B0[9]  = devAddr[3];
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

    uint32_t mic =  (uint32_t)cmac[0]
                  | ((uint32_t)cmac[1] << 8)
                  | ((uint32_t)cmac[2] << 16)
                  | ((uint32_t)cmac[3] << 24);

    return mic;
}



// Builds LoRaWAN PHYPayload (UnconfirmedDataUp) to outBuf.
// Returns packet length in bytes
static uint8_t build_lorawan_uplink(uint8_t       *outBuf,
                                    uint8_t        maxLen,
                                    const uint8_t *appPayload,
                                    uint8_t        appLen,
                                    uint32_t       fCnt)
{
    uint8_t idx = 0;

    // MHDR: MType = 010 (UnconfirmedDataUp), Major = 00 → 0x40
    const uint8_t MHDR = 0x40;
    outBuf[idx++] = MHDR;

    // FHDR
    // DevAddr (LSB first)
    outBuf[idx++] = DEVADDR[0];
    outBuf[idx++] = DEVADDR[1];
    outBuf[idx++] = DEVADDR[2];
    outBuf[idx++] = DEVADDR[3];

    // FCtrl
    uint8_t FCtrl = 0x00;
    outBuf[idx++] = FCtrl;

    // FCnt
    outBuf[idx++] = (uint8_t)(fCnt & 0xFF);
    outBuf[idx++] = (uint8_t)((fCnt >> 8) & 0xFF);

    // FOpts

    // FPort
    uint8_t FPort = 1;
    outBuf[idx++] = FPort;

    // FRMPayload
    uint8_t *frmPtr = outBuf + idx;
    memcpy(frmPtr, appPayload, appLen);

    // Encrypting the FRMPayload with AppSKey
    lorawan_encrypt_frmpayload(frmPtr, appLen, APP_S_KEY, DEVADDR, fCnt, LORAWAN_DIR_UP);

    uint8_t lenNoMic = idx + appLen;

    // MIC -> MHDR..FRMPayload
    uint32_t mic = lorawan_compute_mic(NWK_S_KEY, DEVADDR, fCnt, LORAWAN_DIR_UP,
                                       outBuf, lenNoMic);

    outBuf[lenNoMic + 0] = (uint8_t)(mic & 0xFF);
    outBuf[lenNoMic + 1] = (uint8_t)((mic >> 8) & 0xFF);
    outBuf[lenNoMic + 2] = (uint8_t)((mic >> 16) & 0xFF);
    outBuf[lenNoMic + 3] = (uint8_t)((mic >> 24) & 0xFF);

    return lenNoMic + 4;
}




void setupLora()
{
    Serial.println("Setting up SX1280...");

    radio.SetPacketType(PACKET_TYPE_LORA);
    radio.SetRfFrequency(LORA_FREQ_HZ);

    ModulationParams_t modParams;
    memset(&modParams, 0, sizeof(modParams));
    modParams.PacketType = PACKET_TYPE_LORA;
    modParams.Params.LoRa.SpreadingFactor = LORA_SF7; //set as desired, from SF5 to SF12
    modParams.Params.LoRa.Bandwidth       = LORA_BW_0800;
    modParams.Params.LoRa.CodingRate      = LORA_CR_4_5;

    radio.SetModulationParams(&modParams);

    PacketParams_t pkt;
    memset(&pkt, 0, sizeof(pkt));
    pkt.PacketType                         = PACKET_TYPE_LORA;
    pkt.Params.LoRa.PreambleLength         = 12;
    pkt.Params.LoRa.HeaderType             = LORA_PACKET_VARIABLE_LENGTH;
    pkt.Params.LoRa.PayloadLength          = 16;
    pkt.Params.LoRa.InvertIQ               = LORA_IQ_NORMAL;

    radio.SetPacketParams(&pkt);

    // LoRaWAN syncword = public = 0x21
   radio.WriteRegister(REG_LR_SYNCWORDBASEADDRESS1, 0x21);

    radio.SetBufferBaseAddresses(0x00, 0x00);
    radio.SetTxParams(10, RADIO_RAMP_20_US);

    Serial.println("SX1280 LoRa set succesfully.");
}


// -------------------- ARDUINO SETUP/LOOP --------------------

void setup()
{
    Serial.begin(115200);
    delay(2000);

    Serial.println();
    Serial.println("=== SX1280 2.4 GHz -> TTN test ===");

    Serial.println("Completing radio.Reset()...");
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

    if (now - lastTx > 15000) {  //15000 represents the delay between sending packets
        lastTx = now;

        const char appMsg[] = "HELLO_TTN_SX1280";
        const uint8_t appLen = sizeof(appMsg) - 1;

        uint8_t lorawanBuf[64];

        Serial.print("Building LoRaWAN uplink, FCntUp = ");
        Serial.println(g_uplinkCounter);

        uint8_t phyLen = build_lorawan_uplink(
            lorawanBuf,
            sizeof(lorawanBuf),
            (const uint8_t *)appMsg,
            appLen,
            g_uplinkCounter
        );

        g_uplinkCounter++;

        Serial.print("Sending LoRaWAN PHYPayload, length = ");
        Serial.println(phyLen);

        // Packet Settings
        PacketParams_t pkt;
        memset(&pkt, 0, sizeof(pkt));
        pkt.PacketType                         = PACKET_TYPE_LORA;
        pkt.Params.LoRa.PreambleLength         = 12;
        pkt.Params.LoRa.HeaderType             = LORA_PACKET_VARIABLE_LENGTH;
        pkt.Params.LoRa.PayloadLength          = phyLen;
        pkt.Params.LoRa.Crc                    = LORA_CRC_ON;
        pkt.Params.LoRa.InvertIQ               = LORA_IQ_NORMAL;
        radio.SetPacketParams(&pkt);

        // Writing of the PHYPayload to FIFO
        radio.WriteBuffer(0x00, lorawanBuf, phyLen);

        // timeout 2 s
        TickTime_t timeout;
        timeout.PeriodBase      = RADIO_TICK_SIZE_1000_US;
        timeout.PeriodBaseCount = 2000;

        radio.SetTx(timeout);
    }

    // debug status
    static uint32_t lastStatus = 0;
    if (now - lastStatus > 1000) {
        lastStatus = now;
        RadioStatus_t st = radio.GetStatus();
        Serial.print("RadioStatus.Value = ");
        Serial.println(st.Value);
    }
}
