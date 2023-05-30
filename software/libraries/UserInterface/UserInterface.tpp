// ======== SETTINGS PAGE ========
//
//
//
//
// ======== ======== ======== ========

SettingsPage::SettingsPage() : _state(0), _settingName(""), _footer(""), _buttonOverwriteMask(0), _linkedVariablePtr(0), _linkedVariableEditBuffer(0), _linkedVariableMin(0), _linkedVariableMax(255), _unitSymbol(' '), _aliasList("")
{
}

SettingsPage::SettingsPage(uint8_t state, String settingName, uint8_t buttonOverwriteMask, uint8_t *linkedVarPtr, uint8_t linkedVarMin, uint8_t linkedVarMax, char unitSymbol, String aliasList) : _state(state), _buttonOverwriteMask(buttonOverwriteMask), _linkedVariablePtr(linkedVarPtr), _linkedVariableEditBuffer(0), _linkedVariableMin(linkedVarMin), _linkedVariableMax(linkedVarMax), _unitSymbol(unitSymbol), _aliasList(aliasList)
{
    // Precompute setting name displayed in header. Format: 'Example Sett.: '
    // Calculate how much space is available for the settingName
    int8_t targetLength = DISPLAY_WIDTH - (_SYMBOL_SEPARATOR.length() + VALUE_DISPLAY_WIDTH + UNIT_DISPLAY_WIDTH);

    while (targetLength > settingName.length()) // add left whitespace padding if there is enough space
    {
        settingName = _SYMBOL_SPACE + settingName;
    }

    if (targetLength < settingName.length()) // shorten setting name if name is there is too little space
    {
        settingName = settingName.substring(0, targetLength - _SYMBOL_FULL_STOP.length());
        settingName += _SYMBOL_FULL_STOP;
    }
    _settingName += settingName + _SYMBOL_SEPARATOR;

    // Precompute footer (full 2nd line of the screen)
    String buttonFooters[4];
    for (uint8_t buttonCode = 0; buttonCode < 4; buttonCode++) // collect button actions
    {
        uint8_t buttonAction = (_buttonOverwriteMask >> (2 * buttonCode)) & 0b11;
        if (buttonAction & 0b01)
        {
            if (buttonAction & 0b10)
            {
                // buttonAction 0b11 (increase linked variable)
                buttonFooters[buttonCode] = _SYMBOL_PLUS;
            }
            else
            {
                // buttonAction 0b01 (decrease linked variable)
                buttonFooters[buttonCode] = _SYMBOL_MINUS;
            }
        }
        else
        {
            if (buttonAction & 0b10)
            {
                // buttonAction 0b10 (restore default for linked variable (unimplemented))
                buttonFooters[buttonCode] = _SYMBOL_SPACE;
            }
            else
            {
                // buttonAction 0b00 (button not overwritten)
                if (buttonCode & 0b01)
                {
                    if (buttonCode & 0b10)
                    {
                        // buttonCode 0b11 = 3 "plus"
                        buttonFooters[buttonCode] = _SYMBOL_ARROW_RIGHT;
                    }
                    else
                    {
                        // buttonCode 0b01 = 1 "minus"
                        buttonFooters[buttonCode] = _SYMBOL_ARROW_LEFT;
                    }
                }
                else
                {
                    if (buttonCode & 0b10)
                    {
                        // buttonCode 0b10 = 2 "select"
                        buttonFooters[buttonCode] = _SYMBOL_SAVE;
                    }
                    else
                    {
                        // buttonCode 0b00 = 0 "function"
                        if (_state & 0b00000100) // if immediate changes are on, display SAVE, otherwise display BACK
                        {
                            buttonFooters[buttonCode] = _SYMBOL_SAVE;
                        }
                        else
                        {
                            buttonFooters[buttonCode] = _SYMBOL_BACK;
                        }
                    }
                }
            }
        }
    }

    // compose footer
    _footer = buttonFooters[1] + _SYMBOL_SPACE + buttonFooters[2] + _SYMBOL_SPACE + buttonFooters[3];
    while (_footer.length() + buttonFooters[0].length() < DISPLAY_WIDTH)
    {
        _footer = _SYMBOL_SPACE + _footer;
    }
    _footer = buttonFooters[0] + _footer;
}

bool SettingsPage::isSelected()
{
    return _state & 0b1000;
}

void SettingsPage::select()
{
    if(isSelected())
        return;

    _linkedVariableEditBuffer = (*_linkedVariablePtr); // init buffer to value of linked variable
    _state = _state | 0b1000;
}

void SettingsPage::deselectDiscard()
{
    if(!isSelected())
        return;

    _linkedVariableEditBuffer = (*_linkedVariablePtr); // overwrite linked variable buffer with actual variable value
    _state = _state & 0b0111;
}

