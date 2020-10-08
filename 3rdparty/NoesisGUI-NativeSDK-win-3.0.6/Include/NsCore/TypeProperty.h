////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_TYPEPROPERTY_H__
#define __CORE_TYPEPROPERTY_H__


#include <NsCore/Noesis.h>
#include <NsCore/KernelApi.h>
#include <NsCore/Symbol.h>
#include <NsCore/CompilerTools.h>
#include <NsCore/MetaData.h>


namespace Noesis
{

class Type;
class TypeClass;
class TypeMetaData;
class BaseRefCounted;
class BaseComponent;

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251)

////////////////////////////////////////////////////////////////////////////////////////////////////
/// TypeProperty. Defines a property of a reflection class type.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_CORE_KERNEL_API TypeProperty
{
public:
    NS_DISABLE_COPY(TypeProperty)

    TypeProperty(Symbol name, const Type* type);
    virtual ~TypeProperty() = 0;

    /// Gets property's reflection name
    /// \return A symbol that represents reflection property's name
    inline Symbol GetName() const;

    /// Gets property's reflection type
    /// \return Reflection type of the property
    inline const Type* GetContentType() const;

    /// Gets property's content address
    /// \param ptr Address of the object containing this property
    /// \return Address of the property content
    //@{
    virtual void* GetContent(void* ptr) const = 0;
    virtual const void* GetContent(const void* ptr) const = 0;
    //@}

    /// Gets or sets the value of the property
    /// Property content type is used to check requested type
    /// \param ptr Address of the object containing this property
    //@{
    template<class T>
    inline typename Param<T>::Type Get(const void* ptr) const;

    template<class T>
    inline void Set(void* ptr, typename Param<T>::Type value) const;
    //@}

    /// Gets or sets property value using a boxed value or component
    //@{
    virtual Ptr<BaseComponent> GetComponent(const void* ptr) const = 0;
    virtual void SetComponent(void* ptr, BaseComponent* value) const = 0;
    //@}

    /// Returns the container of metadatas
    inline MetaData& GetMetaData();
    inline const MetaData& GetMetaData() const;

    /// Indicates if property is read-only
    bool IsReadOnly() const;

protected:
    /// Implemented by inherited classes
    //@{
    virtual const void* InternalGet(const void* ptr) const = 0;
    virtual void InternalGet(const void* ptr, void* storage) const = 0;
    virtual void InternalSet(void* ptr, const void* value) const = 0;
    //@}

    /// Default implementation return false
    virtual bool InternalIsReadOnly() const;

private:
    typedef Int2Type<0> ByRef;
    typedef Int2Type<1> ByCopy;

    template<class T>
    typename Param<T>::Type Get(const void* ptr, ByRef) const;

    template <class T>
    typename Param<T>::Type Get(const void* ptr, ByCopy) const;

private:
    Symbol mName;
    const Type* mType;

    MetaData mMetaData;
};

NS_WARNING_POP

}

// Inline Include
#include <NsCore/TypeProperty.inl>

#endif
