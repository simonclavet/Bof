#pragma once

#include <string>
#include <vector>
#include <map>
//
#include "utils/GoodSave.h"
#include "magic_enum.h"



using namespace std;


// this is actually a NoDataComponentVector:
// For each possible tag, a TagVector contains the entities having this tag
// A tag is just a number, and entity is just a number
class TagVector : public GoodSerializable
{
public:
    
    vector<GoodId> m_entities;

    // the location in entity for each entityId in m_entities
    map<GoodId, size_t> m_entityToIndex;

    inline void AddEntityId(GoodId entityId)
    {
        // todo: check if already here
        m_entityToIndex[entityId] = m_entities.size();
        m_entities.push_back(entityId);
    }

    inline bool HasCompForEntity(const GoodId& entityId) const
    {
        return m_entityToIndex.find(entityId) != m_entityToIndex.end();
    }

    inline const vector<GoodId> GetEntities() const { return m_entities; }

    GOOD_SERIALIZABLE(
        TagVector, GOOD_VERSION(1)
        , GOOD(m_entities)
    );

    inline void PostDeserialize()
    {
        m_entityToIndex.clear();

        for (size_t i = 0; i < m_entities.size(); i++)
        {
            m_entityToIndex[m_entities[i]] = i;
        }
    }

    inline size_t Size() const { return m_entities.size(); }

    GoodId operator[](size_t i) const { return m_entities[i]; }

};



// type erased version of the ComponentVector, which is just a tag vector, plus the component data
class ComponentVectorBase : public GoodSerializable
{
    // minimize the number of methods here. Virtual calls only when absolutelly necessary...
public:

    virtual GoodId GetClassIdVirtual() const = 0;
    virtual void PostDeserialize() = 0;

    virtual GoodId GetCompClassIdVirtual() const = 0;
    virtual const char* GetCompClassNameVirtual() const = 0;


    virtual pods::Error serialize(pods::JsonDeserializer<pods::InputBuffer>& jsonDeserializer, pods::Version) = 0;
    virtual pods::Error serialize(pods::BinaryDeserializer<pods::InputBuffer>& binaryDeserializer, pods::Version) = 0;
    virtual pods::Error serialize(pods::JsonSerializer<pods::ResizableOutputBuffer>& jsonSerializer, pods::Version) = 0;
    virtual pods::Error serialize(pods::PrettyJsonSerializer<pods::ResizableOutputBuffer>& jsonSerializer, pods::Version) = 0;
    virtual pods::Error serialize(pods::BinarySerializer<pods::ResizableOutputBuffer>& binarySerializer, pods::Version) = 0;
};



// this is a tag vector, with associated data. 
template<typename CompType>
class ComponentVector final : public ComponentVectorBase
{
public:
    vector<GoodId> m_entities;

    // the location in entity for each entityId in m_entities
    map<GoodId, size_t> m_entityToIndex;

    vector<CompType> m_comps; // the data associated with each entity in m_entities

    constexpr GoodId GetCompClassId() const { return CompType::GetClassId(); }
    constexpr const char* GetCompClassName() const { return CompType::GetClassName(); }

    virtual GoodId GetCompClassIdVirtual() const override { return GetCompClassId(); }
    virtual const char* GetCompClassNameVirtual() const override { return GetCompClassName(); }


    inline bool HasCompForEntity(const GoodId& entityId) const
    {
        return m_entityToIndex.find(entityId) != m_entityToIndex.end();
    }

    inline const vector<GoodId> GetEntities() const { return m_entities; }


    // don't keep this pointer, it will become invalid soonish
    inline CompType* AddEntityId(GoodId entityId)
    {
        // todo: check if already here
        m_entityToIndex[entityId] = m_entities.size();
        m_entities.push_back(entityId);
        CompType& comp = m_comps.emplace_back(CompType());
        return &comp;
    }


    // don't keep this pointer!
    inline CompType* GetCompIfExists(const GoodId& entityId)
    {
        auto it = m_entityToIndex.find(entityId);
        if (it != m_entityToIndex.end())
        {
            return &m_comps[it->second];
        }
        return nullptr;
    }
    // don't keep this pointer!
    inline const CompType* GetCompIfExists(const GoodId& entityId) const
    {
        auto it = const_cast<map<GoodId, size_t>&>(m_entityToIndex).find(entityId);
        if (it != m_entityToIndex.end())
        {
            return &m_comps[it->second];
        }
        return nullptr;
    }


    // take care of superclass serialization here, because speed, and also serializing hierarchies suck.
    GOOD_SERIALIZABLE_PARTIAL(
        ComponentVector, GOOD_VERSION(1));

    virtual void PostDeserialize()
    {
        m_entityToIndex.clear();

        for (size_t i = 0; i < m_entities.size(); i++)
        {
            m_entityToIndex[m_entities[i]] = i;
        }
    }

    inline size_t Size() const { return m_comps.size(); }

    inline GoodId GetEntityAtIndex(size_t index) const
    {
        return m_entities[index];
    }
    inline CompType& GetCompAtIndex(size_t index)
    {
        return m_comps[index];
    }
    inline const CompType& GetCompAtIndex(size_t index) const
    {
        return m_comps[index];
    }


#define THIS_REPEATED_SERIALIZE()\
    PODS_SAFE_CALL(serializer(GOOD(m_entities), GOOD(m_comps)))

