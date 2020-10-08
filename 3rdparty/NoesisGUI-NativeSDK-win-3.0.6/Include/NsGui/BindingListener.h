////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_BINDINGLISTENER_H__
#define __GUI_BINDINGLISTENER_H__


#include <NsCore/Noesis.h>
#include <NsCore/BaseComponent.h>
#include <NsCore/Ptr.h>
#include <NsCore/Delegate.h>
#include <NsCore/Vector.h>
#include <NsCore/String.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsGui/CoreApi.h>


namespace Noesis
{

class TypeProperty;
class Type;
class DependencyObject;
class DependencyProperty;
class FrameworkElement;
class CollectionView;
class BaseBinding;
struct PathElement;
struct DependencyPropertyChangedEventArgs;
struct AncestorNameScopeChangedArgs;
struct PropertyChangedEventArgs;
struct NotifyCollectionChangedEventArgs;
struct NotifyDictionaryChangedEventArgs;
struct EventArgs;


////////////////////////////////////////////////////////////////////////////////////////////////////
/// Used by DataTriggers and MultiDataTriggers to listen to Binding changes
////////////////////////////////////////////////////////////////////////////////////////////////////
class BindingListener
{
public:
    BindingListener();
    virtual ~BindingListener() = 0;

    /// Tries to resolve binding and subscribes to binding changes
    void Register();

    /// Unsubscribes from binding changes
    void Unregister();

    /// Updates source value
    bool UpdateSource(BaseComponent* value) const;

protected:
    virtual DependencyObject* GetTarget() const = 0;
    virtual BaseBinding* GetBinding() const = 0;

    virtual void OnBindingChanged(BaseComponent* sourceValue, const Type* sourceType);

private:
    void Initialize();
    void Shutdown();

    void AddPathElement(const PathElement& element, void* context);

    struct WeakPathElement;
    void RegisterNotification(const WeakPathElement& element);
    void UnregisterNotification(const WeakPathElement& element);

    Ptr<BaseComponent> GetValue(const Type*& valueType) const;
    Ptr<BaseComponent> GetSourceValue(const WeakPathElement& element, const Type*& valueType) const;
    Ptr<BaseComponent> GetConvertedValue(BaseComponent* value, const Type*& valueType) const;

    void Invalidate(bool reevaluate);

    void OnAncestorChanged(FrameworkElement* ancestor);
    void OnNameScopeChanged(FrameworkElement* sender,
        const AncestorNameScopeChangedArgs& args);
    void OnTargetContextChanged(BaseComponent* sender,
        const DependencyPropertyChangedEventArgs& args);

    void InvalidateSource();

    void OnObjectPropertyChanged(BaseComponent* sender,
        const DependencyPropertyChangedEventArgs& args);
    void OnNotifyPropertyChanged(BaseComponent* sender,
        const PropertyChangedEventArgs& args);

    void OnCollectionChanged(BaseComponent* sender,
        const NotifyCollectionChangedEventArgs& args);
    void OnCollectionReset(BaseComponent* sender,
        const NotifyCollectionChangedEventArgs& args);

    void OnDictionaryChanged(BaseComponent* sender,
        const NotifyDictionaryChangedEventArgs& args);
    void OnDictionaryReset(BaseComponent* sender,
        const NotifyDictionaryChangedEventArgs& args);

    void OnCurrentChanged(BaseComponent* sender, const EventArgs& args);

    void OnTargetDestroyed(DependencyObject* sender);
    void OnSourceDestroyed(DependencyObject* sender);

private:
    BaseComponent* mSource;

    struct WeakPathElement
    {
        BaseComponent* source;
        CollectionView* collection;
        const TypeProperty* property;
        const DependencyProperty* dp;
        const char* key;
        int index;
    };

    Vector<WeakPathElement> mPathElements;

    bool mIsPathResolved;
};

}


#endif
