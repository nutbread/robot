#ifndef ___H_SUB_PROCESS
#define ___H_SUB_PROCESS



#define VC_EXTRALEAN
#define WIN32_LEAN_AND_MEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <sstream>
#include <cstdint>
#include <list>
#include <unordered_map>
#include <streambuf>
#include <ostream>



class SubProcess final {
public: // Public static members
	enum class PrimaryPipe {
		None,
		Auto,
		Stdout,
		Stderr,
		Stdin
	};

	class NullOStream final : public std::basic_ostream<char> {
	private:
		class NullBuffer final : public std::basic_streambuf<char> {
		public:
			NullBuffer() :
				std::basic_streambuf<char>()
			{}
			int overflow(int c) { return c; }
		};

		NullBuffer buf;

	public:
		NullOStream() :
			std::basic_ostream<char>(&this->buf),
			buf()
		{}
		~NullOStream() {}
	};

	class NullIStream final : public std::basic_istream<char> {
	private:
		class NullBuffer final : public std::basic_streambuf<char> {
		public:
			NullBuffer() :
				std::basic_streambuf<char>()
			{}
			int underflow() {
				return std::char_traits<char>::eof();
			}
		};

		NullBuffer buf;

	public:
		NullIStream() :
			std::basic_istream<char>(&this->buf),
			buf()
		{}
		~NullIStream() {}
	};


private: // Private static members
	enum : uint32_t {
		Started = 0x2,
		WaitedForInput = 0x4,
		Ended = 0x8,
		RedirectStdout = 0x10,
		RedirectStderr = 0x20,
		RedirectStdin = 0x40,
		RedirectStderrCloneStdout = 0x80,
		Communicated = 0x100,
	};

	enum {
		BufferStdout = 0,
		BufferStderr = 1,
		BufferStdin = 2,
	};

	struct ThreadInitData {
		SubProcess* self;
		HANDLE pipe;
		void* stream;
	};

	static const size_t pipeReadBufferSize;


public: // Public static methods
	static void
	argumentsToCommandLine(
		std::basic_stringstream<wchar_t>& out,
		const std::list<std::wstring>& arguments
	);


private: // Private static methods
	static DWORD WINAPI
	threadReadPipe(
		void* data
	);

	static DWORD WINAPI
	threadWritePipe(
		void* data
	);


private: // Private instance members
	wchar_t* commandLineString;
	wchar_t* environment;
	uint32_t flags;

	union {
		struct {
			void* stdoutBuffer;
			void* stderrBuffer;
			void* stdinBuffer;
		};
		void* buffers[3];
	};

	int returnCode;

	int threadHandleCount;
	HANDLE threadHandles[3];

	HANDLE stdoutRead;
	HANDLE stdoutWrite;
	HANDLE stderrRead;
	HANDLE stderrWrite;
	HANDLE stdinRead;
	HANDLE stdinWrite;

	PROCESS_INFORMATION processInfo;


private: // Private instance methods
	SubProcess();

	bool
	pipeGeneric(
		void* redirect,
		int targetBufferId,
		uint32_t flag,
		HANDLE* read,
		HANDLE* write,
		HANDLE* usedInThis
	);

	void
	closeHandles();

	void
	updateInternalState();

	void
	updateInternalStateComplete();


public: // Public instance methods
	explicit SubProcess(
		const std::list<std::wstring>& arguments
	);

	~SubProcess();

	bool
	pipeStdout(
		std::basic_ostream<char>* redirect=nullptr,
		bool cloneToStderr=false
	);

	bool
	pipeStderr(
		std::basic_ostream<char>* redirect=nullptr
	);

	bool
	pipeStdin(
		std::basic_istream<char>* redirect=nullptr
	);

	bool
	start();

	bool
	communicate(
		bool synchronous=true,
		PrimaryPipe primaryPipe=PrimaryPipe::Auto
	);

	bool
	finishCommunicating();

	bool
	waitUntilReady();

	bool
	join();

	bool
	isStarted() const;

	bool
	isDone() const;

	int
	getReturnCode() const;

	bool
	terminate();

	void
	set_environment(const std::unordered_map<std::wstring, std::wstring>& map);

	void
	clear_environment();


};



#endif // ___H_SUB_PROCESS


