#include "LatchedButton.h"
#include "Arduino.h"

LatchedButton::LatchedButton(uint8_t resetPin, uint8_t readPin)
{
    LatchedButton(resetPin, readPin, 65536);
}

LatchedButton::LatchedButton(uint8_t resetPin, uint8_t readPin, uint16_t holdDelay) : _resetPin(resetPin), _readPin(readPin), _holdDelay(holdDelay), _consecutivePressed(0)
{
    pinMode(_resetPin, OUTPUT);
    digitalWrite(_resetPin, HIGH);
    pinMode(_readPin, INPUT);
}

/**
 * @brief Checks if the latched button is considered "NOT_PRESSED" (=0x0), "PRESSED" (=0x1), or "HELD" (=0x2).
 * A latched button is considered pressed when reading its readPin returns HIGH after having returned LOW on the previous read.
 * Also implements a "hold" feature, where holding down a latched button will count as consecutive presses.
 * For this, a holdDelay must be defined.
 * Note that (bool) of this return value is FALSE if the button is "NOT_PRESSED" and TRUE if the button is "PRESSED or "HELD".
 *
 * @return uint8_t "NOT_PRESSED" (=0x0), "PRESSED" (=0x1), or "HELD" (=0x2), depending on the state of the latch:
 * - "PRESSED" (=0x1) is returned if the button was just pressed (i.e. if the button was not pressed before when the method was last called).
 * - "HELD" (=0x2) if the button was held down for at least _holdDelay queries.                      
 * - "NOT_PRESSED" (=0x0) otherwise. I.e. if the button is not pressed, or if the button is held down, but _holdDelay was not yet reached.
 */
uint8_t LatchedButton::isPressed()
{
    bool currentState = digitalRead(_readPin);

    if (currentState == NOT_PRESSED)
    {
        _consecutivePressed = 0;
        return NOT_PRESSED;
    }

    // reset latch and count press
    digitalWrite(_resetPin, LOW);
    digitalWrite(_resetPin, HIGH);
    _consecutivePressed++;

    if (_consecutivePressed == 1)
        return PRESSED;

    if (_consecutivePressed >= _holdDelay)
    {
        _consecutivePressed = _holdDelay;
        return HELD;
    }

    return NOT_PRESSED;
}
