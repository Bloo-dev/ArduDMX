#include <AudioAnalyzer.h>
#include <DMXFixture.h>
#include <NumericHistory.h>
#include <LatchedButton.h>
#include <UserInterface.h>

// ================================================================
//                           CONSTANTS
// ================================================================
const uint8_t BRIGHTNESS_CAP = 217; // 85% max brightness to increase LED lifetime
const uint16_t PROFILE_CYCLE_PERIOD_MS = 5000;
const uint8_t FRAME_PERIOD_MS = 66;
const uint8_t SAMPLES_PER_FRAME = 1;
const uint8_t SAMPLE_DELAY_MS = 1;
const uint8_t AUDIO_BANDS = 7;
const uint16_t AUDIO_BAND_MAX = 1023;
const uint8_t DMX_CHANNEL_MAX = 255;
const float TARGET_CLIPPING = 196.0;    // target value for fixture cross-frequency duty cycle (time-clipped/time-not-clipped in parts of 1023, e.g. 196=19.2%)
const float AMP_FACTOR_MAX = 64.0;      // maximum allowed amplifaction factor
const float AMP_FACTOR_MIN = 0.0078125; // minimal allowed amplification factor (1/128)

// ================================================================
//                         CONFIGURATION
// ================================================================
DMXFixture FIXTURES[] = {DMXFixture(1, BRIGHTNESS_CAP), DMXFixture(7, BRIGHTNESS_CAP), DMXFixture(13, BRIGHTNESS_CAP), DMXFixture(19, BRIGHTNESS_CAP)};                                      // configured fixtures and their start channels. The maximum amount of supported fixtures is 16.
const FixtureProfile RGB_COLOR_SET[] = {FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0x0000FF, 0x0039000), FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0x00FF00, 0xFF00000)}; // profiles that fixtures can assume. Each profile consists of a hex code for color and a hex code for frequencies the fixture should respond to.
const FixtureProfile CMY_COLOR_SET[] = {FixtureProfile(0x800080, 0x00000FF), FixtureProfile(0xA06000, 0xFF00000), FixtureProfile(0x800080, 0x00000FF), FixtureProfile(0x008080, 0x0039000)};
const FixtureProfile COLD_COLOR_SET[] = {FixtureProfile(0x4B00B4, 0x00000FF), FixtureProfile(0x0000FF, 0xFF00000), FixtureProfile(0x4B00B4, 0x00000FF), FixtureProfile(0x464673, 0x0039000)};
const FixtureProfile UWU_COLOR_SET[] = {FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0x71008E, 0xFF00000), FixtureProfile(0xFF0000, 0x00000FF), FixtureProfile(0xAA0055, 0x0039000)};
const uint8_t FIXTURE_AMOUNT = sizeof(FIXTURES) / sizeof(DMXFixture);
const uint8_t PROFILE_AMOUNT = sizeof(RGB_COLOR_SET) / sizeof(FixtureProfile);
const FixtureProfile *const PROFILE_GROUPS[] = {RGB_COLOR_SET, CMY_COLOR_SET, COLD_COLOR_SET, UWU_COLOR_SET};
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
const SettingsPage SETTINGS_PAGES[] = {SettingsPageFactory("Lights", &whiteLightSetting).setLinkedVariableLimits(0, 4).setDisplayAlias("  OFF  BARTABLE  ALL").finalize(), SettingsPageFactory("Strobe", &strobeFrequencySetting).setLinkedVariableLimits(0, 101).setLinkedVariableUnits('%').finalize(), SettingsPageFactory("Gain", &gainModeSetting).setLinkedVariableLimits(0, 3).setDisplayAlias(" AUTO  LOW HIGH").enableChangePreviews().finalize(), SettingsPageFactory("Colors", &colorSetSetting).setLinkedVariableLimits(0, 4).setDisplayAlias("  RGB  CMY COLD  uwu").enableChangePreviews().finalize(), SettingsPageFactory("Frame ms", &msPerFrameMonitor).makeMonitor().finalize()};

