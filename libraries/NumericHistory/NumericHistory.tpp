template <uint8_t LENGTH>
NumericHistory<LENGTH>::NumericHistory() : _latestEntry(LENGTH - 1)
{
}

template <uint8_t LENGTH>
void NumericHistory<LENGTH>::update(uint16_t value)
{
    _latestEntry = (_latestEntry + 1) % LENGTH;
    _history[_latestEntry] = value;
}

template <uint8_t LENGTH>
uint16_t *NumericHistory<LENGTH>::get()
{
    static uint16_t out[LENGTH];

    for (int index = 0; index < LENGTH; index++)
    {
        out[index] = _history[index];
    }

    return out;
}

template <uint8_t LENGTH>
uint16_t NumericHistory<LENGTH>::get(uint8_t index)
{
    return _history[(_latestEntry + index) % LENGTH];
}

template <uint8_t LENGTH>
uint8_t NumericHistory<LENGTH>::length()
{
    return LENGTH;
}
