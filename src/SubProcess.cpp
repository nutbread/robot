#include <iostream>
#include <sstream>
#include <cassert>
#include <cstring>
#include "SubProcess.hpp"
#include "Windows.hpp"



const size_t SubProcess :: pipeReadBufferSize = 1024;



SubProcess :: SubProcess() :
	commandLineString(nullptr),
	environment(nullptr),
	flags(0),
	stdoutBuffer(nullptr),
	stderrBuffer(nullptr),
	stdinBuffer(nullptr),
	returnCode(0),
	threadHandleCount(0),
	stdoutRead(nullptr),
	stdoutWrite(nullptr),
	stderrRead(nullptr),
	stderrWrite(nullptr),
	stdinRead(nullptr),
	stdinWrite(nullptr)
{
}

SubProcess :: SubProcess(
	const std::list<std::wstring>& arguments
) :
	SubProcess()
{
	// Create the command line string
	wchar_t* argStr;
	std::basic_stringstream<wchar_t> ss;
	SubProcess::argumentsToCommandLine(ss, arguments);

	std::basic_string<wchar_t> str = ss.str();
	size_t strLength = str.length();

	argStr = new_array(wchar_t, strLength + 1);
	str.copy(argStr, strLength, 0);
	argStr[strLength] = '\x00';

	this->commandLineString = argStr;
}

SubProcess :: ~SubProcess() {
	// Delete command line string
	delete_array(this->commandLineString);
	delete_array(this->environment);

	// Close handles
	this->closeHandles();
}

bool
SubProcess :: pipeGeneric(
	void* redirect,
	int targetBufferId,
	uint32_t flag,
	HANDLE* read,
	HANDLE* write,
	HANDLE* usedInThis
) {
	// Disable
	if (redirect == nullptr || (this->flags & flag) != 0) {
		CloseHandle(*read);
		CloseHandle(*write);
		*read = nullptr;
		*write = nullptr;
		this->buffers[targetBufferId] = nullptr;

		this->flags &= ~flag;
	}

	// Enable
	if (redirect != nullptr) {
		// Pipe redirect
		SECURITY_ATTRIBUTES pipeAttr;
		pipeAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		pipeAttr.bInheritHandle = TRUE;
		pipeAttr.lpSecurityDescriptor = nullptr;

		if (!CreatePipe(read, write, &pipeAttr, 0)) return false;
		if (!SetHandleInformation(*usedInThis, HANDLE_FLAG_INHERIT, 0)) {
			// Close and null handles
			CloseHandle(*read);
			CloseHandle(*write);
			*read = nullptr;
			*write = nullptr;
			return false;
		}

		this->buffers[targetBufferId] = redirect;

		this->flags |= flag;
	}

	// Done
	return true;
}

bool
SubProcess :: pipeStdout(
	std::basic_ostream<char>* redirect,
	bool cloneToStderr
) {
	return (
		((this->flags & (SubProcess::Started | SubProcess::Ended)) != SubProcess::Started) &&
		this->pipeGeneric(
			redirect,
			SubProcess::BufferStdout,
			(redirect != nullptr && cloneToStderr) ? (SubProcess::RedirectStdout | SubProcess::RedirectStderrCloneStdout) : SubProcess::RedirectStdout,
			&this->stdoutRead,
			&this->stdoutWrite,
			&this->stdoutRead
		)
	);
}

bool
SubProcess :: pipeStderr(
	std::basic_ostream<char>* redirect
) {
	if (
		((this->flags & (SubProcess::Started | SubProcess::Ended)) != SubProcess::Started) &&
		this->pipeGeneric(
			redirect,
			SubProcess::BufferStderr,
			SubProcess::RedirectStderr,
			&this->stderrRead,
			&this->stderrWrite,
			&this->stderrRead
		)
	) {
		this->flags &= ~SubProcess::RedirectStderrCloneStdout;
		return true;
	}
	return false;
}

