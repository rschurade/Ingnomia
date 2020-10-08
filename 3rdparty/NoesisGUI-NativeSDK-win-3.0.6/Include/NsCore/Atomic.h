////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_ATOMIC_H__
#define __CORE_ATOMIC_H__


#include <NsCore/Noesis.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// An atomic type for signed 32-bits integers
////////////////////////////////////////////////////////////////////////////////////////////////////
struct AtomicInteger
{
    /// Trivial constructor to behave correctly with static instances
    AtomicInteger() = default;

    /// Atomics are neither copyable nor movable
    AtomicInteger(const AtomicInteger&) = delete;
    AtomicInteger &operator=(const AtomicInteger&) = delete;

    /// Assignment operator
    inline int32_t operator=(int32_t v);

    /// Conversion to primitive type
    inline operator int32_t() const;

    /// All these atomic operations return the original value before applying the new value
    //@{
    inline int32_t FetchAndIncrement();
    inline int32_t FetchAndDecrement();
    inline int32_t FetchAndAdd(int32_t v);
    inline int32_t FetchAndStore(int32_t v);
    inline int32_t CompareAndSwap(int32_t newValue, int32_t comparand);
    //@}

    /// Standard Operators
    //@{
    inline int32_t operator++();
    inline int32_t operator--();
    inline int32_t operator++(int);
    inline int32_t operator--(int);
    inline int32_t operator+=(int32_t v);
    inline int32_t operator-=(int32_t v);
    //@}

#if NS_MULTITHREADING
    volatile int32_t val;
#else
    int32_t val;
#endif
};

}

#include <NsCore/Atomic.inl>

#endif
