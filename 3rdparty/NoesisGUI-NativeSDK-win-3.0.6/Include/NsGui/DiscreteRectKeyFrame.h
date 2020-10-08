////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DISCRETERECTKEYFRAME_H__
#define __GUI_DISCRETERECTKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/RectKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates from the *Rect* value of the previous key frame to its own *Value* using discrete
/// interpolation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.discreterectkeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API DiscreteRectKeyFrame final: public RectKeyFrame
{
public:
    // Hides Freezable methods for convenience
    //@{
    Ptr<DiscreteRectKeyFrame> Clone() const;
    Ptr<DiscreteRectKeyFrame> CloneCurrentValue() const;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    Rect InterpolateValueCore(const Rect& baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(DiscreteRectKeyFrame, RectKeyFrame)
};

}


#endif
