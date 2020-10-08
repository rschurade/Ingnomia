////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_NULLABLE_H__
#define __CORE_NULLABLE_H__


#include <NsCore/Noesis.h>
#include <NsCore/ReflectionImplement.h>
#include <NsCore/AssignableFromType.h>
#include <NsCore/NullableApi.h>
#include <NsCore/IdOf.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for Nullable objects
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_CORE_NULLABLE_API BaseNullable
{
public:
    /// Gets a value indicating whether the nullable has a valid value of its underlying type
    inline bool HasValue() const;

protected:
    inline BaseNullable(bool hasValue);

protected:
    bool mHasValue;

    NS_DECLARE_REFLECTION(BaseNullable, NoParent)
};

typedef decltype(nullptr) NullPtrT;

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Represents a value type that can be assigned nullptr
////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T> class Nullable: public BaseNullable
{
public:
    /// Initializes a new null instance
    inline Nullable();
    inline Nullable(NullPtrT);

    /// Constructor from value
    inline Nullable(typename Param<T>::Type value);

    /// Gets the value if it has been assigned a valid underlying value (HasValue() == true)
    inline typename Param<T>::Type GetValue() const;

    /// Gets the value if it has been assigned or the object's default value
    inline typename Param<T>::Type GetValueOrDefault() const;

    /// Assigns from value
    inline Nullable<T>& operator=(typename Param<T>::Type value);

    /// Assigns from nullptr
    inline Nullable<T>& operator=(NullPtrT);

    /// Explicit conversion to value
    inline explicit operator T() const;
    
    /// Equality and inequality operators
    //@{
    inline bool operator==(const Nullable<T>& other) const;
    inline bool operator!=(const Nullable<T>& other) const;
    inline bool operator==(typename Param<T>::Type value) const;
    inline bool operator!=(typename Param<T>::Type value) const;
    inline bool operator==(NullPtrT) const;
    inline bool operator!=(NullPtrT) const;
    //@}

    /// Returns a string that represents the current nullable
    String ToString() const;

private:
    T mValue;

    NS_IMPLEMENT_INLINE_REFLECTION(Nullable, BaseNullable, IdOf("Nullable<", IdOf<T>(), ">"))
    {
        NsMeta<AssignableFromType>(TypeOf<T>());
    }
};

}


#include "Nullable.inl"


#endif
