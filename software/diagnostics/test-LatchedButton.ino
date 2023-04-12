#include <LatchedButton.h>

int FRAME_TIME_MS = 10;
LatchedButton<8> buttonPlus(3, 1000 / FRAME_TIME_MS);
LatchedButton<8> buttonSelect(5 1000 / FRAME_TIME_MS);
LatchedButton<8> buttonMinus(6, 1000 / FRAME_TIME_MS);
LatchedButton<8> buttonFunction(9, 1000 / FRAME_TIME_MS);

uint16_t var = 0;
bool buttonPressed = true;

void setup()
{
    Serial.begin(56700);
}

void loop()
{
    if (buttonPlus.isPressed())
    {
        var++;
        buttonPressed = true;
    }

    if (buttonMinus.isPressed())
    {
        var--;
        buttonPressed = true;
    }

    if (buttonSelect.isPressed())
    {
        var = 0;
        buttonPressed = true;
    }

    if (buttonFunction.isPressed())
    {
        var *= 2;
        buttonPressed = true;
    }

    LatchedButton<8>::resetLatch();
    if (buttonPressed)
    {
        displayVar();
        buttonPressed = false;
    }
    delay(FRAME_TIME_MS);
}

void displayVar()
{
    Serial.println(var);
}
