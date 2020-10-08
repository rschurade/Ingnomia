////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __RENDER_RENDERDEVICE_H__
#define __RENDER_RENDERDEVICE_H__


#include <NsCore/Noesis.h>
#include <NsCore/Ptr.h>
#include <NsCore/BaseComponent.h>
#include <NsRender/RenderDeviceApi.h>


namespace Noesis
{

class RenderTarget;
class Texture;

// Texture formats enumeration
struct TextureFormat
{
    enum Enum
    {
        RGBA8,
        R8,

        Count
    };
};

// Render device capabilities
struct DeviceCaps
{
    // Offset in pixel units from top-left corner to center of pixel
    float centerPixelOffset = 0.0f;

    // When this flag is enabled the device works in 'Linear' mode. All internal textures and
    // offscreens are created in 'sRGB' format. In this mode, the device also expects colors
    // (like the ones in the vertex buffer) in 'sRGB' format. It also indicates that the device
    // writes to the render target in linear space
    bool linearRendering = false;

    // This flag is enabled to indicate that the device supports LCD subpixel rendering. Extra
    // shaders and dual source blending are needed for this feature
    bool subpixelRendering = false;
};

// Shader effect
struct Shader
{
    // List of shaders to be implemented by the device with expected vertex format
    //
    //  Name       Format                   Size (bytes)      Semantic
    //  ---------------------------------------------------------------------------------
    //  Pos        R32G32_FLOAT             8                 Position (x,y)
    //  Color      R8G8B8A8_UNORM           4                 Color (rgba)
    //  Tex0       R32G32_FLOAT             8                 Texture (u,v)
    //  Tex1       R32G32_FLOAT             8                 Texture (u,v)
    //  Tex2       R16G16B16A16_UNORM       8                 Rect (x0,y0, x1,y1)
    //  Coverage   R32_FLOAT                4                 Coverage (x)
    //
    enum Enum
    {
        RGBA,                       // Pos
        Mask,                       // Pos

        Path_Solid,                 // Pos | Color
        Path_Linear,                // Pos | Tex0
        Path_Radial,                // Pos | Tex0
        Path_Pattern,               // Pos | Tex0

        PathAA_Solid,               // Pos | Color | Coverage
        PathAA_Linear,              // Pos | Tex0  | Coverage
        PathAA_Radial,              // Pos | Tex0  | Coverage
        PathAA_Pattern,             // Pos | Tex0  | Coverage

        SDF_Solid,                  // Pos | Color | Tex1
        SDF_Linear,                 // Pos | Tex0  | Tex1
        SDF_Radial,                 // Pos | Tex0  | Tex1
        SDF_Pattern,                // Pos | Tex0  | Tex1

        SDF_LCD_Solid,              // Pos | Color | Tex1
        SDF_LCD_Linear,             // Pos | Tex0  | Tex1
        SDF_LCD_Radial,             // Pos | Tex0  | Tex1
        SDF_LCD_Pattern,            // Pos | Tex0  | Tex1

        Image_Opacity_Solid,        // Pos | Color | Tex1
        Image_Opacity_Linear,       // Pos | Tex0  | Tex1
        Image_Opacity_Radial,       // Pos | Tex0  | Tex1
        Image_Opacity_Pattern,      // Pos | Tex0  | Tex1

        Image_Shadow35V,            // Pos | Color | Tex1 | Tex2
        Image_Shadow63V,            // Pos | Color | Tex1 | Tex2
        Image_Shadow127V,           // Pos | Color | Tex1 | Tex2

        Image_Shadow35H_Solid,      // Pos | Color | Tex1 | Tex2
        Image_Shadow35H_Linear,     // Pos | Tex0  | Tex1 | Tex2
        Image_Shadow35H_Radial,     // Pos | Tex0  | Tex1 | Tex2
        Image_Shadow35H_Pattern,    // Pos | Tex0  | Tex1 | Tex2

        Image_Shadow63H_Solid,      // Pos | Color | Tex1 | Tex2
        Image_Shadow63H_Linear,     // Pos | Tex0  | Tex1 | Tex2
        Image_Shadow63H_Radial,     // Pos | Tex0  | Tex1 | Tex2
        Image_Shadow63H_Pattern,    // Pos | Tex0  | Tex1 | Tex2

        Image_Shadow127H_Solid,     // Pos | Color | Tex1 | Tex2
        Image_Shadow127H_Linear,    // Pos | Tex0  | Tex1 | Tex2
        Image_Shadow127H_Radial,    // Pos | Tex0  | Tex1 | Tex2
        Image_Shadow127H_Pattern,   // Pos | Tex0  | Tex1 | Tex2

        Image_Blur35V,              // Pos | Color | Tex1 | Tex2
        Image_Blur63V,              // Pos | Color | Tex1 | Tex2
        Image_Blur127V,             // Pos | Color | Tex1 | Tex2

