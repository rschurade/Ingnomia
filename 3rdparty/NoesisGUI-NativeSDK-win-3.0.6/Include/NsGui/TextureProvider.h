////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_TEXTUREPROVIDER_H__
#define __GUI_TEXTUREPROVIDER_H__


#include <NsCore/Noesis.h>
#include <NsCore/BaseComponent.h>
#include <NsCore/Delegate.h>


namespace Noesis
{

class Texture;
class RenderDevice;
template<class T> class Ptr;

// Texture metadata
struct TextureInfo
{
    uint32_t width;
    uint32_t height;
};

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for implementing providers of textures
////////////////////////////////////////////////////////////////////////////////////////////////////
class TextureProvider: public BaseComponent
{
public:
    /// Returns metadata for the given texture. 0 x 0 is returned if no texture found
    virtual TextureInfo GetTextureInfo(const char* uri) = 0;

    /// Returns a texture compatible with the given device. Null is returned if no texture found
    virtual Ptr<Texture> LoadTexture(const char* uri, RenderDevice* device) = 0;

    /// Delegate to notify changes to the texture file content
    typedef Delegate<void (const char*)> TextureChangedDelegate;
    TextureChangedDelegate& TextureChanged() { return mTextureChanged; }
    void RaiseTextureChanged(const char* uri) { mTextureChanged(uri); }

private:
    TextureChangedDelegate mTextureChanged;
};

}

#endif
