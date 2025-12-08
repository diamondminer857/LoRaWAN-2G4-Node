# LoRaWAN 2.4 GHz SX1280 Node Implementation

This repository contains the source code for a custom **LoRaWAN 2.4 GHz** node implementation, developed as part of a Bachelor's thesis.

The project demonstrates a functional uplink communication on the 2.4 GHz ISM band using the **Semtech SX1280** radio module controlled by an **ESP32-S3** microcontroller. Unlike standard sub-GHz LoRaWAN, this implementation handles the specific physical layer requirements of the 2.4 GHz band.

## Hardware Setup

* **Node:** [LilyGO T3 S3](https://wiki.lilygo.cc/get_started/en/LoRa_GPS/T3S3/T3S3.html) (ESP32-S3 + Semtech SX1280 LoRa module)
* **Gateway:** iMST iM282A (Lite Gateway) connected to The Things Network (TTN)

## Key Configuration & Optimization

To ensure stable communication in the interference-heavy 2.4 GHz band (Wi-Fi/Bluetooth), this is the recommended configuration:

* Frequency - 2479 MHz - Located above standard Wi-Fi channels (ch 13) to minimize noise.
* Spreading Factor - SF7 - Reduces *Time-on-Air* to ~15ms, significantly lowering collision probability.
* Bandwidth - 812.5 kHz - Standard LoRaWAN 2.4 GHz bandwidth.
* Coding Rate - 4/5 - Default LoRaWAN coding rate.
* Activation - ABP

## How to Run

### 1. Prerequisites
* Install **Arduino IDE**.
* Install **ESP32 Board Support** package (by Espressif).
* No external LoRaWAN library is required (the stack is implemented directly in this repo).

### 2. Configuration
1.  Open `LilyGO_SX1280_LoRaWAN.ino`.
2.  Locate the **LoRaWAN KEYS** section.
3.  Replace the placeholder keys (`0x00...`) with your actual keys from The Things Network console:
    * `DEVADDR` (Device Address)
    * `NWK_S_KEY` (Network Session Key)
    * `APP_S_KEY` (Application Session Key)


### 3. Gateway Configuration
Standard LoRaWAN 2.4 GHz gateways are often pre-configured to listen only on **SF12**. For this project to work efficiently, the gateway must be set to the same SF setting.

### 4. Payload Formatter

This is the payload formatter which should be used in TTN.

 function decodeUplink(input) {
  var data = {};
  
  // Decodes text message from the node
  var text = "";
  for (var i = 0; i < input.bytes.length; i++) {
    text += String.fromCharCode(input.bytes[i]);
  }
  
  data.message = text;
  data.length = input.bytes.length;

  return {
    data: data,
    warnings: [],
    errors: []
  };
} 


## Acknowledgements & Credits

This project is largely based on the **[Sx1280-LoRaWAN-Test-App](https://github.com/MultiTechSystems/Sx1280-LoRaWAN-Test-App)** repository by **MultiTech Systems**.
Significant modifications were made to adapt the original Mbed-based code for the Arduino/ESP32 environment.
Special thanks for their work!