    pods::Error serialize(pods::JsonDeserializer<pods::InputBuffer>& serializer, pods::Version) override
    {
        THIS_REPEATED_SERIALIZE();
        PostDeserialize();
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::BinaryDeserializer<pods::InputBuffer>& serializer, pods::Version) override
    {
        THIS_REPEATED_SERIALIZE();
        PostDeserialize();
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::JsonSerializer<pods::ResizableOutputBuffer>& serializer, pods::Version) override
    {
        THIS_REPEATED_SERIALIZE();
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::PrettyJsonSerializer<pods::ResizableOutputBuffer>& serializer, pods::Version) override
    {
        THIS_REPEATED_SERIALIZE();
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::BinarySerializer<pods::ResizableOutputBuffer>& serializer, pods::Version) override
    {
        THIS_REPEATED_SERIALIZE();
        return pods::Error::NoError;
    }
#undef THIS_REPEATED_SERIALIZE

};



class ComponentGrid : public GoodSerializable
{
public:


    ComponentGrid() = default;

    ComponentGrid(const ComponentGrid& other) = delete;
    //{
    //    m_tagMap = other.m_tagMap;

    //    for (auto& p : other.m_compVectorMap)
    //    {
    //        m_compVectorMap[p.first] = new ComponentVector
    //    }

    //}

    ~ComponentGrid()
    {
        ClearAll();
    }

    void ClearAll()
    {
        m_tagMap.clear();
        for (auto& p : m_compVectorMap)
        {
            delete p.second;
        }
        m_compVectorMap.clear();
    }

    inline TagVector& GetTagVector(GoodId tagId)
    {
        auto it = m_tagMap.find(tagId);
        if (it != m_tagMap.end())
        {
            return it->second;
        }
        m_tagMap[tagId] = TagVector();
        return m_tagMap[tagId];
    }

    template <class T>
    inline ComponentVector<T>* GetCompVector()
    {
        auto it = m_compVectorMap.find(T::GetClassId());
        if (it == m_compVectorMap.end())
        {
            std::cerr << "can't find comp vector " << T::GetClassName() << std::endl;
            return nullptr;
        }
        return static_cast<ComponentVector<T>*>(it->second);
    }
    template <class T>
    inline const ComponentVector<T>* GetConstCompVector() const
    {        
        auto it = const_cast<unordered_map<GoodId, ComponentVectorBase*>&>(m_compVectorMap).find(T::GetClassId());
        if (it == m_compVectorMap.end())
        {
            std::cerr << "can't find comp vector " << T::GetClassName() << std::endl;
            return nullptr;
        }
        return static_cast<const ComponentVector<T>*>(it->second);
    }

    template <class T>
    inline void AddCompVector()
    {        
        if (m_compVectorMap.find(T::GetClassId()) != m_compVectorMap.end())
        {
            std::cerr << "error: adding existing compvector " << T::GetClassName() << std::endl;
            return;
        }
        m_compVectorMap[T::GetClassId()] = new ComponentVector<T>();
    }





    GOOD_SERIALIZABLE_PARTIAL(ComponentGrid, GOOD_VERSION(1));


#define THIS_REPEATED_SERIALIZE()\
    PODS_SAFE_CALL(serializer(GOOD(m_tagMap)));\
    for (auto& p : m_compVectorMap)\
    {\
        PODS_SAFE_CALL(serializer(p.second->GetCompClassNameVirtual(), *p.second));\
    }

    pods::Error serialize(pods::JsonDeserializer<pods::InputBuffer>& serializer, pods::Version)
    {
        THIS_REPEATED_SERIALIZE();
        for (auto& p : m_tagMap)
        {
            p.second.PostDeserialize();
        }
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::BinaryDeserializer<pods::InputBuffer>& serializer, pods::Version)
    {
        THIS_REPEATED_SERIALIZE();
        for (auto& p : m_tagMap)
        {
            p.second.PostDeserialize();
        }
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::JsonSerializer<pods::ResizableOutputBuffer>& serializer, pods::Version)
    {
        THIS_REPEATED_SERIALIZE();
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::PrettyJsonSerializer<pods::ResizableOutputBuffer>& serializer, pods::Version)
    {
        THIS_REPEATED_SERIALIZE();
        return pods::Error::NoError;
    }
    pods::Error serialize(pods::BinarySerializer<pods::ResizableOutputBuffer>& serializer, pods::Version)
    {
        THIS_REPEATED_SERIALIZE();
        return pods::Error::NoError;
    }
#undef THIS_REPEATED_SERIALIZE


    unordered_map<GoodId, TagVector> m_tagMap;

    unordered_map<GoodId, ComponentVectorBase*> m_compVectorMap;

private: 

    //inline ComponentVectorBase* GetCompVector(CompTypes compType)
    //{
    //    return m_impl->GetCompVector(compType);
    //}
};



//#define ADD_COMP(grid, comp, entityId) static_cast<comp*>(grid.AddComp(CompTypes::comp, entityId))
//#define GET_COMP(grid, comp, entityId) static_cast<comp*>(grid.GetCompIfExists(CompTypes::comp, entityId))

#define GET_COMPS(grid, comp) (*grid.GetCompVector<comp>())
#define GET_CONST_COMPS(grid, comp) (*grid.GetConstCompVector<comp>())

#define GET_TAGS(grid, tagId) grid.GetTagVector(tagId)


