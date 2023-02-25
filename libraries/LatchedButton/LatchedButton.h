#ifndef LatchedButton_h
#define LatchedButton_h
#include "Arduino.h"

#define HELD 0x2
#define PRESSED 0x1
#define NOT_PRESSED 0x0

class LatchedButton
{
public:
    LatchedButton(uint8_t resetPin, uint8_t readPin);
    LatchedButton(uint8_t resetPin, uint8_t readPin, uint16_t holdDelay);
    uint8_t isPressed();

private:
    uint8_t _resetPin;
    uint8_t _readPin;
    uint16_t _holdDelay;
    uint16_t _consecutivePressed;
};

#endif
