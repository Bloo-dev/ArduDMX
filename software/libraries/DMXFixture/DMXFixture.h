#ifndef DMXFixture_h
#define DMXFixture_h
#include <Conceptinetics.h>

/**
 * @brief Represents a DMX controlled RGBW light fixture with a channel each reserved for:
 * - overall dimmer (0..255)
 * - red light brightness (0..255)
 * - green light brightness (0..255)
 * - blue light brightness (0..255)
 * - white light brightness (0..255)
 * - strobe frequency (0..255)
 * Additionally, the red, green, and blue brightness values may be modified concurrently using a virtual rgb-dimmer (this however does not corrospond to an actual DMX channel).
 * Setting any of these values via the implemented public functions will not immediately send these values via DMX to the fixture, for this display(...) must be called first.
 * 
 * DMX channels here are limited to channels 0..255 to save on memory. 
 *
 */
class DMXFixture
{
public:
    static const uint8_t localDimmerChannel = 1;
    static const uint8_t localRedChannel = 2;
    static const uint8_t localGreenChannel = 3;
    static const uint8_t localBlueChannel = 4;
    static const uint8_t localWhiteChannel = 5;
    static const uint8_t localStrobeChannel = 6;
    static const uint8_t channelAmount = 6;

    /**
     * @brief Construct a new DMXFixture object that utilizes 6 channels, starting from the supplied start channel.
     * 
     * @param startChannel First channel occupied by this DMXFixture.
     * @param dimmerDefaultValue Default value the overall dimmer should assume after reset() is called.
     */
    DMXFixture(uint8_t startChannel, uint8_t dimmerDefaultValue);

    /**
     * @brief Sets the internal buffers for the rgb values to the supplied values.
     *
     * @param redValue The red value to write to the internal buffer.
     * @param greenValue The green value to write to the internal buffer.
     * @param blueValue The blue value to write to the internal buffer.
     */
    void setRGB(uint8_t redValue, uint8_t greenValue, uint8_t blueValue);

    /**
     * @brief Sets the internal buffer for the white value to the supplied value.
     *
     * @param whiteValue The white value to write to the internal buffer
     */
    void setWhite(uint8_t whiteValue);

    /**
     * @brief Sets the internal buffer for the dimmer value to the supplied value.
     *
     * @param dimmerValue The dimmer value to write to the internal buffer.
     */
    void setDimmer(uint8_t dimmerValue);

    /**
     * @brief Sets the internal buffer for the rgb dimmer value to the supplied value.
     * The rgb dimmer artificially supresses the rgb values sent to the DMX device.
     * This does not corrospond to an actual DMX channel.
     *
     * @param dimmerValue The rgb dimmer value to write to the internal buffer.
     */
    void setRGBDimmer(uint8_t dimmerValue);

    /**
     * @brief Sets the internal buffer for the strobe value to the supplied value.
     *
     * @param strobeValue The strobe value to write to the internal buffer.
     */
    void setStrobe(uint8_t strobeValue);

    /**
     * @brief Resets all internal buffers, except for the dimmers, to 0.
     * The overall dimmer is set to the default value supplied initially.
     * The rgb dimmer is set to 255 (100%).
     * 
     */
    void reset();

    /**
     * @brief Takes the values stored in the internal buffers and sends them to the DMX device via the supplied DMX controller.
     * 
     * @param dmxController DMX_Master that is capable of setting the desired channels.
     */
    void display(DMX_Master &dmxController);

private:
    uint8_t _startChannel;
    uint8_t _dimmerDefaultValue;
    uint8_t _dimmerValue;
    uint8_t _rgbDimmerValue;
    uint8_t _redValue;
    uint8_t _greenValue;
    uint8_t _blueValue;
    uint8_t _whiteValue;
    uint8_t _strobeValue;
};

/**
 * @brief Pair of hex color and frequency response.
 * 
 */
struct FixtureProfile
{
    public:
        /**
         * @brief Construct a new Fixture Profile object with 0x0 for both hex color and frequency response.
         * 
         */
        FixtureProfile();

        /**
         * @brief Construct a new Fixture Profile object with the supplied values.
         * 
         * @param color hex color.
         * @param frequency frequency response. Only the lower 7 half-bytes are used. Each half-byte corrosponds to the response value to a specific frequency band,
         * with the lower bytes corrosponding the the lower frequency bands. 0 represents no response, F maximal response.
         */
        FixtureProfile(uint32_t color, uint32_t frequency);
        uint32_t getHexColor();
        uint32_t getHexFrequency();
    private:
        uint32_t _color;
        uint32_t _frequency;
};
#endif
