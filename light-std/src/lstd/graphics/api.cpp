#include "api.h"

#include "../math.h"
#include "../video/window.h"
#include "buffer.h"
#include "shader.h"
#include "texture.h"

LSTD_BEGIN_NAMESPACE

extern graphics::impl g_D3DImpl;  // Defined in d3d_api.cpp

void graphics::init(graphics_api api) {
    API = api;
    if (api == graphics_api::Direct3D) {
        Impl = g_D3DImpl;
    } else {
        assert(false);
    }
    Impl.Init(this);

    if (TargetWindows.find([](auto x) { return !x.Window; }) == npos) TargetWindows.append();  // Add a null target
    set_target_window(null);
}

// Sets the current render context, so you can draw to multiple windows using the same _graphics_ object.
// If you want to draw to a texture, use _set_custom_render_target_, note that you must still have a valid
// target window, and that window is associated with the resources which get created.

void graphics::set_target_window(window *win) {
    size_t index = TargetWindows.find([&](auto x) { return x.Window == win; });
    target_window *targetWindow;
    if (index == npos) {
        targetWindow = TargetWindows.append();
        targetWindow->Window = win;
        if (win) {
            targetWindow->ClosedCallbackID = win->WindowClosedEvent.connect({this, &graphics::window_closed});
            targetWindow->FramebufferResizedCallbackID =
                win->WindowFramebufferResizedEvent.connect({this, &graphics::window_resized});
            Impl.InitTargetWindow(this, targetWindow);
            window_resized({win, win->get_size().x, win->get_size().y});
        }
    } else {
        targetWindow = &TargetWindows[index];
    }

    CurrentTargetWindow = targetWindow;
    if (win) set_custom_render_target(targetWindow->CustomRenderTarget);
}

rect graphics::get_viewport() {
    assert(CurrentTargetWindow->Window);
    return CurrentTargetWindow->Viewport;
}

void graphics::set_viewport(rect viewport) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->Viewport = viewport;
    Impl.SetViewport(this, viewport);
}

rect graphics::get_scissor_rect() {
    assert(CurrentTargetWindow->Window);
    return CurrentTargetWindow->ScissorRect;
}

void graphics::set_scissor_rect(rect scissorRect) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->ScissorRect = scissorRect;
    Impl.SetScissorRect(this, scissorRect);
}

// Pass _null_ to restore rendering to the back buffer

void graphics::set_custom_render_target(texture_2D *target) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->CustomRenderTarget = target;
    Impl.SetRenderTarget(this, target);

    set_cull_mode(CurrentTargetWindow->CullMode);

    vec2<s32> size = CurrentTargetWindow->Window->get_size();
    if (target) size = {target->Width, target->Height};
    set_viewport({0, 0, size.x, size.y});
    set_scissor_rect({0, 0, size.x, size.y});
}

void graphics::set_blend(bool enabled) { Impl.SetBlend(this, enabled); }

void graphics::set_depth_testing(bool enabled) { Impl.SetDepthTesting(this, enabled); }

void graphics::set_cull_mode(cull mode) {
    assert(CurrentTargetWindow->Window);
    CurrentTargetWindow->CullMode = mode;
    Impl.SetCullMode(this, mode);
}

void graphics::clear_color(v4 color) {
    assert(CurrentTargetWindow->Window);
    if (!CurrentTargetWindow->Window->is_visible()) return;

    Impl.ClearColor(this, color);
}

void graphics::draw(u32 vertices, u32 startVertexLocation) { Impl.Draw(this, vertices, startVertexLocation); }

void graphics::draw_indexed(u32 indices, u32 startIndex, u32 baseVertexLocation) {
    Impl.DrawIndexed(this, indices, startIndex, baseVertexLocation);
}

void graphics::swap() {
    assert(CurrentTargetWindow->Window);
    if (!CurrentTargetWindow->Window->is_visible()) return;

    Impl.Swap(this);
}

void graphics::window_closed(const window_closed_event &e) {
    size_t index = TargetWindows.find([&](auto x) { return x.Window == e.Window; });
    assert(index != npos);

    target_window *targetWindow = &TargetWindows[index];
    targetWindow->Window->WindowClosedEvent.disconnect(targetWindow->ClosedCallbackID);
    targetWindow->Window->WindowFramebufferResizedEvent.disconnect(targetWindow->FramebufferResizedCallbackID);
    Impl.ReleaseTargetWindow(this, targetWindow);

    TargetWindows.remove(index);
}

void graphics::window_resized(const window_framebuffer_resized_event &e) {
    size_t index = TargetWindows.find([&](auto x) { return x.Window == e.Window; });
    assert(index != npos);

    if (!e.Window->is_visible()) return;
    Impl.TargetWindowResized(this, &TargetWindows[index], e.Width, e.Height);
}

void graphics::release() {
    if (Impl.Release) {
        For_as(it_index, range(TargetWindows.Count)) {
            auto *it = &TargetWindows[it_index];
            if (it->Window) {
                it->Window->WindowClosedEvent.disconnect(it->ClosedCallbackID);
                it->Window->WindowFramebufferResizedEvent.disconnect(it->FramebufferResizedCallbackID);
                Impl.ReleaseTargetWindow(this, it);
            }
        }
        TargetWindows.reset();

        Impl.Release(this);

        API = graphics_api::None;
    }
    assert(API == graphics_api::None);
}

LSTD_END_NAMESPACE