void SettingsPage::deselectSave()
{
    if (!isSelected())
        return;

    (*_linkedVariablePtr) = _linkedVariableEditBuffer; // overwrite linked variable buffer with actual variable value
    _state = _state & 0b0111;
}

bool SettingsPage::handleButton(uint8_t buttonCode)
{
    uint8_t buttonAction = (_buttonOverwriteMask & (0b11 << 2 * buttonCode)) >> (2 * buttonCode);
    if (!buttonAction) // if buttonAction is 0b00, then the page does not actually overwrite the button
    {
        return false; // hand back control to the display
    }

    // page actually overwrites the pressed button. Perform the assigned action.
    if (buttonAction & 0b01)
    {
        if (buttonAction & 0b10)
        {
            // buttonAction 0b11 (increase linked variable)
            _linkedVariableEditBuffer = (((_linkedVariableEditBuffer - _linkedVariableMin) + 1) % _linkedVariableMax) + _linkedVariableMin;
        }
        else
        {
            // buttonAction 0b01 (decrease linked variable)
            _linkedVariableEditBuffer = (((_linkedVariableEditBuffer - _linkedVariableMin) + (_linkedVariableMax - 1)) % _linkedVariableMax) + _linkedVariableMin;
        }

        if (_state & 0b0100) // check if changes should apply immediately
        {
            (*_linkedVariablePtr) = _linkedVariableEditBuffer;
        }
    }
    else
    {
        // buttonAction 0b10 (restore default value)
        // TODO unimplemented
    }
    return true;
}

String SettingsPage::getRenderedHeader()
{
    return _settingName + getRenderedValue();
}

String SettingsPage::getRenderedFooter()
{
    return _footer;
}

String SettingsPage::getRenderedValue()
{
    String linkedVariableValue;
    if (_state & 0b10) // use alias instead of raw values
    {
        uint8_t aliasIndex = (_linkedVariableEditBuffer % (_aliasList.length() / VALUE_DISPLAY_WIDTH)) * VALUE_DISPLAY_WIDTH;
        linkedVariableValue = _aliasList.substring(aliasIndex, aliasIndex + VALUE_DISPLAY_WIDTH);
    }
    else // use raw values
    {
        linkedVariableValue = String(_linkedVariableEditBuffer);
    }

    while (linkedVariableValue.length() < VALUE_DISPLAY_WIDTH) // add left whitespace padding if there is enough space
    {
        linkedVariableValue = _SYMBOL_SPACE + linkedVariableValue;
    }
    return linkedVariableValue + String(_unitSymbol); // return rendered value plus unitSymbol (the latter is ' ' if no unit is set)
}

// ======== SETTINGS PAGE FACTORY ========
//
//
//
//
// ======== ======== ======== ========

SettingsPageFactory::SettingsPageFactory(String settingName, uint8_t buttonOverwriteMask, uint8_t *linkedVariablePtr) : _settingName(settingName), _buttonOverwriteMask(buttonOverwriteMask), _linkedVariablePtr(linkedVariablePtr), _state(0), _linkedVariableMin(0), _linkedVariableMax(255), _unitSymbol(' '), _aliasList("")
{
}

SettingsPageFactory SettingsPageFactory::setLinkedVariableLimits(uint8_t min, uint8_t max)
{
    _linkedVariableMin = min;
    _linkedVariableMax = max;
    return *this;
}

SettingsPageFactory SettingsPageFactory::setImmediateChanges(bool immediateChanges)
{
    if (immediateChanges)
    {
        _state = _state | 0b00000100;
    }
    else
    {
        _state = _state & 0b11111011;
    }
    return *this;
}

SettingsPageFactory SettingsPageFactory::setLinkedVariableUnits(char unitSymbol)
{
    _unitSymbol = unitSymbol;
    return *this;
}

SettingsPageFactory SettingsPageFactory::setDisplayAlias(String aliasList)
{
    if (aliasList.length() == 0)
    {
        _state = _state | 0b11111101;
    }
    else
    {
        _state = _state | 0b00000010;
    }
    _aliasList = aliasList;
    return *this;
}

SettingsPage SettingsPageFactory::finalize()
{
    return SettingsPage(_state, _settingName, _buttonOverwriteMask, _linkedVariablePtr, _linkedVariableMin, _linkedVariableMax, _unitSymbol, _aliasList);
}

// ======== SETTINGS DISPLAY ========
//
//
//
//
// ======== ======== ======== ========

