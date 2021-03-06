#include "ResponsiveAnalogRead.h"
#include "SystemController.h"

constexpr unsigned potPin = A0;
ResponsiveAnalogRead pot(potPin, false);

SystemController controller;


void setup()
{
    controller.begin();

    controller.setExposureDelay(250); // set exposure delay (time between two exposures) to 250 µs
    controller.shoot();               // do a manual exposure
    delay(10);
    controller.setExposureDelay(500); // same with 500µs delay between exposures
    controller.shoot();

    controller.continousMode(true);   // start continously shooting
    delay(1000);
    controller.continousMode(false);  // stop after one second

    delay(500);
    controller.continousMode(true);   // start again
}

void loop()
{
    // --> Uncomment if you have a control voltage on the pot pin  <--
    // pot.update();
    // if (pot.hasChanged())
    // {
    //     unsigned expDelay = map(pot.getValue(), 0, 1023, 100, 500); // 0-3.3V analog value, maps to 100-500
    //     controller.setExposureDelay(expDelay);
    //     Serial.printf("Exposure Delay: %u µs\n", expDelay);
    // }
}
