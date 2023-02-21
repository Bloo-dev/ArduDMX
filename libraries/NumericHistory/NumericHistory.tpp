template <typename TYPE, uint8_t LENGTH>
NumericHistory<TYPE, LENGTH>::NumericHistory() : _latestEntry(LENGTH - 1)
{
}

template <typename TYPE, uint8_t LENGTH>
void NumericHistory<TYPE, LENGTH>::update(TYPE value)
{
    _latestEntry = (_latestEntry + 1) % LENGTH;
    _history[_latestEntry] = value;
}

template <typename TYPE, uint8_t LENGTH>
TYPE *NumericHistory<TYPE, LENGTH>::get()
{
    static TYPE out[LENGTH];

    for (int index = 0; index < LENGTH; index++)
    {
        out[index] = _history[index];
    }

    return out;
}

template <typename TYPE, uint8_t LENGTH>
TYPE NumericHistory<TYPE, LENGTH>::get(uint8_t index)
{
    return _history[(_latestEntry + index) % LENGTH];
}

template <typename TYPE, uint8_t LENGTH>
uint8_t NumericHistory<TYPE, LENGTH>::length()
{
    return LENGTH;
}
