#ifndef DMXFixture_h
#define DMXFixture_h
#include "Arduino.h"
#include "../Conceptinetics/Conceptinetics.h"
class DMXFixture
{
public:
    static const uint8_t localDimmerChannel = 1;
    static const uint8_t localRedChannel = 2;
    static const uint8_t localGreenChannel = 3;
    static const uint8_t localBlueChannel = 4;
    static const uint8_t localWhiteChannel = 5;
    static const uint8_t localStrobeChannel = 6;
    static const uint8_t channelAmount = 6;
    DMXFixture(uint16_t startChannel, uint8_t dimmerDefaultValue);
    void setRGB(uint8_t redValue, uint8_t greenVale, uint8_t blueValue);
    void setWhite(uint8_t whiteValue);
    void setDimmer(uint8_t dimmerValue);
    void setRGBDimmer(uint8_t dimmerValue);
    void setStrobe(uint8_t strobeValue);
    void reset();
    void display(DMX_Master &dmxController);

private:
    uint16_t _startChannel;
    uint16_t _redChannel;
    uint16_t _greenChannel;
    uint16_t _blueChannel;
    uint16_t _whiteChannel;
    uint16_t _dimmerChannel;
    uint16_t _strobeChannel;
    uint8_t _dimmerDefaultValue;
    uint8_t _dimmerValue;
    uint8_t _rgbDimmerValue;
    uint8_t _redValue;
    uint8_t _greenValue;
    uint8_t _blueValue;
    uint8_t _whiteValue;
    uint8_t _strobeValue;
};
#endif
