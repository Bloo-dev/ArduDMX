#ifndef UserInterface_h
#define UserInterface_h
#include <LiquidCrystal_I2C.h>
#include "Arduino.h"

#define DISPLAY_WIDTH 16
#define DISPLAY_HEIGHT 2
#define SCREEN_SAVER_OFFSET 15000
#define VALUE_DISPLAY_WIDTH 5
#define UNIT_DISPLAY_WIDTH 1

class SettingsPage
{
public:
    /**
     * @brief Construct a new SettingsPage instance. Not to be used directly. Refer to SettingsPageFactory for SettingsPage intanciation.
     */
    SettingsPage(uint8_t state, String settingName, uint8_t buttonOverwriteMask, uint8_t *linkedVarPtr, uint8_t linkedVarMin, uint8_t linkedVarMax, char unitSymbol, String aliasList);

    /**
     * @brief Construct a new Settings Page object. Not to be used directly. Refer to SettingsPageFactory for SettingsPage intanciation.
     *
     * Default constructor. This only exists for technical reasons.
     *
     */
    SettingsPage();

    /**
     * @brief Checks whether the current page is selected (in "edit" mode).
     *
     * @return true if the page is selected.
     * @return false if the page is not selected.
     */
    bool isSelected();

    /**
     * @brief Selects the page and prepares the internal linked variable buffer.
     *
     */
    void select();

    /**
     * @brief Deselects the page and discards any changes made to the linked variable.
     *
     */
    void deselectDiscard();

    /**
     * @brief Deselects the page and stores any changes made to the linked variable.
     *
     */
    void deselectSave();

    /**
     * @brief Checks if the given button is being overwritten by this page. If so, the corrosponding action is executed.
     *
     * @param buttonCode Button code of the button that was pressed. May be 0b00-0b11.
     * @return true if the button is overwritten by this page.
     * @return false if the button is not overwritten by this page and the SettingsDisplay should execute a default button action.
     */
    bool handleButton(uint8_t buttonCode);

    /**
     * @brief Get a rendered version of this page's complete header (top line).
     * The returned String is pre-rendered therefore calling this function has a minimal perfomance impact.
     *
     * @return String String which represents the entire top line of the page.
     */
    String getRenderedHeader();

    /**
     * @brief Get a rendered version of this page's complete footer (bottom line).
     * The returned String is pre-rendered therefore calling this function has a minimal perfomance impact.
     *
     * @return String String which represents the bottom top line of the page.
     */
    String getRenderedFooter();

    /**
     * @brief Get a rendered version of this page's value.
     *
     * @return String String which represents the value (including unit) on this page.
     */
    String getRenderedValue();

private:
    uint8_t _state; // TODO use lowest bit (currently unused) to indicate whether this page is considered a "monitor page" and should be constantly re-rendered if the page is selected
                    // monitor pages should also not show a "save" option but only display the "back" option. When viewing a monitor page, the function button should remain bound to the function.
                    // -> a function to get the monitor state of a page is required
                    // -> what should incrementing/decrememnting on a monitor page do? -> increment or decrement, this may actually be useful (not for a FPS display, but for other things)
    uint8_t _buttonOverwriteMask;
    uint8_t *_linkedVariablePtr;
    // Temporary buffer that may be used if editing the linked var shouldn't happen immediately, but when changes should only be applied upon exiting edit mode.
    uint8_t _linkedVariableEditBuffer;
    uint8_t _linkedVariableMin;
    uint8_t _linkedVariableMax;

    inline static String _SYMBOL_SEPARATOR = String(": ");
    inline static String _SYMBOL_FULL_STOP = String(".");
    inline static String _SYMBOL_SPACE = String(" ");
    inline static String _SYMBOL_PLUS = String("+");
    inline static String _SYMBOL_MINUS = String("-");
    inline static String _SYMBOL_SAVE = String("SAVE");
    inline static String _SYMBOL_BACK = String("BACK");
    inline static String _SYMBOL_ARROW_LEFT = String("\177");
    inline static String _SYMBOL_ARROW_RIGHT = String("\176");
    String _settingName;
    String _footer;
    char _unitSymbol;
    String _aliasList;
};

/**
 * @brief Constructs SettingsPage instances via finalize().
 * Once finalize is called, all parameters which have not been set explicitely using setter functions will assume default values.
 */
struct SettingsPageFactory
{
public:
    /**
     * @brief Construct a new SettingsPageFactory object.
     *
     * @param settingName String to be displayed as the name of this setting.
     * @param buttonOverwriteMask Mask which specifies which buttons this settings page overwrites (aka replaces those buttons' default actions with page specific actions).
     *                           Each button is assigned a 2-bit field which defines its new page specific action. If the field is left 0b00, the button is not overwritten by the page and falls back to it's default behavior.
     *                           Format: 0b(00)(00)(00)(00) which corospond to 4 buttons, labeled by 'buttonCodes' 3, 2, 1, 0, in order.
     *                           - (0b10) sets the button to restore the linked variable to a default value. TODO unimplemented
     *                           - (0b01) sets the button to decrease the linked variable if pressed.
     *                           - (0b11) sets the button to increase the linked variable if pressed.
     * @param linkedVarPtr A uint8_t variable which is exposed to the user for modification on this SettingsPage.
     */
    SettingsPageFactory(String settingName, uint8_t buttonOverwriteMask, uint8_t *linkedVarPtr);

