////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_RECTANIMATIONUSINGKEYFRAMES_H__
#define __GUI_RECTANIMATIONUSINGKEYFRAMES_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/AnimationTimeline.h>


namespace Noesis
{

struct Rect;
class RectKeyFrame;
template<class T> class FreezableCollection;
typedef FreezableCollection<RectKeyFrame> RectKeyFrameCollection;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates the value of a *Rect* property along a set of *KeyFrames*.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.rectanimationusingkeyframes.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API RectAnimationUsingKeyFrames final: public AnimationTimeline
{
public:
    RectAnimationUsingKeyFrames();
    ~RectAnimationUsingKeyFrames();

    /// Gets the type of value this animation generates
    const Type* GetTargetPropertyType() const override;

    /// Gets the collection of KeyFrame objects that define the animation
    RectKeyFrameCollection* GetKeyFrames() const;

    // Hides Freezable methods for convenience
    //@{
    Ptr<RectAnimationUsingKeyFrames> Clone() const;
    Ptr<RectAnimationUsingKeyFrames> CloneCurrentValue() const;
    //@}

    /// From AnimationTimeline
    //@{
    Ptr<BaseComponent> GetAnimatedValue(BaseComponent* defaultOrigin,
        BaseComponent* defaultDestination, AnimationClock* clock) override;
    //@}

private:
    /// From DependencyObject
    //@{
    void OnInit() override;
    //@}

    /// From Freezable
    //@{
    void CloneCommonCore(const Freezable* source) override;
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From Timeline
    //@{
    Duration GetNaturalDuration(Clock* clock) const override;
    Duration GetEffectiveDuration() const override;
    //@}

    /// From AnimationTimeline
    //@{
    Ptr<AnimationTimeline> CreateTransitionFrom() const override;
    Ptr<AnimationTimeline> CreateTransitionTo() const override;
    //@}

private:
    mutable Ptr<RectKeyFrameCollection> mKeyFrames;
    Vector<Pair<Ptr<RectKeyFrame>, double>> mOrderedFrames;
    mutable Duration mNaturalDuration;

    NS_DECLARE_REFLECTION(RectAnimationUsingKeyFrames, AnimationTimeline)
};

NS_WARNING_POP

}


#endif
