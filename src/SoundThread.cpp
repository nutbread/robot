#include <iostream>
#include "SoundThread.hpp"
#include "Windows.hpp"
#include "Environment.hpp"



std::unordered_map<std::wstring, std::wstring>
get_environment_clone(const std::unordered_map<std::wstring, std::wstring>& original, const std::wstring& audio_device) {
	std::unordered_map<std::wstring, std::wstring> clone = original;
	std::wstring key = L"AUDIODEV";
	auto it = clone.find(key);

	if (audio_device.length() == 0) {
		if (it != clone.end()) {
			clone.erase(it);
		}
	}
	else {
		if (it == clone.end()) {
			clone.emplace(decltype(clone)::value_type(key, audio_device));
		}
	}

	return clone;
}



SoundThread :: SoundThread(
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
) :
	Thread(),
	queue_semaphore(nullptr),
	queue_lock(),
	text_queue(),
	process_lock(),
	processes(),
	last_filename(L""),
	audio_devices(),
	voice(voice),
	voice_volume(voice_volume),
	voice_rate(voice_rate),
	voice_channels(voice_channels),
	voice_sample_rate(voice_sample_rate),
	use_temp_dir(use_temp_dir),
	voice_filters(filters),
	window(window),
	exe_cscript(exe_cscript),
	exe_voice(exe_voice),
	exe_sox(exe_sox)
{
	this->queue_semaphore = CreateSemaphore(nullptr, 0, 0x7FFFFFFF, nullptr);
	InitializeCriticalSection(&this->queue_lock);
	InitializeCriticalSection(&this->process_lock);
}
SoundThread :: ~SoundThread() {
	DeleteCriticalSection(&this->process_lock);
	DeleteCriticalSection(&this->queue_lock);
	CloseHandle(this->queue_semaphore);

	if (this->last_filename.length() > 0) {
		DeleteFile(this->last_filename.c_str());
	}
}

