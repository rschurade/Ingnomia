////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#ifndef __CORE_POOLALLOCATOR_H__
#define __CORE_POOLALLOCATOR_H__


#include <NsCore/Noesis.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
/// Fixed-size memory pool manager with specified growing granularity
////////////////////////////////////////////////////////////////////////////////////////////////////
class PoolAllocator
{
public:
    NS_DISABLE_COPY(PoolAllocator)

    PoolAllocator();
    PoolAllocator(uint32_t blockSize, uint32_t blockCount);
    ~PoolAllocator();

    /// Pool initialization with the given granularity page size
    void Initialize(uint32_t blockSize, uint32_t blockCount);

    /// Clears the allocator and deallocates all internal memory
    void Reset();

    /// Allocates a new block
    void* Allocate();

    /// Frees the given block
    void Deallocate(void* ptr);

private:
    struct Link
    {
        Link* next;
    };

    Link* mChunks;
    Link* mFreeHead;

    uint32_t mPageSize;
    uint16_t mAvailableBlocks;
    uint16_t mBlockSize;
};

}

#include <NsCore/PoolAllocator.inl>

#endif
