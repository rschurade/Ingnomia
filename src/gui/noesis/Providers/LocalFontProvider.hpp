#pragma once

#include <NoesisPCH.h>

namespace AppGUI
{
    struct LocalFontProvider final : public Noesis::CachedFontProvider
    {
        explicit LocalFontProvider(const char* root);

    private:
        virtual auto ScanFolder(const Noesis::Uri& folder) -> void override;
        [[nodiscard]] virtual auto OpenFont(const Noesis::Uri& folder, const char* filename) const -> Noesis::Ptr<Noesis::Stream> override;

        auto ScanFolder(const char* path, const Noesis::Uri& folder, const char* ext) -> void;

        char RootPath_[512] { };
    };
}
