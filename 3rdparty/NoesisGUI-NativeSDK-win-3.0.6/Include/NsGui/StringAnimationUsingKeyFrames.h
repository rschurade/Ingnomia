////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_STRINGANIMATIONUSINGKEYFRAMES_H__
#define __GUI_STRINGANIMATIONUSINGKEYFRAMES_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/StringFwd.h>
#include <NsGui/AnimationTimeline.h>


namespace Noesis
{

class StringKeyFrame;
template<class T> class FreezableCollection;
typedef FreezableCollection<StringKeyFrame> StringKeyFrameCollection;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Animates the value of a *String* property along a set of *KeyFrames*.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.stringanimationusingkeyframes.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API StringAnimationUsingKeyFrames final: public AnimationTimeline
{
public:
    StringAnimationUsingKeyFrames();
    ~StringAnimationUsingKeyFrames();

    /// Gets the type of value this animation generates
    const Type* GetTargetPropertyType() const override;

    /// Gets the collection of KeyFrame objects that define the animation
    StringKeyFrameCollection* GetKeyFrames() const;

    // Hides Freezable methods for convenience
    //@{
    Ptr<StringAnimationUsingKeyFrames> Clone() const;
    Ptr<StringAnimationUsingKeyFrames> CloneCurrentValue() const;
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
    mutable Ptr<StringKeyFrameCollection> mKeyFrames;
    Vector<Pair<Ptr<StringKeyFrame>, double>> mOrderedFrames;
    mutable Duration mNaturalDuration;

    NS_DECLARE_REFLECTION(StringAnimationUsingKeyFrames, AnimationTimeline)
};

NS_WARNING_POP

}


#endif
