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
const uint8_t noiseCutoff = 200;        // lower bound for what is considered noise. 0..1024. Higher values lead to flickery behaviour, but push silent parts of the signal to zero. Low values will cause random noise to create a signal without any actual audio signal.
const uint8_t samplesPerRun = 16;       // number of consecutive samples to take whenever the audio is sampled (these are then averaged). Higher values inhibit random noise spikes.
const uint16_t delayBetweenSamples = 1; // time in ms to wait between samples in a consecutive sample run. High values will decrease temporal resolution drastically.
const uint8_t interRunSmoothing = 0;    // smothing between frames. Requires clear signals, as level-zero signals will pull the channel low for a long time.
const float signalAmplification = 6.0;  // amplification of the signal before it is sent to the light fixtures. Amplifies the signal (0..1024) by this factor (0.0..10.0). This happens AFTER the noise cutoff, meaning any signals lower than noiseCutoff won't be amplified (stay 0).
// =============================

// ===== GLOBAL VARIABLES ======
// DMX Hardware
DMX_Master dmxMaster(fixtures[0].channelAmount * 6, 2);
// FFT Hardware
Analyzer MSGEQ7 = Analyzer(6, 7, 0);
uint16_t frequencyAmplitudes[7];
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

    delay(1000); // wait until data lines stabilize to minimize interference (some flickering does still occur after usb-disconnect)
}

void loop()
{
    // Analyze Audio
    readAudio(frequencyAmplitudes, samplesPerRun, delayBetweenSamples, interRunSmoothing, noiseCutoff, signalAmplification);

    // Cycle Fixtures
    transformResponseTables(fixtureColors, fixtureFrequencies, fixtureAmount);

    // Manage Fixtures
    for (uint16_t fixtureId = 0; fixtureId < fixtureAmount; fixtureId++)
    {
        setFixtureColor(fixtures[fixtureId], frequencyAmplitudes, fixtureColors[fixtureId]);
        setFixtureBrightness(fixtures[fixtureId], frequencyAmplitudes, fixtureFrequencies[fixtureId]);

        // send data to fixtures
        fixtures[fixtureId].display(dmxMaster);
    }
}

/*
    Reads audio from the MSGEQ7 chip and does some signal processing on it. The various parameters can be used to clearly distinguish the audio signal from noise.
    On each call of this method, the MSGEQ7 spectrum analyzer may be queried multiple times to provide some smoothing. One call of the function is also called a Run.

    Modfies the supplied array `*bands` in-place.
    @param *bands An int[7] array to store the read values into.
    @param sampleAmount [0..255] How many times the MSGEQ7 spectrum analyzer should be queried per run. Higher values will create a more stable, less spikey signal, but also reduce temporal resolution significantly.
    @param sampleDelay [0..65536] Delay between each sample in ms. This delay is in addition to the runtime of the smapling code.
    @param smoothing [0..255] Enables weighing the previous run's results against the new raw results. `0` disables this feature, `1` is a 1:1 weight, `2` a 1:2 weight for the old data, and so one. Higher values prevent spikes and flicker, but also require a strong signal as zero-level signals will pull the results down significantly.
    @param noiseLevel [0..1024] Cut-off point for data considered noise. Any incoming signal lwoer than this will be forced to 0 (before amplification).
    @param amplificationFactor [float, recommended 0.0..+10.0] Amplification factor for the signal. This is applied last, even after noiseLevel, meaning that noise (signals with level 0) is not amplified and stays 0.
*/
void readAudio(int *bands, uint8_t sampleAmount, uint16_t sampleDelay, uint8_t smoothing, uint16_t noiseLevel, float amplificationFactor)
{
    // if smoothing is desired, remember old amplitudes
    uint16_t oldAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
    if (smoothing > 0)
    {
        oldAmplitudes[0] = smoothing * bands[0];
        oldAmplitudes[1] = smoothing * bands[1];
        oldAmplitudes[2] = smoothing * bands[2];
        oldAmplitudes[3] = smoothing * bands[3];
        oldAmplitudes[4] = smoothing * bands[4];
        oldAmplitudes[5] = smoothing * bands[5];
        oldAmplitudes[6] = smoothing * bands[6];
    }

    // sample FFT 'samples' times and calculate the average
    uint16_t averageAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
    for (uint8_t sample_count = 0; sample_count < sampleAmount; sample_count++)
    {
        uint16_t sampleAmplitudes[7];
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

    // finalize output
    for (uint8_t band = 0; band < 7; band++)
    {
        averageAmplitudes[band] = averageAmplitudes[band] / sampleAmount; // calculate average
        if (smoothing > 0)
        {
            averageAmplitudes[band] += oldAmplitudes[band]; // smooth with old values
            averageAmplitudes[band] = averageAmplitudes[band] / (smoothing + 1);
        }
        bands[band] = (int)(min(max(max((int32_t)averageAmplitudes[band] - noiseLevel, 0) * amplificationFactor, 0), 1023) / 4); // Cut-off noise, amplify and write to array
    }
}

/*
    Transforms the response tables to cycle colors or change frequency response.
*/
void transformResponseTables(uint32_t *colorResponseTable, uint32_t *audioResponseTable, uint16_t fixtureAmount)
{
    // TODO find a way to transform the arrays in-place or return new multi-dim arrays
    // Or change multi-dim arrays into single dim arrays by providing RGB in hex/binary and frequency response in hex/binary
}

/*
    Sets the color of a single fixture according to the supplied color response values.

    @param &targetFixture Fixture to be adjusted.
    @param *audioAmplitudes 7 element uint32_t array of amplitudes per frequency band.
    @param colorResponse [..0xFFFFFF] hex value that represents the color to be displayed by this fixture.
*/
void setFixtureColor(DMXFixture &targetFixture, int *audioAmplitudes, uint32_t colorResponse)
{
    if (audioAmplitudes[1] > 0 || audioAmplitudes[2] > 0 || audioAmplitudes[3] > 0)
    {
        return; // condition for switching colors: low frequencies are on zero-level
                // TODO proper color cycle condition
                // IDEAS: Switch on low volume, populate multiple fixtures with one color if response on one band is significantly stronger
                // IMPORTANT MOVE THIS TO FUNCTION THAT SHUFFLES THE TABLES
                // in that case, what goes here?
                // -> setting white, strobe and so on depending on lever states
    }

    targetFixture.setRGB(colorResponse);
}

/*
    Sets the brightness of a single fixture according to the supplied audio response values.

    @param &targetFixture Fixture to be adjusted.
    @param *audioAmplitudes 7 element uint32_t array of amplitudes per frequency band.
    @param audioResponse [..0xFFFFFFF] hex value that represents the frequencies this fixture should respond to.
*/
void setFixtureBrightness(DMXFixture &targetFixture, int *audioAmplitudes, uint32_t audioResponse)
{
    uint8_t brightness = 0;
    for (uint8_t band = 0; band < 7; band++)
    {
        if (((audioResponse & (0xF * (band + 1))) >> (band * 4)) >= 0) // TODO allow this to differnetiate between the 16 possible values for each response
        {
            brightness = max(audioAmplitudes[band], brightness);
        }
    }
    targetFixture.setRGBDimmer(brightness);
}
