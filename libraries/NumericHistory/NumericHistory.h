#ifndef NumericHistory_h
#define NumericHistory_h

template <int LENGTH>
class NumericHistory
{
public:
    NumericHistory();
    void update(uint16_t);
    uint16_t *get();
    uint8_t length();

private:
    uint16_t _history[LENGTH];
    uint8_t _oldestEntry;
};

#include "NumericHistory.tpp"
#endif
