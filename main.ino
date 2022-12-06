#include <AudioAnalyzer.h>
#include <Conceptinetics.h>

Analyzer Audio = Analyzer(6, 7, 0); // Setup pins for audio in
DMX_Master dmx_master(6, 2); // Setup pins for DMX out

int FreqVal[7]; // target array for frequency data
int noiseCutoff = 30; // min signal level over background

void setup()
{
    // Enable DMX master interface and start transmitting
    dmx_master.enable();

    // Init spectrum analyzer
    Audio.Init();

    // Set overall dimmer to 75%
    dmx_master.setChannelValue(1, 196);

    // Set all color channels to 0 to start
    dmx_master.setChannelValue(2, 0);
    dmx_master.setChannelValue(3, 0);
    dmx_master.setChannelValue(4, 0);
    dmx_master.setChannelValue(5, 0);
    dmx_master.setChannelValue(6, 0);
}

void loop()
{
    Audio.ReadFreq(FreqVal); // return 7 value of 7 bands pass filiter
                             // Frequency(Hz):63  160  400  1K  2.5K  6.25K  16K
                             // FreqVal[]:      0    1    2    3    4    5    6
    //for (int i = 1; i < 6; i++)

    // read and store
    // adjust for underlying signal noise (subtract 30)
    // and scale to 0-255 (div by 4)
    int red = max(FreqVal[2] / 4 - noiseCutoff ,0);
    int green = max(FreqVal[3] / 4 - noiseCutoff ,0);
    int blue = max(FreqVal[4] / 4 - noiseCutoff ,0);

    if (red>=green && red>=blue) {
        dmx_master.setChannelValue(2, red);
        dmx_master.setChannelValue(3, 0);
        dmx_master.setChannelValue(4, 0);
    }
    if (green>=red && green>=blue) {
        dmx_master.setChannelValue(2, 0);
        dmx_master.setChannelValue(3, green);
        dmx_master.setChannelValue(4, 0);
    }
    if (blue>=red && blue>= green) {
        dmx_master.setChannelValue(2, 0);
        dmx_master.setChannelValue(3, 0);
        dmx_master.setChannelValue(4, blue);
    }

    delay(24);
}
