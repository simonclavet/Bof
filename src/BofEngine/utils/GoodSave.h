#pragma once

#include <assert.h>

#include <string>
#include <fstream>
#include <iostream>
#include "pods/pods.h"
#include "pods/binary.h"

#include "pods/json.h"
#include "pods/buffers.h"
#include "pods/streams.h"

#include "Timer.h"

#include <unordered_map>


constexpr uint32_t Hash32_CT(const char* str, size_t n, uint32_t basis = UINT32_C(2166136261)) 
{
    return n == 0 ? basis : Hash32_CT(str + 1, n - 1, (basis ^ str[0]) * UINT32_C(16777619));
}

constexpr uint64_t Hash64_CT(const char* str, size_t n, uint64_t basis = UINT64_C(14695981039346656037)) 
{
    return n == 0 ? basis : Hash64_CT(str + 1, n - 1, (basis ^ str[0]) * UINT64_C(1099511628211));
}

template< size_t N >
constexpr uint32_t Hash32_CT(const char(&s)[N]) 
{
    return Hash32_CT(s, N - 1);
}

template< size_t N >
constexpr uint64_t Hash64_CT(const char(&s)[N]) 
{
    return Hash64_CT(s, N - 1);
}

typedef uint64_t GoodId;

enum class GoodFormat : int
{
    Binary,
    Json,

    Count,
};




class GoodSerializable
{
public:
    
    virtual ~GoodSerializable() = default;

    // only a couple of virtuals. the rest is static helpers
    virtual GoodId GetClassIdVirtual() const = 0;


    // those are sad, but necessary for typed deserialization
    virtual pods::Error DeserializeVirtual(pods::JsonDeserializer<pods::InputBuffer>& jsonDeserializer) = 0;
    virtual pods::Error DeserializeVirtual(pods::BinaryDeserializer<pods::InputBuffer>& binaryDeserializer) = 0;

    template <class T>
    inline T& Cast() { return static_cast<T&>(*this); }
    template <class T>
    inline const T& Cast() const { return static_cast<const T&>(*this); }
    
    inline bool InstanceOf(GoodId classId) const { return classId == GetClassIdVirtual(); }



    using CreateSerializableFuncType = std::unique_ptr<GoodSerializable>(*)();

    inline static std::unique_ptr<GoodSerializable> Create(GoodId classId)
    {
        const std::unordered_map<GoodId, CreateSerializableFuncType>& createFuncsMap = GoodSerializable::GetCreateFuncs();
        std::unordered_map<GoodId, CreateSerializableFuncType>::const_iterator it = createFuncsMap.find(classId);
        if (it != createFuncsMap.end())
        {
            return it->second();
        }
        std::cerr << "forgot to register class with classId:" << classId << std::endl;
        return nullptr;
    }



    static constexpr pods::Version version() { return 0; }

protected:


    template <typename T>
    inline static void RegisterClassInternal()
    {
        GetCreateFuncs()[T::GetClassId()] = []()->std::unique_ptr<GoodSerializable>
        {
            return std::make_unique<T>();
        };
    }

private:
    inline static std::unordered_map<GoodId, CreateSerializableFuncType>& GetCreateFuncs()
    {
        static std::unordered_map<GoodId, CreateSerializableFuncType> classIdToCreateFuncMap;
        return classIdToCreateFuncMap;
    }

};





#ifdef GOOD
#error Rename the macro
#endif
#define GOOD(field) #field, field

#ifdef GOOD_BIN
#error Rename the macro
#endif
#define PODS_MDR_BIN(field) #field, ::pods::details::makeBinary(field)

#ifdef GOOD_BIN_2
#error Rename the macro
#endif
#define GOOD_BIN_2(field, size) #field, ::pods::details::makeBinary2(field, size)



#ifdef GOOD_VERSION
#error Rename the macro
#endif
#ifdef GOOD_SERIALIZABLE
#error Rename the macro
#endif


#define GOOD_VERSION(__ver) __ver


