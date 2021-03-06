﻿#pragma once

#include <type_traits>

#include "../errors.h"
#include "../types.h"

namespace pods
{
    namespace details
    {
        template <class T>
        struct IsPodsSerializableImpl
        {
        private:
            static void check(...);

            template <class U>
            static decltype(U::version()) check(U*);

        public:
            static constexpr bool value =
                std::is_same<Version, decltype(IsPodsSerializableImpl<T>::check(static_cast<std::remove_reference_t<T>*>(nullptr)))>::value;
        };

        template <class T>
        struct IsPodsSerializable
            : std::integral_constant<bool, IsPodsSerializableImpl<T>::value>
        {
        };

        constexpr bool isOptional(const char* name) noexcept
        {
            return *name == 0;
        }

        constexpr const char* getName(const char* name) noexcept
        {
            return isOptional(name)
                ? name + 1
                : name;
        }

        inline Error checkSize(size_t size) noexcept
        {
            return size < MaxSize
                ? pods::Error::NoError
                : Error::SizeToLarge;
        }
    }
}

#ifdef PODS_SAFE_CALL
#error Rename the macro
#endif

#define USE_PODS_SAFE_CALL 1



#if USE_PODS_SAFE_CALL

#define PODS_SAFE_CALL(foo)                     \
    do                                          \
    {                                           \
        const auto safeCallError = (foo);       \
        if (safeCallError != pods::Error::NoError)    \
        {                                       \
            __debugbreak();\
            return safeCallError;               \
        }                                       \
    } while (false)

#else

#define PODS_SAFE_CALL(foo) foo

#endif