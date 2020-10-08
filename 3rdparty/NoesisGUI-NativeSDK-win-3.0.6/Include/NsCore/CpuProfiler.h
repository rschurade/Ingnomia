////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_CPUPROFILER_H__
#define __CORE_CPUPROFILER_H__


#include <NsCore/Noesis.h>


#ifndef NS_PROFILER_ENABLED
    #define NS_PROFILER_ENABLED 0
#endif


#if NS_PROFILER_ENABLED

    // https://github.com/bombomby/brofiler
    #ifdef NS_BROFILER
        #include "Brofiler.h"
        #pragma comment(lib, "ProfilerCore64.lib")
        #ifndef NS_PROFILER_CPU_FRAME
            #define NS_PROFILER_CPU_FRAME BROFILER_FRAME("Frame")
        #endif
        #ifndef NS_PROFILE_CPU
            #define NS_PROFILE_CPU(name) BROFILER_CATEGORY(name, Profiler::Color::Blue)
        #endif
    #endif

#endif

#ifndef NS_PROFILER_CPU_FRAME
    #define NS_PROFILER_CPU_FRAME NS_NOOP
#endif

#ifndef NS_PROFILE_CPU
    #define NS_PROFILE_CPU(...) NS_NOOP
#endif

#endif
