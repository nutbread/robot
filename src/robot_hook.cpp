// http://www.codeproject.com/Articles/716591/Combining-Raw-Input-and-keyboard-Hook-to-selective
#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <iostream>
#include "Shared.hpp"
#define EXPORTING



#ifdef EXPORTING
	#define DLLSPEC __declspec(dllexport)
#else
	#define DLLSPEC __declspec(dllimport)
#endif



#ifdef __cplusplus
extern "C" {
#endif

DLLSPEC bool hook_input(HINSTANCE instance_handle, HWND main_window);
DLLSPEC bool unhook_input();

#ifdef __cplusplus
}
#endif



HWND main_window_handle = nullptr;
HHOOK hook_handle = nullptr;

LRESULT CALLBACK keyboard_hook(
	int n_code,
	WPARAM w_param,
	LPARAM l_param
) {
	if (SendMessage(main_window_handle, WM_HOOK, w_param, l_param)) {
		return 1;
	}

	return CallNextHookEx(hook_handle, n_code, w_param, l_param);
}

bool hook_input(HINSTANCE instance_handle, HWND main_window) {
	if (hook_handle == nullptr) {
		hook_handle = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) keyboard_hook, instance_handle, 0);
		main_window_handle = main_window;
		return (hook_handle != nullptr);
	}
	return false;
}

bool unhook_input() {
	if (hook_handle != nullptr) {
		BOOL b = UnhookWindowsHookEx(hook_handle);
		hook_handle = nullptr;
		main_window_handle = nullptr;
		return (b != FALSE);
	}
	return false;
}



BOOL APIENTRY DllMain(
	HANDLE h_module,
	DWORD reason,
	LPVOID reserved
) {
	switch (reason) {
		case DLL_PROCESS_ATTACH:
		// A process is loading the DLL.
		break;
		case DLL_THREAD_ATTACH:
		// A process is creating a new thread.
		break;
		case DLL_THREAD_DETACH:
		// A thread exits normally.
		break;
		case DLL_PROCESS_DETACH:
		// A process unloads the DLL.
		break;
	}
	return TRUE;
}


