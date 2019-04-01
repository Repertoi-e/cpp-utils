#pragma once

#include "../string/string.hpp"
#include "memory.hpp"

#include "owned_memory.hpp"

LSTD_BEGIN_NAMESPACE

// A temporary allocator. Init with a set size, (wrapper: temporary_storage_init), and it
// can be used globally to allocate memory that is not meant to last long. With this allocator you don't
// free individual allocations, but instead FREE_ALL the allocator (wrapper: temporary_storage_reset). You can
// set this allocator as the context's one and any code you call uses the very very fast and cheap
// allocator (provided it does not create a custom allocator or not use our context at all).
//
// A typical place to reset the allocator is at the start of every frame, if you are doing a game for example.
//
// Note that calling the allocator with Allocator_Mode::FREE (or the wrapper Delete from memory.hpp) does nothing.
//
struct temporary_storage : non_copyable, non_movable {
    owned_memory<byte> Data;
    size_t Size = 0, Occupied = 0, HighestUsed = 0;
};

inline temporary_storage *g_TemporaryAllocatorData;

namespace fmt {
template <typename... Args>
void print(const string_view &formatString, Args &&... args);
}

inline void *temporary_allocator(allocator_mode mode, void *allocatorData, size_t size, void *oldMemory, size_t oldSize,
                                 uptr_t) {
    auto *storage = (temporary_storage *) allocatorData;

    switch (mode) {
        case allocator_mode::ALLOCATE:
            [[fallthrough]];
        case allocator_mode::RESIZE: {
            if (storage->Occupied + size > storage->Size) {
                bool switched = false;

                if (Context.Allocator.Function == temporary_allocator ||
                    Context.Allocator.Data == g_TemporaryAllocatorData) {
                    // We know what we are doing...
                    const_cast<Implicit_Context *>(&Context)->Allocator = MALLOC;
                    switched = true;
                }
                g_TemporaryAllocatorData = null;

                fmt::print("!!! Warning !!!\n");
                fmt::print(">> Temporary allocator ran out of space, using malloc for allocation...\n");
                fmt::print(">> Invalidating pointer to TemporaryAllocatorData...\n");
                if (switched) fmt::print(">> Context detected with temporary allocator, switching it to malloc...\n");

                return g_DefaultAllocator(mode, allocatorData, size, oldMemory, oldSize, 0);
            }

            void *block = storage->Data.get() + storage->Occupied;

            if (mode == allocator_mode::RESIZE) {
                copy_memory(block, oldMemory, oldSize);
            }

            storage->Occupied += size;
            storage->HighestUsed = max(storage->HighestUsed, storage->Occupied);
            return block;
        }
        case allocator_mode::FREE:
            return null;
        case allocator_mode::FREE_ALL:
            storage->Occupied = 0;
            return null;
        default:
            assert(false);  // We shouldn't get here
    }
    return null;
}

inline void temporary_storage_init(size_t allocatorSize) {
    g_TemporaryAllocatorData = new (MALLOC) temporary_storage;

    g_TemporaryAllocatorData->Data = owned_memory(new (MALLOC) byte[allocatorSize]);
    g_TemporaryAllocatorData->Size = allocatorSize;
}

inline void temporary_storage_reset() { g_TemporaryAllocatorData->Occupied = 0; }

// Use for regions that use a lot of temporary memory but you don't need the memory outside of them.
// This can be used as a "partial" free all of the temporary allocator which can be useful if there is
// not enough temporary storage and you don't want to init it with a larger size.
inline size_t temporary_storage_get_mark() { return g_TemporaryAllocatorData->Occupied; };
inline void temporary_storage_set_mark(size_t mark) { g_TemporaryAllocatorData->Occupied = mark; };

#define temp_var_gen_(LINE) LSTD_NAMESPACE_NAME##lstd_temp_mark##LINE
#define temp_var_gen(LINE) temp_var_gen_(LINE)
#define TEMPORARY_STORAGE_MARK_SCOPE                              \
    size_t temp_var_gen(__LINE__) = temporary_storage_get_mark(); \
    defer { temporary_storage_set_mark(temp_var_gen(__LINE__)); }

#define TEMPORARY_ALLOC \
    allocator_closure { temporary_allocator, g_TemporaryAllocatorData }

LSTD_END_NAMESPACE
