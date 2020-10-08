////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_EVENTTRIGGER_H__
#define __GUI_EVENTTRIGGER_H__


#include <NsCore/Noesis.h>
#include <NsCore/HashMap.h>
#include <NsCore/PoolAllocator.h>
#include <NsCore/String.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/BaseTrigger.h>
#include <NsGui/CoreApi.h>


namespace Noesis
{

class TriggerAction;
class RoutedEvent;
class FrameworkElement;
struct RoutedEventArgs;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a trigger that applies a set of actions in response to an event.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.eventtrigger.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API EventTrigger: public BaseTrigger
{
public:
    EventTrigger();
    ~EventTrigger();
    
    /// Gets the collection of BeginStoryboard objects that this EventTrigger maintains
    TriggerActionCollection* GetActions() const;

    /// Gets or sets the name of the event that initiates the trigger
    //@{
    const RoutedEvent* GetRoutedEvent() const;
    void SetRoutedEvent(const RoutedEvent* event);
    //@}

    /// Gets or sets the name of the object with the event that activates this trigger. This is 
    /// only used by element triggers or template triggers. 
    //@{
    const char* GetSourceName() const;
    void SetSourceName(const char* name);
    //@}

    /// From BaseTrigger
    //@{
    void RegisterEvents(FrameworkElement* target, FrameworkElement* nameScope,
        bool skipSourceName) final;
    void UnregisterEvents(FrameworkElement* target, bool skipSourceName) final;
    void Seal() override;
    //@}

private:
    void EnsureActions() const;

private:
    String mSourceName;
    const RoutedEvent* mRoutedEvent;
    mutable Ptr<TriggerActionCollection> mActions;

    struct Listener
    {
        EventTrigger* trigger;
        FrameworkElement* target;
        FrameworkElement* nameScope;
        mutable FrameworkElement* source;

        Listener(EventTrigger* et, FrameworkElement* t, FrameworkElement* ns,
            FrameworkElement* s);

        void Register() const;
        void Unregister() const;

    private:
        void OnEvent(BaseComponent* source, const RoutedEventArgs& args) const;
        void OnTargetDestroyed(DependencyObject* target);
        void OnSourceDestroyed(DependencyObject* source);
    };

    struct ListenerHashKeyInfo
    {
        static bool IsEmpty(Listener* key) { return key == (Listener*)0x01; }
        static void MarkEmpty(Listener*& key) { key = (Listener*)0x01; } 
        static uint32_t HashValue(Listener* key) { return Hash(key->target); }
        static uint32_t HashValue(FrameworkElement* target) { return Hash(target); }

        static bool IsEqual(Listener* lhs, Listener* rhs)
        {
            return !IsEmpty(lhs) && lhs->target == rhs->target;
        }

        static bool IsEqual(Listener* lhs, FrameworkElement* target)
        {
            return !IsEmpty(lhs) && lhs->target == target;
        }
    };

    typedef HashSet<Listener*, 0, HashBucket_K<Listener*, ListenerHashKeyInfo>> Listeners;
    Listeners mListeners;
    PoolAllocator mListenersPool;

    NS_DECLARE_REFLECTION(EventTrigger, BaseTrigger)
};

NS_WARNING_POP

}

#endif
