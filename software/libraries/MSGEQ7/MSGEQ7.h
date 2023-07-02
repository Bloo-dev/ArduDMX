#ifndef MSGEQ7_h
#define MSGEQ7_h
#include "Arduino.h"

class MSGEQ7
{
public:
    MSGEQ7(uint8_t strobePin, uint8_t resetPin, uint8_t dataPin);
    void init();
    void queryBands(uint16_t *targetArray);
    void queryBands(uint16_t *targetArray, const uint8_t samples, const uint8_t delayMs);

private:
    uint8_t _strobePin;
    uint8_t _resetPin;
    uint8_t _dataPin;

    void reset();
};

#endif
