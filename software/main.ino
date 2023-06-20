#include <AudioAnalyzer.h>
#include <DMXFixture.h>
#include <NumericHistory.h>
#include <LatchedButton.h>
#include <UserInterface.h>

// ===== GLOBAL SETTINGS ======
// Light Fixture Data
const uint8_t maxBrightness = 217; // 85% max brightness to increase LED lifetime
DMXFixture fixtures[] = {DMXFixture(1, maxBrightness), DMXFixture(7, maxBrightness), DMXFixture(13, maxBrightness), DMXFixture(19, maxBrightness)};                                       // configured fixtures and their start channels. The maximum amount of supported fixtures is 16.
const FixtureProfile rgbColorSet[] = {FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0x0000FF, 0x0039000), FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0x00FF00, 0xFF00000)}; // profiles that fixtures can assume. Each profile consists of a hex code for color and a hex code for frequencies the fixture should respond to.
const FixtureProfile cmyColorSet[] = {FixtureProfile(0x800080, 0x00000FF), FixtureProfile(0xA06000, 0xFF00000), FixtureProfile(0x800080, 0x00000FF), FixtureProfile(0x008080, 0x0039000)};
const FixtureProfile coldColorSet[] = {FixtureProfile(0x4B00B4, 0x00000FF), FixtureProfile(0x0000FF, 0xFF00000), FixtureProfile(0x4B00B4, 0x00000FF), FixtureProfile(0x464673, 0x0039000)};
const FixtureProfile uwuColorSet[] = {FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0x71008E, 0xFF00000), FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0xAA0055, 0x0039000)};
const uint8_t targetFrameTimeMs = 66;
// MSGEQ7 Signal Data
const uint8_t samplesPerRun = 1; // number of consecutive samples to take whenever the audio is sampled (these are then averaged). Higher values inhibit random noise spikes, but average out details.
const uint8_t delayBetweenSamples = 1; // time in ms to wait between samples in a consecutive sample run. High values will decrease temporal resolution drastically.
//  =============================

// ===== GLOBAL VARIABLES ======
// Fixture Management
const uint8_t fixtureAmount = sizeof(fixtures) / sizeof(DMXFixture);
const uint8_t profileAmount = sizeof(rgbColorSet) / sizeof(FixtureProfile);
const FixtureProfile *const profiles[] = {rgbColorSet, cmyColorSet, coldColorSet, uwuColorSet}; // rgbColorSet, cmyColorSet, coldColorSet, uwuColorSet};
// Automatic Profile Cycling
uint32_t lastPermutatedAtMs;
uint64_t cachedPermutationCode;
const uint16_t permutationCycleLengthMs = 5000;
// DMX Hardware
DMX_Master dmxMaster(fixtures[0].channelAmount *fixtureAmount, 2);
// FFT Hardware
Analyzer MSGEQ7(7, 4, 0);
uint16_t frequencyAmplitudes[7]; // stores data from MSGEQ7 chip
// User Interface Hardware
uint8_t whiteLightSetting = 0;
uint8_t gainModeSetting = 0;
uint8_t strobeFrequencySetting = 100;
bool strobeEnabled = false;
uint8_t colorSetSetting = 0;
uint8_t msPerFrameMonitor = 0;
void toggleStrobe(bool alternateAction)
{
    strobeEnabled ^= 1;
}
SettingsPage pages[] = {SettingsPageFactory("Lights", &whiteLightSetting).setLinkedVariableLimits(0, 4).setDisplayAlias("  OFF  BARTABLE  ALL").finalize(), SettingsPageFactory("Strobe", &strobeFrequencySetting).setLinkedVariableLimits(0, 101).setLinkedVariableUnits('%').finalize(), SettingsPageFactory("Gain", &gainModeSetting).setLinkedVariableLimits(0, 3).setDisplayAlias(" AUTO  LOW HIGH").enableChangePreviews().finalize(), SettingsPageFactory("Colors", &colorSetSetting).setLinkedVariableLimits(0, 4).setDisplayAlias("  RGB  CMY COLD  uwu").enableChangePreviews().finalize(), SettingsPageFactory("Frame ms", &msPerFrameMonitor).makeMonitor().finalize()};
SettingsDisplay<5> userInterface(pages);
LatchedButton<8> plusButton(3, 1000/targetFrameTimeMs);
LatchedButton<8> selectButton(5, 1000/targetFrameTimeMs);
LatchedButton<8> minusButton(6, 1000/targetFrameTimeMs);
LatchedButton<8> functionButton(9, 1000/targetFrameTimeMs);
// Auto Gain
const uint8_t targetDutyCycle = 196;          // target value for fixture cross-frequency duty cycle (time-clipped/time-not-clipped in parts of 1023, e.g. 196=19.2%)
const float amplificationFactorMax = 64;      // maximum allowed amplifaction factor
const float amplificationFactorMin = 0.015625; // minimal allowed amplification factor
float amplificationFactor = 12.0;             // amplification for signals considered non-noise (ones that should result in a non-zero light response), managed automatically
uint16_t noiseLevel = 0;                      // lower bound for noise, determined automatically at startup
// =============================

