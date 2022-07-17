#pragma once

#include <NoesisPCH.h>
#include <bgfx/bgfx.h>

#include <cstdint>

namespace AppGUI
{
    struct Texture final : Noesis::Texture
    {
        const std::size_t Stride;
        const bgfx::TextureHandle Handle;

        Texture
        (
            bgfx::TextureHandle handle,
            std::uint32_t width,
            std::uint32_t height,
            std::size_t stride,
            bool hasAlpha
        ) noexcept;
        ~Texture();

        virtual auto GetWidth() const noexcept -> std::uint32_t override;
        virtual auto GetHeight() const noexcept -> std::uint32_t override;
        virtual auto HasMipMaps() const noexcept -> bool override;
        virtual auto IsInverted() const noexcept -> bool override;
        virtual auto HasAlpha() const noexcept -> bool override;

        private:
            std::uint32_t Width_, Height_ { };
            bool HasAlpha_ { };
    };
}
