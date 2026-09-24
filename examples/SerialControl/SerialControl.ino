// Interactive serial example for the ultrasonic parking sensor display driver.
#include <Arduino.h>
#include <ParkingDisplay.h>
#include <stdio.h>

const uint8_t DISPLAY_PIN = 32;

ParkingDisplay display(DISPLAY_PIN);

void printStatus() {
    static const ParkingDisplay::Sensor order[] = {
        ParkingDisplay::LEFT_A,
        ParkingDisplay::LEFT_B,
        ParkingDisplay::RIGHT_B,
        ParkingDisplay::RIGHT_A,
    };
    static const char *const labels[] = {
        "A Left outer",
        "B Left mid",
        "C Right mid",
        "D Right outer",
    };

    Serial.print("Sensors [A left outer, B left mid, C right mid, D right outer]: [");
    for (uint8_t position = 0; position < ParkingDisplay::SENSOR_COUNT; ++position) {
        if (position > 0) {
            Serial.print(", ");
        }

        const uint8_t value = display.getSensorTenths(order[position]);
        Serial.print(labels[position]);
        Serial.print('=');
        if (value == ParkingDisplay::NO_READING) {
            Serial.print("--");
        } else if (value <= ParkingDisplay::CLOSE_MAX_TENTHS) {
            Serial.print("close");
        } else {
            Serial.printf("%u.%u", value / 10, value % 10);
        }
    }
    Serial.printf("] %s\n", display.isEnabled() ? "running" : "paused");
}

void setAll(uint8_t tenths) {
    display.setDistancesTenths(tenths, tenths, tenths, tenths);
}

void setSide(bool left, uint8_t tenths) {
    if (left) {
        display.setSensorTenths(ParkingDisplay::LEFT_A, tenths);
        display.setSensorTenths(ParkingDisplay::LEFT_B, tenths);
    } else {
        display.setSensorTenths(ParkingDisplay::RIGHT_A, tenths);
        display.setSensorTenths(ParkingDisplay::RIGHT_B, tenths);
    }
}

void isolateSensor(uint8_t sensor) {
    static const ParkingDisplay::Sensor controllerOrder[] = {
        ParkingDisplay::LEFT_A,
        ParkingDisplay::LEFT_B,
        ParkingDisplay::RIGHT_B,
        ParkingDisplay::RIGHT_A,
    };

    display.clear();
    display.setSensorTenths(controllerOrder[sensor], 3);
}

void adjustSensors(int8_t amount) {
    for (uint8_t sensor = 0; sensor < ParkingDisplay::SENSOR_COUNT; ++sensor) {
        const ParkingDisplay::Sensor id = static_cast<ParkingDisplay::Sensor>(sensor);
        const uint8_t value = display.getSensorTenths(id);
        if (value == ParkingDisplay::NO_READING) {
            continue;
        }

        const int16_t adjusted = static_cast<int16_t>(value) + amount;
        if (adjusted >= 0 && adjusted <= ParkingDisplay::MAX_DISTANCE_TENTHS) {
            display.setSensorTenths(id, adjusted);
        }
    }
}

bool setFourValues(const char *input) {
    float left_outer;
    float left_mid;
    float right_mid;
    float right_outer;
    char extra;

    if (sscanf(input, "%f %f %f %f %c", &left_outer, &left_mid, &right_mid, &right_outer, &extra) != 4) {
        return false;
    }

    display.setDistancesMeters(left_outer, left_mid, right_outer, right_mid);
    return true;
}

void handleSerial() {
    static bool reading_value = false;
    static bool reading_test_sensor = false;
    static bool reading_four_values = false;
    static bool has_digit = false;
    static uint16_t entered_value = 0;
    static char value_target = 'd';
    static char four_values[48];
    static size_t four_values_length = 0;

    while (Serial.available() > 0) {
        const char command = Serial.read();

        if (reading_four_values) {
            if (command == '\r' || command == '\n') {
                four_values[four_values_length] = '\0';
                if (setFourValues(four_values)) {
                    printStatus();
                } else {
                    Serial.println("Use: v 1.6 1.2 1.0 1.1");
                }
                reading_four_values = false;
                four_values_length = 0;
            } else if (four_values_length < sizeof(four_values) - 1) {
                four_values[four_values_length++] = command;
            }
            continue;
        }

        if (reading_test_sensor) {
            if (command >= '0' && command < '0' + ParkingDisplay::SENSOR_COUNT) {
                isolateSensor(command - '0');
                printStatus();
            } else if (command != '\r' && command != '\n') {
                Serial.println("Test channel must be t0, t1, t2, or t3.");
            }
            reading_test_sensor = false;
            continue;
        }

        if (reading_value) {
            if (command >= '0' && command <= '9') {
                entered_value = entered_value * 10 + (command - '0');
                has_digit = true;
            } else if (command == '\r' || command == '\n') {
                if (has_digit && entered_value <= ParkingDisplay::MAX_DISTANCE_TENTHS) {
                    if (value_target == 'l') {
                        setSide(true, entered_value);
                    } else if (value_target == 'r') {
                        setSide(false, entered_value);
                    } else {
                        setAll(entered_value);
                    }
                    printStatus();
                } else {
                    Serial.println("Value must be from 0 to 25; 0-2 means close object.");
                }
                reading_value = false;
                has_digit = false;
                entered_value = 0;
            }
            continue;
        }

        if (command == 'd' || command == 'D' ||
            command == 'l' || command == 'L' ||
            command == 'r' || command == 'R') {
            reading_value = true;
            has_digit = false;
            entered_value = 0;
            value_target = command | 0x20;
        } else if (command == 'v' || command == 'V') {
            reading_four_values = true;
            four_values_length = 0;
        } else if (command == 't' || command == 'T') {
            reading_test_sensor = true;
        } else if (command == '+') {
            adjustSensors(1);
            printStatus();
        } else if (command == '-') {
            adjustSensors(-1);
            printStatus();
        } else if (command == 'x' || command == 'X') {
            display.clear();
            printStatus();
        } else if (command == 'p' || command == 'P') {
            display.setEnabled(!display.isEnabled());
            printStatus();
        } else if (command == 's' || command == 'S') {
            printStatus();
        }
    }
}

void setup() {
    Serial.begin(115200);
    display.begin();
    display.setDistancesMeters(1.6f, 1.6f, 1.6f, 1.6f);

    delay(1000);
    Serial.println("--- Ultrasonic Parking Display Library Example ---");
    Serial.println("v 1.6 1.2 1.1 1.0 = set A left outer, B left mid, C right mid, D right outer in metres");
    Serial.println("d16 = all, l16 = left, r16 = right, t0-t3 = isolate A-D");
    Serial.println("0-2 = close, x = no readings, +/- = adjust, p = pause, s = status");
    printStatus();
}

void loop() {
    handleSerial();
    display.update();
}
