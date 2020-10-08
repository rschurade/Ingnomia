////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_RECTANIMATION_H__
#define __GUI_RECTANIMATION_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/BaseAnimation.h>
#include <NsGui/AnimationApi.h>


namespace Noesis
{

struct Rect;
template<class T> class Nullable;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates the value of a *Rect* property between two target values using linear interpolation
/// over a specified *Duration*.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.rectanimation.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API RectAnimation final: public BaseAnimation
{
public:
    /// Gets the type of value this animation generates
    const Type* GetTargetPropertyType() const override;

    /// Gets or sets the total amount by which the animation changes its starting value
    //@{
    const Nullable<Rect>& GetBy() const;
    void SetBy(const Nullable<Rect>& by);
    //@}

    /// Gets or sets the animation's starting value
    //@{
    const Nullable<Rect>& GetFrom() const;
    void SetFrom(const Nullable<Rect>& from);
    //@}

    /// Gets or sets the animation's ending value
    //@{
    const Nullable<Rect>& GetTo() const;
    void SetTo(const Nullable<Rect>& to);
    //@}

    /// From Freezable
    //@{
    Ptr<RectAnimation> Clone() const;
    Ptr<RectAnimation> CloneCurrentValue() const;
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
    NS_DECLARE_REFLECTION(RectAnimation, BaseAnimation)
};

}

#endif
