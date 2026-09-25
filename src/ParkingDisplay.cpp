#include "ParkingDisplay.h"

// Ultrasonic parking display transmitter implementation.

#include <math.h>

namespace {
const uint16_t SYNC_HIGH_US = 1900;
const uint16_t SYNC_LOW_US = 1000;
const uint16_t BIT_SHORT_US = 100;
const uint16_t BIT_LONG_US = 200;
}

ParkingDisplay::ParkingDisplay(uint8_t pin, uint32_t frameGapUs)
    : _pin(pin),
      _distances{NO_READING, NO_READING, NO_READING, NO_READING},
      _frameGapUs(frameGapUs),
      _lastFrameEndUs(0),
      _enabled(true),
      _begun(false) {}

void ParkingDisplay::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
    _lastFrameEndUs = micros() - _frameGapUs;
    _begun = true;
}

void ParkingDisplay::update() {
    if (!_begun || !_enabled) {
        return;
    }

    if (static_cast<uint32_t>(micros() - _lastFrameEndUs) >= _frameGapUs) {
        writeFrame();
        _lastFrameEndUs = micros();
    }
}

void ParkingDisplay::setEnabled(bool enabled) {
    _enabled = enabled;
    if (!enabled && _begun) {
        digitalWrite(_pin, LOW);
    }
}

bool ParkingDisplay::isEnabled() const {
    return _enabled;
}

void ParkingDisplay::setDistancesTenths(
    uint8_t sensorA,
    uint8_t sensorB,
    uint8_t sensorC,
    uint8_t sensorD
) {
    _distances[SENSOR_A] = encodeTenths(sensorA);
    _distances[SENSOR_B] = encodeTenths(sensorB);
    _distances[SENSOR_C] = encodeTenths(sensorC);
    _distances[SENSOR_D] = encodeTenths(sensorD);
}

void ParkingDisplay::setSensorTenths(Sensor sensor, uint8_t tenths) {
    if (sensor < SENSOR_COUNT) {
        _distances[sensor] = encodeTenths(tenths);
    }
}

void ParkingDisplay::setDistancesMeters(float sensorA, float sensorB, float sensorC, float sensorD) {
    _distances[SENSOR_A] = encodeMeters(sensorA);
    _distances[SENSOR_B] = encodeMeters(sensorB);
    _distances[SENSOR_C] = encodeMeters(sensorC);
    _distances[SENSOR_D] = encodeMeters(sensorD);
}

void ParkingDisplay::setSensorMeters(Sensor sensor, float meters) {
    if (sensor < SENSOR_COUNT) {
        _distances[sensor] = encodeMeters(meters);
    }
}

void ParkingDisplay::setNoReading(Sensor sensor) {
    if (sensor < SENSOR_COUNT) {
        _distances[sensor] = NO_READING;
    }
}

void ParkingDisplay::clear() {
    for (uint8_t sensor = 0; sensor < SENSOR_COUNT; ++sensor) {
        _distances[sensor] = NO_READING;
    }
}

uint8_t ParkingDisplay::getSensorTenths(Sensor sensor) const {
    return sensor < SENSOR_COUNT ? _distances[sensor] : NO_READING;
}

uint8_t ParkingDisplay::encodeTenths(uint8_t tenths) {
    return tenths <= MAX_DISTANCE_TENTHS ? tenths : NO_READING;
}

uint8_t ParkingDisplay::encodeMeters(float meters) {
    if (isnan(meters) || meters < 0.0f || meters > 2.5f) {
        return NO_READING;
    }

    return static_cast<uint8_t>(meters * 10.0f + 0.5f);
}

void ParkingDisplay::writeFrame() {
    static const Sensor wireOrder[SENSOR_COUNT] = {
        SENSOR_A,
        SENSOR_D,
        SENSOR_C,
        SENSOR_B,
    };

    noInterrupts();

    digitalWrite(_pin, HIGH);
    delayMicroseconds(SYNC_HIGH_US);
    digitalWrite(_pin, LOW);
    delayMicroseconds(SYNC_LOW_US);

    digitalWrite(_pin, HIGH);
    delayMicroseconds(BIT_SHORT_US);

    for (uint8_t sensor = 0; sensor < SENSOR_COUNT; ++sensor) {
        for (int8_t bit = 7; bit >= 0; --bit) {
            const bool one = bitRead(_distances[wireOrder[sensor]], bit);

            digitalWrite(_pin, LOW);
            delayMicroseconds(one ? BIT_SHORT_US : BIT_LONG_US);
            digitalWrite(_pin, HIGH);
            delayMicroseconds(one ? BIT_LONG_US : BIT_SHORT_US);
        }
    }

    digitalWrite(_pin, LOW);
    interrupts();
}
