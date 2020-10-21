#pragma once

#ifdef BOF_PLATFORM_WINDOWS

    #ifdef BOF_BUILD_DLL
        #define BOF_API __declspec(dllexport)
    #else
        #define BOF_API __declspec(dllimport) 
    #endif
#else

#error only windows is supported

#endif


namespace Bof
{
    //BOF_API void Allo();

    class BOF_API Application
    {
    public:
        virtual ~Application() {};
        virtual void run() = 0;
    };

    Application* CreateApplication();

}