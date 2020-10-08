////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_THICKNESSANIMATION_H__
#define __GUI_THICKNESSANIMATION_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/BaseAnimation.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

struct Thickness;
template<class T> class Nullable;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates the value of a *Thickness* property between two target values using linear
/// interpolationover a specified *Duration*.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.thicknessanimation.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API ThicknessAnimation final: public BaseAnimation
{
public:
    /// Gets the type of value this animation generates
    const Type* GetTargetPropertyType() const override;

    /// Gets or sets the total amount by which the animation changes its starting value
    //@{
    const Nullable<Thickness>& GetBy() const;
    void SetBy(const Nullable<Thickness>& by);
    //@}

    /// Gets or sets the animation's starting value
    //@{
    const Nullable<Thickness>& GetFrom() const;
    void SetFrom(const Nullable<Thickness>& from);
    //@}

    /// Gets or sets the animation's ending value
    //@{
    const Nullable<Thickness>& GetTo() const;
    void SetTo(const Nullable<Thickness>& to);
    //@}

    /// From Freezable
    //@{
    Ptr<ThicknessAnimation> Clone() const;
    Ptr<ThicknessAnimation> CloneCurrentValue() const;
    //@}

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* ByProperty;
    static const DependencyProperty* FromProperty;
    static const DependencyProperty* ToProperty;
    //@}

private:
    /// From DependencyObject
    //@{
    bool OnPropertyChanged(const DependencyPropertyChangedEventArgs& args) override;
    //@}

    /// From Freezable
    //@{
    void CloneCommonCore(const Freezable* source) override;
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From AnimationTimeline
    //@{
    Ptr<BaseComponent> GetAnimatedValue(BaseComponent* defaultOrigin, 
        BaseComponent* defaultDestination, AnimationClock* clock) override;
    Ptr<AnimationTimeline> CreateTransitionFrom() const override;
    Ptr<AnimationTimeline> CreateTransitionTo() const override;
    //@}

private:
    NS_DECLARE_REFLECTION(ThicknessAnimation, BaseAnimation)
};

}

#endif
