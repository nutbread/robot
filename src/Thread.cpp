#include "Thread.hpp"
#include "Windows.hpp"



Thread :: Thread() :
	flags(0),
	returnCode(0),
	threadHandle(nullptr)
{
}

Thread :: ~Thread() {
	this->updateInternalState();
	if ((this->flags & (Thread::Started | Thread::Ended)) == Thread::Started) {
		// Close
		if (this->threadHandle != nullptr) {
			CloseHandle(this->threadHandle);
		}
	}
}

int
Thread :: execute() {
	// Nothing
	return 0;
}

bool
Thread :: start() {
	this->updateInternalState();
	if ((this->flags & (Thread::Started | Thread::Ended)) == Thread::Started) return false;

	// Attr
	SECURITY_ATTRIBUTES threadAttr;
	threadAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	threadAttr.lpSecurityDescriptor = nullptr;
	threadAttr.bInheritHandle = FALSE;

	// Create thread
	this->threadHandle = CreateThread(
		&threadAttr, // lpThreadAttributes,
		0, // dwStackSize,
		&Thread::threadFunction, // lpStartAddress,
		static_cast<void*>(this), // lpParameter,
		0, // dwCreationFlags,
		nullptr // lpThreadId
	);

	if (this->threadHandle != nullptr) {
		// Clear
		this->flags = Thread::Started;
		this->returnCode = 0;
		return true;
	}

	// Else, failure
	return false;
}

bool
Thread :: join() {
	if ((this->flags & Thread::Started) == 0) return false;
	this->updateInternalState();
	if ((this->flags & Thread::Ended) != 0) return true;

	// Wait
	Windows::WaitResult e = Windows::waitForMultipleObjects2(1, &this->threadHandle);
	if (e != Windows::WaitResult::Okay) return false;

	// State update
	this->updateInternalStateComplete();

	// Okay
	return true;
}

bool
Thread :: isDone() const {
	const_cast<Thread*>(this)->updateInternalState();
	return ((this->flags & Thread::Ended) != 0);
}

int
Thread :: getReturnCode() const {
	const_cast<Thread*>(this)->updateInternalState();
	return this->returnCode;
}

DWORD WINAPI
Thread :: threadFunction(
	void* data
) {
	return static_cast<Thread*>(data)->execute();
}

void
Thread :: updateInternalState() {
	if ((this->flags & (Thread::Started | Thread::Ended)) == Thread::Started) {
		// Check return code
		DWORD r = 0;
		GetExitCodeThread(this->threadHandle, &r);
		if (r == STILL_ACTIVE) {
			DWORD waitRet = WaitForSingleObject(this->threadHandle, 0);
			if (waitRet != WAIT_OBJECT_0) { // WAIT_TIMEOUT or other
				return;
			}
		}

		// Return code
		this->returnCode = r;

		// Close
		CloseHandle(this->threadHandle);
		this->threadHandle = nullptr;

		// Flags
		this->flags |= Thread::Ended;
	}
}

void
Thread :: updateInternalStateComplete() {
	// Check return code
	DWORD r = 0;
	GetExitCodeThread(this->threadHandle, &r);

	// Return code
	this->returnCode = r;

	// Close
	CloseHandle(this->threadHandle);
	this->threadHandle = nullptr;

	// Flags
	this->flags |= Thread::Ended;
}


