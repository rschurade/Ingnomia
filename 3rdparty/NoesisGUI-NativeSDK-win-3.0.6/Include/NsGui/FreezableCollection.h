////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __GUI_FREEZABLECOLLECTION_H__
#define __GUI_FREEZABLECOLLECTION_H__


#include <NsCore/Noesis.h>
#include <NsGui/CoreApi.h>
#include <NsGui/BaseFreezableCollection.h>
#include <NsCore/ReflectionImplementEmpty.h>
#include <NsCore/IdOf.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a collection of DependencyObject, Freezable, or Animatable objects.
/// FreezableCollection is itself an Animatable type.
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
class FreezableCollection: public BaseFreezableCollection
{
public:
    /// Gets the element at the specified index
    inline T* Get(uint32_t index) const;

    /// Sets the element at the specified index
    inline void Set(uint32_t index, T* item);

    /// Adds an item to the collection. Returns The position into which the new element was
    /// inserted, or -1 to indicate that the item was not inserted into the collection
    inline int Add(T* item);

    /// Inserts an item to the collection at the specified index
    inline void Insert(uint32_t index, T* item);

    /// Determines whether the collection contains a specific value
    inline bool Contains(T* item) const;

    /// Determines the index of a specific item in the collection. Returns -1 if not found
    inline int IndexOf(T* item) const;

    /// Removes the first occurrence of a specific object from the collection. Returns true if item
    /// was removed, false to indicate that the item was not found in the collection
    inline bool Remove(T* item);

    // Hides Freezable methods for convenience
    //@{
    Ptr<FreezableCollection<T>> Clone() const;
    Ptr<FreezableCollection<T>> CloneCurrentValue() const;
    //@}

protected:
    inline const TypeClass* GetItemType() const final;
    inline Ptr<Freezable> CreateInstanceCore() const override;

    NS_IMPLEMENT_INLINE_REFLECTION_(FreezableCollection<T>, BaseFreezableCollection,
        IdOf(IdOf<T>(), "Collection"))
};

}

#include <NsGui/FreezableCollection.inl>


#endif
