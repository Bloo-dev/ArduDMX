#include "LatchedButton.h"
#include "Arduino.h"

/**
 * @brief Construct a new LatchedButton<RESET_PIN>::LatchedButton object. The holdDelay is set to 65535, which is assumed to
 * be unreachably high.
 *
 * @tparam RESET_PIN Pin used to reset the latch of the button. Note that this pin is automatically configured as an output
 * and pulled high the first time a LatchedButton object is constructed.
 * @param readPin Pin used to read the value of this button.
 */
template <uint8_t RESET_PIN>
LatchedButton<RESET_PIN>::LatchedButton(uint8_t readPin) : LatchedButton(readPin, 65535)
{
}

/**
 * @brief Construct a new LatchedButton<RESET_PIN>::LatchedButton object.
 *
 * @tparam RESET_PIN Pin used to reset the latch of the button. Note that this pin is automatically configured as an output
 * and pulled high the first time a LatchedButton object is constructed.
 * @param readPin Pin used to read the value of this button.
 * @param holdDelay Delay after "PRESSED" at which the press should be interpreted as "HELD". This delay is in units of queries
 * to the buttons state; e.g. if holdDelay is set to 3 the button is interpreted as "HELD" after 3 successive calls of
 * isPressed() determined the internal state of the button to be pressed, i.e. the state of the readPin is high for
 * 3 consecutive calls of isPressed().
 */
template <uint8_t RESET_PIN>
LatchedButton<RESET_PIN>::LatchedButton(uint8_t readPin, uint16_t holdDelay) : _readPin(readPin), _holdDelay(holdDelay), _consecutivePressed(0)
{
    if (!_resetPinInitialized)
    {
        pinMode(RESET_PIN, OUTPUT);
        digitalWrite(RESET_PIN, HIGH);
        _resetPinInitialized = true;
    }

    pinMode(_readPin, INPUT);
}

/**
 * @brief Checks if the latched button is considered "NOT_PRESSED" (=0x0), "PRESSED" (=0x1), or "HELD" (=0x2).
 * A latched button is considered pressed when reading its readPin returns HIGH after having returned LOW on the previous read.
 * Also implements a "hold" feature, where holding down a latched button will count as consecutive presses.
 * For this, a holdDelay must be defined.
 * Note that (bool) of this return value is FALSE if the button is "NOT_PRESSED" and TRUE if the button is "PRESSED or "HELD".
 *
 * @tparam RESET_PIN Pin used to reset the latch of the button. Note that this pin is automatically configured as an output
 * and pulled high the first time a LatchedButton object is constructed.
 * @return uint8_t "NOT_PRESSED" (=0x0), "PRESSED" (=0x1), or "HELD" (=0x2), depending on the state of the latch:
 * - "PRESSED" (=0x1) is returned if the button was just pressed (i.e. if the button was not pressed before when the method was last called).
 * - "HELD" (=0x2) if the button was held down for at least _holdDelay queries.
 * - "NOT_PRESSED" (=0x0) otherwise. I.e. if the button is not pressed, or if the button is held down, but _holdDelay was not yet reached.
 */
template <uint8_t RESET_PIN>
uint8_t LatchedButton<RESET_PIN>::isPressed()
{
    bool currentState = digitalRead(_readPin);

    if (currentState == NOT_PRESSED)
    {
        _consecutivePressed = 0;
        return NOT_PRESSED;
    }

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

/**
 * @brief Resets the latch of this LatchedButton and ALL LatchedButton instances that specify the same RESET_PIN.
 *
 * @tparam RESET_PIN Pin used to reset the latch of the button. Note that this pin is automatically configured as an output
 * and pulled high the first time a LatchedButton object is constructed.
 */
template <uint8_t RESET_PIN>
void LatchedButton<RESET_PIN>::resetLatch()
{
    digitalWrite(RESET_PIN, LOW);
    digitalWrite(RESET_PIN, HIGH);
}
