
#pragma once

#include <iostream>
#include <vector>
#include <unordered_map>

#include <cstdlib>
#include <algorithm>

using namespace std; // remove this eventually

// goals: Behaves like a sorted map, but
// 1: cache-friendly ordered traversal of values (contiguous memory)
// 2: O(1) find 
// 3: don't care (too much) about insertion, deletion time. Be lazy and dirtyfy at modification time.
// Ideally, Key should be just ints, values should not be pointers. Because hashes for long strings are long to compute anyway.
template <typename Key, typename Value>
class HashSortedVector
{
    vector<pair<Key, Value>> m_vec;
    unordered_map<Key, size_t> m_indices;

    bool m_dirty = true;

public:

    void Add(const Key& key, const Value& val)
    {
        // forget about sorting for now
        m_vec.push_back(make_pair(key, val));
        m_dirty = true;
    }

    // Please don't modify the keys, or the vector directly. Just the values.
    vector<pair<Key, Value>>& GetKeyValuePairs()
    {
        if (m_dirty)
        {
            SortAndBuildIndices();
        }
        return m_vec;
    }

    const vector<pair<Key, Value>>& GetKeyValuePairs() const
    {
        if (m_dirty)
        {
            // bah
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

    Value Remove(const Key& key)
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


int mmain(int argc, char* argv[])
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
        size_t key = rand();
        vec.Add(key, b);
    }



    cout << vec.Get(100)->d << endl;

    for (auto& [key, bouse] : vec.GetKeyValuePairs())
    {
        cout << "key:" << key << ", bouse.a=" << bouse.a << endl;
        bouse.a = 543;
    }

    Bouse removedBouse = vec.Remove(100);

    cout << removedBouse.d << endl;



    return 0;
}