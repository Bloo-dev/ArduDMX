// ======== SETTINGS PAGE ========
//
//
//
//
// ======== ======== ======== ========

SettingsPage::SettingsPage() : _state(0), _settingName(), _footer(), _linkedVariablePtr(0), _linkedVariableEditBuffer(0), _linkedVariableMin(0), _linkedVariableMax(255), _unitSymbol(' '), _aliasList()
{
}

SettingsPage::SettingsPage(uint8_t state, String settingName, uint8_t *linkedVariablePtr, uint8_t linkedVarMin, uint8_t linkedVarMax, char unitSymbol, String aliasList) : _state(state), _linkedVariablePtr(linkedVariablePtr), _linkedVariableEditBuffer((*linkedVariablePtr)), _linkedVariableMin(linkedVarMin), _linkedVariableMax(linkedVarMax), _unitSymbol(unitSymbol), _aliasList(aliasList)
{
    // Precompute setting name displayed in header. Format: 'Example Sett.: '
    // Calculate how much space is available for the settingName
    int8_t targetLength = DISPLAY_WIDTH - (2 + VALUE_DISPLAY_WIDTH + UNIT_DISPLAY_WIDTH); // 2 for length of ": "
    settingName.reserve(targetLength);

    while (targetLength > settingName.length()) // add left whitespace padding if there is enough space
    {
        settingName = _SYMBOL_SPACE + settingName;
    }

    if (targetLength < settingName.length()) // shorten setting name if name is there is too little space
    {
        settingName = settingName.substring(0, targetLength - 1); // -1 for length of "."
        settingName += String(".");
    }
    _settingName += settingName + String(": ");

    // Precompute footer (full 2nd line of the screen)
    _footer = String(F(""));
    _footer.reserve(DISPLAY_WIDTH);
    if (minusButtonDisabled())
    {
        _footer += _SYMBOL_SPACE;
    }
    else
    {
        _footer += String("-");
    }

    _footer += _SYMBOL_SPACE + String("SAVE") + _SYMBOL_SPACE;

    if (plusButtonDisabled())
    {
        _footer += _SYMBOL_SPACE;
    }
    else
    {
        _footer += String("+");
    }

    while (_footer.length() + 4 < DISPLAY_WIDTH) // footer length plus length of "BACK" (=4)
    {
        _footer = _SYMBOL_SPACE + _footer;
    }
    _footer = String("BACK") + _footer;
}

bool SettingsPage::isSelected()
{
    return _state & 0b001000;
}

void SettingsPage::select()
{
    if (isSelected())
        return;

    _linkedVariableEditBuffer = (*_linkedVariablePtr); // init buffer to value of linked variable
    _state = _state | 0b001000;
}

void SettingsPage::deselectDiscard()
{
    if (!isSelected())
        return;

    if (hasChangePreviewsEnabled())
    {
        (*_linkedVariablePtr) = _linkedVariableEditBuffer; // if change previews are on, restore linked variable to value stored in buffer
    }
    else
    {
        _linkedVariableEditBuffer = (*_linkedVariablePtr); // if change previews are off, reset buffer to value of linked variable (important for rendering)
    }

    _state = _state & 0b110111;
}

void SettingsPage::deselectSave()
{
    if (!isSelected())
        return;

    if (hasChangePreviewsEnabled())
    {
        _linkedVariableEditBuffer = (*_linkedVariablePtr); // if change previews are on, set buffer to value of linked variable (important for rendering)
    }
    else
    {
        (*_linkedVariablePtr) = _linkedVariableEditBuffer; // if change previews are off, store edited value to linked variable
    }

    _state = _state & 0b110111;
}

bool SettingsPage::hasChangePreviewsEnabled()
{
    return _state & 0b000100;
}

bool SettingsPage::isMonitor()
{
    return _state & 0b000001;
}

void SettingsPage::storeValue(uint8_t value)
{
    if (hasChangePreviewsEnabled())
    {
        (*_linkedVariablePtr) = value;
    }
    else
    {
        _linkedVariableEditBuffer = value;
    }
}

uint8_t SettingsPage::loadValue()
{
    if (hasChangePreviewsEnabled())
    {
        return (*_linkedVariablePtr);
    }
    else
    {
        return _linkedVariableEditBuffer;
    }
}

void SettingsPage::minusButton()
{
    if (minusButtonDisabled()) // if the button is marked as disabled, return
        return;
    storeValue((((loadValue() - _linkedVariableMin) + (_linkedVariableMax - 1)) % _linkedVariableMax) + _linkedVariableMin);
}

bool SettingsPage::minusButtonDisabled()
{
    return _state & 0b100000;
}

void SettingsPage::plusButton()
{
    if (plusButtonDisabled()) // if the button is marked as disabled, return
        return;
    storeValue((((loadValue() - _linkedVariableMin) + 1) % _linkedVariableMax) + _linkedVariableMin);
}

