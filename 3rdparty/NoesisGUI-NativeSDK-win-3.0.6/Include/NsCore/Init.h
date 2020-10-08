////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_INIT_H__
#define __CORE_INIT_H__


#include <NsCore/Noesis.h>
#include <NsCore/KernelApi.h>


namespace Noesis
{

/// Initializes internal subsystems. Make sure to invoke SetLogHandler(), SetErrorHandler(),
/// SetMemoryCallbacks(), etc., before this function. The current implementation does not support
/// calling this function after Shutdown()
/// Read 'NoesisLicense.h' for more information about licensing parameters
NS_CORE_KERNEL_API void Init(const char* licenseName = "", const char* licenseKey = "");

/// Frees allocated resources and shutdown internal subsystems. Make sure to release all Noesis
/// objects and memory before invoking this function
NS_CORE_KERNEL_API void Shutdown();

}

#endif
