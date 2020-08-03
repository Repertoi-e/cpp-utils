#pragma once

/// Defines the structure of allocators in this library.
/// Provides a default thread-safe global allocator and thread local temporary allocator.

#include <new>

#include "../internal/common.h"
#include "../thread.h"

LSTD_BEGIN_NAMESPACE

// By default we do some extra work when allocating to make it easier to catch memory related bugs.
// That work is measureable in performance so we don't want to do it in Dist configuration.
// If you want to disable it for debug/release as well, define FORCE_NO_DEBUG_MEMORY.
// You can read the comments in this file (around where DEBUG_MEMORY is mentioned)
// and see what extra stuff we do.

#if defined DEBUG || defined RELEASE
#if !defined DEBUG_MEMORY && !defined FORCE_NO_DEBUG_MEMORY
#define DEBUG_MEMORY
#endif
#else
// Don't enable extra info when in Dist configuration unless predefined
#endif

//
// Allocators:
//

// Maximum size of an allocation we will attemp to request
#define MAX_ALLOCATION_REQUEST 0xFFFFFFFFFFFFFFE0  // Around 16384 PiB

enum class allocator_mode { ALLOCATE = 0,
                            RESIZE,
                            FREE,
                            FREE_ALL };

// This is an option when allocating.
// When specified, the allocated memory is initialized to 0.
// This is handled internally when passed, so allocator implementations needn't pay attention to it.
constexpr u64 DO_INIT_0 = 1ull << 63;

// This is an option when allocating.
// Allocations marked explicitly as leaks don't get reported when the program terminates and Context.CheckForLeaksAtTermination is true.
// This is handled internally when passed, so allocator implementations needn't pay attention to it.
constexpr u64 LEAK = 1ull << 62;

// When logging all allocations (Context.LogAllAllocations == true), sometimes for e.g. if logging to a string_builder_writer,
// string_builder allocates a buffer which causes an allocation to be made and that creates an infinite calling chain.
// Allocations with this flag don't get logged.
constexpr u64 XXX_AVOID_RECURSION = 1ull << 61;

// This specifies what the signature of each allocation function should look like.
//
// _mode_ is what we are doing currently, allocating, resizing,
//      freeing a block or freeing everything
//      (*implementing FREE_ALL is NOT a requirement, depends on the use case of the user)
// _context_ is used as a pointer to any data the allocator needs (state)
// _size_ is the size of the allocation
// _oldMemory_ is used when resizing or freeing a block
// _oldSize_ is the old size of memory block, used only when resizing
//
// the last pointer to u64 is reserved for options. It is a pointer to allow the allocator function
// to modify it and propagate it to the library implementation of allocate/reallocate/free.
//
// One example use is for our debug check for leaks when the library terminates.
// The temporary allocator doesn't allow freeing so there is no reason for the library to report unfreed blocks from it.
// So the temporary allocator modifies the options to include LEAK (that flag is defined just above this comment).
//
//
// !!! When called with FREE_ALL, a return value of null means success!
//     To signify that the allocator doesn't support FREE_ALL (or the operation failed) return: (void*) -1.
//
// !!! When called with RESIZE, this doesn't mean "reallocate"!
//     Only valid return here is _oldMemory_ (memory was grown/shrank in place)
//     or null - memory can't be resized and needs to be moved.
//     In the second case we allocate a new block and copy the old data there.
//
// Alignment is handled internally. Allocator implementations needn't pay attention to it.
// When an aligned allocation is being made, we send a request at least _alignment_ bytes larger,
// so when the allocator function returns an unaligned pointer we can freely bump it.
// The information about the alignment is saved in the header, that's how we know what
// the old pointer was when freeing or reallocating.
using allocator_func_t = add_pointer_t<void *(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *)>;

