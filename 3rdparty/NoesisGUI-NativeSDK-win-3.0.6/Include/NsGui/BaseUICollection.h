////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_BASEUICOLLECTION_H__
#define __GUI_BASEUICOLLECTION_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionDeclare.h>
#include <NsCore/Delegate.h>
#include <NsGui/CoreApi.h>
#include <NsGui/BaseCollection.h>
#include <NsGui/IComponentInitializer.h>
#include <NsGui/IUITreeNode.h>
#include <NsGui/INotifyCollectionChanged.h>
#include <NsGui/INotifyPropertyChanged.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Provides the base class for collections exposed in Noesis API.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_GUI_CORE_API BaseUICollection: public BaseCollection, public IComponentInitializer,
    public IUITreeNode, public INotifyCollectionChanged, public INotifyPropertyChanged
{
public:
    BaseUICollection();
    virtual ~BaseUICollection() = 0;

    /// From IComponentInitializer
    //@{
    bool IsInitialized() const final;
    void Init() final;
    //@}

    /// From IUITreeNode
    //@{
    IUITreeNode* GetNodeParent() const final;
    void SetNodeParent(IUITreeNode* parent) final;
    BaseComponent* FindNodeResource(const char* key, bool fullElementSearch) const final;
    BaseComponent* FindNodeName(const char* name) const final;
    ObjectWithNameScope FindNodeNameAndScope(const char* name) const final;
    //@}

    /// From INotifyCollectionChanged
    //@{
    NotifyCollectionChangedEventHandler& CollectionChanged() final;
    //@}

    /// From INotifyPropertyChanged
    //@{
    PropertyChangedEventHandler& PropertyChanged() final;
    //@}

    NS_IMPLEMENT_INTERFACE_FIXUP

protected:
    /// From BaseCollection
    //@{
    void InsertItem(uint32_t index, BaseComponent* item) override final;
    void SetItem(uint32_t index, BaseComponent* item) override final;
    void RemoveItem(uint32_t index) override final;
    void ClearItems() override final;
    //@}

    // Indicates if null items are permitted in this collection. Default is false
    virtual bool IsNullItemValid() const;

    // Called when a new item is added to the collection
    virtual void OnItemAdded(BaseComponent* item);

    // Called when an item from the collection is removed
    virtual void OnItemRemoved(BaseComponent* item);

    // Called when all items from the collection are removed
    virtual void OnItemsRemoved();

private:
    void DisconnectItems();

private:
    bool mIsInitialized;
    IUITreeNode* mOwner;

    NotifyCollectionChangedEventHandler mCollectionChanged;
    PropertyChangedEventHandler mPropertyChanged;


    NS_DECLARE_REFLECTION(BaseUICollection, BaseCollection)
};

NS_WARNING_POP

}


#endif
