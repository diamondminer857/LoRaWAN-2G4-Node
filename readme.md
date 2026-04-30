# LoRaWAN 2.4 GHz Node Implementation

This repository contains the source code for a custom **LoRaWAN 2.4 GHz** node implementation, developed as part of a Bachelor's thesis at VŠB - Technical University of Ostrava.

The project demonstrates a functional uplink communication on the 2.4 GHz ISM band using the **Semtech SX1280** transceiver controlled by an **ESP32-S3** microcontroller. Unlike standard sub-GHz LoRaWAN, this implementation handles the specific physical layer requirements of the 2.4 GHz band and includes a custom-built LoRaWAN 1.0.2 stack (frame assembly, AES-128 payload encryption, MIC computation), since no publicly available Arduino library supporting the LoRaWAN protocol on the SX1280 transceiver existed at the time of development.

The node periodically reads temperature and humidity from a **DHT11** sensor, displays the values on a **0.96" OLED** display, encodes them into a 4-byte payload, and transmits the data as a LoRaWAN UnconfirmedDataUp uplink.

## Hardware Setup

* **End Node:** [LilyGO T3 S3](https://wiki.lilygo.cc/get_started/en/LoRa_GPS/T3S3/T3S3.html) (ESP32-S3 + Semtech SX1280 with integrated PA)
* **Sensor:** DHT11 (temperature & humidity) on GPIO 15
* **Display:** SSD1306 OLED 128x64 (I²C, SDA = GPIO 18, SCL = GPIO 17)
* **Gateway:** [MikroTik wAP LR2](https://mikrotik.com/product/wap_lr2_kit) (RBwAPR-2nD&R11e-LR2) running RouterOS, configured via WinBox
* **Network Server:** [The Things Stack Sandbox](https://www.thethingsnetwork.org/) (TTSS, formerly TTN V3)

## Key Configuration

To ensure stable communication in the interference-heavy 2.4 GHz band (Wi-Fi/Bluetooth), this is the configuration used and verified in the thesis:

* **Frequency:** 2479 MHz - located above standard Wi-Fi channels 1-13 to minimize noise. Matches one of the three default channels defined by the LoRaWAN 2.4 GHz specification.
* **Spreading Factor:** SF7 - reduces *Time-on-Air* to ~15 ms, significantly lowering collision probability.
* **Bandwidth:** 812.5 kHz (`LORA_BW_0800`)
* **Coding Rate:** 4/5 - default LoRaWAN coding rate.
* **Sync Word:** `0x34` (LoRaWAN public network)
* **Activation:** ABP (Activation By Personalization)
* **LoRaWAN Specification:** 1.0.2

## Field Test Results

The communication chain was tested in two scenarios:

| Scenario | Distance | Avg. RSSI | Avg. SNR | PDR |
|----------|----------|-----------|----------|------|
| Line of sight (gateway on Cvilín lookout tower, ~464 m a.s.l.) | **25 km** (Moravice) | -93 dBm | 8 dB | 96.7 % |
| Line of sight | 19.9 km (Slavkov u Opavy) | -91 dBm | 8 dB | 93.3 % |
| Line of sight | 10.35 km (Holasovice) | -89 dBm | 11 dB | 96.7 % |
| Urban (gateway on 5th-floor balcony) | 1.66 km (Křižkovského St.) | -102 dBm | 2 dB | 83.3 % |
| Urban | 1.12 km (Hlubčická St.) | -89 dBm | 7 dB | 93.3 % |
| Urban | 670 m (Krnov - Kaufland) | -80 dBm | 4 dB | 96.7 % |

The 25 km result confirms that LoRaWAN at 2.4 GHz remains practically usable over distances of tens of kilometers, provided that direct line of sight between the end node and the gateway is maintained.

## How to Run

### 1. Prerequisites

* **Arduino IDE** (tested with version 2.x)
* **ESP32 Board Support** package by Espressif (install via Boards Manager)
* The following Arduino libraries:
  * `Adafruit GFX Library`
  * `Adafruit SSD1306`
  * `DHT sensor library` by Adafruit
* No external LoRaWAN library is required - the stack is implemented directly in this repository.

### 2. Board Configuration

In the Arduino IDE, select:

* Board: **ESP32S3 Dev Module**
* CPU Frequency: **240 MHz**
* USB CDC On Boot: **Enabled** (for serial output)
* Upload Mode: **USB-OTG CDC (TinyUSB)**

### 3. LoRaWAN Keys Configuration

1. Register a new ABP device in [The Things Stack Sandbox](https://eu1.cloud.thethings.network/) (or any other LoRaWAN network server supporting the 2.4 GHz band).
2. Choose the frequency plan **"LoRa 2.4 GHz with 3 channels (Draft 2)"** and LoRaWAN version **1.0.2**.
3. Generate `DevAddr`, `NwkSKey` and `AppSKey`.
4. Open `LoRaWAN-2G4-Node.ino` and locate the **LoRaWAN ABP KEYS** section.
5. Replace the placeholder zeros with your actual keys:

```cpp
static const uint8_t DEVADDR[4] = {
    // LSB order - if TTSS shows "26 0B E6 4B", write { 0x4B, 0xE6, 0x0B, 0x26 }
    0x00, 0x00, 0x00, 0x00
};

static const uint8_t NWK_S_KEY[16] = {
    // MSB order - copy directly as displayed in TTSS console
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t APP_S_KEY[16] = {
    // MSB order - copy directly as displayed in TTSS console
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
```

### 4. Gateway Configuration

The gateway must be configured to listen on the same channel and parameters used by the node. For a MikroTik wAP LR2 (RouterOS):

* Open the **LoRa** section in WinBox.
* Set the **Network Server** to your LoRaWAN server address (e.g. `eu1.cloud.thethings.network` on port `1700` for TTSS).
* Add a channel at **2479 MHz**, **SF7**, bandwidth **800 kHz** (closest match to the 812.5 kHz used by SX1280).
* Save and enable the configuration.

> **Note:** Many off-the-shelf 2.4 GHz LoRa gateways come pre-configured to listen only on SF12 by default. Make sure your gateway's channel plan matches the node's settings (SF7 / 2479 MHz) or no packets will be received.

### 5. Payload Formatter

To decode the 4-byte payload (signed 16-bit temperature and humidity, both scaled by 100), set the following JavaScript decoder in the TTSS application's Payload Formatters section:

```javascript
function decodeUplink(input) {
    var bytes = input.bytes;
    var data = {};

    if (bytes.length < 4) {
        return { errors: ["Invalid payload length"] };
    }

    var tempInt = (bytes[0] << 8) | bytes[1];
    if (tempInt & 0x8000) {
        tempInt -= 0x10000;
    }
    data.temperature = tempInt / 100.0;

    var humInt = (bytes[2] << 8) | bytes[3];
    data.humidity = humInt / 100.0;

    return { data: data, warnings: [], errors: [] };
}
```

After activating the formatter, each received uplink will display the decoded temperature (°C) and humidity (%) values together with transmission metadata (RSSI, SNR, frequency, spreading factor).

## Project Structure

| File | Description |
|------|-------------|
| `LoRaWAN-2G4-Node.ino` | Main sketch - LoRaWAN frame assembly, AES-128 encryption, MIC computation, sensor reading, OLED display, transmission loop |
| `sx1280.h` / `sx1280.cpp` | Original Semtech SX1280 driver (BSD license, © 2016 Semtech) |
| `sx1280-hal.h` / `sx1280-hal.cpp` | Hardware Abstraction Layer rewritten for Arduino/ESP32 (replaces the original Mbed OS implementation) |
| `radio.h` | Radio command and IRQ definitions (BSD license, © 2016 Semtech) |
| `config.h` / `config.cpp` | Configuration manager for default settings (based on MultiTech Systems code) |

## Acknowledgements & Credits

This project is based on the **[Sx1280-LoRaWAN-Test-App](https://github.com/MultiTechSystems/Sx1280-LoRaWAN-Test-App)** repository by **MultiTech Systems**, which provided a simple LoRaWAN ISM2400 stack for the SX1280 driver in polling mode. Significant modifications were made to port the original Mbed OS-based code to the Arduino/ESP32 environment, adapt it for the LilyGO T3 S3 development board, and add the LoRaWAN MAC layer (frame assembly, AES-128 encryption, MIC computation).

The underlying SX1280 driver itself comes from **Semtech** (BSD license, © 2016 Semtech).

## Documentation

The full project documentation, design rationale, and field test results are available in the bachelor's thesis and on the project page at <https://lora.vsb.cz>.

## License

The original Semtech and MultiTech Systems source files retain their original licenses (Revised BSD License). The custom additions and modifications by Maxmilián Babič are released under the same terms.
