#ifndef ___H_WINDOWS
#define ___H_WINDOWS

#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>



#define new_item(type, ...) \
	(new type ( __VA_ARGS__ ))

#define new_array(type, length) \
	(new type [ length ])

#define delete_item(object) \
	delete object

#define delete_array(array) \
	delete [] array



namespace Windows {

enum class WaitResult {
	Okay = 0,
	Timeout,
	Error,
};

void
defaultMessagePump();

WaitResult
waitForMultipleObjects2(
	DWORD handleCount,
	const HANDLE* handles
);

}



#endif // ___H_WINDOWS


