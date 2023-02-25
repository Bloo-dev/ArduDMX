#ifndef LatchedButton_h
#define LatchedButton_h
#include "Arduino.h"

#define HELD 0x2
#define PRESSED 0x1
#define NOT_PRESSED 0x0

class LatchedButtonUnit
{
public:
    LatchedButtonUnit(uint8_t readPin);
    LatchedButtonUnit(uint8_t readPin, uint16_t holdDelay);
    uint8_t isPressed();

private:
    uint8_t _readPin;
    uint16_t _holdDelay;
    uint16_t _consecutivePressed;
};

template <uint8_t SIZE>
class LatchedButtonGroup
{
public:
    LatchedButtonGroup(uint8_t resetPin, uint8_t *readPins);
    LatchedButtonGroup(uint8_t resetPin, uint8_t *readPins, uint16_t *holdDelays);
    LatchedButtonGroup(uint8_t resetPin, LatchedButtonUnit* buttonUnits);
    uint8_t isPressed(uint8_t position);
    void reset();

private:
    uint8_t _resetPin;
    LatchedButtonUnit _buttonUnits[SIZE];
    void initResetPin();
};

class LatchedButton
{
public:
    LatchedButton(uint8_t resetPin, uint8_t readPin);
    LatchedButton(uint8_t resetPin, uint8_t readPin, uint16_t holdDelay);
    LatchedButton(uint8_t resetPin, LatchedButtonUnit buttonUnit);
    uint8_t isPressed();

private:
    LatchedButtonGroup<1> _buttonUnit;
};

#include "LatchedButton.tpp"
#endif
