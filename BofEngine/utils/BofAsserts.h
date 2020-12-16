#pragma once


#include <cstdio>
#include <cstdarg>




// prevent unused variable warnings
// just for simple variable lists. no expressions
#define UNUSED(...) do {(void)sizeof(__VA_ARGS__);} while(0)



// this is set only by release. Which means final/retail...
#ifndef NO_BOF_ASSERTS

// This should be true only when testing a build without a debugger attached
// so artists skip asserts but still log them.
// Automated tools should warn adequate people.
// todo: Pick this up from the commandline?
inline bool g_BofAssertsNoDebuggerLogButContinue = false;

#define BOF_HALT 0
#define BOF_CONTINUE 1

#pragma warning(push)

#pragma warning(disable:4710) // prevent warning function not inlined

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


#pragma warning(pop)





#define BOF_DEBUG_BREAK() { __debugbreak(); }

// don't do useful things in asserts. They are completelly removed in retail
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

#define BOF_ASSERT(x) do {(void)sizeof(x);} while(0)
#define BOF_ASSERT_MSG(x, msg, ...) do {(void)sizeof(x);(void)sizeof(msg);} while(0)
#define BOF_FAIL(msg, ...) do {(void)sizeof(msg);} while(0)


#endif // BOF_ASSERTS_ENABLE

// If some function really wants to throw, and you want to assert on throw instead, wrap the call with this
// In release it will just catch and continue, so don't expect much. Throwing is just the same as asserting for us.
// Games never use throwing as a control flow. Assert fast even in release (but not retail) and fix stuff so it never happens again.
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







