////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_TEXTURESOURCE_H__
#define __GUI_TEXTURESOURCE_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/ImageSource.h>
#include <NsCore/ReflectionDeclare.h>


namespace Noesis
{

class Texture;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Defines an ImageSource constructed from a Texture.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API TextureSource: public ImageSource
{
public:
    /// Constructor
    TextureSource();
    TextureSource(Texture* texture);

    /// Destructor
    ~TextureSource();

    /// Gets or sets texture
    //@{
    Texture* GetTexture() const;
    void SetTexture(Texture* texture);
    //@}

    /// Gets the width of the texture in pixels
    /// \prop
    int32_t GetPixelWidth() const;

    /// Gets the height of the texture in pixels
    /// \prop
    int32_t GetPixelHeight() const;

    // Hides Freezable methods for convenience
    //@{
    Ptr<TextureSource> Clone() const;
    Ptr<TextureSource> CloneCurrentValue() const;
    //@}

    /// From IRenderProxyCreator
    //@{
    void CreateRenderProxy(RenderTreeUpdater& updater, uint32_t proxyIndex);
    void UpdateRenderProxy(RenderTreeUpdater& updater, uint32_t proxyIndex);
    void UnregisterRenderer(ViewId viewId);
    //@}

protected:
    /// From Freezable
    //@{
    void CloneCommonCore(const Freezable* source);
    Ptr<Freezable> CreateInstanceCore() const;
    //@}

    /// From ImageSource
    //@{
    float GetWidthCore() const;
    float GetHeightCore() const;
    //@}

private:
    RenderProxyCreatorFlags mUpdateFlags;

    enum UpdateFlags
    {
        UpdateFlags_Texture
    };

    Ptr<Texture> mTexture;

    NS_DECLARE_REFLECTION(TextureSource, ImageSource)
};

NS_WARNING_POP

}


#endif
