#pragma once

#include <array>

using namespace std;

// Like a vector, but with a fixed size N (say 100)
// Then if you Add 250 things to it, you can get the 100 things at index from 150 to 249 included.
// The current item is the one at index 249
// If you ask a thing outside of this range it crashes. 
// This enables saving a history of most recent things, without thinking too much about it.
// At the beginning the current index is -1, because there is nothing.
// Can't remove things, because that would be stupid.

template <class T, size_t N>
class RingBuffer
{
public:

    RingBuffer() : m_currentIndex(-1) {}

    inline T& Push()
    {
        m_currentIndex++;
        return m_array[m_currentIndex % N];
    }
    inline T& Push(const T& element)
    {
        m_currentIndex++;
        m_array[m_currentIndex % N] = element;
        return m_array[m_currentIndex % N];
    }

    int64_t GetCurrentIndex() const { return m_currentIndex; }


    inline T& GetCurrent() { return m_array[m_currentIndex]; }
    inline const T& GetCurrent() const { return m_array[m_currentIndex]; }

    inline int64_t GetMinimumAvailableIndex() const { return std::max((int64_t)0, (int64_t)(m_currentIndex - N + 1)); }
    inline int64_t GetMaximumAvailableIndex() const { return m_currentIndex; }

    inline T& Get(int64_t index)
    {
        assert(index <= GetMaximumAvailableIndex());
        assert(index >= GetMinimumAvailableIndex());

        return m_array[index % N];
    }
    inline const T& Get(int64_t index) const
    {
        assert(index <= GetMaximumAvailableIndex());
        assert(index >= GetMinimumAvailableIndex());

        return m_array[index % N];
    }

    // zero means current. one means the last one.
    const T& GetPastItem(int64_t pastIndex) const
    {
        return Get(m_currentIndex - pastIndex);
    }
    T& GetPastItem(int64_t pastIndex)
    {
        return Get(m_currentIndex - pastIndex);
    }

    constexpr size_t GetCapacity() const { return N; }
    constexpr size_t GetSize() const { return std::min(m_currentIndex + 1, (int64_t)N); }

    constexpr size_t GetWithInternalIndex(int64_t index) const { return m_array[index]; }

private:
    array<T, N> m_array;

    int64_t m_currentIndex;

};