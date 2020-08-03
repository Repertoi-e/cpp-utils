#pragma once

#include "lstd/math.h"
#include "vendor/imgui/imgui.h"
#include "vendor/imguizmo/ImGuizmo.h"

#if OS == WINDOWS

#undef MAC
#undef _MAC
#include <Objbase.h>
#include <Windows.h>
#include <Windowsx.h>
#include <Xinput.h>
#include <d2d1.h>
#include <d3d10_1.h>
#include <d3d11.h>
#include <d3dcommon.h>
#include <d3dcompiler.h>
#include <dbghelp.h>
#include <dbt.h>
#include <ddraw.h>
#include <dwmapi.h>
#include <dwrite.h>
#include <dxgidebug.h>
#include <initguid.h>
#include <objbase.h>
#include <process.h>
#include <shellapi.h>  // @DependencyCleanup: CommandLineToArgvW
#include <wctype.h>
#include <wincodec.h>
// CLANG FORMAT NO
#include <hidclass.h>

#endif