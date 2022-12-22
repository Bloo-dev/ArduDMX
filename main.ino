#include <AudioAnalyzer.h>
#include <Conceptinetics.h>
#include <DMXFixture.h>

// ===== GLOBAL SETTINGS ======
// Light Fixture Data
const uint8_t maxBrightness = 217;                                                    // 85% max brightness to increase LED lifetime
DMXFixture fixtures[] = {DMXFixture(1, maxBrightness), DMXFixture(7, maxBrightness)}; // configured fixtures and their start channels.
const uint32_t fixtureColors[] = {0xFF0000, 0x0000FF};                                // colors for the configured fixtures to start out with, in order, stored in hex. Should be normalized to avoid differing fixture brightness values from fixture to fixture.
const uint32_t fixtureFrequencies[] = {0xFF00000, 0x000FFF0};                         // frequency responses of the fixtures stored in hex. Each digit corresponds to a frequency band, meaning each frequency band can have a response between 15 (max) and 0 (min).
const uint16_t fixtureAmount = sizeof(fixtures) / sizeof(DMXFixture);
// MSGEQ7 Signal Data
const uint8_t samplesPerRun = 16;       // number of consecutive samples to take whenever the audio is sampled (these are then averaged). Higher values inhibit random noise spikes.
const uint16_t delayBetweenSamples = 1; // time in ms to wait between samples in a consecutive sample run. High values will decrease temporal resolution drastically.
// const float signalAmplification = 6.0;  // amplification of the signal before it is sent to the light fixtures. Amplifies the signal (0..1024) by this factor (0.0..10.0). This happens AFTER the noise cutoff, meaning any signals lower than noiseCutoff won't be amplified (stay 0).
//  =============================
// Settings for basement: noiseLevel=200, signalAmplification=6.0

// ===== GLOBAL VARIABLES ======
// DMX Hardware
DMX_Master dmxMaster(fixtures[0].channelAmount *fixtureAmount, 2);
// FFT Hardware
Analyzer MSGEQ7 = Analyzer(6, 7, 0);
uint16_t frequencyAmplitudes[7]; // stores data from MSGEQ7 chip
uint16_t noiseLevel = 200;         // lower bound for noise, managed automatically
// Auto Gain
uint16_t averageAmplitudeHistory[64]; // stores the history of the cross-band average amplitude
uint8_t currentHistoryEntry = 0;
float amplificationFactor = 6.0;
// =============================

void setup()
{
    // Start FFT
    MSGEQ7.Init();

    // Start DMX
    dmxMaster.setAutoBreakMode();
    dmxMaster.enable();

    // Initialize Light Fixtures
    for (int i = 0; i < fixtureAmount; ++i)
    {
        fixtures[i].reset(); // reset to default values
    }

    // Analyze Noise Levels (THERE MUST NOT BE AUDIO ON THE JACK FOR THIS TO WORK)
    int noiseData[] = {0, 0, 0, 0, 0, 0, 0};
    sampleMSGEQ7(32, 1, noiseData);
    noiseLevel = getAverage(noiseData, 7, 12); // average over all frequencies and add some extra buffer

    delay(500); // wait a bit for everything to stabalize
}

void loop()
{
    // get FFT data from MSGEQ7 chip
    sampleMSGEQ7(samplesPerRun, delayBetweenSamples, frequencyAmplitudes);
    transformAudioSignal(noiseLevel, amplificationFactor, frequencyAmplitudes);

    // remember amplitude history for auto-gain
    //averageAmplitudeHistory[currentHistoryEntry++] = getAverage(frequencyAmplitudes, 7, 0);
    //if (currentHistoryEntry > 63)
    //    currentHistoryEntry = 0;
    //amplificationFactor = autoGain(getAverage(averageAmplitudeHistory, 64, 0), 84);

    // Cycle Fixtures
    uint32_t transformedColorResponseTable[fixtureAmount];
    uint32_t transformedAudioResponseTable[fixtureAmount];
    transformResponseTables(fixtureColors, transformedColorResponseTable, fixtureFrequencies, transformedAudioResponseTable, fixtureAmount);

    // Manage Fixtures
    for (uint16_t fixtureId = 0; fixtureId < fixtureAmount; fixtureId++)
    {
        setFixtureColor(fixtures[fixtureId], frequencyAmplitudes, transformedColorResponseTable[fixtureId]);
        setFixtureBrightness(fixtures[fixtureId], frequencyAmplitudes, transformedAudioResponseTable[fixtureId]);

        // send data to fixtures
        fixtures[fixtureId].display(dmxMaster);
    }
}

float autoGain(uint16_t currentMean, uint16_t targetMean)
{

    return 1.0 + ((targetMean - currentMean) / 255.0);
}

/**
 * @brief Gets the average value of an array.
 *
 * @param array Array of values to be averaged.
 * @param elements [0..63] The amount of elements in the array.
 * @param buffer [0..1023] Buffer value to be added onto the average after calculation.
 * @return Arithmetic average of the signal levels on all 7 bands plus the buffer value. Capped at 1023.
 */
uint16_t getAverage(int *array, uint16_t elements, uint16_t buffer)
{
    uint16_t sum = 0;
    for (int i = 0; i < elements; i++)
    {
        sum += array[i];
    }

    return min(buffer + (sum / elements), 1023);
}