void setup()
{
    // Start LCD
    userInterface.setQuickSettingFunction(toggleStrobe);
    userInterface.initializeDisplay(0x27);
    userInterface.print(F("    arduDMX     "), F(" ver 2023-06-19 "));
    delay(1000);

    // Start FFT
    userInterface.print(F("    Starting    "), F("Audio Analyzer.."));
    MSGEQ7.Init();

    // Start DMX
    userInterface.print(F("    Starting    "), F("DMX Controller.."));
    dmxMaster.setAutoBreakMode();
    dmxMaster.enable();

    // Initialize Light Fixtures
    for (uint8_t fixtureId = 0; fixtureId < fixtureAmount; fixtureId++)
    {
        fixtures[fixtureId].reset(); // reset to default values
    }

    // Generate Profile Permutation Code
    cachedPermutationCode = 0xFEDCBA9876543210ul & ((0x1ul << (4 * profileAmount)) - 0x1); // ul postfix required to format literal as unsigned long
    lastPermutatedAtMs = 0;

    // Analyze Noise Levels (THERE MUST NOT BE AUDIO ON THE JACK FOR THIS TO WORK)
    userInterface.print(F("    Probing     "), F("     Noise...    "));
    int noiseData[] = {0, 0, 0, 0, 0, 0, 0};
    sampleMSGEQ7(32, 1, noiseData);
    noiseLevel = getAverage(noiseData, 7, 12); // average over all frequencies and add some extra buffer

    userInterface.print(F("     Setup      "), F("   Complete!    "));
    delay(500); // wait a bit for everything to stabalize
    userInterface.showPages();
}

void loop()
{
    // Store frame start time
    uint32_t frameStartTime = millis();

    // Get FFT data from MSGEQ7 chip
    sampleMSGEQ7(samplesPerRun, delayBetweenSamples, frequencyAmplitudes);

    // Store history of average signal across all bands (with background noise already subtracted) // TODO move these calculations into a function to free local variable memory space from temp variables
    uint16_t averageAmplitudeNoNoise = max((int32_t)getAverage(frequencyAmplitudes, 7, 0) - noiseLevel, 0);
    amplitudeHistory.update(averageAmplitudeNoNoise);
    uint16_t signalMean = getAverage(amplitudeHistory.get(), amplitudeHistory.length(), 0);

    // Store history of duty cycle and transform values in frequencyAmplitudes into range [0..255]
    clippingHistory.update(transformAudioSignal(noiseLevel + signalMean, amplificationFactor, frequencyAmplitudes));

    if (gainModeSetting == 0)
    {
        // Get new amplification factor based on duty cycle mean (duty cycle of 196 is about 19.1%)
        uint16_t dutyCycleMean = getAverage(clippingHistory.get(), clippingHistory.length(), 0);
        float dutyCycleDeviance = (float)targetDutyCycle / dutyCycleMean;
        amplificationFactor = constrain(dutyCycleDeviance, amplificationFactorMin, amplificationFactorMax);
    }
    else if (gainModeSetting == 1)
    {
        amplificationFactor = 0.5; // manual gain setting "low"
    } else if (gainModeSetting == 2)
    {
        amplificationFactor = 48; // manual gain setting "high"
    }

    // Select and Cycle Fixture Profiles
    FixtureProfile permutatedProfiles[fixtureAmount];
    permutateProfiles(generatePermutationCode(cachedPermutationCode, permutationCycleLengthMs), profiles[colorSetSetting], permutatedProfiles, fixtureAmount);

    // Manage Fixtures
    for (uint8_t fixtureId = 0; fixtureId < fixtureAmount; fixtureId++)
    {
        setFixtureColor(fixtures[fixtureId], frequencyAmplitudes, permutatedProfiles[fixtureId].getHexColor()); // set color data
        setFixtureBrightness(fixtures[fixtureId], frequencyAmplitudes, permutatedProfiles[fixtureId].getHexFrequency()); // set brightness data
        setFixtureWhite(fixtures[fixtureId], fixtureId, strobeEnabled, strobeFrequencySetting, whiteLightSetting);

        // send data to fixtures
        fixtures[fixtureId].display(dmxMaster);
    }

    // user interface keep alive
    if (plusButton.isPressed())
    {
        userInterface.input(3, false);
    }

    if (minusButton.isPressed())
    {
        userInterface.input(1, false);
    }

    if (selectButton.isPressed())
    {
        userInterface.input(2, false);
    }

    if (functionButton.isPressed())
    {
        userInterface.input(0, false);
    }
    LatchedButton<8>::resetLatch();
    userInterface.checkScreenSaver();
    userInterface.updateMonitor();

    // Wait until frame time is over
    msPerFrameMonitor = (uint8_t)(millis() - frameStartTime);
    int16_t remainingFrameTimeMs = targetFrameTimeMs - msPerFrameMonitor;
    delay(max(remainingFrameTimeMs, 0));
}

