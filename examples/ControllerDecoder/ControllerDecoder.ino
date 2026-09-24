#include <Arduino.h>
#include <ParkingController.h>

const uint8_t CONTROLLER_PIN = 32;

ParkingController controller(CONTROLLER_PIN);

void printReading(const char *name, uint8_t tenths) {
    Serial.print(name);
    Serial.print('=');

    if (ParkingController::isNoReading(tenths)) {
        Serial.print("--");
    } else if (ParkingController::isClose(tenths)) {
        Serial.print("close");
    } else {
        Serial.printf("%u.%u", tenths / 10, tenths % 10);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("--- Ultrasonic Parking Controller Decoder ---");
    if (!controller.begin()) {
        Serial.println("Controller decoder could not start.");
    }
}

void loop() {
    ParkingControllerFrame frame;
    if (!controller.read(frame)) {
        return;
    }

    printReading("A Left outer", frame.leftA);
    Serial.print("  ");
    printReading("B Left mid", frame.leftB);
    Serial.print("  ");
    printReading("C Right mid", frame.rightB);
    Serial.print("  ");
    printReading("D Right outer", frame.rightA);
    Serial.printf("  invalid=%lu\n", controller.invalidFrameCount());
}
