/*
 * PROJECT: LoRaWAN 2.4 GHz Node Implementation
 * FILE: sx1280-hal.h
 * AUTHOR: Maxmilián Babič
 * DATE: December 2025
 * DESCRIPTION:
 * Hardware Abstraction Layer (HAL) header for Semtech SX1280.
 * Connects the generic Semtech driver logic with Arduino SPI and GPIO functions.
 */
#pragma once

#ifndef __SX1280_HAL_H__
#define __SX1280_HAL_H__

#include <Arduino.h>
#include <SPI.h>
#include "sx1280.h"


class SX1280Hal : public SX1280 {
public:
    // SPI variant
    SX1280Hal(int8_t mosi, int8_t miso, int8_t sclk,
              int8_t nss, int8_t busy,
              int8_t dio1, int8_t dio2, int8_t dio3,
              int8_t rst,
              RadioCallbacks_t* callbacks);

    virtual ~SX1280Hal();

    virtual void Reset(void) override;
    virtual void Wakeup(void) override;

    void SetSpiSpeed(uint32_t spiSpeed);

    virtual void WriteCommand(RadioCommands_t opcode, uint8_t* buffer, uint16_t size) override;
    virtual void ReadCommand(RadioCommands_t opcode, uint8_t* buffer, uint16_t size) override;

    virtual void WriteRegister(uint16_t address, uint8_t* buffer, uint16_t size) override;
    virtual void WriteRegister(uint16_t address, uint8_t value) override;

    virtual void ReadRegister(uint16_t address, uint8_t* buffer, uint16_t size) override;
    virtual uint8_t ReadRegister(uint16_t address) override;

    virtual void WriteBuffer(uint8_t offset, uint8_t* buffer, uint8_t size) override;
    virtual void ReadBuffer(uint8_t offset, uint8_t* buffer, uint8_t size) override;

    virtual uint8_t GetDioStatus(void) override;

protected:
    SPIClass* _spi;

    int8_t _pinMosi;
    int8_t _pinMiso;
    int8_t _pinSclk;
    int8_t _pinNss;
    int8_t _pinBusy;
    int8_t _pinDio1;
    int8_t _pinDio2;
    int8_t _pinDio3;
    int8_t _pinReset;

    void SpiInit(void);
    void IoIrqInit(DioIrqHandler irqHandler);
};

#endif