#if defined DEBUG_MEMORY
//
// In DEBUG we put extra stuff to make bugs more obvious. These constants are like the ones MSVC's debug CRT model uses.
// Like them, we use specific values for bytes outside the allocated range, for freed memory and for uninitialized memory.
//
// The values of the constants and the following comment is taken from MSVC:
//
// The following values are non-zero, constant, odd, large, and atypical.
// - Non-zero values help find bugs that assume zero-filled data
// - Constant values are good so that memory filling is deterministic (to help
//   make bugs reproducible). Of course, it is bad if the constant filling of
//   weird values masks a bug.
// - Mathematically odd numbers are good for finding bugs assuming a cleared
//   lower bit (e.g. properly aligned pointers to types other than char are not odd).
// - Large byte values are less typical and are useful for finding bad addresses.
// - Atypical values are good because they typically cause early detection in code.
// - For the case of the no-man's land and free blocks, if you store to any of
//   these locations, the memory integrity checker will detect it.
//

inline constexpr s64 NO_MANS_LAND_SIZE = 4;

// _NO_MANS_LAND_SIZE_ (4) extra bytes with this value before and after the allocation block
// which help detect reading out of range errors
inline constexpr u8 NO_MANS_LAND_FILL = 0xFD;

// When freeing we fill the block with this value (detects bugs when accessing memory that's freed)
inline constexpr u8 DEAD_LAND_FILL = 0xDD;

// When allocating a new block and DO_INIT_0 was not specified we fill it with this value
// (detects bugs when accessing memory before initializing it)
inline constexpr u8 CLEAN_LAND_FILL = 0xCD;
#endif

struct allocation_header {
#if defined DEBUG_MEMORY
    allocation_header *DEBUG_Next, *DEBUG_Previous;

    // Useful for debugging (you can set a breakpoint with the ID in general_allocate() in allocator.cpp).
    // Every allocation has an unique ID equal to the ID of the previous allocation + 1.
    // This is useful for debugging bugs related to allocations because (assuming your program isn't multithreaded)
    // each time you run your program the ID of each allocation is easily reproducible (assuming no randomness from the user side).
    s64 ID;

    // This ID is used to keep track of how many times this block has been reallocated.
    // When reallocate_array() is called we check if the block can be directly resized in place (using
    // allocation_mode::RESIZE). If not, we allocate a new block and transfer all the information to it there. In both
    // cases the ID above stays the same and this local ID is incremented. This starts at 0.
    s64 RID;

    // We mark the source of the allocation if such information was provided.
    // On reallocation we overwrite these with the source provided there.
    const char *FileName;
    s64 FileLine;
#endif

    // The allocator used when allocating the memory
    allocator_func_t Function;
    void *Context;

    // The size of the allocation (NOT including the size of the header and padding)
    s64 Size;

    void *Owner;  // Points to the object that owns the block (null is valid, this is mainly used by containers). Manage
                  // this with functions from "owner_pointers.h".

#if defined DEBUG_MEMORY
    // This is another guard to check that the header is valid.
    // This points to (allocation_header *) header + 1 (the pointer we return after the allocation).
    void *DEBUG_Pointer;
#endif

    // The padding (in bytes) which was added to the pointer after allocating to make sure the resulting pointer is
    // aligned. The structure of an allocation is basically this:
    //
    // User requests allocation of _size_. The underlying allocator is called with
    //     _size_ + sizeof(allocation_header) + (sizeof(allocation_header) % alignment)
    //
    // The result:
    //   ...[..Alignment padding..][............Header............]............
    //      ^ The pointer returned by the allocator                ^ The resulting pointer (aligned)
    //
    u16 Alignment;         // We allow a maximum of 65536 bit (8192 byte) alignment
    u16 AlignmentPadding;  // Offset from the block that needs to be there in order for the result to be aligned

#if defined DEBUG_MEMORY
    // When allocating we can mark the next allocation as a leak.
    // That means that it's irrelevant if we don't free it before the end of the program (since the OS claims back the memory anyway).
    // When the Context has CheckForLeaksAtTermination set to true we log a list of unfreed allocations. Headers with this marked get skipped.
    bool MarkedAsLeak;