        Image_Blur35H_Solid,        // Pos | Color | Tex1 | Tex2
        Image_Blur35H_Linear,       // Pos | Tex0  | Tex1 | Tex2
        Image_Blur35H_Radial,       // Pos | Tex0  | Tex1 | Tex2
        Image_Blur35H_Pattern,      // Pos | Tex0  | Tex1 | Tex2

        Image_Blur63H_Solid,        // Pos | Color | Tex1 | Tex2
        Image_Blur63H_Linear,       // Pos | Tex0  | Tex1 | Tex2
        Image_Blur63H_Radial,       // Pos | Tex0  | Tex1 | Tex2
        Image_Blur63H_Pattern,      // Pos | Tex0  | Tex1 | Tex2

        Image_Blur127H_Solid,       // Pos | Color | Tex1 | Tex2
        Image_Blur127H_Linear,      // Pos | Tex0  | Tex1 | Tex2
        Image_Blur127H_Radial,      // Pos | Tex0  | Tex1 | Tex2
        Image_Blur127H_Pattern,     // Pos | Tex0  | Tex1 | Tex2

        Count
    };

    uint8_t v;
};

// Alpha blending mode
struct BlendMode
{
    enum Enum
    {
        Src,
        SrcOver,
        SrcOver_Dual,

        Count
    };
};

// Stencil buffer mode
struct StencilMode
{
    enum Enum
    {
        Disabled,
        Equal_Keep,
        Equal_Incr,
        Equal_Decr,

        Count
    };
};

// Render state
union RenderState
{
    struct
    {
        uint8_t scissorEnable:1;
        uint8_t colorEnable:1;
        uint8_t blendMode:2;
        uint8_t stencilMode:2;
        uint8_t wireframe:1;
        uint8_t unused:1;
    } f;

    uint8_t v;
};

// Texture wrapping mode
struct WrapMode
{
    enum Enum
    {
        // Clamp between 0.0 and 1.0
        ClampToEdge,
        // Out of range texture coordinates return transparent zero (0,0,0,0)
        ClampToZero,
        // Wrap to the other side of the texture
        Repeat,
        // The same as repeat but flipping horizontally
        MirrorU,
        // The same as repeat but flipping vertically
        MirrorV,
        // The combination of MirrorU and MirrorV
        Mirror,

        Count
    };
};

// Texture minification and magnification filter
struct MinMagFilter
{
    enum Enum
    {
        // Select the single pixel nearest to the sample point
        Nearest,
        // Select two pixels in each dimension and interpolate linearly between them
        Linear,

        Count
    };
};

// Texture Mipmap filter
struct MipFilter
{
    enum Enum
    {
        // Texture sampled from mipmap level 0
        Disabled,
        // The nearest mipmap level is selected
        Nearest,
        // Both nearest levels are sampled and linearly interpolated
        Linear,

        Count
    };
};

// Texture sampler state
union SamplerState
{
    struct
    {
        uint8_t wrapMode:3;
        uint8_t minmagFilter:1;
        uint8_t mipFilter:2;
        uint8_t unused:2;
    } f;

    uint8_t v;
};

// Render batch information
struct Batch
{
    // Render state
    Shader shader;
    RenderState renderState;
    uint8_t stencilRef;

    // Draw parameters
    uint32_t vertexOffset;
    uint32_t numVertices;
    uint32_t startIndex;
    uint32_t numIndices;

    // Textures (Unused textures are set to null)
    Texture* pattern;
    SamplerState patternSampler;

    Texture* ramps;
    SamplerState rampsSampler;

    Texture* image;
    SamplerState imageSampler;

    Texture* glyphs;
    SamplerState glyphsSampler;

    Texture* shadow;
    SamplerState shadowSampler;

    // Effect parameters
    float* effectParams;
    uint32_t effectParamsSize;
    uint32_t effectParamsHash;

    // Shader constants (Unused constants are set to null)
    const float (*projMtx)[16];
    uint32_t projMtxHash;

    const float* opacity;
    uint32_t opacityHash;

    const float (*rgba)[4];
    uint32_t rgbaHash;

