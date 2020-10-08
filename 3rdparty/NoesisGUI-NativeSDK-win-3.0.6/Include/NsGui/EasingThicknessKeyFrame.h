////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_EASINGTHICKNESSKEYFRAME_H__
#define __GUI_EASINGTHICKNESSKEYFRAME_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/ThicknessKeyFrame.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

class EasingFunctionBase;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A class that enables you to associate easing functions with a ThicknessAnimationUsingKeyFrames
/// key frame animation.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.easingthicknesskeyframe.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API EasingThicknessKeyFrame final: public ThicknessKeyFrame
{
public:
    /// Gets or sets the easing function applied to the key frame
    //@{
    EasingFunctionBase* GetEasingFunction() const;
    void SetEasingFunction(EasingFunctionBase* function);
    //@}

    // Hides Freezable methods for convenience
    //@{
    Ptr<EasingThicknessKeyFrame> Clone() const;
    Ptr<EasingThicknessKeyFrame> CloneCurrentValue() const;
    //@}

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* EasingFunctionProperty;
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From KeyFrame
    //@{
    Thickness InterpolateValueCore(const Thickness& baseValue, float keyFrameProgress) override;
    //@}

    NS_DECLARE_REFLECTION(EasingThicknessKeyFrame, ThicknessKeyFrame)
};

}


#endif
