#pragma once

#include <Arduino.h>

// Transmits four ultrasonic sensor readings to the parking display.

class ParkingDisplay {
public:
    // Controller labels: A left outer, B left mid, C right mid, D right outer.
    enum Sensor : uint8_t {
        SENSOR_A = 0,
        SENSOR_B = 1,
        SENSOR_C = 2,
        SENSOR_D = 3,
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
    // Argument order follows the controller labels: A, B, C, D.
    void setDistancesTenths(uint8_t sensorA, uint8_t sensorB, uint8_t sensorC, uint8_t sensorD);
    void setSensorTenths(Sensor sensor, uint8_t tenths);

    // Negative, NaN, or values above 2.5 m are treated as no reading.
    void setDistancesMeters(float sensorA, float sensorB, float sensorC, float sensorD);
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
