////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_MULTIDATATRIGGER_H__
#define __GUI_MULTIDATATRIGGER_H__


#include <NsCore/Noesis.h>
#include <NsCore/HashMap.h>
#include <NsCore/PoolAllocator.h>
#include <NsGui/CoreApi.h>
#include <NsGui/BaseTrigger.h>
#include <NsGui/ConditionListener.h>


namespace Noesis
{

class BaseSetter;
class Condition;

template<class T> class UICollection;
typedef Noesis::UICollection<Noesis::BaseSetter> BaseSetterCollection;
typedef Noesis::UICollection<Noesis::Condition> ConditionCollection;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a trigger that applies property values or performs actions when the bound data meets
/// a set of conditions.
///
/// A MultiDataTrigger object is similar to a MultiTrigger, except that the conditions of a
/// MultiDataTrigger are based on property values of bound data instead of those of a UIElement.
/// In a MultiDataTrigger, a condition is met when the property value of the data item matches the
/// specified Value. You can then use setters or the *EnterActions* and *ExitActions* properties to
/// apply changes or start actions when all of the conditions are met.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.multidatatrigger.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API MultiDataTrigger: public BaseTrigger
{
public:
    MultiDataTrigger();
    ~MultiDataTrigger();

    /// Gets a collection of Condition objects. Changes to property values are applied when all of
    /// the conditions in the collection are met.
    ConditionCollection* GetConditions() const;

    /// Gets a collection of Setter objects, which describe the property values to apply when all
    /// of the conditions of the MultiTrigger are met.
    BaseSetterCollection* GetSetters() const;

    /// From BaseTrigger
    //@{
    void RegisterBindings(FrameworkElement* target, FrameworkElement* nameScope,
        bool skipTargetName, uint8_t priority) final;
    void UnregisterBindings(FrameworkElement* target) final;
    BaseComponent* FindValue(FrameworkElement* target, FrameworkElement* nameScope,
        DependencyObject* object, const DependencyProperty* dp, bool skipSourceName,
        bool skipTargetName) final;
    void Invalidate(FrameworkElement* target, FrameworkElement* nameScope, bool skipSourceName,
        bool skipTargetName, uint8_t priority) final;
    void Seal() override;
    //@}

    ConditionCollection* InternalGetConditions() const; // can return null
    BaseSetterCollection* InternalGetSetters() const; // can return null

private:
    void EnsureConditions() const;
    void EnsureSetters() const;

    void CheckConditions(ConditionCollection* conditions) const;

    void ForceInvalidate(FrameworkElement* target, FrameworkElement* nameScope,
        bool skipTargetName, bool fireEnterActions, uint8_t priority);

    bool Matches(FrameworkElement* target) const;

private:
    mutable Ptr<ConditionCollection> mConditions;
    mutable Ptr<BaseSetterCollection> mSetters;

    struct Listener
    {
        Listener(MultiDataTrigger* dt, FrameworkElement* t, FrameworkElement* ns, bool sk,
            uint8_t p);

        void Register();
        void Unregister();

        bool Matches() const;
        void UpdateMatches(bool allMatches);

        class Cond final: public ConditionListener
        {
        public:
            Cond(Listener* listener, Condition* c);

        protected:
            DependencyObject* GetTarget() const override;
            BaseBinding* GetBinding() const override;
            BaseComponent* GetValue() const override;
            void Invalidate(bool matches) const override;

        private:
            Listener* listener;
            Condition* condition;
        };

        Vector<Cond> conditions;

        MultiDataTrigger* trigger;
        FrameworkElement* target;
        FrameworkElement* nameScope;
        uint8_t priority;
        bool matches : 1;
        bool skipTargetName : 1;
        bool skipInvalidate : 1;
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

    NS_DECLARE_REFLECTION(MultiDataTrigger, BaseTrigger)
};

NS_WARNING_POP

}


#endif
