#include "LocalFontProvider.hpp"

#include "spdlog/spdlog.h"

#include <cstddef>
#include <cstdint>

using namespace Noesis;

namespace AppGUI
{
    LocalFontProvider::LocalFontProvider(const char* const root)
    {
        StrCopy(this->RootPath_, sizeof(this->RootPath_), root);
    }

    auto LocalFontProvider::ScanFolder(const Uri& folder) -> void
    {
        char uri[512] = "";

        if (!StrIsNullOrEmpty(this->RootPath_))
        {
            StrCopy(uri, sizeof(uri), this->RootPath_);
            StrAppend(uri, sizeof(uri), "/");
        }

        FixedString<512> path;
        folder.GetPath(path);

        StrAppend(uri, sizeof(uri), path.Str());

        this->ScanFolder(uri, folder, ".ttf");
        this->ScanFolder(uri, folder, ".otf");
        this->ScanFolder(uri, folder, ".ttc");
    }

    auto LocalFontProvider::OpenFont(const Uri& folder, const char* const filename) const -> Noesis::Ptr<Noesis::Stream>
    {
        char uri[512] = "";

        if (!StrIsNullOrEmpty(this->RootPath_))
        {
            StrCopy(uri, sizeof(uri), this->RootPath_);
            StrAppend(uri, sizeof(uri), "/");
        }

        FixedString<512> path;
        folder.GetPath(path);

        StrAppend(uri, sizeof(uri), path.Str());
        StrAppend(uri, sizeof(uri), "/");
        StrAppend(uri, sizeof(uri), filename);

        spdlog::info("[GUI-Frontend]: LocalFontProvider request: {}", uri);
        return OpenFileStream(uri);
    }

    auto LocalFontProvider::ScanFolder(const char* const path, const Uri& folder, const char* const ext) -> void
    {
        FindData findData { };
        if (FindFirst(path, ext, findData))
        {
            do
            {
                RegisterFont(folder, findData.filename);
            }
            while (FindNext(findData));

            FindClose(findData);
        }
    }
}
