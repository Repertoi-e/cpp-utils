#pragma once

#include <game.h>

#include "vendor/pybind11/eval.h"
#include "vendor/pybind11/numpy.h"
#include "vendor/pybind11/pybind11.h"

namespace py = pybind11;

struct camera {
    v2 Position = {no_init};
    v2 Scale = {no_init};
    f32 Roll;

    f32 PanSpeed;
    f32 RotationSpeed;
    f32 ZoomSpeed;

    f32 ZoomMin, ZoomMax;
};

void camera_reinit(camera *cam);
void camera_reset_constants(camera *cam);
void camera_update(camera *cam);

struct game_state {
    v4 ClearColor = {0.0f, 0.017f, 0.099f, 1.0f};

    camera Camera;

    m33 ViewMatrix = {no_init};
    m33 InverseViewMatrix = {no_init};

    ImDrawList *ViewportDrawlist;
    v2 ViewportPos = {no_init};
    v2 ViewportSize = {no_init};

    // We scale coordinates by this amount to appear better on the screen
    f32 PixelsPerMeter = 50;

    string PyCurrentDemo;
    array<string> PyDemoFiles;

    bool PyLoaded = false;
    py::module PyModule;
    py::function PyFrame, PyMouseClick, PyMouseRelease, PyMouseMove;

// We need these in python.pyd
// @Hack @Hack @Hack @Hack @Hack @Hack @Hack @Hack @Hack @Hack
#if defined DEBUG_MEMORY

    allocation_header *DEBUG_Head;
    thread::mutex *DEBUG_Mutex;
#endif
    u64 AllocationCount;
    game_memory *Memory;
};

void reload_global_state();

void load_python_demo(string fileName);
void refresh_python_demo_files();

void report_python_error(py::error_already_set &e);

void editor_main();
void editor_scene_properties();

void viewport_render();

inline game_state *GameState = null;

inline bucket_array<shader> *Shaders = null;
inline bucket_array<texture_2D> *Texture2Ds = null;
