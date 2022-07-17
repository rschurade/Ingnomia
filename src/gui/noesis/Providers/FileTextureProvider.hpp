#pragma once

#include <NoesisPCH.h>

namespace AppGUI
{
    struct FileTextureProvider : Noesis::TextureProvider
    {
    protected:
        virtual auto OpenStream(const Noesis::Uri& uri) -> Noesis::Ptr<Noesis::Stream> = 0;
        [[nodiscard]] virtual auto GetTextureInfo(const Noesis::Uri& uri) -> Noesis::TextureInfo override;
        [[nodiscard]] virtual auto LoadTexture(const Noesis::Uri& uri, Noesis::RenderDevice* device) -> Noesis::Ptr<Noesis::Texture> override;
    };
}
