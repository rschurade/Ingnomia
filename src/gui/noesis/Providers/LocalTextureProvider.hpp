#pragma once

#include "FileTextureProvider.hpp"

namespace AppGUI
{
    struct LocalTextureProvider final : public FileTextureProvider
    {
        explicit LocalTextureProvider(const char* root);

    private:
        [[nodiscard]] auto OpenStream(const Noesis::Uri& uri) -> Noesis::Ptr<Noesis::Stream> override;
        char RootPath_[512] { };
    };
}