bool
SubProcess :: pipeStdin(
	std::basic_istream<char>* redirect
) {
	return (
		((this->flags & (SubProcess::Started | SubProcess::Ended)) != SubProcess::Started) &&
		this->pipeGeneric(
			redirect,
			SubProcess::BufferStdin,
			SubProcess::RedirectStdin,
			&this->stdinRead,
			&this->stdinWrite,
			&this->stdinWrite
		)
	);
}

bool
SubProcess :: start() {
	// Already started
	this->updateInternalState();
	if ((this->flags & (SubProcess::Started | SubProcess::Ended)) == SubProcess::Started) return false;
	if ((this->flags & SubProcess::Ended) != 0) {
		// Re-setup pipes
		if ((this->flags & SubProcess::RedirectStdout) != 0) {
			this->pipeStdout(static_cast<std::basic_ostream<char>*>(this->stdoutBuffer), ((this->flags & SubProcess::RedirectStderrCloneStdout) != 0));
		}
		if ((this->flags & SubProcess::RedirectStderr) != 0) {
			this->pipeStderr(static_cast<std::basic_ostream<char>*>(this->stderrBuffer));
		}
		if ((this->flags & SubProcess::RedirectStdin) != 0) {
			this->pipeStdin(static_cast<std::basic_istream<char>*>(this->stdinBuffer));
		}
	}

	// Setup
	SECURITY_ATTRIBUTES processAttr;
	processAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	processAttr.lpSecurityDescriptor = nullptr;
	processAttr.bInheritHandle = TRUE;

	SECURITY_ATTRIBUTES threadAttr;
	threadAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	threadAttr.lpSecurityDescriptor = nullptr;
	threadAttr.bInheritHandle = TRUE;

    ZeroMemory(&this->processInfo, sizeof(PROCESS_INFORMATION));

	HANDLE out = ((this->flags & SubProcess::RedirectStdout) == 0) ? GetStdHandle(STD_OUTPUT_HANDLE) : this->stdoutWrite;
	HANDLE err = ((this->flags & SubProcess::RedirectStderrCloneStdout) == 0) ?
		(((this->flags & SubProcess::RedirectStderr) == 0) ? GetStdHandle(STD_ERROR_HANDLE) : this->stderrWrite) :
		this->stdoutWrite;
	HANDLE in = ((this->flags & SubProcess::RedirectStdin) == 0) ? GetStdHandle(STD_INPUT_HANDLE) : this->stdinRead;

	// Start process
	STARTUPINFOW startupInfo;
	ZeroMemory(&startupInfo, sizeof(STARTUPINFOW));
	startupInfo.cb = sizeof(STARTUPINFOW);
	startupInfo.hStdOutput = out;
	startupInfo.hStdError = err;
	startupInfo.hStdInput = in;
	startupInfo.dwFlags |= STARTF_USESTDHANDLES;

	BOOL b = CreateProcessW(
		nullptr, // lpApplicationName
		this->commandLineString, // lpCommandLine
		&processAttr, // lpProcessAttributes
		&threadAttr, // lpThreadAttributes
		TRUE, // bInheritHandles
		CREATE_UNICODE_ENVIRONMENT, // dwCreationFlags
		this->environment, // lpEnvironment
		nullptr, // lpCurrentDirectory
		&startupInfo, // lpStartupInfo
		&this->processInfo // lpProcessInformation
	);

	// Validate
	if (b) {
		// Close handles
		if (this->stdoutWrite != nullptr) {
			CloseHandle(this->stdoutWrite);
			this->stdoutWrite = nullptr;
		}
		if (this->stderrWrite != nullptr) {
			CloseHandle(this->stderrWrite);
			this->stderrWrite = nullptr;
		}
		if (this->stdinRead != nullptr) {
			CloseHandle(this->stdinRead);
			this->stdinRead = nullptr;
		}

		// Success
		this->returnCode = 0;
		this->flags |= SubProcess::Started;
		this->flags &= ~(SubProcess::Ended | SubProcess::WaitedForInput | SubProcess::Communicated);
		return true;
	}

	// Error
	return false;
}

