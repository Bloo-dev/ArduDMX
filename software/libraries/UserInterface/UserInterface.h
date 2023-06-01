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
    const inline static String _SYMBOL_SPACE = String(" ");
    
    /**
     * @brief Construct a new SettingsPage instance. Not to be used directly. Refer to SettingsPageFactory for SettingsPage intanciation.
     */
    SettingsPage(uint8_t state, String settingName, uint8_t *linkedVariablePtr, uint8_t linkedVarMin, uint8_t linkedVarMax, char unitSymbol, String aliasList);

    /**
     * @brief Construct a new Settings Page object. Not to be used directly. Refer to SettingsPageFactory for SettingsPage intanciation.
     *
     * Default constructor. This only exists for technical reasons.
     *
     */
    SettingsPage();

    /**
     * @brief Checks whether this page is selected (in "edit" mode).
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
     * @brief Checks whether change previews are enabled for this page.
     * 
     * @return true If change previews are enabled for this page.
     * @return false If change previews are disabled for this page.
     */
    bool hasChangePreviewsEnabled();

    /**
     * @brief Checks whether this page is a monitor page. When selected, Monitor pages constantly update the displayed value from the linked variable.
     * 
     * @return true If the page is a Monitor.
     * @return false If the page is not a Monitor.
     */
    bool isMonitor();

    /**
     * @brief Decrements the linked variable, unless this button has been disabled upon creation of this SettingsPage.
     * 
     */
    void minusButton();

    /**
     * @brief Checks whether the minus button was disabled upon creation of this SettingsPage.
     *
     * @return true If the minus button is disabled. If so, pressing it should not yield any action.
     * @return false If the minus button is available. If so, pressing it should decrement the linked variable.
     */
    bool minusButtonDisabled();

    /**
     * @brief Increments the linked variable, unless this button has been disabled upon creation of this SettingsPage.
     *
     */
    void plusButton();

    /**
     * @brief Checks whether the plus button was disabled upon creation of this SettingsPage.
     *
     * @return true If the plus button is disabled. If so, pressing it should not yield any action.
     * @return false If the plus button is available. If so, pressing it should increment the linked variable.
     */
    bool plusButtonDisabled();

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
    uint8_t _state;
    uint8_t *_linkedVariablePtr;
    // Temporary buffer that may be used if editing the linked var shouldn't happen immediately, but when changes should only be applied upon exiting edit mode.
    uint8_t _linkedVariableEditBuffer;
    uint8_t _linkedVariableMin;
    uint8_t _linkedVariableMax;
    String _settingName;
    String _footer;
    char _unitSymbol;
    String _aliasList;

    /**
     * @brief Stores the supplied value to a storage location.
     * This storage location is either the linked variable, or the linked variable buffer, depending on whether change previews are on.
     * If change previews are on, the value is written directly to the linked variable.
     * If change previews are off, the value is only written to the linked variable buffer.
     * 
     * @param value The value to be stored to the storage location.
     */
    void storeValue(uint8_t value);

    /**
     * @brief Load the value from a storage location.
     * This storage location is either the linked variable, or the linked variable buffer, depnding on whether change previews are on.
     * If change previews are on, the value is read directly from the linked variable.
     * If change previews are off, the value is read from the linked variable buffer.
     *  
     * @return uint8_t The value loaded from the storage location.
     */
    uint8_t loadValue();
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
     * @param linkedVarPtr A uint8_t variable which is exposed to the user for modification on this SettingsPage.
     */
    SettingsPageFactory(String settingName, uint8_t *linkedVarPtr);

    /**
     * @brief Produces a SettingsPage from this SettingsPageFactory.
     * All parameters that have not been set explicitely via a setter method will assume default values.
     *
     * @return SettingsPage based on the set parameters of this SettingsPageFactory.
     */
    SettingsPage finalize();

    /**
     * @brief Disables button 0b01 when the page is selected.
     * Therefore, the linked variable can no longer be decremented when the page is selected (except for roll-overs).
     *
     * @return SettingsPageFactory based on the set parameters of this SettingsPageFactory.
     */
    SettingsPageFactory disableMinusButton();

    /**
     * @brief Disables button 0b11 when the page is selected.
     * Therefore, the linked variable can no longer be incremented when the page is selected (except for roll-overs).
     *
     * @return SettingsPageFactory based on the set parameters of this SettingsPageFactory.
     */
    SettingsPageFactory disablePlusButton();

    /**
     * @brief Set limits for the values the linked variable may be set to via the user interface.
     *
     * @param min Minimum value of the linked variable (inclusive).
     * @param max Minimum value of the linked variable (exclusive).
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setLinkedVariableLimits(uint8_t min, uint8_t max);

    /**
     * @brief Sets the unit to be displayed behind the value of the linked variable on the screen.
     *
     * @param unitSymbol A one-character unit symbol.
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setLinkedVariableUnits(char unitSymbol);

    /**
     * @brief Enables change previews.
     * With change previews enabled, changes made in edit mode will be applied immediately instead of only upon saving.
     * The user may then SAVE to keep these changes or BACK to discard the changes.
     *
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory enableChangePreviews();

    /**
     * @brief Turns this page into a Monitor.
     * Monitor pages can be used to display changing values, but Monitors can not be selected ("edited").
     * Calling SettingsDisplay::updateMonitor() updates the value displayed by the currently displayed page,
     * given the page is a monitor.
     * As Monitor pages can not be edited, setting enableChangePreviews() will not have an effect (but changePreviews are enabled automatically for technical reasons).
     * However, Monitor pages still respect the setLinkedVariableLimits(), setLinkedVariableUnits() and setDisplayAlias() configurations.
     * setLinkedVariableLimits() will modulate the underlying variable to fit the limits, but do note that this does only affect the display, the underlying variable is not limited.
     * setLinkedVariableUnits() will add a unit symbol to be displayed behind the variable, as expected.
     * setDisplayAlias() will render the numbers provided by the underlying linked variable as the provided aliases, if possible.
     *
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory makeMonitor();

    /**
     * @brief Sets display aliases for the linked variable. These alias will replace the numbers of the linked variable with 5-character strings.
     * E.g. this may be used to replace 1 with "ON" and 0 with "OFF".
     *
     * @param aliasList String object which represents a concatination of possible aliases. Must have a lenght of 5, 10, 15, 20, ... .
     * E.g. "   ON  OFF" could be used to introduce the two aliases "ON" and "OFF".
     * @return SettingsPageFactory This instance of the SettingsPageFactory, with values updated.
     */
    SettingsPageFactory setDisplayAlias(String aliasList);

