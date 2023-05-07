#include <AudioAnalyzer.h>

Analyzer FFT1(7, 4, 0); // MSGEQ7 on Mono|Left channel
Analyzer FFT2(7, 4, 1); // MSGEQ7 on Blank|Right channel

uint16_t amplitudesLeft[7];
uint16_t amplitudesRight[7];

void setup()
{
    Serial.begin(57600);
    FFT1.Init();
    FFT2.Init();
}

void loop()
{
    // Frequency(Hz):63  160  400  1K  2.5K  6.25K  16K
    // index:         0    1    2    3    4     5    6
    FFT1.ReadFreq(amplitudesLeft);
    FFT1.ReadFreq(amplitudesRight);
    for (int band = 0; band < 7; band++)
    {
        Serial.print(amplitudesLeft[band]);
        Serial.print(",");
        Serial.print(amplitudesRight[band]);
        if (band < 6)
            Serial.print(",");
        else
            Serial.println();
    }
    delay(20);
}
