#include <AudioAnalyzer.h>
#include <Conceptinetics.h>
#include <DMXFixture.h>

// =============================
// ===== GLOBAL SETTINGS ======
// Light Fixture Data
const uint8_t maxBrightness = 217; // 85% max brightness to increase LED lifetime
DMXFixture fixtures[] = {DMXFixture(1, maxBrightness), DMXFixture(7, maxBrightness)};
const uint16_t fixtureAmount = sizeof(fixtures) / sizeof(DMXFixture);
// MSGEQ7 Signal Data
const uint8_t noiseCutoff = 70;         // lower bound for what is considered noise. 0..1024. Higher values lead to flickery behaviour, but push silent parts of the signal to zero. Low values will cause random noise to create a signal without any actual audio signal.
const uint8_t samplesPerRun = 16;       // number of consecutive samples to take whenever the audio is sampled (these are then averaged). Higher values inhibit random noise spikes.
const uint16_t delayBetweenSamples = 1; // time in ms to wait between samples in a consecutive sample run. High values will decrease temporal resolution drastically.
const uint8_t interRunSmoothing = 0;    // smothing between frames. Requires clear signals, as level-zero signals will pull the channel low for a long time.
const uint8_t signalAmplification = 6;  // amplification of the signal before it is sent to the light fixtures. Amplifies the signal (0..255) by this factor. This happens AFTER the noise cutoff, meaning any signals lower than noiseCutoff won't be amplified (stay 0).

// =============================
// ===== GLOBAL VARIABLES ======
// DMX Hardware
DMX_Master dmxMaster(fixtures[0].channelAmount * 6, 2);
// FFT Hardware
Analyzer MSGEQ7 = Analyzer(6, 7, 0);
uint16_t frequencyAmplitudes[7];

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
        // fixtures[i] = DMXFixture(dmxMaster, (i*6)+1, maxBrightness); // create new fixture
        fixtures[i].reset(); // reset to default values
        // fixtures[i].setWhite(255); // turn on white LEDs
    }

    delay(1000); // wait until data lines stabilize to minimize interference (some flickering does still occur after usb-disconnect)
}

void loop()
{
    // Analyze Audio
    readAudio(samplesPerRun, delayBetweenSamples, interRunSmoothing, noiseCutoff, signalAmplification);

    // Send Audio To Fixtures
    fixtures[0].setRGB(frequencyAmplitudes[2], 0, 0);
    fixtures[1].setRGB(0, 0, frequencyAmplitudes[5]);

    // Display Set Colors
    for (int i = 0; i < fixtureAmount; i++)
    {
        fixtures[i].display(dmxMaster); // actually display colors
    }
}

void readAudio(uint8_t samples, uint16_t sampleDelay, uint8_t smoothing, uint8_t noiseLevel, uint8_t amplificationFactor)
{
    // if smoothing is desired, remember old amplitudes
    uint16_t oldAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
    if (smoothing > 0)
    {
        for (uint8_t band = 0; band < 7; band++)
        {
            oldAmplitudes[band] = smoothing * frequencyAmplitudes[band]; // store old amplitude scaled by smoothing
        }
    }

    // sample FFT 'samples' times and calculate the average
    uint16_t averageAmplitudes[] = {0, 0, 0, 0, 0, 0, 0};
    for (uint8_t sample_count = 0; sample_count < samples; sample_count++)
    {
        uint16_t sampleAmplitudes[7];
        MSGEQ7.ReadFreq(sampleAmplitudes); // store amplitudes of frequency bands into array
                                           // Frequency(Hz):        63  160  400  1K  2.5K  6.25K  16K
                                           // frequencyAmplitudes[]: 0    1    2   3     4      5    6
        for (uint8_t band = 0; band < 7; band++)
        {
            averageAmplitudes[band] += sampleAmplitudes[band]; // sum up samples
        }

        delay(sampleDelay); // wait before acquisition of next sample
    }

    // finalize output
    for (uint8_t band = 0; band < 7; band++)
    {
        averageAmplitudes[band] = averageAmplitudes[band] / samples; // calculate average
        if (smoothing > 0)
        {
            averageAmplitudes[band] += oldAmplitudes[band]; // smooth with old values
            averageAmplitudes[band] = averageAmplitudes[band] / (smoothing + 1);
        }
        frequencyAmplitudes[band] = min(max(((int32_t)averageAmplitudes[band] - noiseLevel) / 4, 0) * amplificationFactor, 255); // scale to 0..255 and transfer back into array
    }
}
