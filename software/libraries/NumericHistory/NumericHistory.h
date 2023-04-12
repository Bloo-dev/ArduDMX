#ifndef NumericHistory_h
#define NumericHistory_h

/**
 * @brief NumericHistory is a simple, internally manged discarding queue capable of storing up to 256 `TYPE` numbers.
 * Once `LENGTH` amount of entries were stored, the oldest entries will start to be overwritten by new entries in chronological order.
 * 
 * @tparam LENGTH Length of the queue. This defines when to start overwriting old entries with new ones. Smaller values save on memory.
 */
template <typename TYPE, uint8_t LENGTH>
class NumericHistory
{
public:
    /**
     * @brief Construct a new Numeric History object.
     */
    NumericHistory();

    /**
     * @brief Adds a new value to the history, thereby replacing the oldest known value within the history.
     * Only the oldest element of the history is accessed by this operation, making it perform equally independent of history length.
     *
     * @param value The value to be written to the queue.
     */
    void update(TYPE value);

    /**
     * @brief Returns the full history.
     * 
     * @return TYPE* A pointer to the start of the history array in memory.
     * The user must be careful not to access memory past the last entry, this can be ensure via checking the length of the history.
     * 
     * @see length
     */
    TYPE *get();

    /**
     * @brief Returns a single entry from history.
     * 
     * @param index The index of the entry to be returned, relative to the latest entry. `index=0` will yield the latest entry itself, `index=1` will yield the nextoldest entry and so on.
     * The user must ensure not to query entries past the history's length.
     * @return TYPE 
     */
    TYPE get(uint8_t index);

    /**
     * @brief Returns the length of the history, and thereby the internal array.
     * 
     * @return uint8_t Amount of elements in the internal array used by this instance of NumericHistory.
     */
    uint8_t length();

private:
    TYPE _history[LENGTH];
    uint8_t _latestEntry;
};

#include "NumericHistory.tpp"
#endif