// everything except serialize, that you will define yourself
#define GOOD_SERIALIZABLE_PARTIAL(NAME, __goodVersion, ...)\
    \
    static constexpr pods::Version version() { return __goodVersion; }\
    \
    static constexpr const char* GetClassName() {return #NAME;}\
    \
    static constexpr GoodId GetClassId() {return Hash64_CT(#NAME);}\
    \
    inline GoodId GetClassIdVirtual() const override {return GetClassId();}\
    \
    inline static void RegisterClass() {RegisterClassInternal<NAME>();}\
    \
    inline bool Equals(const NAME& other) const { return GoodHelpers::AreEqual(*this, other); } \
    \
    inline pods::Error DeserializeVirtual(pods::JsonDeserializer<pods::InputBuffer>& jsonDeserializer) override \
    {\
        return jsonDeserializer.load(*this);\
    }\
    inline pods::Error DeserializeVirtual(pods::BinaryDeserializer<pods::InputBuffer>& binaryDeserializer) override \
    {\
        return binaryDeserializer.load(*this);\
    }\



#define GOOD_SERIALIZABLE(NAME, __goodVersion, ...)\
    \
    template <class Serializer>\
    pods::Error serialize(Serializer& serializer, pods::Version)\
    {\
        return serializer(__VA_ARGS__);\
    }\
    GOOD_SERIALIZABLE_PARTIAL(NAME, __goodVersion)






class GoodHelpers
{
public:

    template <class T>
    inline static pods::Error SerializeTyped(
        pods::ResizableOutputBuffer& out,
        const T& thing, // this needs to be a goodserializable
        GoodFormat format = GoodFormat::Binary)
    {
        pods::Error error = out.put(T::GetClassId());

        if (error != pods::Error::NoError)
        {
            return error;
        }

        switch (format)
        {
        case GoodFormat::Binary:
        {
            pods::BinarySerializer<decltype(out)> serializer(out);
            return serializer.save(thing);
        }
        case GoodFormat::Json:
        {
            pods::PrettyJsonSerializer<decltype(out)> serializer(out);
            return serializer.save(thing);
        }
        default: assert(false);
        }
        return error;
    }

    inline static std::unique_ptr<GoodSerializable> DeserializeTyped(
        pods::InputBuffer& inBuffer,
        GoodFormat format = GoodFormat::Binary)
    {
        GoodId classId;
        pods::Error error = inBuffer.get(classId);

        if (error != pods::Error::NoError)
        {
            std::cerr << "error:" << (int)error << std::endl;
            return nullptr;
        }

        std::unique_ptr<GoodSerializable> thing = GoodSerializable::Create(classId);

        if (thing == nullptr)
        {
            std::cerr << "error: can't find class type " << classId << std::endl;
            return nullptr;
        }

        switch (format)
        {
        case GoodFormat::Json:
        {
            pods::JsonDeserializer<pods::InputBuffer> jsonDeserializer(inBuffer);
            error = thing->DeserializeVirtual(jsonDeserializer);
            break;
        }
        case GoodFormat::Binary:
        {
            pods::BinaryDeserializer<pods::InputBuffer> binaryDeserializer(inBuffer);
            error = thing->DeserializeVirtual(binaryDeserializer);
            break;
        }
        }

        if (error != pods::Error::NoError)
        {
            std::cerr << "error:" << (int)error << std::endl;
            return nullptr;
        }

        return thing;
    }


    template <typename T>
    static pods::Error WriteToFile(const T& thing, std::string filenameWithoutExt, GoodFormat format = GoodFormat::Binary)
    {
        std::string filename = filenameWithoutExt;

        pods::ResizableOutputBuffer out;

        pods::Error error = pods::Error::NoError;

        switch (format)
        {
        case GoodFormat::Binary:
        {
            filename.append(".bin");
            pods::BinarySerializer<decltype(out)> serializer(out);
            {
                TIMED << "Serializing binary... ";
                error = serializer.save(thing); 
            }
            break;
        }
        case GoodFormat::Json:
        {
            filename.append(".json");
            pods::PrettyJsonSerializer<decltype(out)> serializer(out);
            {
                TIMED << "Serializing json... ";
                error = serializer.save(thing);
            }
            break;
        }
        default: assert(false);
        }

        size_t outLength = out.size();

        if (error != pods::Error::NoError)
        {
            std::cerr << "serialization error " << (uint32_t)error << " while writing " << filename << std::endl;
            return error;
        }


        //auto myfile = std::fstream(filename, std::ios::out | std::ios::binary);
        //myfile.write((char*)&data[0], bytes);


        std::ofstream myfile;
        myfile.open(filename, std::ios_base::out | std::ios::binary);
        if (myfile.is_open())
        {
            {
                TIMED << "Writing " << out.size() << " bytes...";
                myfile.write((char*)&out.data()[0], out.size());
            }
        }
        else
        {
            myfile.close();
            std::cerr << "can't save file " << filename << std::endl;
            return pods::Error::WriteError;
        }
        myfile.close();

        // make sure reading the file gives the same data
        static bool checkRead = false;
        if (checkRead)
        {
            std::ifstream file(filename, std::ios::in | std::ios::binary);
            if (file.is_open())
            {
                std::vector<char> charVecBuffer;

                file.seekg(0, file.end);
                size_t inLength = (size_t)file.tellg();

                file.seekg(0, file.beg);

                if (inLength > 0)
                {
                    charVecBuffer.resize(inLength);
                    file.read(&charVecBuffer[0], inLength);
                }
                else
                {
                    return pods::Error::CorruptedArchive;
                }

                if (inLength != outLength)
                {
                    std::cout << "fuck" << std::endl;
                }
                for (size_t i = 0; i < inLength; i++)
                {
                    //char a = charVecBuffer[i];
                    //char b = (char)out.data()[i];
                    if (charVecBuffer[i] != out.data()[i])
                    {
                        std::cout << "fuck" << std::endl;
                    }
                }                
            }
            file.close();
        }
        return error;
    }




    template <typename T>
    static pods::Error ReadFromFile(T& thing, const std::string& filenameWithoutExt, GoodFormat format = GoodFormat::Binary)
    {
        std::string filename = filenameWithoutExt;
        switch (format)
        {
        case GoodFormat::Binary:
        {
            filename.append(".bin");
            break;
        }
        case GoodFormat::Json:
        {
            filename.append(".json");
            break;
        }
        default: assert(false);
        }

        std::ifstream file(filename, std::ios::in | std::ios::binary);
        if (file.is_open())
        {
            std::vector<char> charVecBuffer;
            size_t length;
            {
                TIMED << "Reading bytes...";
                file.seekg(0, file.end);
                length = file.tellg();

                file.seekg(0, file.beg);

                if (length > 0)
                {
                    charVecBuffer.resize(length);
                    file.read(&charVecBuffer[0], length);
                }
                else
                {
                    return pods::Error::CorruptedArchive;
                }
            }
            
            pods::InputBuffer buffer(&charVecBuffer[0], length);


            pods::Error error = pods::Error::NoError;

            switch (format)
            {
            case GoodFormat::Binary:
            {
                pods::BinaryDeserializer<decltype(buffer)> deserializer(buffer);
                {
                    TIMED << "Deserializing binary... ";
                    error = deserializer.load(thing);
                }
                break;
            }
            case GoodFormat::Json:
            {
                pods::JsonDeserializer<decltype(buffer)> deserializer(buffer);
                {
                    TIMED << "Deserializing json... ";
                    error = deserializer.load(thing);
                }
                break;
            }
            default: file.close(); assert(false && "missing format");
            }

            if (error != pods::Error::NoError)
            {
                file.close();
                std::cerr << "deserialization error " << (uint32_t)error << " while reading " << filename << std::endl;
                return error;
            }
        }
        else
        {
            std::cerr << "can't find json file " << filename << std::endl;
            return pods::Error::ReadError;
        }
        file.close();

        return pods::Error::NoError;
    }

    static bool AreEqual(const pods::ResizableOutputBuffer& buffer, const pods::ResizableOutputBuffer& other)
    {
        if (buffer.size() != other.size())
        {
            return false;
        }
        size_t s = buffer.size();
        for (size_t i = 0; i < s; i++)
        {
            if (buffer.data()[i] != other.data()[i])
            {
                return false;
            }
        }
        return true;
    }

    template <typename T>
    inline static bool AreEqual(const T& thing0, const T& thing1)
    {
        pods::ResizableOutputBuffer out0;
        pods::BinarySerializer<decltype(out0)> serializer0(out0);
        pods::Error error = serializer0.save(thing0);
        assert(error == pods::Error::NoError);

        pods::ResizableOutputBuffer out1;
        pods::BinarySerializer<decltype(out1)> serializer1(out1);
        error = serializer1.save(thing1);
        assert(error == pods::Error::NoError);

        return AreEqual(out0, out1);
    }

};


