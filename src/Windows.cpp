#include <cassert>
#include "Windows.hpp"



namespace Windows {

void
defaultMessagePump() {
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

WaitResult
waitForMultipleObjects2(
	DWORD handleCount,
	const HANDLE* handles
) {
	assert(handleCount > 0);
	assert(handles != nullptr);

	// WaitForMultipleObjects(handleCount, handles, waitAll=true, INFINITE);

	WaitResult retval = WaitResult::Okay;
	HANDLE* handlesEdit = nullptr;

	while (true) {
		DWORD ret = MsgWaitForMultipleObjects(
			handleCount, // nCount
			handles, // pHandles
			false, // bWaitAll
			INFINITE, // dwMilliseconds
			QS_ALLINPUT // dwWakeMask
		);

		if (ret >= WAIT_OBJECT_0 && ret < WAIT_OBJECT_0 + handleCount) {
			// Complete?
			if (--handleCount <= 0) {
				goto complete;
			}

			// Shorten array
			DWORD i = ret - WAIT_OBJECT_0;
			if (handlesEdit == nullptr) { // first; create new
				handlesEdit = new_array(HANDLE, handleCount);

				for (DWORD j = 0, k = 0; j <= handleCount; ++j) {
					if (j == i) continue;
					handlesEdit[k] = handles[j];
					++k;
				}

				handles = handlesEdit;
			}
			else { // not first; shift
				for (; i < handleCount; ++i) {
					handlesEdit[i] = handlesEdit[i + 1];
				}
			}
		}
		else if (ret == WAIT_TIMEOUT) {
			retval = WaitResult::Timeout;
			goto complete;
		}
		else if (ret != WAIT_OBJECT_0 + handleCount) {
			retval = WaitResult::Error;
			goto complete;
		}
		else {
			defaultMessagePump();
		}
	}

	// Cleanup
	complete:
	delete_array(handlesEdit);
	return retval;
}

}


