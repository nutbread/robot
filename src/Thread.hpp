#ifndef ___H_THREAD
#define ___H_THREAD



#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <cstdint>



class Thread {
private: // Private types
	enum : uint32_t {
		Started = 0x1,
		Ended = 0x2,
	};


private: // Private static methods
	static DWORD WINAPI
	threadFunction(
		void* data
	);


private: // Private instance members
	uint32_t flags;
	int returnCode;
	HANDLE threadHandle;


public: // Public instance methods
	Thread();

	virtual
	~Thread();

	virtual int
	execute() = 0;

	bool
	start();

	bool
	join();

	bool
	isDone() const;

	int
	getReturnCode() const;


private: // Private instance methods
	void
	updateInternalState();

	void
	updateInternalStateComplete();


};



#endif // ___H_THREAD


