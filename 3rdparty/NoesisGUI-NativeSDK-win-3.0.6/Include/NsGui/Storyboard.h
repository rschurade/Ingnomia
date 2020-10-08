////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_STORYBOARD_H__
#define __GUI_STORYBOARD_H__


#include <NsCore/Noesis.h>
#include <NsGui/AnimationApi.h>
#include <NsGui/ParallelTimeline.h>
#include <NsGui/IStoryboard.h>
#include <NsGui/TimeSpan.h>
#include <NsGui/Clock.h>
#include <NsGui/HandoffBehavior.h>
#include <NsCore/ReflectionDeclareEnum.h>
#include <NsCore/Vector.h>
#include <NsCore/HashMap.h>


namespace Noesis
{

class PropertyPath;
class FrameworkElement;
class AnimationClock;
class AnimationTimeline;
struct AnimationTarget;
struct PathElement;

////////////////////////////////////////////////////////////////////////////////////////////////////
struct AnimationTarget
{
    inline bool operator==(const AnimationTarget& other) const
    {
        return object == other.object && dp == other.dp;
    }
    
    DependencyObject* object;
    const DependencyProperty* dp;
};

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// A container timeline that provides object and property targeting information for its 
/// child animations. 
///
/// http://msdn.microsoft.com/en-us/library/system.windows.media.animation.storyboard.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_ANIMATION_API Storyboard: public ParallelTimeline, public IStoryboard
{
public:
    Storyboard();
    ~Storyboard();

    /// Retrieves the TargetName value of the specified Timeline.
    static const char* GetTargetName(const DependencyObject* element);

    /// Makes the specified Timeline target the dependency object with the specified name.
    static void SetTargetName(DependencyObject* element, const char* name);
    
    /// Retrieves the TargetProperty value of the specified Timeline.
    static PropertyPath* GetTargetProperty(const DependencyObject* element);
    
    /// Makes the specified Timeline target the specified dependency property.
    static void SetTargetProperty(DependencyObject* element, PropertyPath* path);

    /// Retrieves the Target value of the specified Timeline.
    static DependencyObject* GetTarget(const DependencyObject* element);
    
    /// Makes the specified Timeline target the dependency object.
    static void SetTarget(DependencyObject* element, DependencyObject* target);

    /// From IStoryboard
    //@{
    void Begin() override;
    void Begin(FrameworkElement* target) override;
    void Begin(FrameworkElement* target, bool isControllable) override;
    void Begin(FrameworkElement* target, HandoffBehavior handoffBehavior) override;
    void Begin(FrameworkElement* target, HandoffBehavior handoffBehavior,
        bool isControllable) override;
    void Begin(FrameworkElement* target, FrameworkElement* nameScope) override;
    void Begin(FrameworkElement* target, FrameworkElement* nameScope,
        bool isControllable) override;
    void Begin(FrameworkElement* target, FrameworkElement* nameScope,
        HandoffBehavior handoffBehavior) override;
    void Begin(FrameworkElement* target, FrameworkElement* nameScope,
        HandoffBehavior handoffBehavior, bool isControllable) override;
    void Pause() override;
    void Pause(FrameworkElement* target) override;
    void Resume() override;
    void Resume(FrameworkElement* target) override;
    void Stop() override;
    void Stop(FrameworkElement* target) override;
    void Remove() override;
    void Remove(FrameworkElement* target) override;
    bool IsPlaying() const override;
    bool IsPlaying(FrameworkElement* target) const override;
    bool IsPaused() const override;
    bool IsPaused(FrameworkElement* target) const override;
    //@}

    /// From Freezable
    //@{
    Ptr<Storyboard> Clone() const;
    Ptr<Storyboard> CloneCurrentValue() const;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

public:
    /// Dependency properties
    //@{
    static const DependencyProperty* TargetNameProperty; // Attached
    static const DependencyProperty* TargetProperty; // Attached
    static const DependencyProperty* TargetPropertyProperty; // Attached
    //@}

protected:
    /// From Freezable
    //@{
    Ptr<Freezable> CreateInstanceCore() const override;
    //@}

    /// From Timeline
    //@{
    void OnClockDestroyed(const Clock* clock) override;
    //@}

private:
    void InternalBegin(FrameworkElement* target, FrameworkElement* nameScope,
        HandoffBehavior handoffBehavior, bool isControllable);
    void InternalPause(FrameworkElement* target);
    void InternalResume(FrameworkElement* target);
    void InternalRemove(FrameworkElement* target);

    bool InternalIsPlaying(FrameworkElement* target) const;
    bool InternalIsPaused(FrameworkElement* target) const;

    friend class TimeManager;
    void RegisterControllableClock(FrameworkElement* target, Clock* clock);
    void UnregisterControllableClock(FrameworkElement* target, bool removeClock, bool clearValue);

    typedef HashMap<FrameworkElement*, Clock*> Controllables;

    void UnregisterControllableClock(Controllables::Iterator it, bool removeClock, bool clearValue);

    void OnTargetDestroyed(DependencyObject* object);

    FrameworkElement* FindTarget() const;

    struct Animation
    {
        DependencyObject* target;
        const DependencyProperty* dp;
        AnimationClock* clock;
    };

    typedef Vector<Animation, 64> Animations;

    void ResolveTargets(FrameworkElement* fe, FrameworkElement* ns, Clock* clock,
        Animations& targets) const;

    AnimationTarget ResolveTarget(Timeline* timeline, FrameworkElement* fe,
        FrameworkElement* ns = 0) const;

    void AddPathElement(const PathElement& pathElement, void* context) const;

private:
    // NOTE: There is no CloneCommonCore because mControllables are not transferred to the
    //       cloned Storyboard

    Controllables mControllables;

    NS_DECLARE_REFLECTION(Storyboard, ParallelTimeline)
};

NS_WARNING_POP

}

#endif