int SoundThread :: execute() {
	std::wstring text;
	Action::Type action_type;
	std::basic_stringstream<char> text_stream;
	std::wstringstream temp;
	SubProcess* encode_proc = nullptr;
	size_t playback_proc_count = this->audio_devices.size();
	if (playback_proc_count == 0) playback_proc_count = 1;
	SubProcess** playback_proc = new SubProcess*[playback_proc_count];
	std::list<std::wstring> cmd;
	SubProcess::NullOStream null_out;
	SubProcess::NullIStream null_in;
	auto env = get_environment_map();
	size_t i;


	while (true) {
		Windows::waitForMultipleObjects2(1, &this->queue_semaphore);
		EnterCriticalSection(&this->queue_lock);

		if (this->text_queue.empty()) {
			LeaveCriticalSection(&this->queue_lock);
			continue;
		}

		action_type = this->text_queue.front().type;
		if (action_type == Action::Type::Text) {
			text = this->text_queue.front().text;
		}
		this->text_queue.pop();

		LeaveCriticalSection(&this->queue_lock);

		// End
		if (action_type == Action::Type::End) {
			break;
		}

		if (action_type == Action::Type::Text) {
			this->window.set_status_bar_color(240, 20, 0);

			// Speak
			text_stream.str("");
			text_stream.clear();
			for (i = 0; i < text.length(); ++i) {
				text_stream << static_cast<char>(static_cast<unsigned char>(static_cast<std::make_unsigned<wchar_t>::type>(text[i])));
			}

			// Encode
			if (this->last_filename.length() > 0) {
				DeleteFile(this->last_filename.c_str());
			}

			this->last_filename = L"temp.wav";
			this->generate_temp_filename(this->last_filename, this->use_temp_dir);

			// Command
			cmd.clear();
			cmd.emplace(cmd.end(), this->exe_cscript);
			cmd.emplace(cmd.end(), L"//Nologo");
			cmd.emplace(cmd.end(), this->exe_voice);

			cmd.emplace(cmd.end(), this->last_filename); // filename
			cmd.emplace(cmd.end(), this->voice); // voice
			temp << this->voice_volume; // volume
			cmd.emplace(cmd.end(), temp.str());
			temp.str(L"");
			temp << this->voice_rate; // rate
			cmd.emplace(cmd.end(), temp.str());
			temp.str(L"");
			temp << this->voice_channels; // channels
			cmd.emplace(cmd.end(), temp.str());
			temp.str(L"");
			temp << this->voice_sample_rate; // sample rate
			cmd.emplace(cmd.end(), temp.str());
			temp.str(L"");


			// Exec
			bool okay = false;
			encode_proc = new SubProcess(cmd);
			encode_proc->pipeStdout(&null_out);
			encode_proc->pipeStderr(&null_out);
			encode_proc->pipeStdin(&text_stream);
			if (encode_proc->start()) {
				EnterCriticalSection(&this->process_lock);
				this->processes.push_back(encode_proc);
				LeaveCriticalSection(&this->process_lock);

				encode_proc->communicate();

				EnterCriticalSection(&this->process_lock);
				this->processes.clear();
				LeaveCriticalSection(&this->process_lock);

				okay = (encode_proc->getReturnCode() == 0);
			}
			else {
				std::wcerr << L"Failed to execute " << *cmd.cbegin() << L"\n";
			}

			delete encode_proc;
			encode_proc = nullptr;

			if (!okay) {
				this->last_filename = L"";
				goto reset;
			}
		}
		else if (action_type == Action::Type::ReplayLast) {
			if (this->last_filename.length() == 0) {
				continue;
			}
		}
		else {
			continue;
		}

		// Play
		this->window.set_status_bar_color(7, 116, 199);

		// Command
		cmd.clear();
		cmd.emplace(cmd.end(), this->exe_sox);
		cmd.emplace(cmd.end(), this->last_filename);
		cmd.emplace(cmd.end(), L"--default-device");
		for (auto it = this->voice_filters.begin(); it != this->voice_filters.end(); ++it) {
			cmd.emplace(cmd.end(), *it);
		}

		// Exec
		i = 0;
		if (this->audio_devices.size() == 0) {
			playback_proc[i] = new SubProcess(cmd);
			playback_proc[i]->pipeStdout(&null_out);
			playback_proc[i]->pipeStderr(&null_out);
			playback_proc[i]->pipeStdin(&null_in);
			playback_proc[i]->set_environment(env);
			if (playback_proc[i]->start()) ++i;
		}
		else {
			for (auto it = this->audio_devices.cbegin(); it != this->audio_devices.cend(); ++it) {
				playback_proc[i] = new SubProcess(cmd);
				playback_proc[i]->pipeStdout(&null_out);
				playback_proc[i]->pipeStderr(&null_out);
				playback_proc[i]->pipeStdin(&null_in);
				playback_proc[i]->set_environment(get_environment_clone(env, *it));
				if (playback_proc[i]->start()) {
					++i;
				}
				else {
					break;
				}
			}
		}

		if (i < playback_proc_count) {
			std::wcerr << L"Failed to execute " << *cmd.cbegin() << L"\n";
		}
		else {
			EnterCriticalSection(&this->process_lock);
			for (i = 0; i < playback_proc_count && playback_proc[i] != nullptr; ++i) {
				this->processes.push_back(playback_proc[i]);
			}
			LeaveCriticalSection(&this->process_lock);

			for (i = 0; i < playback_proc_count && playback_proc[i] != nullptr; ++i) {
				playback_proc[i]->communicate();
			}

			EnterCriticalSection(&this->process_lock);
			this->processes.clear();
			LeaveCriticalSection(&this->process_lock);
		}

		for (i = 0; i < playback_proc_count && playback_proc[i] != nullptr; ++i) {
			delete playback_proc[i];
			playback_proc[i] = nullptr;
		}

		// Done
		reset:
		this->window.set_status_bar_color(-1, 0, 0);
	}

	delete [] playback_proc;

	return 0;
}

