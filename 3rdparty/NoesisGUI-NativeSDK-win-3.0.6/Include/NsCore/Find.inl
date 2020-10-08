////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/StringUtils.h>
#include <NsCore/UTF8.h>


#if defined(NS_PLATFORM_WINDOWS)
    #include <sal.h>

    struct _WIN32_FIND_DATAW;
    typedef _WIN32_FIND_DATAW* LPWIN32_FIND_DATAW;
    enum _FINDEX_INFO_LEVELS;
    typedef _FINDEX_INFO_LEVELS FINDEX_INFO_LEVELS;
    enum _FINDEX_SEARCH_OPS;
    typedef _FINDEX_SEARCH_OPS FINDEX_SEARCH_OPS;

    extern "C" __declspec(dllimport) void* __stdcall FindFirstFileExW(
        _In_ _Null_terminated_ const wchar_t* lpFileName,
        _In_ FINDEX_INFO_LEVELS fInfoLevelId,
        _Out_writes_bytes_(sizeof(WIN32_FIND_DATAW)) void* lpFindFileData,
        _In_ FINDEX_SEARCH_OPS fSearchOp,
        _Reserved_ void* lpSearchFilter,
        _In_ unsigned long dwAdditionalFlags);

    extern "C" __declspec(dllimport) int __stdcall FindNextFileW(_In_ void* hFindFile, _Out_ LPWIN32_FIND_DATAW lpFindFileData);
    extern "C" __declspec(dllimport) int __stdcall FindClose(_Inout_ void* hFindFile);

#else
    #include <sys/stat.h>
    #include <dirent.h>
    #include <errno.h>
#endif


namespace Noesis
{

#ifdef NS_PLATFORM_WINDOWS
struct _FileTime
{
    unsigned long dwLowDateTime;
    unsigned long dwHighDateTime;
};

struct _FindData
{
    unsigned long dwFileAttributes;
    _FileTime ftCreationTime;
    _FileTime ftLastAccessTime;
    _FileTime ftLastWriteTime;
    unsigned long nFileSizeHigh;
    unsigned long nFileSizeLow;
    unsigned long dwReserved0;
    unsigned long dwReserved1;
    _Field_z_ wchar_t cFileName[260];
    _Field_z_ wchar_t cAlternateFileName[14];
};
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool FindFirst(const char* directory, const char* extension, FindData& findData)
{
#if defined(NS_PLATFORM_WINDOWS)
    char fullPath[sizeof(findData.filename)];
    StrCopy(fullPath, sizeof(fullPath), directory);
    StrAppend(fullPath, sizeof(fullPath), "/*");
    StrAppend(fullPath, sizeof(fullPath), extension);

    uint16_t u16str[sizeof(fullPath)];
    uint32_t numChars = UTF8::UTF8To16(fullPath, u16str, sizeof(fullPath));
    NS_ASSERT(numChars <= sizeof(fullPath));

    _FindData fd;
    void* h = FindFirstFileExW((const wchar_t*)u16str, (FINDEX_INFO_LEVELS)1, &fd, (FINDEX_SEARCH_OPS)0, 0, 0);
    if (h != ((void*)(__int64)-1))
    {
        numChars = UTF8::UTF16To8((uint16_t*)fd.cFileName, findData.filename, sizeof(fullPath));
        NS_ASSERT(numChars <= sizeof(fullPath));
        StrCopy(findData.extension, sizeof(findData.extension), extension);
        findData.handle = h;
        return true;
    }

    return false;

#else
    DIR* dir = opendir(directory);

    if (dir != 0)
    {
        StrCopy(findData.extension, sizeof(findData.extension), extension);
        findData.handle = dir;

        if (FindNext(findData))
        {
            return true;
        }
        else
        {
            FindClose(findData);
            return false;
        }
    }

    return false;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline bool FindNext(FindData& findData)
{
#if defined(NS_PLATFORM_WINDOWS)
    _FindData fd;
    int res = FindNextFileW(findData.handle, (LPWIN32_FIND_DATAW)&fd);
    
    if (res)
    {
        const int MaxFilename = sizeof(findData.filename);
        uint32_t n = UTF8::UTF16To8((uint16_t*)fd.cFileName, findData.filename, MaxFilename);
        NS_ASSERT(n <= MaxFilename);
        return true;
    }

    return false;

#else
    DIR* dir = (DIR*)findData.handle;

    while (true)
    {
        dirent* entry = readdir(dir);

        if (entry != 0)
        {
            if (StrCaseEndsWith(entry->d_name, findData.extension))
            {
                StrCopy(findData.filename, sizeof(findData.filename), entry->d_name);
                return true;
            }
        }
        else
        {
            return false;
        }
    }
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void FindClose(FindData& findData)
{
#if defined(NS_PLATFORM_WINDOWS)
    int r = ::FindClose(findData.handle);
    NS_ASSERT(r != 0);

#else
    DIR* dir = (DIR*)findData.handle;
    int r = closedir(dir);
    NS_ASSERT(r == 0);
#endif
}

}