/**
 * @brief Transforms a given 12-bit audio signal to an 8 -it signal that can be used to control DMXFixtures. Also performs some cleanup on the signal, like removing noise and scaling the signal to use the entire 8-bit space.
 *
 * @param noiseLevel [0..1023] Signals to be ignored due to insufficient signal level. Signals below this threshold will be set to 0. Is applied to the raw signal [0..1023] before the amplification factor.
 * @param amplificationFactor [0.0..10.0] (recommended) Amplification factor to be applied to the signal. Is applied to the raw signal [0..1023] after the noise level was subtracted, meaning noise is not amplified.
 * @param targetArray The array holding the audio data to be modified. Should have seven (7) entries.
 */
void transformAudioSignal(uint16_t noiseLevel, float amplificationFactor, int *targetArray)
{
    for (uint8_t band = 0; band < 7; band++)
    {
        targetArray[band] = (int)(min(max(max((int32_t)targetArray[band] - noiseLevel, 0) * amplificationFactor, 0), 1023) / 4); // Cut-off noise, amplify and write to array
    }
}

/**
 * @brief Gets values from all seven bands provided by the MSGEQ7 spectrum analyzer chip and stores the results for all seven bands to the target array.
 *
 * @param sampleAmount [0..63] The amount of samples to be taken. For each band, the samples taken will be summed up and averaged.
 * @param sampleDelay [0..65535] Delay between taking samples. This is added on-top of the run time of a sample capture.
 * @param targetArray The array to store the resulting data to. Should have at least seven elements.
 */
void sampleMSGEQ7(int8_t sampleAmount, uint16_t sampleDelay, int *targetArray)
{
    uint16_t averageAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
    for (uint8_t sample_count = 0; sample_count < sampleAmount; sample_count++)
    {
        uint16_t sampleAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
        MSGEQ7.ReadFreq(sampleAmplitudes); // store amplitudes of frequency bands into array
                                           // Frequency(Hz):        63  160  400  1K  2.5K  6.25K  16K
                                           // frequencyAmplitudes[]: 0    1    2   3     4      5    6

        averageAmplitudes[0] += sampleAmplitudes[0]; // sum up samples
        averageAmplitudes[1] += sampleAmplitudes[1];
        averageAmplitudes[2] += sampleAmplitudes[2];
        averageAmplitudes[3] += sampleAmplitudes[3];
        averageAmplitudes[4] += sampleAmplitudes[4];
        averageAmplitudes[5] += sampleAmplitudes[5];
        averageAmplitudes[6] += sampleAmplitudes[6];

        delay(sampleDelay); // wait before acquisition of next sample
    }

    targetArray[0] = averageAmplitudes[0] / sampleAmount; // calculate averages and store to target array
    targetArray[1] = averageAmplitudes[1] / sampleAmount;
    targetArray[2] = averageAmplitudes[2] / sampleAmount;
    targetArray[3] = averageAmplitudes[3] / sampleAmount;
    targetArray[4] = averageAmplitudes[4] / sampleAmount;
    targetArray[5] = averageAmplitudes[5] / sampleAmount;
    targetArray[6] = averageAmplitudes[6] / sampleAmount;
}

/**
    @brief Transforms the response tables to cycle colors or change frequency response.
*/
void transformResponseTables(uint32_t *constColorResponseTable, uint32_t *shuffledColorResponseTable, uint32_t *constAudioResponseTable, uint32_t *shuffledAudioResponseTable, uint16_t fixtureAmount)
{
    // TODO find a way to transform the arrays (switch indices and/or remove some entries temporarily)
    // Figure out whether to return new, modified arrays or whether to modify in-place (would require a copy of the input arrays to be made before input)

    for (uint16_t fixtureId = 0; fixtureId < fixtureAmount; fixtureId++) // ID operation for testing purposes (colors are not swapped)
    {
        shuffledColorResponseTable[fixtureId] = constColorResponseTable[fixtureId];
        shuffledAudioResponseTable[fixtureId] = constAudioResponseTable[fixtureId];
    }
}

/**
    @brief Sets the color of a single fixture according to the supplied color response values.

    @param &targetFixture Fixture to be adjusted.
    @param *audioAmplitudes 7 element uint32_t array of amplitudes per frequency band.
    @param colorResponse [..0xFFFFFF] hex value that represents the color to be displayed by this fixture.
*/
void setFixtureColor(DMXFixture &targetFixture, int *audioAmplitudes, uint32_t colorResponse)
{
    // what else goes here?
    // -> setting white, strobe and so on depending on lever states, conversion hex->rgb

    // convert colors to rgb and send to fixture
    targetFixture.setRGB(colorResponse >> 16, (colorResponse & 0x00FF00) >> 8, colorResponse & 0x0000FF);
}

/**
    @brief Sets the brightness of a single fixture according to the supplied audio response values.

    @param targetFixture Fixture to be adjusted.
    @param audioAmplitudes 7 element uint32_t array of amplitudes per frequency band.
    @param audioResponse [..0xFFFFFFF] hex value that represents the frequencies this fixture should respond to.
*/
void setFixtureBrightness(DMXFixture &targetFixture, int *audioAmplitudes, uint32_t audioResponse)
{
    uint8_t brightness = 0;
    for (uint8_t band = 0; band < 7; band++)
    {
        if (((audioResponse & (0xF * 10 ^ band)) >> (band * 4)) >= 0) // TODO allow this to differnetiate between the 16 possible values for each response, also check whether the bit-shift math here checks out
        {
            brightness = max(audioAmplitudes[band], brightness);
        }
    }
    targetFixture.setRGBDimmer(brightness);
}