    const float (*radialGrad)[8];
    uint32_t radialGradHash;
};

// A tile is a region of render target. The origin is located at the lower left corner
struct Tile
{
    uint32_t x;
    uint32_t y;
    uint32_t width;
    uint32_t height;
};

// Max requested bytes for MapVertices
#ifndef DYNAMIC_VB_SIZE
    #define DYNAMIC_VB_SIZE 512 * 1024
#endif

// Max requested bytes for MapIndices
#ifndef DYNAMIC_IB_SIZE
    #define DYNAMIC_IB_SIZE 128 * 1024
#endif

// Max requested bytes for UpdateTexture
#ifndef DYNAMIC_TEX_SIZE
    #define DYNAMIC_TEX_SIZE 128 * 1024
#endif

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Abstraction of a graphics rendering device
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_RENDER_RENDERDEVICE_API RenderDevice: public BaseComponent
{
public:
    NS_DISABLE_COPY(RenderDevice)

    RenderDevice();
    ~RenderDevice();

    /// Retrieves device render capabilities
    virtual const DeviceCaps& GetCaps() const = 0;

    /// Creates render target surface with given dimensions and number of samples
    virtual Ptr<RenderTarget> CreateRenderTarget(const char* label, uint32_t width, uint32_t height,
        uint32_t sampleCount) = 0;

    /// Creates render target sharing transient (stencil, colorAA) buffers with the given surface
    virtual Ptr<RenderTarget> CloneRenderTarget(const char* label, RenderTarget* surface) = 0;

    /// Creates texture with given dimensions and format. For immutable textures, the content of
    /// each mipmap is given in 'data'. The passed data is tightly packed (no extra pitch). When 
    /// 'data' is null the texture is considered dynamic and will be updated using UpdateTexture()
    virtual Ptr<Texture> CreateTexture(const char* label, uint32_t width, uint32_t height,
        uint32_t numLevels, TextureFormat::Enum format, const void** data) = 0;

    /// Updates texture mipmap copying the given data to desired position. The passed data is
    /// tightly packed (no extra pitch). Origin is located at the left of the first scanline
    virtual void UpdateTexture(Texture* texture, uint32_t level, uint32_t x, uint32_t y,
        uint32_t width, uint32_t height, const void* data) = 0;

    /// Begins rendering offscreen or onscreen commands
    virtual void BeginRender(bool offscreen) = 0;

    /// Binds render target and sets viewport to cover the entire surface
    virtual void SetRenderTarget(RenderTarget* surface) = 0;

    /// Clears the given region to transparent (#000000) and sets the scissor rectangle to fit it.
    /// Until next call to EndTile() all rendering commands will only update the extents of the tile
    virtual void BeginTile(const Tile& tile, uint32_t surfaceWidth, uint32_t surfaceHeight) = 0;

    /// Completes rendering to the tile specified by BeginTile()
    virtual void EndTile() = 0;

    /// Resolves multisample render target
    virtual void ResolveRenderTarget(RenderTarget* surface, const Tile* tiles, uint32_t numTiles) = 0;

    /// Ends rendering
    virtual void EndRender() = 0;

    /// Gets a pointer to stream vertices
    virtual void* MapVertices(uint32_t bytes) = 0;

    /// Invalidates the pointer previously mapped
    virtual void UnmapVertices() = 0;

    /// Gets a pointer to stream 16-bit indices
    virtual void* MapIndices(uint32_t bytes) = 0;

    /// Invalidates the pointer previously mapped
    virtual void UnmapIndices() = 0;

    /// Draws primitives for the given batch
    virtual void DrawBatch(const Batch& batch) = 0;

    /// Width of offscreen textures (0 = automatic). Default is automatic
    void SetOffscreenWidth(uint32_t width);
    uint32_t GetOffscreenWidth() const;

    /// Height of offscreen textures (0 = automatic). Default is automatic
    void SetOffscreenHeight(uint32_t height);
    uint32_t GetOffscreenHeight() const;

    /// Multisampling of offscreen textures. Default is 1x
    void SetOffscreenSampleCount(uint32_t sampleCount);
    uint32_t GetOffscreenSampleCount() const;

    /// Number of offscreen textures created at startup. Default is 0
    void SetOffscreenDefaultNumSurfaces(uint32_t numSurfaces);
    uint32_t GetOffscreenDefaultNumSurfaces() const;

    /// Maximum number of offscreen textures (0 = unlimited). Default is unlimited
    void SetOffscreenMaxNumSurfaces(uint32_t numSurfaces);
    uint32_t GetOffscreenMaxNumSurfaces() const;

    /// Width of texture used to cache glyphs. Default is 1024
    void SetGlyphCacheWidth(uint32_t width);
    uint32_t GetGlyphCacheWidth() const;

    /// Height of texture used to cache glyphs. Default is 1024
    void SetGlyphCacheHeight(uint32_t height);
    uint32_t GetGlyphCacheHeight() const;

    /// Vector graphics context
    void SetVGContext(BaseComponent* context);
    BaseComponent* GetVGContext() const;

protected:
    /// Returns whether the passed render state is compatible with the given shader
    bool IsValidState(Shader shader, RenderState state) const;

private:
    uint32_t mOffscreenWidth;
    uint32_t mOffscreenHeight;
    uint32_t mOffscreenSampleCount;
    uint32_t mOffscreenDefaultNumSurfaces;
    uint32_t mOffscreenMaxNumSurfaces;
    uint32_t mGlyphCacheWidth;
    uint32_t mGlyphCacheHeight;

    Ptr<BaseComponent> mVGContext;
};

NS_WARNING_POP

}

#endif
