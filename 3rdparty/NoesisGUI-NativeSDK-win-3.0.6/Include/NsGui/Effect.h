////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_EFFECT_H__
#define __GUI_EFFECT_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/Animatable.h>
#include <NsGui/IRenderProxyCreator.h>
#include <NsCore/ReflectionDeclare.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Provides a custom bitmap effect.
///
/// https://docs.microsoft.com/en-us/dotnet/api/system.windows.media.effects.effect
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API Effect: public Animatable, public IRenderProxyCreator
{
public:
    NS_DISABLE_COPY(Effect)

    Effect();
    virtual ~Effect() = 0;

    /// From IRenderProxyCreator
    //@{
    void UnregisterRenderer(ViewId viewId) override;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

protected:
    RenderProxyCreatorFlags mUpdateFlags;

    enum UpdateFlags
    {
        UpdateFlags_Sentinel
    };

    NS_DECLARE_REFLECTION(Effect, Animatable)
};

NS_WARNING_POP

}


#endif
