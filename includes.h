#pragma once
#include <Windows.h>
#include <cstdint>   // uintptr_t / PTR below (no longer pulled in transitively under /permissive-)
#include <string>
#include <d3d11.h>
#include <dxgi.h>
#include "kiero/kiero.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx11.h"

// x64 has a single calling convention, so __stdcall / __cdecl are ignored there.
#ifdef _M_IX86
#define HK_CALL __stdcall
#else
#define HK_CALL
#endif

typedef HRESULT(HK_CALL* Present) (IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags);
typedef LRESULT(CALLBACK* WNDPROC)(HWND, UINT, WPARAM, LPARAM);

typedef uintptr_t PTR;