private:
    // Stores the state of the page. Format: 0b000000
    // 0b(0|1)00000 encodes whether the button 0b01 "minus" is disabled when the page is selected (if this is 0, then this button can be used to decrement the linked variable)
    // 0b0(0|1)0000 encodes whether the button 0b11 "plus" is disabled when the page is selected (if this is 0, then this button can be used to increment the linked variable).
    // 0b00(0|1)000 encodes whether the page is currently selected.
    // 0b000(0|1)00 encodes whether changes previews are enabled.
    // 0b0000(0|1)0 encodes whether the displayAlias should be used.
    // 0b00000(0|1) encodes whether the page is a Monitor page.
    uint8_t _state;
    String _settingName;
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

    /**
     * @brief Enables Monitor pages to update their values whilst they are being displayed.
     * Call this function regularly to keep updating the displayed values of Monitor pages.
     * If the page currently displayed is not a monitor page, or is not selected, then this function will return immediately.
     * Not calling this function will freeze the values displayed on Monitor pages.
     * 
     */
    void updateMonitor();

    /**
     * @brief Prints the supplied Strings to the attached screen. The Strings are padded or trimmed to the width of a display line.
     * This function is slow and should not be called inside loops.
     * 
     * @param header The string to be printed on the top line of the display.
     * @param footer The string to be printed on the bottom line of the display.
     */
    void print(String header, String footer);

    /**
     * @brief Renders pages view. Must be called at least once after object creation, otherwise the user will have to press a button.
     * 
     */
    void showPages();

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