    // There may be padding after this (because we have modified this struct before but forgot)
    // but it's ok, we just need at least 4 bytes free. We always set the last 4 bytes of the header.
    char DEBUG_NoMansLand[NO_MANS_LAND_SIZE];
#endif

    // This header is followed by:
    // char Data[Size];
    // char NoMansLand[NO_MANS_LAND_SIZE]; (only in DEBUG)
};

// Calculates the required padding in bytes which needs to be added to _ptr_ in order to be aligned
inline u16 calculate_padding_for_pointer(void *ptr, s32 alignment) {
    assert(alignment > 0 && is_pow_of_2(alignment));
    return (u16)((((u64) ptr + (alignment - 1)) & -alignment) - (u64) ptr);
}

// Like calculate_padding_for_pointer but padding must be at least the header size
inline u16 calculate_padding_for_pointer_with_header(void *ptr, s32 alignment, u32 headerSize) {
    u16 padding = calculate_padding_for_pointer(ptr, alignment);
    if (padding < headerSize) {
        headerSize -= padding;
        if (headerSize % alignment) {
            padding += alignment * (1 + (headerSize / alignment));
        } else {
            padding += alignment * (headerSize / alignment);
        }
    }
    return padding;
}

struct allocator {
    allocator_func_t Function = null;
    void *Context = null;

    inline static s64 AllocationCount = 0;

#if defined DEBUG_MEMORY
    // We keep a linked list of all allocations. You can use this list to visualize them.
    inline static allocation_header *DEBUG_Head = null;

    // Currently this mutex should be released in the OS implementations (e.g. windows_common.cpp).
    inline static thread::mutex DEBUG_Mutex;

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   DEBUG_MEMORY
    // Use _DEBUG_unlink_header_ in your allocator implementation to make sure you don't corrupt the heap
    // (e.g. by freeing the entire allocator, but the headers still being in the linked list).
    // An example on how to use this properly in FREE_ALL is in temporary_allocator.cpp.
    // Note that it's not required for your allocator to implement FREE_ALL at all.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    // Removes a header from the list (thread-safe)
    static void DEBUG_unlink_header(allocation_header *header);

    // This adds the header to the front - making it the new DEBUG_Head (thread-safe)
    static void DEBUG_add_header(allocation_header *header);

    // Replaces _oldHeader_ with _newHeader_ in the list. (thread-safe)
    static void DEBUG_swap_header(allocation_header *oldHeader, allocation_header *newHeader);

    // Assuming that the heap is not corrupted, this reports any unfreed allocations.
    // Yes, the OS claims back all the memory the program has allocated anyway, and we are not promoting C++ style RAII
    // which make even program termination slow, we are just providing this information to the user because they might
    // want to load/unload DLLs during the runtime of the application, and those DLLs might use all kinds of complex
    // cross-boundary memory stuff things, etc. This is useful for debugging crashes related to that.
    static void DEBUG_report_leaks();
#endif
    void *general_allocate(s64 userSize, u32 alignment, u64 options = 0, const char *fileName = "", s64 fileLine = -1) const;

    // This is static, because it doesn't depend on the allocator object you call it from.
    // Each pointer has a header which has information about the allocator it was allocated with.
    static void *general_reallocate(void *ptr, s64 newSize, u64 options = 0, const char *fileName = "", s64 fileLine = -1);

    // This is static, because it doesn't depend on the allocator object you call it from.
    // Each pointer has a header which has information about the allocator it was allocated with.
    // Calling free on a null pointer doesn't do anything.
    static void general_free(void *ptr, u64 options = 0);

    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //   DEBUG_MEMORY
    //     See the comment above DEBUG_unlink_header.
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    //
    // Note: Not all allocators must support this.
    void free_all(u64 options = 0) const;

    // Verifies the integrity of headers in all allocations (only if DEBUG_MEMORY is on).
    // You can call this anytime even outside the class.
    static void verify_heap();

    // Verifies the integrity of a single header (only if DEBUG_MEMORY is on).
    // You can call this anytime even outside the class.
    static void verify_header(allocation_header *header);

