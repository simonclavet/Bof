#pragma once


#include <cstdio>
#include <cstdarg>





#define BOF_UNUSED(x) do {(void)sizeof(x);} while(0)
#define BOF_UNUSED2(x, y) do {(void)sizeof(x);(void)sizeof(y);} while(0)
#define BOF_UNUSED3(x, y, z) do {(void)sizeof(x);(void)sizeof(y);(void)sizeof(z);} while(0)
#define BOF_UNUSED4(x, y, z, w) do {(void)sizeof(x);(void)sizeof(y);(void)sizeof(z);(void)sizeof(w);} while(0)

// this is set only by release. Which means final/retail...
#ifndef NO_BOF_ASSERTS

// This should be true only when testing a build without a debugger attached
// so artists automatically skip asserts but still log them.
// Automated tools could check logs and warn the adequate people.
// todo: Pick this up from the commandline?
inline bool g_BofAssertsNoDebuggerLogButContinue = false;

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

    printf("\nASSERT! %s(%d): ", file, line);
    if (condition != nullptr)
    {
        printf("'%s' ", condition);
    }
    if (message != nullptr)
    {
        printf("%s", message);
    }
    printf("\n\n");

    if (g_BofAssertsNoDebuggerLogButContinue)
    {
        return BOF_CONTINUE;
    }
    else
    {
        return BOF_HALT;
    }
}




#define BOF_DEBUG_BREAK() { __debugbreak(); }


#define BOF_ASSERT(condition)\
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


#define BOF_ASSERT_MSG(condition, msg, ...)\
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

#define BOF_ASSERT(x) BOF_UNUSED(x)
#define BOF_ASSERT_MSG(x, msg, ...) BOF_UNUSED2(x, msg)
#define BOF_FAIL(msg, ...) BOF_UNUSED(msg)


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







