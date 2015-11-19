#ifndef ___H_SOUND_THREAD
#define ___H_SOUND_THREAD

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <string>
#include <sstream>
#include <list>
#include <queue>
#include <type_traits>
#include "Thread.hpp"
#include "Subprocess.hpp"
#include "Window.hpp"



class SoundThread final : public Thread {
private:
	struct Action {
		enum class Type {
			Text,
			ReplayLast,
			End
		};

		Type type;
		std::wstring text;

		Action(Type type, const std::wstring& text) :
			type(type),
			text(text)
		{}
		Action(Action&& other) :
			type(other.type),
			text(std::move(other.text))
		{}
	};

	HANDLE queue_semaphore;
	CRITICAL_SECTION queue_lock;
	std::queue<Action> text_queue;
	CRITICAL_SECTION process_lock;
	std::list<SubProcess*> processes;
	std::wstring last_filename;
	std::list<std::wstring> audio_devices;
	std::wstring voice;
	int voice_volume;
	int voice_rate;
	int voice_channels;
	int voice_sample_rate;
	bool use_temp_dir;
	std::list<std::wstring> voice_filters;
	Window& window;
	std::wstring exe_cscript;
	std::wstring exe_voice;
	std::wstring exe_sox;

public:
	SoundThread(
		const std::wstring& voice,
		int voice_volume,
		int voice_rate,
		int voice_channels,
		int voice_sample_rate,
		const std::list<std::wstring>& filters,
		bool use_temp_dir,
		Window& window,
		const std::wstring& exe_cscript,
		const std::wstring& exe_voice,
		const std::wstring& exe_sox
	);
	virtual ~SoundThread();

	void add_audio_device(const std::wstring& audio_device);

	int execute();

	void queue(const std::wstring& text);

	void stop();

	void stop_playing();

	void replay_last();

	void clear_queue();

private:
	void generate_temp_filename(std::wstring& filename, bool use_temp_dir);

};



#endif // ___H_SOUND_THREAD


