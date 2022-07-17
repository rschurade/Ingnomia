#pragma once

#include <NoesisPCH.h>
#include <bgfx/bgfx.h>

#include "Texture.hpp"

namespace AppGUI
{
    struct RenderTarget final : public Noesis::RenderTarget
    {
        bgfx::TextureHandle StencilHandle { bgfx::kInvalidHandle };
        bgfx::FrameBufferHandle FBOHandle { bgfx::kInvalidHandle };
        const std::uint32_t Width, Height;

        RenderTarget
        (
            std::uint32_t width,
            std::uint32_t height
        ) noexcept;
        ~RenderTarget() override;

        virtual auto GetTexture() -> Noesis::Texture* override;

    private:
        Noesis::Ptr<Texture> Texture_ { };
    };
}