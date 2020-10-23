#pragma once


#include <cstdio>
#include <cstdarg>


#define BOF_ASSERTS_ENABLE

//#define BOF_ASSERTS_PRINT_BUT_CONTINUE




#define BOF_UNUSED(x) do {(void)sizeof(x);} while(0)



#ifdef BOF_ASSERTS_ENABLE


#define BOF_HALT 0
#define BOF_CONTINUE 1

inline int ReportAssertFailure(
    const char* condition,
    const char* file,
    const int line,
    const char* msg, ...)
{
    const char* message = nullptr;
    if (msg != nullptr)
    {
        char messageBuffer[1024];
        {
            va_list args;
            va_start(args, msg);
            vsnprintf(messageBuffer, 1024, msg, args);
            va_end(args);
        }
        message = messageBuffer;
    }

    printf("%s(%d): Assert: ", file, line);
    if (condition != nullptr)
    {
        printf("'%s' ", condition);
    }
    if (message != nullptr)
    {
        printf("%s", message);
    }
    printf("\n");

#ifdef BOF_ASSERTS_PRINT_BUT_CONTINUE
    return BOF_CONTINUE;
#else
    return BOF_HALT;
#endif

}




#define BOF_DEBUG_BREAK() {__debugbreak(); }


#define BOF_ASSERT_NO_MESSAGE(condition)\
    do\
    {\
        if (!(condition))\
        {\
            if (ReportAssertFailure(#condition, __FILE__, __LINE__, 0) == BOF_HALT)\
            {\
                BOF_DEBUG_BREAK();\
            }\
        }\
    }\
    while (0)


#define BOF_ASSERT(condition, msg, ...)\
    do\
    {\
        if (!(condition))\
        {\
            if (ReportAssertFailure(#condition, __FILE__, __LINE__, (msg), __VA_ARGS__) == BOF_HALT)\
            {\
                BOF_DEBUG_BREAK();\
            }\
        }\
    }\
    while (0)


#define BOF_FAIL(msg, ...)\
    do\
    {\
        if (ReportAssertFailure(nullptr, __FILE__, __LINE__, (msg), __VA_ARGS__) == BOF_HALT)\
        {\
            BOF_DEBUG_BREAK();\
        }\
    }\
    while (0)



#else // BOF_ASSERTS_ENABLE

#define BOF_ASSERT_NO_MESSAGE(x) do {(void)sizeof(x);} while(0)
#define BOF_ASSERT(x, msg, ...) do {(void)sizeof(x);(void)sizeof(msg);} while(0)
#define BOF_FAIL(msg, ...) do {(void)sizeof(msg);} while(0)



#endif // BOF_ASSERTS_ENABLE

// if some function really wants to throw, and you want to assert on throw instead, wrap the call with this
// In release it will just catch and continue, so don't expect much. Throwing is just the same as asserting for us.
#define BOF_TRY_CATCH(expression)\
    do\
    {\
        try\
        {\
            expression;\
        }\
        catch (std::runtime_error e)\
        {\
            BOF_FAIL(e.what());\
        }\
    }\
    while (0)