    bool operator==(allocator other) const { return Function == other.Function && Context == other.Context; }
    bool operator!=(allocator other) const { return Function != other.Function || Context != other.Context; }

    operator bool() const { return Function; }
};

//
// Default allocator:
//

// General purpose allocator (like malloc)
void *default_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *);

inline allocator Malloc = {default_allocator, null};

//
// Temporary allocator:
//

struct temporary_allocator_data {
    struct page {
        void *Storage = null;
        s64 Reserved = 0;
        s64 Used = 0;

        page *Next = null;
    };

    page Base;
    s64 TotalUsed = 0;
};

// This allocator works like an arena allocator.
// It's super fast because it basically bumps a pointer.
// It can be used globally to allocate memory that is not meant to last long
// (e.g. return value of a function that converts utf8 to utf16 to pass to a windows call)
//
// With this allocator you don't free individual allocations, but instead free the entire thing (with FREE_ALL)
// when you are sure nobody uses memory the "temporary memory" anymore.
//
// It initializes itself the first time you allocate with it, the available space is always a multiple of 8 KiB,
// when we run out of space we allocate "overflow pages" and keep a list of them. The next time you FREE_ALL,
// these pages are merged and the default buffer is resized (new size is the combined size of all allocated buffers).
//
// Example use case: if you are programming a game and you need to calculate some stuff for a given frame,
//     using this allocator means having the freedom of dynamically allocating without performance implications.
//     At the end of the frame when the memory is no longer used you FREE_ALL and start the next frame.
void *temporary_allocator(allocator_mode mode, void *context, s64 size, void *oldMemory, s64 oldSize, u64 *options);

// Frees the memory held by the temporary allocator (if any).
void release_temporary_allocator();

LSTD_END_NAMESPACE

// These currently don't get namespaced.

template <typename T>
T *lstd_allocate_impl(s64 count, u32 alignment, allocator alloc, u64 options, const char *fileName = "", s64 fileLine = -1) {
    static_assert(!is_same_v<T, void>);
    static_assert(!is_const_v<T>);

    s64 size = count * sizeof(T);

    if (!alloc) alloc = Context.Alloc;
    auto *result = (T *) alloc.general_allocate(size, alignment, options, fileName, fileLine);

    if constexpr (!is_scalar_v<T>) {
        auto *p = result;
        auto *end = result + count;
        while (p != end) {
            new (p) T;
            ++p;
        }
    }
    return result;
}

template <typename T>
T *lstd_allocate_impl(s64 count, u32 alignment, allocator alloc, const char *fileName = "", s64 fileLine = -1) {
    return lstd_allocate_impl<T>(count, alignment, alloc, 0, fileName, fileLine);
}

template <typename T>
T *lstd_allocate_impl(s64 count, u32 alignment, u64 options, const char *fileName = "", s64 fileLine = -1) {
    return lstd_allocate_impl<T>(count, alignment, Context.Alloc, options, fileName, fileLine);
}

template <typename T>
T *lstd_allocate_impl(s64 count, u32 alignment, const char *fileName = "", s64 fileLine = -1) {
    return lstd_allocate_impl<T>(count, alignment, Context.Alloc, 0, fileName, fileLine);
}

// Note: We don't support "non-trivially copyable" types (types that can have logic in the copy constructor).
// We assume your type can be copied to another place in memory and just work.
// We assume that the destructor of the old copy doesn't invalidate the new copy.
template <typename T>
T *lstd_reallocate_array_impl(T *block, s64 newCount, u64 options, const char *fileName = "", s64 fileLine = -1) {
    static_assert(!is_same_v<T, void>);
    static_assert(!is_const_v<T>);

    if (!block) return null;

    // I think the standard implementation frees in this case but we need to decide
    // what _options_ should go there (no options or the ones passed to reallocate?),
    // so we leave that up to the call site.
    assert(newCount != 0);

    auto *header = (allocation_header *) block - 1;
    s64 oldCount = header->Size / sizeof(T);

    if constexpr (!is_scalar_v<T>) {
        if (newCount < oldCount) {
            auto *p = block + newCount;
            auto *end = block + oldCount;
            while (p != end) {
                p->~T();
                ++p;
            }
        }
    }

    s64 newSize = newCount * sizeof(T);
    auto *result = (T *) allocator::general_reallocate(block, newSize, options, fileName, fileLine);

    if constexpr (!is_scalar_v<T>) {
        if (oldCount < newCount) {
            auto *p = result + oldCount;
            auto *end = result + newCount;
            while (p != end) {
                new (p) T;
                ++p;
            }
        }
    }
    return result;
}

