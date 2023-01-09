template <int LENGTH>
NumericHistory<LENGTH>::NumericHistory() : _oldestEntry(0)
{
}

template <int LENGTH>
void NumericHistory<LENGTH>::update(uint16_t value)
{
    _history[_oldestEntry++] = value;
    if (_oldestEntry > (LENGTH - 1))
        _oldestEntry = 0;
}

template <int LENGTH>
uint16_t *NumericHistory<LENGTH>::get()
{
    static uint16_t out[LENGTH];

    for (int index = 0; index < LENGTH; index++)
    {
        out[index] = _history[index];
    }

    return out;
}

template <int LENGTH>
uint8_t NumericHistory<LENGTH>::length()
{
    return LENGTH;
}