/**
 * @brief Calculates the temporal mean value of an audio signal, with the noise level removed.
 * 
 * @param bandAmplitudes The amplitudes of the 7 frequency bands provided by the MSGEQ7 chip.
 * @param noiseLevel A base noise level to be subtracted from the supplied bandAmplitudes before processing.
 * @return uint16_t The temporal mean of the cross-band signal amplitude.
 */
uint16_t calculateSignalMean(uint16_t *bandAmplitudes, uint16_t noiseLevel)
{
    static NumericHistory<uint16_t,32> amplitudeHistory = NumericHistory<uint16_t,32>();

    int32_t crossBandSignal = getAverage(bandAmplitudes, 7, 0) - noiseLevel;
    amplitudeHistory.update(max(crossBandSignal, 0));

    return getAverage(amplitudeHistory.get(), amplitudeHistory.length(), 0);
}

uint16_t mapAudioAmplitudeToLightLevel(uint16_t *bandAmplitudes, uint16_t )
{
    // TODO
}

void updateAmplificationFactor(float &amplificationFactor, uint16_t crossBandClipping, uint16_t targetCrossBandClipping)
{
    static NumericHistory<uint16_t,32> clippingHistory = NumericHistory<uint16_t,32>();

    clippingHistory.update(crossBandClipping);
    if (gainModeSetting == 0)
    {
        float clippingDeviation = ((float) targetCrossBandClipping) / ((float) getAverage(clippingHistory.get(), clippingHistory.length(), 0));
        amplificationFactor = constrain(clippingDeviation, amplificationFactorMin, amplificationFactorMax);
    }
    else if (gainModeSetting == 1)
    {
        amplificationFactor = 0.5;
    }
    else if (gainModeSetting == 2)
    {
        amplificationFactor = 48;
    }
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

    sum = buffer + (sum / elements);
    return min(sum, 1023);
}

/**
 * @brief Transforms a given 12-bit audio signal to an 8-bit signal that can be used to control DMXFixtures. Also performs some cleanup on the signal, like removing noise and scaling the signal to use the entire 8-bit space.
 *
 * @param lowSignalCutOff [0..1023] Signals lower or equal than this will be forced to 0. Applied before amplification.
 * @param amplificationFactor [0.0..10.0] (recommended) Multiplicative amplification factor to be applied to the signal.
 * @param targetArray The array holding the audio data to be modified. Should have seven (7) entries.
 *
 * @return [0..1023] The duty cycle of the average over all frequency bands in terms of clipping, i.e. how often the signal clipped the upper signal limit of 1023/255.
 */