bool SettingsPage::plusButtonDisabled()
{
    return _state & 0b010000;
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
    linkedVariableValue.reserve(VALUE_DISPLAY_WIDTH);
    if (_state & 0b000010) // use alias instead of raw values
    {
        uint8_t aliasIndex = (loadValue() % (_aliasList.length() / VALUE_DISPLAY_WIDTH)) * VALUE_DISPLAY_WIDTH;
        linkedVariableValue = _aliasList.substring(aliasIndex, aliasIndex + VALUE_DISPLAY_WIDTH);
    }
    else // use raw values
    {
        linkedVariableValue = String(loadValue());
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

SettingsPageFactory::SettingsPageFactory(String settingName, uint8_t *linkedVariablePtr) : _settingName(settingName), _linkedVariablePtr(linkedVariablePtr), _state(0), _linkedVariableMin(0), _linkedVariableMax(255), _unitSymbol(' '), _aliasList("")
{
}

SettingsPage SettingsPageFactory::finalize()
{
    return SettingsPage(_state, _settingName, _linkedVariablePtr, _linkedVariableMin, _linkedVariableMax, _unitSymbol, _aliasList);
}

SettingsPageFactory SettingsPageFactory::disableMinusButton()
{
    _state = _state | 0b100000;
    return *this;
}

SettingsPageFactory SettingsPageFactory::disablePlusButton()
{
    _state = _state | 0b010000;
    return *this;
}

SettingsPageFactory SettingsPageFactory::setLinkedVariableLimits(uint8_t min, uint8_t max)
{
    _linkedVariableMin = min;
    _linkedVariableMax = max;
    return *this;
}

SettingsPageFactory SettingsPageFactory::setLinkedVariableUnits(char unitSymbol)
{
    _unitSymbol = unitSymbol;
    return *this;
}

SettingsPageFactory SettingsPageFactory::enableChangePreviews()
{
    _state = _state | 0b00000100;
    return *this;
}

SettingsPageFactory SettingsPageFactory::makeMonitor()
{
    enableChangePreviews();
    _state = _state | 0b00000001;
    return *this;
}

SettingsPageFactory SettingsPageFactory::setDisplayAlias(String aliasList)
{
    _state = _state | 0b00000010;
    _aliasList = aliasList;
    return *this;
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
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::input(uint8_t buttonCode, bool alternateAction)
{
    // update screen saver timestamp
    if (setScreenSaverTimestamp(SCREEN_SAVER_OFFSET))
    {
        return; // if the screen saver was active, absorb the button press
    }

    // figure out which button was pressed
    if (buttonCode & 0b01)
    {
        if (buttonCode & 0b10)
        {
            // buttonCode 0b11 "plus"
            if (_pages[_currentPageIndex].isSelected())
            {
                _pages[_currentPageIndex].plusButton(); // if the current page is selected, let the page handle the button action
                refreshValue();
            }
            else
            {
                nextPage(); // default action if the current page is not selected
            }
        }
        else
        {
            // buttonCode 0b01 "minus"
            if (_pages[_currentPageIndex].isSelected())
            {
                _pages[_currentPageIndex].minusButton(); // if the current page is selected, let the page handle the button action
                refreshValue();
            }
            else
            {
                previousPage(); // default action if the current page is not selected
            }
        }
    }
    else
    {
        if (buttonCode & 0b10)
        {
            // buttonCode 0b10 "select"
            if (!_pages[_currentPageIndex].isMonitor()) // make sure page is not a monitor, monitors can not be edited
            {
                if (_pages[_currentPageIndex].isSelected())
                {
                    deselectPage(false); // deselect selected page, do NOT discard changes.
                }
                else
                {
                    selectPage(); // select unselected page
                }
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
                refreshAll();
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
{
    if (_screenSaverOn) // return if the screen saver is already on
        return;

    if (_screenSaverTurnOnTimestamp > millis()) // return if the screen saver shouldnt be turned on yet
        return;

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
    { // page is not selected, generate a default footer
        String defaultFooter = String(F("FUNC    "));
        defaultFooter += String("\177") + SettingsPage::_SYMBOL_SPACE;
        if (_pages[_currentPageIndex].isMonitor())
        {
            defaultFooter += SettingsPage::_SYMBOL_SPACE + SettingsPage::_SYMBOL_SPACE + SettingsPage::_SYMBOL_SPACE + SettingsPage::_SYMBOL_SPACE;
        }
        else
        {
            defaultFooter += String("EDIT");
        }
        defaultFooter += SettingsPage::_SYMBOL_SPACE + String("\176");

        _screen.print(defaultFooter);
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

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::updateMonitor()
{
    if (_screenSaverOn) // dont update the monitor if the screen saver is on
        return;

    if (_pages[_currentPageIndex].isSelected()) // if the page is not selected, return
        return;

    if (!_pages[_currentPageIndex].isMonitor()) // if the page is not a monitor, return
        return;

    refreshValue(); // refresh value if the page is selected and a monitor
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::print(String header, String footer)
{
    if (header.length() > DISPLAY_WIDTH)
    {
        header = header.substring(0,DISPLAY_WIDTH);
    }

    while (header.length() < DISPLAY_WIDTH)
    {
        header += SettingsPage::_SYMBOL_SPACE;
    }

    if (footer.length() > DISPLAY_WIDTH)
    {
        footer = footer.substring(0, DISPLAY_WIDTH);
    }

    while (footer.length() < DISPLAY_WIDTH)
    {
        footer += SettingsPage::_SYMBOL_SPACE;
    }

    _screen.setCursor(0,0);
    _screen.print(header);
    _screen.setCursor(0, 1);
    _screen.print(footer);
}

template <uint8_t PAGE_AMOUNT>
void SettingsDisplay<PAGE_AMOUNT>::showPages()
{
    setScreenSaverTimestamp(2 * SCREEN_SAVER_OFFSET);
    refreshAll();
}
