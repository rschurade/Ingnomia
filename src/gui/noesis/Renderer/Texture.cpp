#include <cstdint>
#include <cstddef>

#include "Texture.hpp"

namespace AppGUI
{
    Texture::Texture
    (
        const bgfx::TextureHandle handle,
        const std::uint32_t width,
        const std::uint32_t height,
        const std::size_t stride,
        const bool hasAlpha
    ) noexcept : Handle { handle }, Stride { stride }, Width_ { width }, Height_ { height }, HasAlpha_ { hasAlpha } { }

    Texture::~Texture()
    {
        bgfx::destroy(this->Handle);
    }

    auto Texture::GetWidth() const noexcept -> std::uint32_t
    {
        return this->Width_;
    }

    auto Texture::GetHeight() const noexcept -> std::uint32_t
    {
        return this->Height_;
    }

    auto Texture::HasMipMaps() const noexcept -> bool
    {
        return false;
    }

    auto Texture::IsInverted() const noexcept -> bool
    {
        return false;
    }

    auto Texture::HasAlpha() const noexcept -> bool
    {
        return this->HasAlpha_;
    }
}