void SoundThread :: add_audio_device(const std::wstring& audio_device) {
	this->audio_devices.emplace(this->audio_devices.end(), audio_device);
}

void SoundThread :: replay_last() {
	if (this->last_filename.length() > 0) {
		EnterCriticalSection(&this->queue_lock);

		std::queue<Action> text_queue_temp;
		while (this->text_queue.size() > 0) {
			text_queue_temp.emplace(std::move(this->text_queue.front()));
			this->text_queue.pop();
		}

		this->text_queue.emplace(Action::Type::ReplayLast, L"");

		while (text_queue_temp.size() > 0) {
			this->text_queue.emplace(std::move(text_queue_temp.front()));
			text_queue_temp.pop();
		}

		LeaveCriticalSection(&this->queue_lock);

		this->stop_playing();

		ReleaseSemaphore(this->queue_semaphore, 1, nullptr);
	}
}

void SoundThread :: stop_playing() {
	EnterCriticalSection(&this->process_lock);

	for (auto it = this->processes.begin(); it != this->processes.end(); ++it) {
		(*it)->terminate();
	}
	this->processes.erase(this->processes.begin(), this->processes.end());

	LeaveCriticalSection(&this->process_lock);
}

void SoundThread :: stop() {
	EnterCriticalSection(&this->queue_lock);

	while (this->text_queue.size() > 0) {
		this->text_queue.pop();
	}
	while (WaitForSingleObject(this->queue_semaphore, 0) == WAIT_OBJECT_0);

	this->text_queue.emplace(Action::Type::End, L"");

	LeaveCriticalSection(&this->queue_lock);
	ReleaseSemaphore(this->queue_semaphore, 1, nullptr);
}

void SoundThread :: clear_queue() {
	EnterCriticalSection(&this->queue_lock);

	while (this->text_queue.size() > 0) {
		this->text_queue.pop();
	}
	while (WaitForSingleObject(this->queue_semaphore, 0) == WAIT_OBJECT_0);

	LeaveCriticalSection(&this->queue_lock);
}

void SoundThread :: queue(const std::wstring& text) {
	EnterCriticalSection(&this->queue_lock);

	this->text_queue.emplace(Action::Type::Text, text);

	LeaveCriticalSection(&this->queue_lock);
	ReleaseSemaphore(this->queue_semaphore, 1, nullptr);
}

void SoundThread :: generate_temp_filename(std::wstring& filename, bool use_temp_dir) {
	std::wstring filename_base;
	std::wstring filename_ext;
	size_t pos = filename.rfind(L'.');
	if (pos == std::wstring::npos) {
		filename_base = filename;
	}
	else {
		filename_base = filename.substr(0, pos);
		filename_ext = filename.substr(pos, std::wstring::npos);
	}

	std::wstring temp_dir = L".\\";
	if (use_temp_dir) {
		const wchar_t* env = L"TMP";
		DWORD len = GetEnvironmentVariable(env, nullptr, 0);
		if (len == 0) {
			env = L"TEMP";
			len = GetEnvironmentVariable(env, nullptr, 0);
		}
		if (len > 0) {
			wchar_t* temp = new wchar_t[len + 1];
			if (GetEnvironmentVariable(env, temp, len + 1) > 0) {
				temp_dir = temp;
				if (temp_dir.length() > 0 && temp_dir[temp_dir.length() - 1] != L'\\') {
					temp_dir += L'\\';
				}
			}
			delete [] temp;
		}
	}

	size_t index = 0;
	std::wstringstream fn;
	fn << temp_dir << filename_base << filename_ext;
	filename = fn.str();
	while (PathFileExists(filename.c_str())) {
		++index;
		fn.str(L"");
		fn << temp_dir << filename_base << L'(' << index << L')' << filename_ext;
		filename = fn.str();
		if (index >= 100) break;
	}
}


