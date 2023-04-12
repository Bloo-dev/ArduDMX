#include <DMXFixture.h>

// Diagnostics script for testing DMX output.
// Running this script will cause the fixture on channel 1 to blink.

const uint8_t maxBrightness = 217; // 85% max brightness to increase LED lifetime
DMXFixture fixture = DMXFixture(1, maxBrightness);
DMX_Master dmxMaster(fixture.channelAmount, 2);
bool state = false;

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    // Start DMX
    dmxMaster.setAutoBreakMode();
    dmxMaster.enable();

    // Initialize Light Fixtures
    fixture.reset();

    delay(500);
}

void loop()
{
    if (state)
    {
      state = false;
    } else
    {
      state = true;
    }

    //Serial.println(state);
    fixture.setWhite(255 * (uint8_t) state);
    fixture.setDimmer(255);

    // send data to fixtures
    fixture.display(dmxMaster);
    digitalWrite(LED_BUILTIN, (uint8_t) state);
    delay(250); 
}