// ================================================================
//                           SUBSYSTEMS
// ================================================================
DMX_Master dmxMaster(FIXTURES[0].channelAmount *FIXTURE_AMOUNT, 2);
Analyzer MSGEQ7(7, 4, 0);
uint16_t bandAmplitudes[AUDIO_BANDS];
float amplificationFactor = 12.0; // amplification for signals considered non-noise (ones that should result in a non-zero light response), managed automatically
uint16_t noiseLevel = 0;          // lower bound for noise, determined automatically at startup
SettingsDisplay<5> userInterface(SETTINGS_PAGES);
LatchedButton<8> plusButton(3, 1000 / FRAME_PERIOD_MS);
LatchedButton<8> selectButton(5, 1000 / FRAME_PERIOD_MS);
LatchedButton<8> minusButton(6, 1000 / FRAME_PERIOD_MS);
LatchedButton<8> functionButton(9, 1000 / FRAME_PERIOD_MS);

// ================================================================
//                       STARTUP SEQUENCE
// ================================================================
void setup()
{
    // Start LCD
    userInterface.setQuickSettingFunction(toggleStrobe);
    userInterface.initializeDisplay(0x27);
    userInterface.print(F("    arduDMX     "), F(" ver 2023-06-28 "));
    delay(1000);

    // Start FFT
    userInterface.print(F("    Starting    "), F("Audio Analyzer.."));
    MSGEQ7.Init();

    // Start DMX
    userInterface.print(F("    Starting    "), F("DMX Controller.."));
    dmxMaster.setAutoBreakMode();
    dmxMaster.enable();

    // Initialize Light Fixtures
    for (uint8_t fixtureId = 0; fixtureId < FIXTURE_AMOUNT; fixtureId++)
    {
        FIXTURES[fixtureId].reset(); // reset to default values
    }

    // Analyze Noise Levels (THERE MUST NOT BE AUDIO ON THE JACK FOR THIS TO WORK)
    userInterface.print(F("    Probing     "), F("     Noise...    "));
    int noiseData[] = {0, 0, 0, 0, 0, 0, 0};
    sampleMSGEQ7(32, 1, noiseData);
    noiseLevel = getAverage(noiseData, AUDIO_BANDS, 12); // average over all frequencies and add some extra buffer

    userInterface.print(F("     Setup      "), F("   Complete!    "));
    delay(500); // wait a bit for everything to stabalize
    userInterface.showPages();
}

// ================================================================
//                           MAIN LOOP
// ================================================================
void loop()
{
    // Store frame start time
    uint32_t frameStartTime = millis();

    // Get FFT data from MSGEQ7 chip
    sampleMSGEQ7(SAMPLES_PER_FRAME, SAMPLE_DELAY_MS, bandAmplitudes);

    // Transform audio signal levels to light signal levels and apply amplification
    uint16_t signalMean = calculateSignalMean(bandAmplitudes, noiseLevel);
    uint16_t crossBandClipping = mapAudioAmplitudeToLightLevel(bandAmplitudes, signalMean + noiseLevel, amplificationFactor);
    updateAmplificationFactor(amplificationFactor, crossBandClipping);

    // Select and Cycle Fixture Profiles
    FixtureProfile permutatedProfiles[FIXTURE_AMOUNT];
    permutateProfiles(generatePermutationCode(), permutatedProfiles);

    // Manage Fixtures
    for (uint8_t fixtureId = 0; fixtureId < FIXTURE_AMOUNT; fixtureId++)
    {
        setFixtureColor(FIXTURES[fixtureId], bandAmplitudes, permutatedProfiles[fixtureId].getHexColor());          // set color data
        setFixtureBrightness(FIXTURES[fixtureId], bandAmplitudes, permutatedProfiles[fixtureId].getHexFrequency()); // set brightness data
        setFixtureWhite(FIXTURES[fixtureId], fixtureId, strobeEnabled, strobeFrequencySetting, whiteLightSetting);

        // send data to fixtures
        FIXTURES[fixtureId].display(dmxMaster);
    }

    // Send Button inputs to UI and update UI accordingly
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
    int16_t remainingFrameTimeMs = FRAME_PERIOD_MS - msPerFrameMonitor;
    delay(max(remainingFrameTimeMs, 0));
}

