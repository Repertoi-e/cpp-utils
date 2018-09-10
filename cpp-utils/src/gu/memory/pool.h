#pragma once

#include "dynamic_array.h"

GU_BEGIN_NAMESPACE

struct Pool {
    size_t BlockSize = 65536;
    size_t Alignment = 8;

    Dynamic_Array<u8 *> UnusedMemblocks;
    Dynamic_Array<u8 *> UsedMemblocks;
    Dynamic_Array<u8 *> ObsoletedMemblocks;

    u8 *CurrentMemblock = 0;
    u8 *CurrentPosition = 0;
    size_t BytesLeft = 0;

    // The allocator used for reserving the initial memory block
    // If we pass a null allocator to a New/Delete wrapper it uses the context's one automatically.
    Allocator_Closure BlockAllocator;
};

// Gets _size_ bytes of memory from the pool.
// Handles running out of memory in the current block.
void *get(Pool &pool, size_t size);

// Resets the pool without releasing the allocated memory.
void reset(Pool &pool);

// Resets and frees the pool
void release(Pool &pool);

// The allocator function that works with a pool.
// As you can see, there is no "free" function defined above,
// that's because Pool doesn't manage freeing of invidual pieces
// of memory. So calling __pool_allocator with Allocator_Mode::FREE,
// doesn't do anything. Allocator_Mode::FREE_ALL does tho.
void *__pool_allocator(Allocator_Mode mode, void *allocatorData, size_t size, void *oldMemory, size_t oldSize,
                       s32 options);

GU_END_NAMESPACE