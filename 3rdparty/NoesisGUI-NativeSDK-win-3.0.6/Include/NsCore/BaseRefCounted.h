////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_BASEREFCOUNTED_H__
#define __CORE_BASEREFCOUNTED_H__


#include <NsCore/Noesis.h>
#include <NsCore/KernelApi.h>
#include <NsCore/BaseObject.h>
#include <NsCore/Atomic.h>
#include <NsCore/ReflectionDeclare.h>


namespace Noesis
{

NS_WARNING_PUSH
NS_MSVC_WARNING_DISABLE(4251 4275)

#ifndef BASEREFCOUNTED_VIRTUAL_REFS
    #ifdef NS_MANAGED
        #define BASEREFCOUNTED_VIRTUAL_REFS 1
    #else
        #define BASEREFCOUNTED_VIRTUAL_REFS 0
    #endif
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Base class for types that control their lifetime with a reference counter. Instances of this
/// class are usually stored inside a *Ptr* smart pointer. See *Ptr* class for more information.
///
/// The reference counter is initialized to 1 and can be incremented or decremented by using 
/// AddReference() or Release() respectively. When the reference counter reaches the value zero
/// the instance is destroyed by invoking its destructor.
///
/// As the lifetime is automatically managed, using *delete* on this instances is not recommended.
///
/// .. code-block:: c++
///
///     Mesh* mesh = new Mesh();
///     mesh->Release();
///
/// Note that BaseRefCounted instances can be created in the stack. In this scenario the destructor
/// will assert if there are extra references pending at destruction time.
////////////////////////////////////////////////////////////////////////////////////////////////////
class NS_CORE_KERNEL_API BaseRefCounted: public BaseObject
{
public:
    NS_DISABLE_COPY(BaseRefCounted)

    BaseRefCounted();
    virtual ~BaseRefCounted() = 0;

    /// Increments reference count and returns the new value
    int32_t AddReference() const;

    /// Decrements reference count and returns the new value
    /// When the reference count on an object reaches zero, it is deleted
    int32_t Release() const;

    /// Gets current reference count for the object
    int32_t GetNumReferences() const;

protected:
    /// Invoked when the reference counter reaches 0. By default the instance is deleted
    /// Can be reimplemented by child classes to avoid the default destruction
    /// Return the reference counter (0 if the object was destroyed)
    virtual int32_t OnDestroy() const;

#if BASEREFCOUNTED_VIRTUAL_REFS
    virtual int32_t InternalAddReference() const;
    virtual int32_t InternalRelease() const;
#else
    int32_t InternalAddReference() const;
    int32_t InternalRelease() const;
#endif    

private:
    mutable AtomicInteger mRefCount;

    NS_DECLARE_REFLECTION(BaseRefCounted, BaseObject)
};

NS_WARNING_POP

}

#include <NsCore/BaseRefCounted.inl>

#endif
