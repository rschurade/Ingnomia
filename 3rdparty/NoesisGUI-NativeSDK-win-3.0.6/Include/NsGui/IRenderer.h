////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_IRENDERER_H__
#define __GUI_IRENDERER_H__


#include <NsCore/Noesis.h>
#include <NsCore/Interface.h>


namespace Noesis
{

class RenderDevice;
class Matrix4;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// This interface renders the UI tree. Using IView and IRenderer in different threads is allowed.
/// In fact, IRenderer is designed to be used in the render thread of your application.
////////////////////////////////////////////////////////////////////////////////////////////////////
NS_INTERFACE IRenderer: public Interface
{
    /// Initializes the renderer with the given render device
    virtual void Init(RenderDevice* device) = 0;

    /// Free allocated render resources and render tree
    virtual void Shutdown() = 0;

    /// Determines the visible region. By default it is set to cover the view dimensions
    virtual void SetRenderRegion(float x, float y, float width, float height) = 0;

    /// Applies last changes happened in the view. This function does not interact with the
    /// render device. Returns 'false' to indicate that no changes were applied and subsequent
    /// RenderOffscreen() and Render() calls could be avoided if last render was preserved
    virtual bool UpdateRenderTree() = 0;

    /// Generates all the needed offscreen textures. It is recommended, specially on tiled
    /// architectures, to invoke this function before bindind the main render target.
    /// Returns 'false' when no commands are generated and restoring the GPU state is not needed
    virtual bool RenderOffscreen() = 0;

    /// Generates all the needed offscreen textures. This function overrides the default
    /// view projection matrix. Can be used for example to render each eye in VR
    virtual bool RenderOffscreen(const Matrix4& projection) = 0;

    /// Renders UI into the current render target and viewport
    virtual void Render(bool flipY = false) = 0;

    /// Renders UI into the current render target and viewport. This function overrides the default
    /// view projection matrix. Can be used for example to render each eye in VR
    virtual void Render(const Matrix4& projection, bool flipY = false) = 0;

    NS_IMPLEMENT_INLINE_REFLECTION_(IRenderer, Interface)
};

}

#endif
