#include "FileTextureProvider.hpp"

#include "spdlog/spdlog.h"

#include <cstddef>
#include <cstdint>

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_RESIZE_STATIC
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include <stb_image_resize.h>

using namespace Noesis;

namespace AppGUI
{
    static auto Read(void* const user, char* const data, const signed size) -> signed
    {
        auto* const stream { static_cast<Stream*>(user) };
        return static_cast<signed>(stream->Read(data, size));
    }

    static auto Skip(void* const user, const signed n) -> void
    {
        auto* const stream { static_cast<Stream*>(user) };
        stream->SetPosition(stream->GetPosition() + n);
    }

    static auto Eof(void* const user) -> signed
    {
        auto* const stream { static_cast<Stream*>(user) };
        return stream->GetPosition() >= stream->GetLength();
    }

    auto FileTextureProvider::GetTextureInfo(const Noesis::Uri& uri) -> Noesis::TextureInfo
    {
        TextureInfo info { 0, 0 };
        Ptr<Stream> file { OpenStream(uri) };
        if (file) [[likely]]
        {
            signed x, y, n;
            constexpr stbi_io_callbacks callbacks { Read, Skip, Eof };
            if (stbi_info_from_callbacks(&callbacks, file, &x, &y, &n))
            {
                info.width = x;
                info.height = y;
            }
            else
            {
                spdlog::warn("Failed to query texture info: {} -> {}", uri.Str(), stbi_failure_reason());
            }

            file->Close();
        }

        return info;
    }

    auto FileTextureProvider::LoadTexture(const Noesis::Uri& uri, Noesis::RenderDevice* const device) -> Noesis::Ptr<Noesis::Texture>
    {
        Ptr<Stream> file { OpenStream(uri) };
        if (!file) [[unlikely]]
        {
            return nullptr;
        }

        signed x, y, n;
        constexpr stbi_io_callbacks callbacks { Read, Skip, Eof };
        stbi_uc* const img { stbi_load_from_callbacks(&callbacks, file, &x, &y, &n, STBI_rgb_alpha) };
        if (!img) [[unlikely]]
        {
			spdlog::warn("Failed to load texture: {} -> {}", uri.Str(), stbi_failure_reason());
            return nullptr;
        }

        const bool hasAlpha { n == 2 || n == 4 };
        // Premultiply alpha
        if (hasAlpha)
        {
            if (device->GetCaps().linearRendering)
            {
                for (signed i { }; i < x * y; i++)
                {
                    float a { static_cast<float>(img[4 * i + 3]) / 255.F };
                    float r { stbir__srgb_uchar_to_linear_float[img[4 * i + 0]] * a };
                    float g { stbir__srgb_uchar_to_linear_float[img[4 * i + 1]] * a };
                    float b { stbir__srgb_uchar_to_linear_float[img[4 * i + 2]] * a };
                    img[4 * i + 0] = stbir__linear_to_srgb_uchar(r);
                    img[4 * i + 1] = stbir__linear_to_srgb_uchar(g);
                    img[4 * i + 2] = stbir__linear_to_srgb_uchar(b);
                }
            }
            else
            {
                for (signed i { }; i < x * y; i++)
                {
                    const stbi_uc a { img[4 * i + 3] };
                    img[4 * i + 0] = static_cast<stbi_uc>(static_cast<std::uint32_t>(img[4 * i] * a) / 255U);
                    img[4 * i + 1] = static_cast<stbi_uc>(static_cast<std::uint32_t>(img[4 * i + 1] * a) / 255U);
                    img[4 * i + 2] = static_cast<stbi_uc>(static_cast<std::uint32_t>(img[4 * i + 2] * a) / 255U);
                }
            }
        }

        // Create Mipmaps
        void* data[32] = { img };
        uint32_t numLevels { Max(Log2(x), Log2(y)) + 1 };
        for (std::uint32_t i { }; i < numLevels; i++)
        {
            const std::uint32_t inW { Max(static_cast<std::uint32_t>(x) >> (i - 1), 1U) };
            const std::uint32_t inH { Max(static_cast<std::uint32_t>(y) >> (i - 1), 1U) };
            const std::uint32_t outW { Max(static_cast<std::uint32_t>(x) >> i, 1U) };
            const std::uint32_t outH { Max(static_cast<std::uint32_t>(y) >> i, 1U) };

            data[i] = Alloc(outW * outH * 4);
            unsigned char* const in { static_cast<unsigned char*>(data[i - 1]) };
            unsigned char* const out { static_cast<unsigned char*>(data[i]) };

            const signed r
            {
                stbir_resize_uint8_generic
                (
                    in,
                    static_cast<signed>(inW),
                    static_cast<signed>(inH),
                    static_cast<signed>(4 * inW),
                    out,
                    static_cast<signed>(outW),
                    static_cast<signed>(outH),
                    static_cast<signed>(4 * outW),
                    4,
                    3,
                    STBIR_FLAG_ALPHA_PREMULTIPLIED,
                    STBIR_EDGE_CLAMP,
                    STBIR_FILTER_DEFAULT,
                    STBIR_COLORSPACE_SRGB,
                    nullptr
                )
            };

            assert(r == 1 && "Failed to create mipmaps!");
        }

        const signed index { StrFindLast(uri.Str(), "/") };
        const char* const label { index == -1 ? uri.Str() : uri.Str() + index + 1 };

        // Not using compressed textures is far from ideal but it would complicate the pipeline as
        // having to support and detect availability for all formats in all supported GPUs is not a
        // simple task. Here we are just following the easy path, the texture is created uncompressed
        // using the RenderDevice. This is fine for the App framework and for our examples.
        // For production, we do *not* recommend doing this. Just preprocess your textures, compressing
        // them offline and wrap them inside a Texture object, similar to D3D11Fatory::WrapTexture

        const TextureFormat::Enum format { hasAlpha ? TextureFormat::RGBA8 : TextureFormat::RGBX8 };
        Ptr<Texture> texture { device->CreateTexture(label, x, y, numLevels, format, const_cast<const void**>(data)) };

        stbi_image_free(img);
        file->Close();

        for (std::uint32_t i { 1 }; i < numLevels; ++i)
        {
            Dealloc(data[i]);
        }

        return texture;
    }
}
