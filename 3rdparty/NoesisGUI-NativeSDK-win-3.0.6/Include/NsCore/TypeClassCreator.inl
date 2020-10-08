////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Symbol.h>
#include <NsCore/CompilerTools.h>
#include <NsCore/Error.h>
#include <NsCore/Ptr.h>
#include <NsCore/TypePropertyImpl.h>
#include <NsCore/TypeClass.h>


namespace Noesis
{

template<class T> struct TypeOfHelper;
template<class T> const typename TypeOfHelper<T>::ReturnType* TypeOf();

////////////////////////////////////////////////////////////////////////////////////////////////////
TypeClassCreator::TypeClassCreator(TypeClassBuilder* typeClass): mTypeClass(typeClass)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT>
Type* TypeClassCreator::Create(Symbol name)
{
    return new TypeClass(name, IsInterface<ClassT>::Result);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class BaseT> 
void TypeClassCreator::Fill(Type* type)
{
    TypeClassCreator helper((TypeClassBuilder*)(type));
    helper.Base<ClassT, BaseT>(Int2Type<IsSame<BaseT, NoParent>::Result>());

    ClassT::template StaticFillClassType<void>(helper);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T, class ...Args>
T* TypeClassCreator::Meta(Args... args)
{
    Ptr<T> metaData = *new T(args...);
    mTypeClass->GetMetaData().Add(metaData);
    return metaData;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
void TypeClassCreator::Impl()
{
    static_assert(IsDerived<ClassT, T>::Result, "class does not implement specified interface");
    static_assert(IsInterface<T>::Result, "NsImpl used with non interface class");

    uint32_t offset = CalculateParentOffset<ClassT, T>();
    mTypeClass->AddInterface(T::StaticGetClassType((TypeTag<T>*)nullptr), offset);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T> uint32_t OffsetOf(T ClassT::* prop)
{
    return (uint32_t)((ptrdiff_t)&reinterpret_cast<const volatile char&>((((ClassT *)0)->*prop)));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Prop(const char* name,
    T ClassT::* prop)
{
    uint32_t offset = OffsetOf(prop);
    TypeProperty* p = new TypePropertyOffset<T>(Symbol::Static(name), offset);
    mTypeClass->AddProperty(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T, int N>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Prop(const char* name,
    T (ClassT::* prop)[N])
{
    uint32_t offset = OffsetOf(prop);
    TypeProperty* p = new TypePropertyArray<T, N>(Symbol::Static(name), offset);
    mTypeClass->AddProperty(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T, int N>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Prop(const char* name,
    T (ClassT::* prop)[N], uint32_t index)
{
    NS_ASSERT(index < N);
    
    uint32_t offset = OffsetOf(prop) + (index * sizeof(T));
    TypeProperty* p = new TypePropertyOffset<T>(Symbol::Static(name), offset);
    mTypeClass->AddProperty(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Prop(const char* name,
    uint32_t offset)
{
    TypeProperty* p = new TypePropertyOffset<T>(Symbol::Static(name), offset);
    mTypeClass->AddProperty(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Prop(const char* name,
    T (ClassT::*getter)() const)
{
    typedef RemoveConst<RemoveReference<T>> Type;
    TypeProperty* p = new TypePropertyFunction<ClassT, Type>(Symbol::Static(name), getter, 0);
    mTypeClass->AddProperty(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Prop(const char* name,
    T (ClassT::*getter)() const, void (ClassT::*setter)(T))
{
    typedef RemoveConst<RemoveReference<T>> Type;
    TypeProperty* p = new TypePropertyFunction<ClassT, Type>(Symbol::Static(name), getter, setter);
    mTypeClass->AddProperty(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Event(const char* name,
    Delegate<T> ClassT::* event)
{
    uint32_t offset = OffsetOf(event);
    TypeProperty* p = new TypePropertyOffsetEvent(Symbol::Static(name), offset);
    mTypeClass->AddEvent(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
typename TypeClassCreator::TypePropertyCreator TypeClassCreator::Event(const char* name,
    Delegate<T>& (ClassT::*getter_)())
{
    auto getter = (void* (ClassT::*)())(getter_);
    TypeProperty* p = new TypePropertyFunctionEvent<ClassT>(Symbol::Static(name), getter);
    mTypeClass->AddEvent(p);
    return TypePropertyCreator(p);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
void TypeClassCreator::Base(Int2Type<0>)
{
    static_assert(IsDerived<ClassT, T>::Result, "class does not derive from specified base");

    mTypeClass->AddBase(TypeOf<T>());
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class ClassT, class T>
void TypeClassCreator::Base(Int2Type<1>)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
TypeClassCreator::TypePropertyCreator::TypePropertyCreator(TypeProperty* typeProperty):
    mTypeProperty(typeProperty)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
template<class T, class ...Args>
typename TypeClassCreator::TypePropertyCreator&
TypeClassCreator::TypePropertyCreator::Meta(Args... args)
{
    Ptr<T> meta = *new T(args...);
    mTypeProperty->GetMetaData().Add(meta);
    return *this;
}

}
