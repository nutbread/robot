#ifndef ___H_SHARED
#define ___H_SHARED



#ifndef DLL_NAME
#define DLL_NAME robot.dll
#endif

#ifndef EXE_NAME
#define EXE_NAME robot.exe
#endif

#define STRING(x) #x
#define WSTRING_1(x) L##x
#define WSTRING_2(x) WSTRING_1(#x)
#define WSTRING(x) WSTRING_2(x)

#define WM_HOOK (WM_USER + 0x0001)



#endif // ___H_SHARED


