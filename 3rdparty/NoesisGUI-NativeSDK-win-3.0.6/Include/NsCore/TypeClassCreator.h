////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_TYPECLASSCREATOR_H__
#define __CORE_TYPECLASSCREATOR_H__


#include <NsCore/Noesis.h>
#include <NsCore/CompilerTools.h>


namespace Noesis
{

class Type;
class TypeClassBuilder;
class TypeProperty;
class TypeMetaData;
class Symbol;
template<class T> class Delegate;

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class Class, class Parent> uint32_t CalculateParentOffset()
{
    static_assert(IsDerived<Class, Parent>::Result, "class must inherit from parent");

    // First we define a pointer to the Class, then we cast to the Parent class, and finally we
    // calculate the offset from the Class pointer to the Parent class pointer
    Class* ptr = reinterpret_cast<Class*>(0x10000000);
    uint8_t* classPtr = reinterpret_cast<uint8_t*>(ptr);
    uint8_t* parentPtr = reinterpret_cast<uint8_t*>(static_cast<Parent*>(ptr));

    return static_cast<uint32_t>(parentPtr - classPtr);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
/// TypeClassCreator implements helper functions for building types using Noesis reflection macros.
///
///  NS_IMPLEMENT_REFLECTION(Square)
///  {
///    NsMeta<Desc>("Defines a square shape that can be drawn");
///    NsMeta<Tag>("Visual");
///    NsImpl<IShape>();
///    NsProp("side", &Square::mSide)
///        .Meta<Desc>("Length of the square side")
///        .Meta<Tag>("Editable");
///  }
///
////////////////////////////////////////////////////////////////////////////////////////////////////
class TypeClassCreator
{
public:
    NS_DISABLE_COPY(TypeClassCreator)

    /// Constructor
    inline TypeClassCreator(TypeClassBuilder* typeClass);

    /// Creates a TypeClass for the specified class
    template<class ClassT> 
    static Type* Create(Symbol name);

    /// Fills TypeClass with members of the class
    template<class ClassT, class BaseT> 
    static void Fill(Type* type);

    /// Adds meta data to the type class
    template<class T, class ...Args>
    T* Meta(Args... args);

    /// Specifies that the class implements the interface T
    template<class ClassT, class T>
    void Impl();

    /// Helper returned to add metadata to properties
    struct TypePropertyCreator
    {
        NS_FORCE_INLINE TypePropertyCreator(TypeProperty* typeProperty);
        template<class T, class ...Args> NS_FORCE_INLINE TypePropertyCreator& Meta(Args... args);

        TypeProperty* mTypeProperty;
    };

    /// Adds a property to the type class
    //@{
    template<class ClassT, class T>
    TypePropertyCreator Prop(const char* name, T ClassT::* prop);

    template<class ClassT, class T, int N>
    TypePropertyCreator Prop(const char* name, T (ClassT::* prop)[N]);

    template<class ClassT, class T, int N>
    TypePropertyCreator Prop(const char* name, T (ClassT::* prop)[N], uint32_t index);

    template<class T>
    TypePropertyCreator Prop(const char* name, uint32_t offset);

    template<class ClassT, class T>
    TypePropertyCreator Prop(const char* name, T (ClassT::*getter)() const);

    template<class ClassT, class T>
    TypePropertyCreator Prop(const char* name, T (ClassT::*getter)() const,
        void (ClassT::*setter)(T));
    //@}

    /// Adds a event to the type class
    template<class ClassT, class T>
    TypePropertyCreator Event(const char* name, Delegate<T> ClassT::* event);

    template<class ClassT, class T>
    TypePropertyCreator Event(const char* name, Delegate<T>& (ClassT::*getter)());

private:
    /// Adds base parent to the type class (only if T is different of NoParent)
    //@{
    template<class ClassT, class T>
    void Base(Int2Type<0>);

    template<class ClassT, class T>
    void Base(Int2Type<1>);
    //@}

private:
    TypeClassBuilder* mTypeClass;
};

}

#include <NsCore/TypeClassCreator.inl>

#endif
