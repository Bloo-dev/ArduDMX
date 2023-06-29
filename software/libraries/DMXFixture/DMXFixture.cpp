#include <Conceptinetics.h>
#include "DMXFixture.h"

DMXFixture::DMXFixture(uint8_t startChannel, uint8_t dimmerDefaultValue) : _startChannel(startChannel), _dimmerDefaultValue(dimmerDefaultValue)
{
}

void DMXFixture::setRGB(uint8_t redValue, uint8_t greenValue, uint8_t blueValue)
{
    _redValue = redValue;
    _greenValue = greenValue;
    _blueValue = blueValue;
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
    _rgbDimmerValue = 255;
    _redValue = 0;
    _greenValue = 0;
    _blueValue = 0;
    _whiteValue = 0;
    _strobeValue = 0;
}

void DMXFixture::display(DMX_Master &dmxController)
{
    dmxController.setChannelValue(_startChannel + localDimmerChannel, _dimmerValue);
    dmxController.setChannelValue(_startChannel + localRedChannel, (uint8_t) (_redValue * ((float)_rgbDimmerValue / 255.0)));
    dmxController.setChannelValue(_startChannel + localGreenChannel, (uint8_t) (_greenValue * ((float)_rgbDimmerValue / 255.0)));
    dmxController.setChannelValue(_startChannel + localBlueChannel, (uint8_t) (_blueValue * ((float)_rgbDimmerValue / 255.0)));
    dmxController.setChannelValue(_startChannel + localWhiteChannel, _whiteValue);
    dmxController.setChannelValue(_startChannel + localStrobeChannel, _strobeValue);
}


FixtureProfile::FixtureProfile(): _color(0x0), _frequency(0x0)
{
}

FixtureProfile::FixtureProfile(uint32_t color, uint32_t frequency): _color(color), _frequency(frequency)
{
}

uint32_t FixtureProfile::getHexColor()
{
    return _color;
}

uint32_t FixtureProfile::getHexFrequency()
{
    return _frequency;
}
