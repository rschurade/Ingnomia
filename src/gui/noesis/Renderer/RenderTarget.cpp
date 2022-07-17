#include <cstdint>
#include <cstddef>

#include "RenderTarget.hpp"

namespace AppGUI
{
    RenderTarget::RenderTarget
    (
        const std::uint32_t width,
        const std::uint32_t height
    ) noexcept : Width { width }, Height { height }
    {
        this->Texture_ = *new Texture(bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT), width, height, 4, true);
    }

    RenderTarget::~RenderTarget()
    {
        bgfx::destroy(this->FBOHandle);
        bgfx::destroy(this->StencilHandle);
    }

    auto RenderTarget::GetTexture() -> Noesis::Texture*
    {
        return this->Texture_;
    }
}