// ================================================================
//                       HELPER FUNCTIONS
// ================================================================

/**
 * @brief Calculates the temporal mean value of an audio signal, with the noise level removed.
 *
 * @param bandAmplitudes The amplitudes of the 7 frequency bands provided by the MSGEQ7 chip.
 * @param noiseLevel A base noise level to be subtracted from the supplied bandAmplitudes before processing.
 * @return uint16_t The temporal mean of the cross-band signal amplitude.
 */
uint16_t calculateSignalMean(uint16_t *bandAmplitudes, uint16_t noiseLevel)
{
    static NumericHistory<uint16_t, 32> amplitudeHistory = NumericHistory<uint16_t, 32>();

    int32_t crossBandSignal = getAverage(bandAmplitudes, AUDIO_BANDS, 0) - noiseLevel;
    amplitudeHistory.update(max(crossBandSignal, 0));

    return getAverage(amplitudeHistory.get(), amplitudeHistory.length(), 0);
}

/**
 * @brief Transforms a supplied array of 12-bit band amplitude values to an array of 8-bit band amplitude values which can then be used to address DMX channels.
 *
 * @param bandAmplitudes An array of 12-bit band amplitudes provided by the MSGEQ7 chip. This array will be modified in-place.
 * @param bandAverage The average absolute value to be expected across all bands. All band amplitudes which are less than this value will be set to `0` in the output.
 * @param amplificationFactor The amplification factor to be applied to the signal.
 *
 * @return [0..1023] The average of the cross-band clipping. This is calculated as follows:
 * Each band which is detected to be clipping (i.e. has a value of `1023` or larger after the amplification factor was applied) is assigned the value `1023`,
 * each band which is not clipping is assigned a value of `0` in a temporary array. The average of this array is what is returned.
 */
uint16_t mapAudioAmplitudeToLightLevel(uint16_t *bandAmplitudes, uint16_t bandAverage, float &amplificationFactor)
{
    uint16_t bandClippings[] = {0, 0, 0, 0, 0, 0, 0};
    for (uint8_t band = 0; band < AUDIO_BANDS; band++)
    {
        int32_t halfSignalWidth = (int32_t)bandAmplitudes[band] - bandAverage;    // calculate average absolute value of this band
        uint16_t signalAmplified = max(halfSignalWidth, 0) * amplificationFactor; // amplify parts of the signal that are larger than the supplied bandAverage
        if (signalAmplified >= AUDIO_BAND_MAX)
        {
            bandClippings[band] = AUDIO_BAND_MAX; // remember the signal clipped
        }

        signalAmplified = signalAmplified >> ((AUDIO_BAND_MAX + 1) / (2 * (DMX_CHANNEL_MAX + 1))); // scale [0,AUDIO_BAND_MAX] signal to be within [0,DMX_CHANNEL_MAX], here we use that x/2 == x>>1
        bandAmplitudes[band] = (int)min(signalAmplified, DMX_CHANNEL_MAX);                         // scale to [0..255] for use in light fixtures
    }

    return getAverage(bandClippings, AUDIO_BANDS, 0); // return average cross-band clipping
}

/**
 * @brief Updates the amplification factor according to the `gainModeSetting` (global variable).
 *
 * @param amplificationFactor The amplification factor to be updated.
 * If `gainModeSetting` is `1`, it will always be set to `0.5`.
 * If `gainModeSetting` is `2`, it will always be set to `48`.
 * If `gainModeSetting` is `0`, it will be dynamically calculated from the latest clipping history.
 * @param crossBandClipping The latest cross-band clipping average,
 * aka the average created when every band that has clipped is assigned 1023, and every band that has not clipped is assigned 0.
 */