// We assume your type can be copied to another place in memory and just work.
// We assume that the destructor of the old copy doesn't invalidate the new copy.
template <typename T>
T *lstd_reallocate_array_impl(T *block, s64 newCount, const char *fileName = "", s64 fileLine = -1) {
    return lstd_reallocate_array_impl(block, newCount, 0, fileName, fileLine);
}

template <typename T>
constexpr s64 lstd_size_of_or_1_for_void() {
    if constexpr (is_same_v<T, void>) {
        return 1;
    } else {
        return sizeof(T);
    }
}

// Make sure you pass _block_ correctly as a T* otherwise we can't ensure it gets uninitialized correctly.
template <typename T>
void lstd_free_impl(T *block, u64 options = 0) {
    if (!block) return;

    static_assert(!is_const_v<T>);

    constexpr s64 sizeT = lstd_size_of_or_1_for_void<T>();

    auto *header = (allocation_header *) block - 1;
    s64 count = header->Size / sizeT;

    if constexpr (!is_same_v<T, void> && !is_scalar_v<T>) {
        auto *p = block;
        while (count--) {
            p->~T();
            ++p;
        }
    }

    allocator::general_free(block, options);
}

// T is used to initialize the resulting memory (uses placement new).
// When you pass DO_INIT_0 we initialize the memory with zeroes before initializing T.

#if defined DEBUG_MEMORY
#define allocate(T, ...) lstd_allocate_impl<T>(1, 0, __VA_ARGS__, __FILE__, __LINE__)
#define allocate_aligned(T, alignment, ...) lstd_allocate_impl<T>(1, alignment, __VA_ARGS__, __FILE__, __LINE__)
#define allocate_array(T, count, ...) lstd_allocate_impl<T>(count, 0, __VA_ARGS__, __FILE__, __LINE__)
#define allocate_array_aligned(T, count, alignment, ...) lstd_allocate_impl<T>(count, alignment, __VA_ARGS__, __FILE__, __LINE__)

#define reallocate_array(block, newCount, ...) lstd_reallocate_array_impl(block, newCount, __VA_ARGS__, __FILE__, __LINE__)
#define free lstd_free_impl
#else
#define allocate(T, ...) lstd_allocate_impl<T>(1, 0, __VA_ARGS__)
#define allocate_aligned(T, alignment, ...) lstd_allocate_impl<T>(1, alignment, __VA_ARGS__)
#define allocate_array(T, count, ...) lstd_allocate_impl<T>(count, 0, __VA_ARGS__)
#define allocate_array_aligned(T, count, alignment, ...) lstd_allocate_impl<T>(count, alignment, __VA_ARGS__)

#define reallocate_array(block, newCount, ...) lstd_reallocate_array_impl(block, newCount, __VA_ARGS__)
#define free lstd_free_impl
#endif

//
// We overload the new/delete operators so we handle the allocations. The allocator used is the one specified in the Context.
//
void *operator new(std::size_t size);
void *operator new[](std::size_t size);

void *operator new(std::size_t size, std::align_val_t alignment);
void *operator new[](std::size_t size, std::align_val_t alignment);

void operator delete(void *ptr) noexcept;
void operator delete[](void *ptr) noexcept;

void operator delete(void *ptr, std::align_val_t alignment) noexcept;
void operator delete[](void *ptr, std::align_val_t alignment) noexcept;