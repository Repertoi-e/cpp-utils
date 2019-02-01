#include "lstd/common.hpp"

#if OS == WINDOWS

#if COMPILER == MSVC && defined LSTD_NO_CRT
extern "C" {
int _fltused;
}
#endif

#include "lstd/io.hpp"

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

LSTD_BEGIN_NAMESPACE

#if defined LSTD_NO_CRT
void *windows_allocator(Allocator_Mode mode, void *data, size_t size, void *oldMemory, size_t oldSize, s32) {
    switch (mode) {
        case Allocator_Mode::ALLOCATE:
            return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
        case Allocator_Mode::RESIZE:
            return HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, oldMemory, size);
        case Allocator_Mode::FREE:
            HeapFree(GetProcessHeap(), 0, oldMemory);
            return null;
        case Allocator_Mode::FREE_ALL:
            return null;
        default:
            assert(false);  // We shouldn't get here
    }
    return null;
}
Allocator_Func DefaultAllocator = windows_allocator;
#endif

void os_exit_program(int code) { _exit(code); }

void os_assert_failed(const char *file, int line, const char *condition) {
    fmt::print("{}>>> {}:{}, Assert failed: {}{}\n", fmt::FG::Red, file, line, condition, fmt::FG::Reset);
#if COMPILER == MSVC
    __debugbreak();
#else
    os_exit_program(-1);
#endif
}

#define CONSOLE_BUFFER_SIZE 1_KiB

io::Console_Writer::Console_Writer() {
    Buffer = New<byte>(CONSOLE_BUFFER_SIZE);
    Current = Buffer;
    Available = CONSOLE_BUFFER_SIZE;
}

void io::Console_Writer::write(const Memory_View &str) {
    if (str.ByteLength > Available) {
        flush();
    }

    copy_memory(Current, str.Data, str.ByteLength);
    Current += str.ByteLength;
    Available -= str.ByteLength;
}

static HANDLE g_CoutHandle = null;

void io::Console_Writer::flush() {
    if (!g_CoutHandle) {
        g_CoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
        if (!SetConsoleOutputCP(CP_UTF8)) {
            string_view warning =
                ">>> Warning, couldn't set console code page to UTF-8. Some characters might be messed up.\n";

            DWORD ignored;
            WriteFile(g_CoutHandle, warning.Data, (DWORD) warning.ByteLength, &ignored, null);
        }

        // Enable colors with escape sequences
        DWORD dw = 0;
        GetConsoleMode(g_CoutHandle, &dw);
        SetConsoleMode(g_CoutHandle, dw | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }

    DWORD ignored;
    WriteFile(g_CoutHandle, Buffer, (DWORD)(CONSOLE_BUFFER_SIZE - Available), &ignored, null);

    Current = Buffer;
    Available = CONSOLE_BUFFER_SIZE;
}

io::Console_Reader::Console_Reader() {
    // Leak, but doesn't matter since the object is global
    Buffer = New<byte>(CONSOLE_BUFFER_SIZE);
    Current = Buffer;
}

static HANDLE g_CinHandle = null;

byte io::Console_Reader::request_byte() {
    if (!g_CinHandle) {
        g_CinHandle = GetStdHandle(STD_INPUT_HANDLE);
    }
    assert(Available == 0);  // Sanity

    DWORD read;
    ReadFile(g_CinHandle, (char *) Buffer, (DWORD) CONSOLE_BUFFER_SIZE, &read, null);

    Current = Buffer;
    Available = read;

    return (read == 0) ? io::eof : (*Current);
}

static LARGE_INTEGER g_PerformanceFrequency = {0};

s64 os_get_wallclock() {
    if (g_PerformanceFrequency.QuadPart == 0) {
        if (!QueryPerformanceFrequency(&g_PerformanceFrequency)) {
            return 0;
        }
    }
    LARGE_INTEGER time;
    if (!QueryPerformanceCounter(&time)) {
        return 0;
    }
    return time.QuadPart;
}

f64 os_get_elapsed_in_seconds(s64 begin, s64 end) { return (f64)(end - begin) / (f64) g_PerformanceFrequency.QuadPart; }
f64 os_get_wallclock_in_seconds() { return (f64) os_get_wallclock() / (f64) g_PerformanceFrequency.QuadPart; }

// All windows terminals support colors
bool fmt::internal::does_terminal_support_color() { return true; }

LSTD_END_NAMESPACE

#endif  // OS == WINDOWS