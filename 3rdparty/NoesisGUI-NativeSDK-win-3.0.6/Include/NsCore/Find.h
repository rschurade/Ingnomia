////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_FIND_H__
#define __CORE_FIND_H__


#include <NsCore/Noesis.h>


namespace Noesis
{

struct FindData
{
    char filename[512];
    char extension[16];
    void* handle;
};

/// Searches a directory for files with the given extension (".dll" for example).
/// Returns false if no files found.
bool FindFirst(const char* directory, const char* extension, FindData& findData);

/// Continues a file search from a previous call to the FindFirst function
bool FindNext(FindData& findData);

/// Closes a file search handle opened by the FindFirst function
void FindClose(FindData& findData);

}

#include "Find.inl"

#endif
