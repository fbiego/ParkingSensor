#pragma once

#include <Arduino.h>

// Transmits four ultrasonic sensor readings to the parking display.

class ParkingDisplay {
public:
    enum Sensor : uint8_t {
        LEFT_A = 0,
        RIGHT_A = 1,
        RIGHT_B = 2,
        LEFT_B = 3,
        SENSOR_COUNT = 4,
    };

    enum : uint8_t {
        CLOSE_MAX_TENTHS = 2,
        MIN_NUMERIC_TENTHS = 3,
        MAX_DISTANCE_TENTHS = 25,
        NO_READING = 0xFF,
    };

    explicit ParkingDisplay(uint8_t pin, uint32_t frameGapUs = 20000);

    void begin();
    void update();
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Values are tenths of a meter. 0-2 select the close-object indication,
    // 3-25 display 0.3-2.5, and NO_READING blanks that sensor.
    // Public order is grouped by side: left outer, left mid, right outer, right mid.
    void setDistancesTenths(uint8_t leftA, uint8_t leftB, uint8_t rightA, uint8_t rightB);
    void setSensorTenths(Sensor sensor, uint8_t tenths);

    // Negative, NaN, or values above 2.5 m are treated as no reading.
    void setDistancesMeters(float leftA, float leftB, float rightA, float rightB);
    void setSensorMeters(Sensor sensor, float meters);

    void setNoReading(Sensor sensor);
    void clear();
    uint8_t getSensorTenths(Sensor sensor) const;

private:
    static uint8_t encodeTenths(uint8_t tenths);
    static uint8_t encodeMeters(float meters);
    void writeFrame();

    uint8_t _pin;
    uint8_t _distances[SENSOR_COUNT];
    uint32_t _frameGapUs;
    uint32_t _lastFrameEndUs;
    bool _enabled;
    bool _begun;
};
