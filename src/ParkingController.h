#pragma once

#include <Arduino.h>

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

struct ParkingControllerFrame {
    uint8_t sensorA;
    uint8_t sensorB;
    uint8_t sensorC;
    uint8_t sensorD;
};

class ParkingController {
public:
    static const uint8_t NO_READING = 0xFF;
    static const uint8_t CLOSE_MAX_TENTHS = 2;
    static const uint8_t MAX_DISTANCE_TENTHS = 25;

    explicit ParkingController(uint8_t pin);

    bool begin(uint8_t inputMode = INPUT);
    void end();
    bool available() const;
    bool read(ParkingControllerFrame &frame);
    uint32_t invalidFrameCount() const;

    static bool isClose(uint8_t tenths);
    static bool isNoReading(uint8_t tenths);
    static float toMeters(uint8_t tenths);

private:
    enum State : uint8_t {
        WAIT_SYNC_HIGH,
        WAIT_SYNC_LOW,
        WAIT_LEAD_HIGH,
        WAIT_DATA_LOW,
        WAIT_DATA_HIGH,
    };

    static void IRAM_ATTR interruptHandler();
    void IRAM_ATTR handleEdge();
    void IRAM_ATTR resetFrame();
    static bool IRAM_ATTR isShort(uint32_t durationUs);
    static bool IRAM_ATTR isLong(uint32_t durationUs);

    static ParkingController *_activeReceiver;

    uint8_t _pin;
    volatile State _state;
    volatile uint32_t _lastEdgeUs;
    volatile uint32_t _workingBits;
    volatile uint8_t _bitCount;
    volatile bool _currentBit;
    volatile uint8_t _pendingWireBytes[4];
    volatile bool _frameAvailable;
    volatile uint32_t _invalidFrames;
};