bool
SubProcess :: waitUntilReady() {
	if ((this->flags & SubProcess::Started) == 0) return false;
	this->updateInternalState();
	if ((this->flags & (SubProcess::Ended | SubProcess::WaitedForInput)) != 0) return true;

	// Wait
	if (WaitForInputIdle(processInfo.hProcess, INFINITE) == 0) {
		// Complete
		this->flags |= SubProcess::WaitedForInput;
		return true;
	}

	// Some other error
	return false;
}

bool
SubProcess :: communicate(
	bool synchronous,
	PrimaryPipe primaryPipe
) {
	if (
		(this->flags & (SubProcess::RedirectStdout | SubProcess::RedirectStderr | SubProcess::RedirectStdin)) == 0 ||
		(this->flags & SubProcess::Started) == 0 ||
		(this->flags & SubProcess::Communicated) != 0
	) {
		return false;
	}

	// Flag
	this->flags |= SubProcess::Communicated;

	// Setup
	bool ret = true;
	this->threadHandleCount = 0;
	SECURITY_ATTRIBUTES threadAttr;
	threadAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	threadAttr.lpSecurityDescriptor = nullptr;
	threadAttr.bInheritHandle = TRUE;
	ThreadInitData* data;

	const int pipeCount = 3;
	int primaryId = -1;
	int i;
	struct PipeInfo {
		int32_t flag;
		PrimaryPipe primaryPipe;
		HANDLE pipe;
		void* buffer;
		LPTHREAD_START_ROUTINE threadRoutine;
	};
	const PipeInfo pipes[] = {
		{ SubProcess::RedirectStdout , SubProcess::PrimaryPipe::Stdout , this->stdoutRead , this->stdoutBuffer , &SubProcess::threadReadPipe },
		{ SubProcess::RedirectStderr , SubProcess::PrimaryPipe::Stderr , this->stderrRead , this->stderrBuffer , &SubProcess::threadReadPipe },
		{ SubProcess::RedirectStdin , SubProcess::PrimaryPipe::Stdin , this->stdinWrite , this->stdinBuffer , &SubProcess::threadWritePipe },
	};

	// Auto
	if (synchronous && primaryPipe == SubProcess::PrimaryPipe::Auto) {
		for (i = 0; i < pipeCount; ++i) {
			if ((this->flags & pipes[i].flag) != 0) {
				primaryPipe = pipes[i].primaryPipe;
				break;
			}
		}
	}

	// Begin threads
	for (i = 0; i < pipeCount; ++i) {
		if ((this->flags & pipes[i].flag) != 0 && pipes[i].pipe != nullptr) {
			if (primaryPipe == pipes[i].primaryPipe) {
				primaryId = i;
			}
			else {
				// Threaded
				data = new_item(ThreadInitData);
				data->self = this;
				data->pipe = pipes[i].pipe;
				data->stream = pipes[i].buffer;

				this->threadHandles[this->threadHandleCount] = CreateThread(
					&threadAttr, // lpThreadAttributes,
					0, // dwStackSize,
					pipes[i].threadRoutine, // lpStartAddress,
					static_cast<void*>(data), // lpParameter,
					0, // dwCreationFlags,
					nullptr // lpThreadId
				);

				if (this->threadHandles[this->threadHandleCount] == nullptr) {
					ret = false;
				}
				else {
					++this->threadHandleCount;
				}
			}
		}
	}

	// Primary
	if (primaryId >= 0) {
		// Threaded
		data = new_item(ThreadInitData);
		data->self = this;
		data->pipe = pipes[primaryId].pipe;
		data->stream = pipes[primaryId].buffer;

		(pipes[primaryId].threadRoutine)(static_cast<void*>(data));
	}


	// Join threads
	if (synchronous) {
		Windows::waitForMultipleObjects2(this->threadHandleCount, this->threadHandles);

		for (int i = 0; i < this->threadHandleCount; ++i) {
			CloseHandle(this->threadHandles[i]);
		}
		this->threadHandleCount = 0;
	}


	// Done
	return ret;
}