template <uint8_t PAGE_AMOUNT>
SettingsDisplay<PAGE_AMOUNT>::SettingsDisplay(SettingsPage *pages) : _currentPageIndex(0), _quickSettingFunction(0), _hasQuickSettingFunction(false), _screen(0, 0, 0), _screenInitialized(false), _screenSaverTurnOnTimestamp(0), _screenSaverOn(false)
{
    for (int i = 0; i < PAGE_AMOUNT; i++)
    {
        _pages[i] = pages[i];
    }
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::initializeDisplay(uint8_t screenAddress)
{
    _screen = LiquidCrystal_I2C(screenAddress, 16, 2);
    _screen.init();
    _screen.clear();
    setScreenSaverTimestamp(2 * SCREEN_SAVER_OFFSET);
    refreshAll();
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::input(uint8_t buttonCode, bool alternateAction)
{
    // update screen saver timestamp
    if (setScreenSaverTimestamp(SCREEN_SAVER_OFFSET))
    {
        return; // if the screen saver was active, absorb the button press
    }

    // check if the page handles the pressed button
    if (_pages[_currentPageIndex].isSelected()) // check if page is in edit mode, if so, let the page check if it overwrites the pressed button
    {
        if (_pages[_currentPageIndex].handleButton(buttonCode))
        {
            refreshValue();
            return; // page handled the button, display does not need to handle it
        }
    }

    // page does not handle the pressed button, fall back to default button actions
    if (buttonCode & 0b01)
    {
        if (buttonCode & 0b10)
        {
            // buttonCode 0b11 "plus"
            nextPage();
        }
        else
        {
            // buttonCode 0b01 "minus"
            previousPage();
        }
    }
    else
    {
        if (buttonCode & 0b10)
        {
            // buttonCode 0b10 "select"
            if (_pages[_currentPageIndex].isSelected())
            {
                deselectPage(false); // deselect page, do NOT discard changes.
            }
            else
            {
                selectPage();
            }
        }
        else
        {
            // buttonCode 0b00 "function"
            if (_pages[_currentPageIndex].isSelected())
            {
                deselectPage(true); // if a page is selected, pressing FUN will deselect the page, NOT storing changes from the buffer to the linked variable
            }
            else if (_hasQuickSettingFunction) // if a quick setting function was specified, execute it
            {
                _quickSettingFunction(alternateAction);
            }
        }
    }
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::setQuickSettingFunction(void (*quickSettingFunction)(bool))
{
    _quickSettingFunction = quickSettingFunction;
    _hasQuickSettingFunction = true;
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::checkScreenSaver()
{ // TODO use this function to also update linked variables on the current page from their source (-> will be useful for FPS display)
    if (_screenSaverOn) // return if the screen saver is already on
        return;

    if (_screenSaverTurnOnTimestamp > millis()) // return if the screen saver shouldnt be turned on yet
    {
        Serial.print("Turn on screen saver in: ");
        Serial.println(_screenSaverTurnOnTimestamp - millis());
        return;
    }

    // turn on screen saver, also discard any pending changes
    _screen.noDisplay();
    deselectPage(true);
    _screenSaverOn = true;
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::selectPage()
{
    _pages[_currentPageIndex].select();
    refreshAll();
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::deselectPage(bool discardChanges)
{
    if (discardChanges)
    {
        _pages[_currentPageIndex].deselectDiscard();
    }
    else
    {
        _pages[_currentPageIndex].deselectSave();
    }
    refreshAll();
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::nextPage()
{
    _currentPageIndex = (_currentPageIndex + 1) % PAGE_AMOUNT;
    refreshAll();
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::previousPage()
{
    _currentPageIndex = (_currentPageIndex + (PAGE_AMOUNT - 1)) % PAGE_AMOUNT;
    refreshAll();
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::refreshAll()
{
    _screen.setCursor(0, 0);
    _screen.print(_pages[_currentPageIndex].getRenderedHeader());

    // footer
    _screen.setCursor(0, 1);
    if (_pages[_currentPageIndex].isSelected())
    { // if page is selected, get page specific footer
        _screen.print(_pages[_currentPageIndex].getRenderedFooter());
    }
    else
    {                                            // page is not selected, use default footer
        _screen.print("FUNC    \177 EDIT \176"); // 177 is left arrow, 176 is right arrow
    }
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::refreshValue()
{
    _screen.setCursor(DISPLAY_WIDTH - (VALUE_DISPLAY_WIDTH + 1), 0); // -1 is from unit symbol, which is one character
    _screen.print(_pages[_currentPageIndex].getRenderedValue());
}

template <uint8_t PAGE_AMOUNT>
bool SettingsDisplay<PAGE_AMOUNT>::setScreenSaverTimestamp(uint16_t offset)
{
    // store new target time for turning on the screen saver again
    _screenSaverTurnOnTimestamp = millis() + offset;

    if (_screenSaverOn)
    {
        // turn off screen saver if it was on
        _screen.display();
        _screenSaverOn = false;

        return true;
    }

    return false;
}
