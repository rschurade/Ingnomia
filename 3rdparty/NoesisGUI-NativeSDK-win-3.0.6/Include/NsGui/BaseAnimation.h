////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_BASEANIMATION_H__
#define __GUI_BASEANIMATION_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/AnimationTimeline.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

class EasingFunctionBase;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for Animation classes. It stores the easing function that can be applied.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API BaseAnimation: public AnimationTimeline
{
public:
    BaseAnimation();
    virtual ~BaseAnimation() = 0;

    /// Gets or sets the easing function applied to this animation
    //@{
    EasingFunctionBase* GetEasingFunction() const;
    void SetEasingFunction(EasingFunctionBase* function);
    //@}

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* EasingFunctionProperty;
    //@}

protected:
    int mAnimationType;

    NS_DECLARE_REFLECTION(BaseAnimation, AnimationTimeline)
};

}


#endif
