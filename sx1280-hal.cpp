/*
 * PROJECT: LoRaWAN 2.4 GHz Node Implementation
 * FILE: sx1280-hal.cpp
 * AUTHOR: Maxmilián Babič
 * DATE: December 2025
 * DESCRIPTION:
 * Implementation of Hardware Abstraction Layer for SX1280 on ESP32/Arduino.
 * Handles SPI communication and GPIO control.
 */
#include "sx1280-hal.h"

// Macros
#define WAIT_ON_BUSY()       while (digitalRead(_pinBusy) == HIGH) {}



//  Constructor / Destructor

SX1280Hal::SX1280Hal(int8_t mosi, int8_t miso, int8_t sclk,
                     int8_t nss, int8_t busy,
                     int8_t dio1, int8_t dio2, int8_t dio3,
                     int8_t rst,
                     RadioCallbacks_t* callbacks)
    : SX1280(callbacks),
      _spi(&SPI),
      _pinMosi(mosi),
      _pinMiso(miso),
      _pinSclk(sclk),
      _pinNss(nss),
      _pinBusy(busy),
      _pinDio1(dio1),
      _pinDio2(dio2),
      _pinDio3(dio3),
      _pinReset(rst)
{
    // Pin Inicialization
    pinMode(_pinNss, OUTPUT);
    digitalWrite(_pinNss, HIGH);

    pinMode(_pinReset, OUTPUT);
    digitalWrite(_pinReset, HIGH);

    pinMode(_pinBusy, INPUT);

    if (_pinDio1 >= 0) pinMode(_pinDio1, INPUT);
    if (_pinDio2 >= 0) pinMode(_pinDio2, INPUT);
    if (_pinDio3 >= 0) pinMode(_pinDio3, INPUT);

    SpiInit();
}

SX1280Hal::~SX1280Hal()
{
    
}


//  SPI basic functions                                 


void SX1280Hal::SpiInit(void)
{
    // Pin settings
    _spi->begin(_pinSclk, _pinMiso, _pinMosi, _pinNss);
    _spi->beginTransaction(SPISettings(8000000, MSBFIRST, SPI_MODE0));
}

void SX1280Hal::SetSpiSpeed(uint32_t spiSpeed)
{
    // SPI speed
    _spi->endTransaction();
    _spi->beginTransaction(SPISettings(spiSpeed, MSBFIRST, SPI_MODE0));
}

void SX1280Hal::Reset(void)
{
    // Radio hard reset
    pinMode(_pinReset, OUTPUT);
    digitalWrite(_pinReset, LOW);
    delay(50);
    digitalWrite(_pinReset, HIGH);
    delay(20);
    pinMode(_pinReset, INPUT_PULLUP);
}

void SX1280Hal::Wakeup(void)
{

    digitalWrite(_pinNss, LOW);
    _spi->transfer(RADIO_GET_STATUS);
    _spi->transfer(0x00);
    digitalWrite(_pinNss, HIGH);

    WAIT_ON_BUSY();
}


// Commands


void SX1280Hal::WriteCommand(RadioCommands_t command, uint8_t* buffer, uint16_t size)
{
    WAIT_ON_BUSY();

    digitalWrite(_pinNss, LOW);
    _spi->transfer((uint8_t)command);
    for (uint16_t i = 0; i < size; i++) {
        _spi->transfer(buffer[i]);
    }
    digitalWrite(_pinNss, HIGH);

    if (command != RADIO_SET_SLEEP) {
        WAIT_ON_BUSY();
    }
}

void SX1280Hal::ReadCommand(RadioCommands_t command, uint8_t* buffer, uint16_t size)
{
    WAIT_ON_BUSY();

    digitalWrite(_pinNss, LOW);

    if (command == RADIO_GET_STATUS) {
        buffer[0] = _spi->transfer((uint8_t)command);
        _spi->transfer(0x00);
        _spi->transfer(0x00);
    } else {
        _spi->transfer((uint8_t)command);
        _spi->transfer(0x00);
        for (uint16_t i = 0; i < size; i++) {
            buffer[i] = _spi->transfer(0x00);
        }
    }

    digitalWrite(_pinNss, HIGH);

    WAIT_ON_BUSY();
}


// Registers


void SX1280Hal::WriteRegister(uint16_t address, uint8_t* buffer, uint16_t size)
{
    WAIT_ON_BUSY();

    digitalWrite(_pinNss, LOW);
    _spi->transfer(RADIO_WRITE_REGISTER);
    _spi->transfer((address & 0xFF00) >> 8);
    _spi->transfer(address & 0x00FF);
    for (uint16_t i = 0; i < size; i++) {
        _spi->transfer(buffer[i]);
    }
    digitalWrite(_pinNss, HIGH);

    WAIT_ON_BUSY();
}

void SX1280Hal::WriteRegister(uint16_t address, uint8_t value)
{
    WriteRegister(address, &value, 1);
}

void SX1280Hal::ReadRegister(uint16_t address, uint8_t* buffer, uint16_t size)
{
    WAIT_ON_BUSY();

    digitalWrite(_pinNss, LOW);
    _spi->transfer(RADIO_READ_REGISTER);
    _spi->transfer((address & 0xFF00) >> 8);
    _spi->transfer(address & 0x00FF);
    _spi->transfer(0x00);
    for (uint16_t i = 0; i < size; i++) {
        buffer[i] = _spi->transfer(0x00);
    }
    digitalWrite(_pinNss, HIGH);

    WAIT_ON_BUSY();
}

uint8_t SX1280Hal::ReadRegister(uint16_t address)
{
    uint8_t v;
    ReadRegister(address, &v, 1);
    return v;
}


//  Payload buffer


void SX1280Hal::WriteBuffer(uint8_t offset, uint8_t* buffer, uint8_t size)
{
    WAIT_ON_BUSY();

    digitalWrite(_pinNss, LOW);
    _spi->transfer(RADIO_WRITE_BUFFER);
    _spi->transfer(offset);
    for (uint16_t i = 0; i < size; i++) {
        _spi->transfer(buffer[i]);
    }
    digitalWrite(_pinNss, HIGH);

    WAIT_ON_BUSY();
}

void SX1280Hal::ReadBuffer(uint8_t offset, uint8_t* buffer, uint8_t size)
{
    WAIT_ON_BUSY();

    digitalWrite(_pinNss, LOW);
    _spi->transfer(RADIO_READ_BUFFER);
    _spi->transfer(offset);
    _spi->transfer(0x00);
    for (uint16_t i = 0; i < size; i++) {
        buffer[i] = _spi->transfer(0x00);
    }
    digitalWrite(_pinNss, HIGH);

    WAIT_ON_BUSY();
}


//  Stav DIO pinů


uint8_t SX1280Hal::GetDioStatus(void)
{
    uint8_t d1 = (_pinDio1 >= 0) ? digitalRead(_pinDio1) : 0;
    uint8_t d2 = (_pinDio2 >= 0) ? digitalRead(_pinDio2) : 0;
    uint8_t d3 = (_pinDio3 >= 0) ? digitalRead(_pinDio3) : 0;
    uint8_t busy = digitalRead(_pinBusy);

    return (d3 << 3) | (d2 << 2) | (d1 << 1) | (busy << 0);
}


//  IRQ init


void SX1280Hal::IoIrqInit(DioIrqHandler irqHandler)
{
    // Not needed as of now
    (void)irqHandler;
}
