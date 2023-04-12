#ifndef LatchedButton_h
#define LatchedButton_h
#include "Arduino.h"

#define HELD 0x2
#define PRESSED 0x1
#define NOT_PRESSED 0x0

template <uint8_t RESET_PIN>
class LatchedButton
{
public:
    LatchedButton(uint8_t readPin);
    LatchedButton(uint8_t readPin, uint16_t holdDelay);
    uint8_t isPressed();
    static void resetLatch();

private:
    inline static bool _resetPinInitialized = false;
    uint8_t _readPin;
    uint16_t _holdDelay;
    uint16_t _consecutivePressed;
};

#include "LatchedButton.tpp"
#endif
