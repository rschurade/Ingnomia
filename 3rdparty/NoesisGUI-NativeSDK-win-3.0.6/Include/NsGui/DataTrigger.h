////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_DATATRIGGER_H__
#define __GUI_DATATRIGGER_H__


#include <NsCore/Noesis.h>
#include <NsCore/HashMap.h>
#include <NsCore/PoolAllocator.h>
#include <NsCore/Ptr.h>
#include <NsGui/CoreApi.h>
#include <NsGui/BaseTrigger.h>
#include <NsGui/ConditionListener.h>


namespace Noesis
{

class BaseSetter;
class BaseBinding;
template<class T> class UICollection;
typedef Noesis::UICollection<Noesis::BaseSetter> BaseSetterCollection;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a trigger that applies property values or performs actions when the bound data meets
/// a specified condition.
///
/// Style, ControlTemplate, and DataTemplate all have a triggers collection. A DataTrigger
/// allows you to set property values when the property value of the data object matches a
/// specified Value. For example, if you are displaying a list of *Employee* objects, you may want
/// the foreground color to be different based on each *Employee's* current attendance. In some
/// scenarios it may be more suitable to create a converter or to use a DataTemplateSelector.
///
/// http://msdn.microsoft.com/en-us/library/system.windows.datatrigger.aspx
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API DataTrigger: public BaseTrigger
{
public:
    /// Constructor
    DataTrigger();

    /// Destructor
    ~DataTrigger();

    /// Gets or sets the binding that produces the property value of the data object.
    //@{
    BaseBinding* GetBinding() const;
    void SetBinding(BaseBinding* binding);
    //@}

    /// Gets a collection of Setter objects, which describe the property values to apply when the
    /// data item meets the specified condition.
    //@{
    BaseSetterCollection* GetSetters() const;
    //@}

    /// Gets or sets the value to be compared with the property value of the data object.
    //@{
    BaseComponent* GetValue() const;
    void SetValue(BaseComponent* value);
    //@}

    /// From BaseTrigger
    //@{
    void RegisterBindings(FrameworkElement* target, FrameworkElement* nameScope,
        bool skipTargetName, uint8_t priority) final;
    void UnregisterBindings(FrameworkElement* target) final;
    BaseComponent* FindValue(FrameworkElement* target, FrameworkElement* nameScope,
        DependencyObject* object, const DependencyProperty* dp, bool skipSourceName,
        bool skipTargetName) final;
    void Invalidate(FrameworkElement* target, FrameworkElement* nameScope,
        bool skipSourceName, bool skipTargetName, uint8_t priority) final;
    void Seal() override;
    //@}

    BaseSetterCollection* InternalGetSetters() const; // can return null

private:
    void EnsureSetters() const;

    void ForceInvalidate(FrameworkElement* target, FrameworkElement* nameScope,
        bool skipTargetName, bool fireEnterActions, uint8_t priority);

    bool Matches(FrameworkElement* target) const;

private:
    Ptr<BaseBinding> mBinding;
    Ptr<BaseComponent> mValue;
    mutable Ptr<BaseSetterCollection> mSetters;

    struct Listener final: public ConditionListener
    {
        Listener(DataTrigger* dt, FrameworkElement* t, FrameworkElement* ns, bool sk, uint8_t p);

        DependencyObject* GetTarget() const override;
        BaseBinding* GetBinding() const override;
        BaseComponent* GetValue() const override;
        void Invalidate(bool matches) const override;

        DataTrigger* trigger;
        FrameworkElement* target;
        FrameworkElement* nameScope;
        bool skipTargetName;
        uint8_t priority;
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

    NS_DECLARE_REFLECTION(DataTrigger, BaseTrigger)
};

NS_WARNING_POP

}

#endif
