////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifdef NS_COMPILER_MSVC
#include <intrin.h>
#endif


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
AtomicInteger::operator int32_t() const
{
#if NS_MULTITHREADING
    #ifdef NS_COMPILER_MSVC
        return _InterlockedCompareExchange((volatile long*)(&val), 0, 0);
    #else
        return __sync_val_compare_and_swap((int32_t*)&val, 0, 0);
    #endif
#else
    return val;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator=(int32_t v)
{
    FetchAndStore(v);
    return v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::FetchAndIncrement()
{
#if NS_MULTITHREADING
    #ifdef NS_COMPILER_MSVC
        return _InterlockedIncrement((volatile long*)(&val)) - 1;
    #else
        return __sync_fetch_and_add(&val, 1);
    #endif
#else
    return val++;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::FetchAndDecrement()
{
#if NS_MULTITHREADING
    #ifdef NS_COMPILER_MSVC
        return _InterlockedDecrement((volatile long*)(&val)) + 1;
    #else
        return __sync_fetch_and_add(&val, -1);
    #endif
#else
    return val--;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::FetchAndAdd(int32_t v)
{
#if NS_MULTITHREADING
    #ifdef NS_COMPILER_MSVC
        return _InterlockedExchangeAdd((volatile long*)(&val), v);
    #else
        return __sync_fetch_and_add(&val, v);
    #endif
#else
    int32_t tmp = val;
    val += v;
    return tmp;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::FetchAndStore(int32_t v)
{
#if NS_MULTITHREADING
    #ifdef NS_COMPILER_MSVC
        return _InterlockedExchange((volatile long*)(&val), v);
    #else
        // Note that __sync_lock_test_and_set() only has Acquire semantics
        __sync_synchronize();
        return __sync_lock_test_and_set(&val, v);
    #endif
#else
    int32_t tmp = val;
    val = v;
    return tmp;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::CompareAndSwap(int32_t newValue, int32_t comparand)
{
#if NS_MULTITHREADING
    #ifdef NS_COMPILER_MSVC
        return _InterlockedCompareExchange((volatile long*)(&val), newValue, comparand);
    #else
        return __sync_val_compare_and_swap(&val, comparand, newValue);
    #endif
#else
    int32_t tmp = val;
    val = val == comparand ? newValue : val;
    return tmp;
#endif
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator++()
{
    return FetchAndIncrement() + 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator--()
{
    return FetchAndDecrement() - 1;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator++(int)
{
    return FetchAndIncrement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator--(int)
{
    return FetchAndDecrement();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator+=(int32_t v)
{
    return FetchAndAdd(v) + v;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
int32_t AtomicInteger::operator-=(int32_t v)
{
    return operator+=(int32_t(0) - v);
}

}