void updateAmplificationFactor(float &amplificationFactor, uint16_t crossBandClipping)
{
    static NumericHistory<uint16_t, 32> clippingHistory = NumericHistory<uint16_t, 32>();

    clippingHistory.update(crossBandClipping);
    if (gainModeSetting == 0)
    {
        float clippingDeviation = TARGET_CLIPPING / ((float)getAverage(clippingHistory.get(), clippingHistory.length(), 0)); // target ('wanted') value for the cross-band clipping is 196.0 = 19.1%
        amplificationFactor = constrain(clippingDeviation, AMP_FACTOR_MIN, AMP_FACTOR_MAX);                                  // limit the amplification factor to be within this range
    }
    else if (gainModeSetting == 1)
    {
        amplificationFactor = 0.5;
    }
    else if (gainModeSetting == 2)
    {
        amplificationFactor = 48.0;
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
    return min(sum, AUDIO_BAND_MAX);
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
                                           // bandAmplitudes[]: 0    1    2   3     4      5    6

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
 * @param permutatedProfiles An array of FixtureProfiles that will have the permutation stored into it.
 */
void permutateProfiles(uint64_t permutation, FixtureProfile *permutatedProfiles)
{
    // store shuffled profiles into arrays for the fixtures to read from.
    // Note that these arrays only require a length of fixtureAmount, as any additional profiles will not be displayed on a fixture anyways.
    for (uint8_t profileSlot = 0; profileSlot < FIXTURE_AMOUNT; profileSlot++)
    {
        // extract lowest instruction and shift remaining instructions
        uint8_t profileSource = (permutation & 0xF);
        permutation = (permutation >> 4);

        // store to shuffled profile
        permutatedProfiles[profileSlot] = PROFILE_GROUPS[colorSetSetting][profileSource];
    }
}

/**
 * @brief Generates a new permutation code of the fixture to profile mapping, based on the last permutation.
 * Once the cycle length has been exceeded, the last permutation is cycled by one step,
 * otherwise the method returns the provided permutation without modifications.
 * This cycling is done to equally utilize LEDs over all fixture, preventing "burn in".
 *
 * @return uint64_t The (modified) permutation.
 */
uint64_t generatePermutationCode()
{
    static uint32_t permutationTimestamp = 0;
    static uint64_t permutationCode = 0xFEDCBA9876543210ul & ((0x1ul << (4 * PROFILE_AMOUNT)) - 0x1);

    // if last permutation shift wasn't too long, return last permutation
    uint32_t timeNow = millis();
    if (timeNow - permutationTimestamp < PROFILE_CYCLE_PERIOD_MS)
        return permutationCode;

    // shift last permutation
    permutationCode = (permutationCode >> 4) + ((permutationCode & ((0x1ul << 4) - 0x1)) << ((4 * PROFILE_AMOUNT) - 4));
    permutationTimestamp = timeNow;
    return permutationCode;
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
    for (uint8_t band = 0; band < AUDIO_BANDS; band++)
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
        targetFixture.setWhite(DMX_CHANNEL_MAX);
        targetFixture.setStrobe(strobeFrequency * (DMX_CHANNEL_MAX / 100));
    }
    else if (whiteSetting) // if strobe is off, white is only enabled on fixtures depending on white setting. If whiteSetting == 0, none of these special rules apply
    {
        if ((whiteSetting == 1 && fixtureId == 1) || (whiteSetting == 2 && fixtureId == 3))
        {
            targetFixture.setWhite(32); // spotlight on bar or table, low light level
        }
        else if (whiteSetting == 3)
        {
            targetFixture.setWhite(DMX_CHANNEL_MAX); // full bright mode for all lights on
        }
    }
}
