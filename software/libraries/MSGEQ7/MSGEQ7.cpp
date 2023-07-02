#include "MSGEQ7.h"
#include "Arduino.h"

/**
 * @brief Construct a new MSGEQ7 object.
 *
 * @param strobePin The arduino pin to which the strobe pin (pin 4 of the MSGEQ7) is connected.
 * @param resetPin The arduino pin to which the reset pin (pin 7 of the MSGEQ7) is connected.
 * @param dataPin The arduino analog pin to which the data pin (pin 3 of the MSGEQ7) is connected.
 */
MSGEQ7::MSGEQ7(uint8_t strobePin, uint8_t resetPin, uint8_t dataPin) : _strobePin(strobePin), _resetPin(resetPin), _dataPin(dataPin)
{
}

/**
 * @brief Initializes the MSGEQ7 chip and prepares the pins on the arduino connected to the MSGEQ7.
 * This MUST BE CALLED before the MSGEQ7 can function properly.
 * 
 */
void MSGEQ7::init()
{
    pinMode(_strobePin, OUTPUT);
    pinMode(_resetPin, OUTPUT);
    reset();
}

/**
 * @brief Takes readings from all the bands and stores them into the supplied targetArray.
 * This function also automatically sends a reset sequence to the MSGEQ7 every 2000ms. This technically no required,
 * but acts as a safety feature: The reset pulse forces the multiplexer on the MSGEQ7 back to the first band (63Hz).
 * So in case this code and the MSGEQ7 multiplexer ever get out of sync, the problem will fix itself after 2000ms.
 * This reset sequence is not sent on every call of the function as it takes a significant amount of time to execute.
 *
 * @param targetArray A length 7 uint16_t array into which the values obtained from the MSGEQ7 should be stored.
 * The entries are associated with the frequency bands of the MSGEQ7 in the following configuration:
 * Frequency(Hz):   63  160  400  1K  2.5K  6.25K  16K
 * targetArray[]:    0    1    2   3     4      5    6
 */
void MSGEQ7::queryBands(uint16_t *targetArray)
{
    static uint32_t lastResetMs = 0;
    if (millis() - lastResetMs > 2000) // Reset the MSGEQ7 every two seconds.
    {                                  // This forces the MSGEQ7 back onto the 63Hz band and is technically
        reset();                       // not required, but a good safety feature in case the MSGEQ7's
    }                                  // multiplexer and this code somehow get out of sync.

    for (uint8_t band = 0; band < 7; band++)
    {
        delayMicroseconds(10);
        targetArray[band] = analogRead(_dataPin);
        delayMicroseconds(50);
        digitalWrite(_strobePin, HIGH);
        delayMicroseconds(18);
        digitalWrite(_strobePin, LOW);
    }
}

/**
 * @brief Calles queryBands(uint16_t *targetArray) multiple times with a certain delay between calls.
 * The obtained data will be averaged before being stored into targetArray.
 *
 * For more information see MSGEQ7::queryBands(uint16_t *targetArray).
 *
 * @param targetArray The array to write the data to.
 * @param samples The amount of samples to take, i.e. how many times queryBands(uint16_t *targetArray) should be called.
 * @param delayMs The amount of time to wait between calls, in ms.
 */
void MSGEQ7::queryBands(uint16_t *targetArray, const uint8_t samples, const uint8_t delayMs)
{
    uint16_t averageAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
    for (uint8_t samplesTaken = 0; samplesTaken < samples; samplesTaken++)
    {
        uint16_t sampleAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
        queryBands(sampleAmplitudes);

        averageAmplitudes[0] += sampleAmplitudes[0]; // sum up samples
        averageAmplitudes[1] += sampleAmplitudes[1];
        averageAmplitudes[2] += sampleAmplitudes[2];
        averageAmplitudes[3] += sampleAmplitudes[3];
        averageAmplitudes[4] += sampleAmplitudes[4];
        averageAmplitudes[5] += sampleAmplitudes[5];
        averageAmplitudes[6] += sampleAmplitudes[6];

        delay(delayMs); // wait before acquisition of next sample
    }

    targetArray[0] = averageAmplitudes[0] / samples; // calculate averages and store to target array
    targetArray[1] = averageAmplitudes[1] / samples;
    targetArray[2] = averageAmplitudes[2] / samples;
    targetArray[3] = averageAmplitudes[3] / samples;
    targetArray[4] = averageAmplitudes[4] / samples;
    targetArray[5] = averageAmplitudes[5] / samples;
    targetArray[6] = averageAmplitudes[6] / samples;
}

/**
 * @brief Resets the MSGEQ7. This forces the MSGEQ7's multiplexer back to the first frequency (63Hz).
 * 
 */
void MSGEQ7::reset()
{
    digitalWrite(_strobePin, LOW);
    digitalWrite(_resetPin, HIGH);
    digitalWrite(_strobePin, HIGH);
    digitalWrite(_strobePin, LOW);
    digitalWrite(_resetPin, LOW);
    delayMicroseconds(72);
}
