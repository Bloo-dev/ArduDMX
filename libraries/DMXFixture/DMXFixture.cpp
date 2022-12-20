#include "Arduino.h"
#include "../Conceptinetics/Conceptinetics.h"
#include "DMXFixture.h"

DMXFixture::DMXFixture(uint16_t startChannel, uint8_t dimmerDefaultValue) : _redChannel(startChannel + localRedChannel - 1), _greenChannel(startChannel + localGreenChannel - 1), _blueChannel(startChannel + localBlueChannel - 1), _whiteChannel(startChannel + localWhiteChannel - 1), _dimmerChannel(startChannel + localDimmerChannel - 1), _strobeChannel(startChannel + localStrobeChannel - 1), _dimmerDefaultValue(dimmerDefaultValue)
{
}

void DMXFixture::setRGB(uint8_t redValue, uint8_t greenValue, uint8_t blueValue)
{
    _redValue = redValue;
    _greenValue = greenValue;
    _blueValue = blueValue;
}

void DMXFixture::setWhite(uint8_t whiteValue)
{
    _whiteValue = whiteValue;
}

void DMXFixture::setDimmer(uint8_t dimmerValue)
{
    _dimmerValue = dimmerValue;
}

void DMXFixture::setStrobe(uint8_t strobeValue)
{
    _strobeValue = strobeValue;
}

void DMXFixture::reset()
{
    _dimmerValue = _dimmerDefaultValue;
    _redValue = 0;
    _greenValue = 0;
    _blueValue = 0;
    _whiteValue = 0;
    _strobeValue = 0;
}

void DMXFixture::display(DMX_Master &dmxController)
{
    dmxController.setChannelValue(_dimmerChannel, _dimmerValue);
    dmxController.setChannelValue(_redChannel, _redValue);
    dmxController.setChannelValue(_greenChannel, _greenValue);
    dmxController.setChannelValue(_blueChannel, _blueValue);
    dmxController.setChannelValue(_whiteChannel, _whiteValue);
    dmxController.setChannelValue(_strobeChannel, _strobeValue);
}
