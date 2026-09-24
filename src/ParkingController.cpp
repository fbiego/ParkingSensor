#include "ParkingController.h"

namespace {
const uint32_t SYNC_HIGH_MIN_US = 1500;
const uint32_t SYNC_HIGH_MAX_US = 2400;
const uint32_t SYNC_LOW_MIN_US = 700;
const uint32_t SYNC_LOW_MAX_US = 1300;
const uint32_t SHORT_MIN_US = 50;
const uint32_t SHORT_MAX_US = 150;
const uint32_t LONG_MIN_US = 151;
const uint32_t LONG_MAX_US = 280;
}

ParkingController *ParkingController::_activeReceiver = nullptr;

ParkingController::ParkingController(uint8_t pin)
    : _pin(pin),
      _state(WAIT_SYNC_HIGH),
      _lastEdgeUs(0),
      _workingBits(0),
      _bitCount(0),
      _currentBit(false),
      _pendingWireBytes{NO_READING, NO_READING, NO_READING, NO_READING},
      _frameAvailable(false),
      _invalidFrames(0) {}

bool ParkingController::begin(uint8_t inputMode) {
    if (_activeReceiver != nullptr && _activeReceiver != this) {
        return false;
    }

    pinMode(_pin, inputMode);
    resetFrame();
    _lastEdgeUs = micros();
    _activeReceiver = this;
    attachInterrupt(digitalPinToInterrupt(_pin), interruptHandler, CHANGE);
    return true;
}

void ParkingController::end() {
    detachInterrupt(digitalPinToInterrupt(_pin));
    if (_activeReceiver == this) {
        _activeReceiver = nullptr;
    }
}

bool ParkingController::available() const {
    return _frameAvailable;
}

bool ParkingController::read(ParkingControllerFrame &frame) {
    noInterrupts();
    if (!_frameAvailable) {
        interrupts();
        return false;
    }

    const uint8_t leftA = _pendingWireBytes[0];
    const uint8_t rightA = _pendingWireBytes[1];
    const uint8_t rightB = _pendingWireBytes[2];
    const uint8_t leftB = _pendingWireBytes[3];
    _frameAvailable = false;
    interrupts();

    frame.leftA = leftA;
    frame.leftB = leftB;
    frame.rightA = rightA;
    frame.rightB = rightB;
    return true;
}

uint32_t ParkingController::invalidFrameCount() const {
    noInterrupts();
    const uint32_t count = _invalidFrames;
    interrupts();
    return count;
}

bool ParkingController::isClose(uint8_t tenths) {
    return tenths <= CLOSE_MAX_TENTHS;
}

bool ParkingController::isNoReading(uint8_t tenths) {
    return tenths == NO_READING || tenths > MAX_DISTANCE_TENTHS;
}

float ParkingController::toMeters(uint8_t tenths) {
    return isNoReading(tenths) ? NAN : tenths / 10.0f;
}

void IRAM_ATTR ParkingController::interruptHandler() {
    if (_activeReceiver != nullptr) {
        _activeReceiver->handleEdge();
    }
}

void IRAM_ATTR ParkingController::handleEdge() {
    const uint32_t now = micros();
    const uint32_t duration = now - _lastEdgeUs;
    const bool level = digitalRead(_pin);
    _lastEdgeUs = now;

    switch (_state) {
        case WAIT_SYNC_HIGH:
            if (!level && duration >= SYNC_HIGH_MIN_US && duration <= SYNC_HIGH_MAX_US) {
                _state = WAIT_SYNC_LOW;
            }
            break;

        case WAIT_SYNC_LOW:
            if (level && duration >= SYNC_LOW_MIN_US && duration <= SYNC_LOW_MAX_US) {
                _state = WAIT_LEAD_HIGH;
            } else {
                ++_invalidFrames;
                resetFrame();
            }
            break;

        case WAIT_LEAD_HIGH:
            if (!level && isShort(duration)) {
                _workingBits = 0;
                _bitCount = 0;
                _state = WAIT_DATA_LOW;
            } else {
                ++_invalidFrames;
                resetFrame();
            }
            break;

        case WAIT_DATA_LOW:
            if (!level || (!isShort(duration) && !isLong(duration))) {
                ++_invalidFrames;
                resetFrame();
                break;
            }

            _currentBit = isShort(duration);
            _workingBits = (_workingBits << 1) | (_currentBit ? 1U : 0U);
            _state = WAIT_DATA_HIGH;
            break;

        case WAIT_DATA_HIGH:
            if (level || (_currentBit ? !isLong(duration) : !isShort(duration))) {
                ++_invalidFrames;
                resetFrame();
                break;
            }

            ++_bitCount;
            if (_bitCount == 32) {
                _pendingWireBytes[0] = static_cast<uint8_t>(_workingBits >> 24);
                _pendingWireBytes[1] = static_cast<uint8_t>(_workingBits >> 16);
                _pendingWireBytes[2] = static_cast<uint8_t>(_workingBits >> 8);
                _pendingWireBytes[3] = static_cast<uint8_t>(_workingBits);
                _frameAvailable = true;
                resetFrame();
            } else {
                _state = WAIT_DATA_LOW;
            }
            break;
    }
}

void IRAM_ATTR ParkingController::resetFrame() {
    _state = WAIT_SYNC_HIGH;
    _workingBits = 0;
    _bitCount = 0;
    _currentBit = false;
}

bool IRAM_ATTR ParkingController::isShort(uint32_t durationUs) {
    return durationUs >= SHORT_MIN_US && durationUs <= SHORT_MAX_US;
}

bool IRAM_ATTR ParkingController::isLong(uint32_t durationUs) {
    return durationUs >= LONG_MIN_US && durationUs <= LONG_MAX_US;
}
