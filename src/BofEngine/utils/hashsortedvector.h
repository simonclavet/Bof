#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

#include <cstdlib>
#include <algorithm>


// goals: Behaves like a sorted map, but
// 1: cache-friendly ordered traversal of values (simple contiguous memory, without complicated sparse bin map thing)
// 2: O(1) find. No compromise there. logn is for pussies.
// 3: Don't care (too much) about insertion, deletion time.
// Be lazy and dirtyfy at modification time, the first find after a modif can be super long.
// Ideally, Key should be just int (because hashes for long strings are long to compute).
// Values should not be pointers. That would defeat the purpose. Ideal values are medium size POD data.
template <typename Key, typename Value>
class HashSortedVector
{
    std::vector<std::pair<Key, Value>> m_vec;
    std::unordered_map<Key, size_t> m_indices;

    bool m_dirty = true;

public:

    void Add(const Key& key, const Value& val)
    {
        // forget about sorting for now
        m_vec.push_back(std::make_pair(key, val));
        m_dirty = true;
    }

    // Please don't modify the keys, or the vector directly. Just the values.
    // I don't have time to deal with const with just the values. Be carefull.
    std::vector<std::pair<Key, Value>>& GetKeyValuePairs()
    {
        if (m_dirty)
        {
            SortAndBuildIndices();
        }
        return m_vec;
    }

    const std::vector<std::pair<Key, Value>>& GetKeyValuePairs() const
    {
        if (m_dirty)
        {
            // bah. Sorry.
            HashSortedVector<Key, Value>& deconstThis = const_cast<HashSortedVector<Key, Value>&>(*this);
            deconstThis.SortAndBuildIndices();
        }
        return m_vec;
    }


    const Value* Get(const Key& key)
    {
        if (m_dirty)
        {
            SortAndBuildIndices();
        }
        auto it = m_indices.find(key);
        if (it != m_indices.end())
        {
            size_t index = it->second;
            return &m_vec[index].second;
        }
        return nullptr;
    }

    std::optional<Value> Remove(const Key& key)
    {
        // todo: remove many
        if (m_dirty)
        {
            SortAndBuildIndices();
        }

        auto it = m_indices.find(key);
        if (it != m_indices.end())
        {
            size_t index = it->second;
            m_indices.erase(it);
            
            Value erasedValue = m_vec[index].second;

            m_vec.erase(m_vec.begin() + index);
            
            m_dirty = true;

            return erasedValue;
        }
        return {};
    }

private:
    void SortAndBuildIndices()
    {
        std::sort(m_vec.begin(), m_vec.end(), [](const pair<Key, Value>& pair0, const pair<Key, Value>& pair1)
            {
                return pair0.first < pair1.first;
            });

        m_indices.clear();
        for (size_t i = 0; i < m_vec.size(); i++)
        {
            m_indices[m_vec[i].first] = i;
        }
        m_dirty = false;
    }


};


class Bouse
{
public:
    int a, b, c, d, e, f, g, h, i, j;
};


int mmain(int /*argc*/, char* /*argv[]*/)
{
    HashSortedVector<int, Bouse> vec;

    Bouse bouse;
    bouse.d = 123;
    vec.Add(100, bouse);


    for (int i = 0; i < 10; i++)
    {
        Bouse b;
        b.a = 2 + i;
        b.b = 3;
        int key = rand();
        vec.Add(key, b);
    }



    std::cout << vec.Get(100)->d << std::endl;

    for (auto& [key, boute] : vec.GetKeyValuePairs())
    {
        std::cout << "key:" << key << ", bouse.a=" << boute.a << std::endl;
        boute.a = 543;
    }

    Bouse removedBouse = vec.Remove(100).value();

    std::cout << removedBouse.d << std::endl;



    return 0;
}