uint16_t transformAudioSignal(uint16_t lowSignalCutOff, float amplificationFactor, int *targetArray)
{
    uint16_t bandClippings[] = {0, 0, 0, 0, 0, 0, 0};
    for (uint8_t band = 0; band < 7; band++)
    {
        uint16_t signalNoNoise = max((int32_t)targetArray[band] - lowSignalCutOff, 0);
        uint16_t signalAmplified = amplificationFactor * signalNoNoise; // shift signal down, removing noise and static parts of the signal
        if (signalAmplified >= 1023)
        {
            bandClippings[band] = 1023; // remember the signal clipped
        }

        signalAmplified = signalAmplified / 4;
        targetArray[band] = (int)min(signalAmplified, 255); // scale to [0..255] for use in light fixtures
    }

    return getAverage(bandClippings, 7, 0);
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
 * @brief Stores a permutated version of the supplied profile array `constProfiles` into `permutedProfiles` according to a supplied permutation instruction.
 *
 * @param permutation The permutation to be used when assembling `permutatedProfiles` from `constProfiles`. The permutation is encoded as a 64-bit number,
 * consisting of 16 consequtive source addresses. The target address is deducted from the position of the source address within the 64-bit instruction.
 * E.g. `0x01234567` would load the profile in slot 7 into the fixture in slot 0, the profile in slot 6 into the fixture in slot 1 and so on.
 * @param constProfiles An array of FixtureProfiles used to read FixtureProfiles from. This should be constant.
 * @param permutatedProfiles An array of FixtureProfiles that will have the permutation stored into it.
 * @param fixtureAmount The amount of fixtures.
 */
void permutateProfiles(uint64_t permutation, FixtureProfile *constProfiles, FixtureProfile *permutatedProfiles, uint16_t fixtureAmount)
{
    // store shuffled profiles into arrays for the fixtures to read from.
    // Note that these arrays only require a length of fixtureAmount, as any additional profiles will not be displayed on a fixture anyways.
    for (uint8_t profileSlot = 0; profileSlot < fixtureAmount; profileSlot++)
    {
        // extract lowest instruction and shift remaining instructions
        uint8_t profileSource = (permutation & 0xF);
        permutation = (permutation >> 4);

        // store to shuffled profile
        permutatedProfiles[profileSlot] = constProfiles[profileSource];
    }
}

/**
 * @brief Generates a new permutation code of the fixture to profile mapping, based on the last permutation.
 * Once the cycle length has been exceeded, the last permutation is cycled by one step,
 * otherwise the method returns the provided permutation without modifications.
 * This cycling is done to equally utilize LEDs over all fixture, preventing "burn in".
 * 
 * @param previousPermutationCode Reference to the previous permutation code.
 * @param cycleLengthMs How long once permutation should last until shuffled.
 * @return uint64_t The (modified) permutation.
 */
uint64_t generatePermutationCode(uint64_t &previousPermutationCode, uint16_t cycleLengthMs)
{
    // if last permutation shift wasn't too long, return last permutation
    uint32_t timeNow = millis();
    if (timeNow - lastPermutatedAtMs < cycleLengthMs)
        return previousPermutationCode;

    // shift last permutation
    previousPermutationCode = (previousPermutationCode >> 4) + ((previousPermutationCode & ((0x1ul << 4) - 0x1)) << ((4 * profileAmount) - 4));
    lastPermutatedAtMs = timeNow;
    return previousPermutationCode;
}

/**
    @brief Sets the color of a single fixture according to the supplied color response values.

    @param &targetFixture Fixture to be adjusted.
    @param *audioAmplitudes 7 element uint32_t array of amplitudes per frequency band.
    @param colorResponse [0..0xFFFFFF] hex value that represents the color to be displayed by this fixture.
*/
void setFixtureColor(DMXFixture &targetFixture, int *audioAmplitudes, uint32_t colorResponse)
{
    // convert colors to rgb and send to fixture
    targetFixture.setRGB(colorResponse >> 16, (colorResponse & 0x00FF00) >> 8, colorResponse & 0x0000FF);
}

/**
    @brief Sets the brightness of a single fixture according to the supplied audio response values.

    @param targetFixture Fixture to be adjusted.
    @param audioAmplitudes 7 element uint32_t array of amplitudes per frequency band.
    @param audioResponse [0..0xFFFFFFF] hex value that represents the frequencies this fixture should respond to.
*/
void setFixtureBrightness(DMXFixture &targetFixture, int *audioAmplitudes, uint32_t audioResponse)
{
    uint8_t brightness = 0;
    uint8_t observedBands = 0;
    for (uint8_t band = 0; band < 7; band++)
    {
        uint8_t bandResponse = ((audioResponse & ((uint32_t)0xF << (band * 4))) >> (band * 4));
        if (bandResponse > 0)
        {
            brightness += (bandResponse / 16.0) * audioAmplitudes[band];
            observedBands++;
        }
    }
    targetFixture.setRGBDimmer((uint8_t)(brightness / observedBands));
}

void setFixtureWhite(DMXFixture &targetFixture, uint8_t fixtureId, bool strobeEnabled, uint8_t strobeFrequency, uint8_t whiteSetting)
{
    // reset white value to 0
    targetFixture.setWhite(0);

    // reset strobe frequency to 0
    targetFixture.setStrobe(0);

    if (strobeEnabled) // if strobe is on, enable white on all fixtures
    {
        targetFixture.setWhite(255);
        targetFixture.setStrobe(strobeFrequency * (255/100));
    }
    else if (whiteSetting) // if strobe is off, white is only enabled on fixtures depending on white setting. If whiteSetting == 0, none of these special rules apply
    {
        if ((whiteSetting == 1 && fixtureId == 1) || (whiteSetting == 2 && fixtureId == 3))
        {
            targetFixture.setWhite(32); // spotlight on bar or table, low light level
        }
        else if (whiteSetting == 3)
        {
            targetFixture.setWhite(255); // full bright mode for all lights on
        }
    }
}
