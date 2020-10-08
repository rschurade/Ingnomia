////////////////////////////////////////////////////////////////////////////////////////////////////
// NoesisGUI - http://www.noesisengine.com
// Copyright (c) 2013 Noesis Technologies S.L. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////////////////////////////


#include <NsCore/Error.h>
#include <NsCore/Memory.h>


namespace Noesis
{

////////////////////////////////////////////////////////////////////////////////////////////////////
inline PoolAllocator::PoolAllocator(): mChunks(nullptr), mFreeHead(nullptr), mAvailableBlocks(0)
{
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline PoolAllocator::PoolAllocator(uint32_t blockSize, uint32_t blockCount): mChunks(nullptr),
    mFreeHead(nullptr), mPageSize(blockSize * blockCount + sizeof(Link)), mAvailableBlocks(0),
    mBlockSize((uint16_t)blockSize)
{
    NS_ASSERT(blockSize <= 0xffff);
    NS_ASSERT(blockSize >= sizeof(Link));
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline PoolAllocator::~PoolAllocator()
{
    Reset();
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void PoolAllocator::Initialize(uint32_t blockSize, uint32_t blockCount)
{
    NS_ASSERT(blockSize <= 0xffff);
    NS_ASSERT(blockSize >= sizeof(Link));
    mBlockSize = (uint16_t)blockSize;
    mPageSize = blockSize * blockCount + sizeof(Link);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void PoolAllocator::Reset()
{
    while (mChunks != nullptr)
    {
        Link* next = mChunks->next;
        Dealloc(mChunks);
        mChunks = next;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void* PoolAllocator::Allocate()
{
    if (mFreeHead != nullptr)
    {
        Link* node = mFreeHead;
        mFreeHead = node->next;
        return node;
    }
    else
    {
        if (mAvailableBlocks == 0)
        {
            // Allocate a new chunk
            mAvailableBlocks = uint16_t((mPageSize - sizeof(Link)) / mBlockSize);
            Link* chunk = (Link*)Alloc(mPageSize);
            chunk->next = mChunks;
            mChunks = chunk;
        }

        Link* node = (Link*)((uintptr_t)mChunks + mPageSize - mAvailableBlocks * mBlockSize);
        mAvailableBlocks--;
        return node;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
inline void PoolAllocator::Deallocate(void* ptr)
{
    ((Link*)ptr)->next = mFreeHead;
    mFreeHead = (Link*)ptr;
}

}
