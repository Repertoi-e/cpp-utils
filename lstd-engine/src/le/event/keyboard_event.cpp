#include "keyboard_event.hpp"

// This file has been automatically generated

static const char KEYID_NAME[598] =
    "0\0001\0002\0003\0004\0005\0006\0007\0008\0009\0A\0B\0Backslash\0C\0CapsLoc"
    "k\0Comma\0D\0Delete\0DeleteForward\0Down\0E\0End\0Enter\0Equals\0Escape\0F"
    "\0F1\0F10\0F11\0F12\0F13\0F14\0F15\0F16\0F17\0F18\0F19\0F2\0F20\0F21\0F22\0"
    "F23\0F24\0F3\0F4\0F5\0F6\0F7\0F8\0F9\0G\0Grave\0H\0Help\0Home\0I\0Insert\0J"
    "\0K\0KP0\0KP1\0KP2\0KP3\0KP4\0KP5\0KP6\0KP7\0KP8\0KP9\0KPAdd\0KPDivide\0KPE"
    "nter\0KPEquals\0KPMultiply\0KPNumLock\0KPPoint\0KPSubtract\0L\0Left\0LeftAl"
    "t\0LeftBracket\0LeftControl\0LeftGUI\0LeftShift\0M\0Menu\0Minus\0N\0NonUSBa"
    "ckslash\0O\0P\0PageDown\0PageUp\0Pause\0Period\0PrintScreen\0Q\0Quote\0R\0R"
    "ight\0RightAlt\0RightBracket\0RightControl\0RightGUI\0RightShift\0S\0Scroll"
    "Lock\0Semicolon\0Slash\0Space\0T\0Tab\0U\0Up\0V\0W\0X\0Y\0Z";

static u16 KEYID_OFF[256] = {
    -1,  -1,  -1,  -1,  20,  22,  34,  51,  79,  105, 194, 202, 214, 223, 225, 339, 396, 409, 426, 428, 471, 479,
    542, 577, 583, 588, 590, 592, 594, 596, 2,   4,   6,   8,   10,  12,  14,  16,  18,  0,   85,  98,  53,  579,
    571, 403, 91,  354, 496, 24,  -1,  555, 473, 196, 45,  452, 565, 36,  107, 150, 173, 176, 179, 182, 185, 188,
    191, 110, 114, 118, 459, 544, 446, 216, 209, 439, 60,  81,  430, 481, 341, 74,  585, 310, 273, 299, 328, 267,
    282, 231, 235, 239, 243, 247, 251, 255, 259, 263, 227, 320, 411, -1,  -1,  290, 122, 126, 130, 134, 138, 142,
    146, 153, 157, 161, 165, 169, -1,  204, 398, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  366, 386, 346, 378, 509, 531, 487, 522, -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
    -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1};

static byte KEYID_ORDER[119] = {
    39,  30, 31, 32, 33,  34,  35,  36,  37,  38,  4,   5,   49,  6,   57,  54,  7,   42,  76,  81, 8,  77, 40, 46,
    41,  9,  58, 67, 68,  69,  104, 105, 106, 107, 108, 109, 110, 59,  111, 112, 113, 114, 115, 60, 61, 62, 63, 64,
    65,  66, 10, 53, 11,  117, 74,  12,  73,  13,  14,  98,  89,  90,  91,  92,  93,  94,  95,  96, 97, 87, 84, 88,
    103, 85, 83, 99, 86,  15,  80,  226, 47,  224, 227, 225, 16,  118, 45,  17,  100, 18,  19,  78, 75, 72, 55, 70,
    20,  52, 21, 79, 230, 48,  228, 231, 229, 22,  71,  51,  56,  44,  23,  43,  24,  82,  25,  26, 27, 28, 29};

u32 key_code_from_name(const string_view &name) {
    u32 l = 0, r = 119;
    s32 x, c;
    while (l < r) {
        u32 m = (l + r) / 2;
        x = KEYID_ORDER[m];
        c = name.compare(string_view(KEYID_NAME + KEYID_OFF[x]));
        if (c < 0) {
            r = m;
        } else if (c > 0) {
            l = m + 1;
        } else {
            return x;
        }
    }
    return -1;
}

string_view key_name_from_code(u32 code) {
    s32 off;
    if (code < 0 || code > 255) return "";
    off = KEYID_OFF[code];
    if (off == (u16) -1) return "";
    return string_view(KEYID_NAME + off);
}