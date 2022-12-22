#include "Arduino.h"
#include "../Conceptinetics/Conceptinetics.h"
#include "DMXFixture.h"

DMXFixture::DMXFixture(uint16_t startChannel, uint8_t dimmerDefaultValue) : _redChannel(startChannel + localRedChannel - 1), _greenChannel(startChannel + localGreenChannel - 1), _blueChannel(startChannel + localBlueChannel - 1), _whiteChannel(startChannel + localWhiteChannel - 1), _dimmerChannel(startChannel + localDimmerChannel - 1), _strobeChannel(startChannel + localStrobeChannel - 1), _dimmerDefaultValue(dimmerDefaultValue)
{
}

void DMXFixture::setRGB(uint8_t *rgbValue)
{
    _redValue = rgbValue[0];
    _greenValue = rgbValue[1];
    _blueValue = rgbValue[2];
}

void DMXFixture::setRGBDimmer(uint8_t dimmerValue)
{
    _rgbDimmerValue = dimmerValue;
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
    dmxController.setChannelValue(_redChannel, (uint8_t) (_redValue * ((float)_rgbDimmerValue / 255.0)));
    dmxController.setChannelValue(_greenChannel,(uint8_t) (_greenValue * ((float)_rgbDimmerValue / 255.0)));
    dmxController.setChannelValue(_blueChannel, (uint8_t) (_blueValue * ((float)_rgbDimmerValue / 255.0)));
    dmxController.setChannelValue(_whiteChannel, _whiteValue);
    dmxController.setChannelValue(_strobeChannel, _strobeValue);
}
