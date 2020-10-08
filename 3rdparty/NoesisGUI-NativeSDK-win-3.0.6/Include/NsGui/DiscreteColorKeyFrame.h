////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DISCRETECOLORKEYFRAME_H__
#define __GUI_DISCRETECOLORKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/ColorKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Color* value of the previous key frame to its own *Value* using discrete
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.discretecolorkeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API DiscreteColorKeyFrame final: public ColorKeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<DiscreteColorKeyFrame> Clone() const;
    Ptr<DiscreteColorKeyFrame> CloneCurrentValue() const;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    Color InterpolateValueCore(const Color& baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(DiscreteColorKeyFrame, ColorKeyFrame)
};

}


#endif
