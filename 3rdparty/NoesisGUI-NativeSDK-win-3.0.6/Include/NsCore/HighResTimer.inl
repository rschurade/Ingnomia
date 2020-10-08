////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Error.h>

#if defined(NS_PLATFORM_WINDOWS)
    union _LARGE_INTEGER;
    typedef _LARGE_INTEGER LARGE_INTEGER;
    extern "C" __declspec(dllimport) int __stdcall QueryPerformanceCounter(_Out_ LARGE_INTEGER*);
    extern "C" __declspec(dllimport) int __stdcall QueryPerformanceFrequency(_Out_ LARGE_INTEGER*);
#elif defined(NS_PLATFORM_APPLE)
    #include <mach/mach_time.h>
#elif defined(NS_PLATFORM_EMSCRIPTEN)
    #include <emscripten.h>
#else
    #include <sys/time.h>
#endif


namespace Noesis
{
namespace HighResTimer
{

////////////////////////////////////////////////////////////////////////////////////////////////////
inline uint64_t Ticks()
{
#if defined(NS_PLATFORM_WINDOWS)
    uint64_t count;
    QueryPerformanceCounter((LARGE_INTEGER*)&count); // will always succeed
    return count;

#elif defined(NS_PLATFORM_APPLE)
    return mach_absolute_time();

#elif defined(NS_PLATFORM_EMSCRIPTEN)
    return (uint64_t)(emscripten_get_now() * 1000.0);

#else
    timeval tv;
    int error = gettimeofday(&tv, 0);
    NS_ASSERT(error == 0);
    return (uint64_t)tv.tv_usec + ((uint64_t)tv.tv_sec * 1000000);
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline double Seconds(uint64_t ticks)
{
#if defined(NS_PLATFORM_WINDOWS)
    static uint64_t frequency;
    if (NS_UNLIKELY(frequency == 0))
    {
        QueryPerformanceFrequency((LARGE_INTEGER*)&frequency); // will always succeed
    }

    return (double)ticks / frequency;

#elif defined(NS_PLATFORM_APPLE)
    static mach_timebase_info_data_t tbi;
    if (NS_UNLIKELY(tbi.denom == 0))
    {
        (void)mach_timebase_info(&tbi);
    }

    return (double)ticks * tbi.numer / (tbi.denom * 1e9);

#else
    return double(ticks) / 1000000.0;

#endif
}

}
}