    /**
     * @brief Produces a SettingsPage from this SettingsPageFactory.
     * All parameters that have not been set explicitely via a setter method will assume default values.
     *
     * @return SettingsPage based on the set parameters of this SettingsPageFactory.
     */
    SettingsPage finalize();

    /**
     * @brief Set limits for the values the linked variable may be set to via the user interface.
     *
     * @param min Minimum value of the linked variable (inclusive).
     * @param max Minimum value of the linked variable (exclusive).
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setLinkedVariableLimits(uint8_t min, uint8_t max);

    /**
     * @brief Sets whether changes made on a selected page (in "edit" mode) should be applied immeadiately or whether to only apply them when the page is unselected.
     *
     * @param immediateChanges 'true' if the changes made on a selected page should be applied immediately.
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setImmediateChanges(bool immediateChanges);

    /**
     * @brief Sets the unit to be displayed behind the value of the linked variable on the screen.
     *
     * @param unitSymbol A one-character unit symbol.
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setLinkedVariableUnits(char unitSymbol);

    /**
     * @brief Sets display aliases for the linked variable. These alias will replace the numbers of the linked variable with 5-character strings.
     * E.g. this may be used to replace 1 with "ON" and 0 with "OFF".
     *
     * @param aliasList String object which represents a concatination of possible aliases. Must have a lenght of 5, 10, 15, 20, ... .
     * E.g. "   ON  OFF" could be used to introduce the two aliases "ON" and "OFF". Turns aliases off if the supplied String is "".
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setDisplayAlias(String aliasList);

private:
    // Stores the state of the page. Format: 0b0000
    // 0b(0|1)000 encodes whether the page is currently selected.
    // 0b0(0|1)00 encodes whether the changes to the linked variable should be applied immediately (1) or whether the buffer should be used (0).
    // 0b00(0|1)0 encodes whether the displayAlias should be used.
    uint8_t _state;
    String _settingName;
    uint8_t _buttonOverwriteMask;
    uint8_t *_linkedVariablePtr;
    uint8_t _linkedVariableMin;
    uint8_t _linkedVariableMax;
    char _unitSymbol;
    String _aliasList;
};

template <uint8_t PAGE_AMOUNT>
class SettingsDisplay
{
public:
    SettingsDisplay(SettingsPage *pages);

    /**
     * @brief Initializes the connected 1602 display. This is necessary to establish communications over I2C.
     *
     * @param screenAdress I2C address of the screen.
     */
    void initializeDisplay(uint8_t screenAddress);

    /**
     * @brief Inputs a command into this SettingsDisplay. This method is designed to be hooked up to user-controlled buttons.
     *        If the current page is not in selected (in "edit" mode) or if the current page is selected but does not overwrite one of the buttons via its _buttonOverwriteMask, then the SettingsDisplay will use default actions for the buttons 0b00 to 0b11.
     *        These are:
     *          - 0b00: Custom function call. Specified by defining a function called 'void displayQuickSetting(bool alternateAction)' and storing it to this 'SettingsDisplay.quickSettingFunction'.
     *          - 0b01: Go to previous page.
     *          - 0b10: Select current page (enter "edit" mode).
     *          - 0b11: Go to next page.
     *
     * @param buttonCode An indentifier for the button that was pressed. 0b00 to 0b11 are supported.
     * @param alternateAction If an alternate action should be executed. This is designed for system that can tell button presses apart from button long-pressed ("holds").
     *                        If this is set to true, an alternate action will be triggered, if available.
     */
    void input(uint8_t buttonCode, bool alternateAction);

    /**
     * @brief Sets a function to be executed upon pressing button 0b00 (unless the button is overwritten by the page).
     *
     * @param quickSettingFunction A function pointer. Must accept exactly one parameter. The lone parameter must be of type bool. True is handed if an alternate action should be executed.
     */
    void setQuickSettingFunction(void (*quickSettingFunction)(bool));

    /**
     * @brief Enables the screen saver (turns of the display) if the internal screen saver timeout has passed.
     * Call this function regularly (at least every 7500ms) to enable the screen saver feature.
     * Not calling this function will effectively disable the screen saver.
     */
    void checkScreenSaver();

private:
    // Array of pages held by this SettingsDisplay.
    SettingsPage _pages[PAGE_AMOUNT];
    // Index of the page currently shown on this SettingsDisplay.
    uint8_t _currentPageIndex;
    // Function pointer to the function to be executed as a default for button 0b00.
    bool _hasQuickSettingFunction;
    void (*_quickSettingFunction)(bool);
    // Connected screen
    LiquidCrystal_I2C _screen;
    bool _screenInitialized;
    uint32_t _screenSaverTurnOnTimestamp;
    bool _screenSaverOn;

    /**
     * @brief Refreshes the full image on the screen. Segments being refreshed will flicker shortly.
     *
     */
    void refreshAll();

    /**
     * @brief Refreshes only the value part of the current page, reducing flicker to that part.
     *
     */
    void refreshValue();
    void nextPage();
    void previousPage();
    void selectPage();

    /**
     * @brief Deselects the currently selected page.
     *
     * @param discardChanges 'true' if any changes made should be discarded. 'false' if the changes should be saved.
     */
    void deselectPage(bool discardChanges);

    /**
     * @brief Sets a new timestamp for the screen save to turn on on.
     * Disables the screen saver if it is turned on.
     *
     * @param offset milliscond offset from the current time.
     * @return true If the screen saver was active when this function was called.
     * @return false If the screen saver was inactive when this function was called.
     */
    bool setScreenSaverTimestamp(uint16_t offset);
};

#include "UserInterface.tpp"
#endif