bool
SubProcess :: finishCommunicating() {
	if (this->threadHandleCount == 0) return false;

	Windows::waitForMultipleObjects2(this->threadHandleCount, this->threadHandles);

	for (int i = 0; i < this->threadHandleCount; ++i) {
		CloseHandle(this->threadHandles[i]);
	}
	this->threadHandleCount = 0;

	return true;
}

bool
SubProcess :: join() {
	if ((this->flags & SubProcess::Started) == 0) return false;
	this->updateInternalState();
	if ((this->flags & SubProcess::Ended) != 0) return true;

	// Wait
	if (Windows::waitForMultipleObjects2(1, &this->processInfo.hProcess) == Windows::WaitResult::Okay) {
		// Close handles
		this->updateInternalStateComplete();

		// Complete
		return true;
	}

	// Some other error
	return false;
}

bool
SubProcess :: isStarted() const {
	return ((this->flags & SubProcess::Started) != 0);
}

bool
SubProcess :: isDone() const {
	const_cast<SubProcess*>(this)->updateInternalState();
	return ((this->flags & SubProcess::Ended) != 0);
}

int
SubProcess :: getReturnCode() const {
	const_cast<SubProcess*>(this)->updateInternalState();
	return this->returnCode;
}

void
SubProcess :: updateInternalState() {
	if ((this->flags & (SubProcess::Started | SubProcess::Ended)) == SubProcess::Started) {
		DWORD r = 0;
		GetExitCodeProcess(this->processInfo.hProcess, &r);
		if (r == STILL_ACTIVE) {
			DWORD waitRet = WaitForSingleObject(this->processInfo.hProcess, 0);
			if (waitRet != WAIT_OBJECT_0) { // WAIT_TIMEOUT or other
				return;
			}
		}

		// Done
		this->flags |= SubProcess::Ended;
		this->returnCode = r;
		this->closeHandles();
	}
}

void
SubProcess :: updateInternalStateComplete() {
	DWORD r = 0;
	GetExitCodeProcess(this->processInfo.hProcess, &r);

	// Done
	this->flags |= SubProcess::Ended;
	this->returnCode = r;
	this->closeHandles();
}

void
SubProcess :: closeHandles() {
	if (this->stdoutRead != nullptr) {
		CloseHandle(this->stdoutRead);
		this->stdoutRead = nullptr;
	}
	if (this->stdoutWrite != nullptr) {
		CloseHandle(this->stdoutWrite);
		this->stdoutWrite = nullptr;
	}
	if (this->stderrRead != nullptr) {
		CloseHandle(this->stderrRead);
		this->stderrRead = nullptr;
	}
	if (this->stderrWrite != nullptr) {
		CloseHandle(this->stderrWrite);
		this->stderrWrite = nullptr;
	}
	if (this->stdinRead != nullptr) {
		CloseHandle(this->stdinRead);
		this->stdinRead = nullptr;
	}
	if (this->stdinWrite != nullptr) {
		CloseHandle(this->stdinWrite);
		this->stdinWrite = nullptr;
	}

	if ((this->flags & SubProcess::Started) != 0) {
		if (this->processInfo.hProcess != nullptr) {
			CloseHandle(this->processInfo.hProcess);
			this->processInfo.hProcess = nullptr;
		}
		if (this->processInfo.hThread != nullptr) {
			CloseHandle(this->processInfo.hThread);
			this->processInfo.hThread = nullptr;
		}
	}

	for (int i = 0; i < this->threadHandleCount; ++i) {
		CloseHandle(this->threadHandles[i]);
	}
	this->threadHandleCount = 0;
}

