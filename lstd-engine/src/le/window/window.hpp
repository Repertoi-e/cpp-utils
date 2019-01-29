#pragma once

#include "../core.hpp"

#include <lstd/signal/signal.hpp>
#include <lstd/string/string.hpp>

#include "../events.h"

namespace le {

struct LE_API Window {
    bool Closed = false;

    // Reserve 256 bytes for any platform data needed by implementations.
    byte PlatformData[256] = {};

    Window(const string &title, u32 width, u32 height);

    void update();

    const string &title() { return Title; }
    void set_title(const string &title);

    bool vsync() { return VSyncEnabled; }
    void set_vsync(bool enabled);

    u32 left() { return Left; }
    void set_left(u32 left);

    u32 top() { return Top; }
    void set_top(u32 top);

    u32 width() { return Width; }
    void set_width(u32 width);

    u32 height() { return Height; }
    void set_height(u32 height);

    // Event signals. Connect to these to receive callbacks for this window.
    // Callbacks with return type of bool means whether the event has been handled.
    // This is useful for example when you want to stop the mouse left click event
    // passing "through" the UI onto the game world.
    // Returning true means stop emitting the event to the other callbacks.

    Signal<void(const Window_Closed_Event &)> WindowClosedEvent;
    Signal<void(const Window_Resized_Event &)> WindowResizedEvent;
    Signal<void(const Window_Gained_Focus_Event &)> WindowGainedFocusEvent;
    Signal<void(const Window_Lost_Focus_Event &)> WindowLostFocusEvent;
    Signal<void(const Window_Moved_Event &)> WindowMovedEvent;

    Signal<bool(const Key_Pressed_Event &), Collector_While0<bool>> KeyPressedEvent;
    Signal<void(const Key_Released_Event &)> KeyReleasedEvent;
    Signal<bool(const Key_Typed_Event &), Collector_While0<bool>> KeyTypedEvent;

    Signal<bool(const Mouse_Button_Pressed_Event &), Collector_While0<bool>> MouseButtonPressedEvent;
    Signal<void(const Mouse_Button_Released_Event &)> MouseButtonReleasedEvent;
    Signal<bool(const Mouse_Scrolled_Event &), Collector_While0<bool>> MouseScrolledEvent;
    Signal<void(const Mouse_Entered_Event &)> MouseEnteredEvent;
    Signal<void(const Mouse_Left_Event &)> MouseLeftEvent;
    Signal<bool(const Mouse_Moved_Event &), Collector_While0<bool>> MouseMovedEvent;

   private:
    string Title;
    u32 Left, Top, Width, Height;
    bool VSyncEnabled = false;

    void on_window_resized(const Window_Resized_Event &e) {
        Width = e.Width;
        Height = e.Height;
    }

    void on_window_moved(const Window_Moved_Event &e) {
        Left = e.Left;
        Top = e.Top;
    }
};
}  // namespace le