DWORD WINAPI
SubProcess :: threadReadPipe(
	void* data
) {
	// Setup
	ThreadInitData* tData = static_cast<ThreadInitData*>(data);
	DWORD bytesRead;
	char* buffer = new_array(char, SubProcess::pipeReadBufferSize);
	std::basic_ostream<char>* targetStream = static_cast< std::basic_ostream<char>* >(tData->stream);
	BOOL okay;

	// Read loop
	while (true) {
		okay = ReadFile(
			tData->pipe, // hFile
			static_cast<void*>(buffer), // lpBuffer
			SubProcess::pipeReadBufferSize, // nNumberOfBytesToRead
			&bytesRead, // lpNumberOfBytesRead
			nullptr // lpOverlapped
		);

		// Stop
		if (!okay || bytesRead == 0) break;

		// Update stream
		targetStream->write(buffer, bytesRead);
	}

	// Close
	CloseHandle(tData->pipe);

	// Done
	delete_array(buffer);
	delete_item(tData);
	return 0;
}

DWORD WINAPI
SubProcess :: threadWritePipe(
	void* data
) {
	// Setup
	ThreadInitData* tData = static_cast<ThreadInitData*>(data);
	DWORD bytesToWrite, bytesWritten;
	size_t byteOffset;
	char* buffer = new_array(char, SubProcess::pipeReadBufferSize);
	std::basic_istream<char>* sourceStream = static_cast< std::basic_istream<char>* >(tData->stream);
	BOOL okay;

	// Read loop
	while (true) {
		// Read into buffer
		sourceStream->read(buffer, SubProcess::pipeReadBufferSize);
		bytesToWrite = static_cast<DWORD>(sourceStream->gcount());
		if (bytesToWrite == 0) break;
		byteOffset = 0;

		// Write
		while (true) {
			okay = WriteFile(
				tData->pipe, // hFile
				static_cast<void*>(&buffer[byteOffset]), // lpBuffer
				bytesToWrite, // nNumberOfBytesToRead
				&bytesWritten, // lpNumberOfBytesRead
				nullptr // lpOverlapped
			);

			// Done?
			if (!okay) goto double_break;
			if (bytesWritten == 0 || (byteOffset += bytesWritten) >= bytesToWrite) break;
		}

		// Check if end of input stream
		if (sourceStream->eof()) break;
	}
	double_break:

	// Close
	CloseHandle(tData->pipe);

	// Done
	delete_array(buffer);
	delete_item(tData);
	return 0;
}

void
SubProcess :: argumentsToCommandLine(
	std::basic_stringstream<wchar_t>& out,
	const std::list<std::wstring>& arguments
) {
	size_t i, j, last;
	wchar_t c;
	const wchar_t* str;
	size_t escaped;

	i = 0;
	for (auto it = arguments.cbegin(); it != arguments.cend(); ++it) {
		str = it->c_str();
		escaped = 0;

		if (i > 0) out.put(' ');
		out.put(L'"');

		last = 0;
		for (j = 0; (c = str[j]) != L'\x00'; ++j) {
			if (c == L'\\') {
				++escaped;
			}
			else if (c == L'"') {
				if (j > last) {
					out.write(&(str[last]), j - last);
					last = j;

					if (escaped > 0) {
						out.write(&(str[j - escaped]), escaped);
						escaped = 0;
					}

					out.put(L'\\');
				}
			}
			else {
				escaped = 0;
			}
		}
		out.write(&(str[last]), j - last);
		if (escaped > 0) {
			out.write(&(str[j - escaped]), escaped);
		}

		out.put(L'"');

		++i;
	}
}

bool
SubProcess :: terminate() {
	if ((this->flags & SubProcess::Started) == 0) return false;
	this->updateInternalState();
	if ((this->flags & SubProcess::Ended) != 0) return true;

	TerminateProcess(this->processInfo.hProcess, -1);

	return true;
}

void
SubProcess :: set_environment(const std::unordered_map<std::wstring, std::wstring>& map) {
	delete_array(this->environment);

	std::wstringstream s;

	for (auto it = map.begin(); it != map.end(); ++it) {
		s << it->first << L'=' << it->second << L'\x00';
	}

	s << L'\x00';

	std::wstring env = s.str();
	this->environment = new_array(wchar_t, env.length());
	std::memcpy(this->environment, env.c_str(), env.length() * sizeof(wchar_t));
}

void
SubProcess :: clear_environment() {
	delete_array(this->environment);
	this->environment = nullptr;
}


