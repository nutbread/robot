#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <Mmsystem.h>
#include <shellapi.h>
#include <Shlwapi.h>
#include <iostream>
#include <string>
#include <cwchar>
#include <cstdlib>
#include <cctype>
#include "Window.hpp"
#include "Thread.hpp"
#include "Subprocess.hpp"
#include "SoundThread.hpp"
#include "Shared.hpp"



int main(int argc, char** argv);



int umain(wchar_t** arguments, int argument_count, HINSTANCE main_handle);



#ifdef APPLICATION_USE_WINMAIN
int WINAPI WinMain(HINSTANCE instance_handle, HINSTANCE prev_instance_handle, PSTR cmd_line, int cmd_show)
#else
int main(int argc, char** argv)
#endif
{
#ifndef APPLICATION_USE_WINMAIN
	HINSTANCE instance_handle = GetModuleHandle(nullptr);
#endif

	// Setup wchar command line
	const wchar_t* wide_cmd_line = ::GetCommandLineW();
	if (wide_cmd_line == nullptr) return -1;

	int arg_count = 0;
	wchar_t** arg_values = ::CommandLineToArgvW(wide_cmd_line, &arg_count);
	if (arg_values == nullptr) return -1;

	// Execute
	int return_code = 0;
	try {
		return_code = umain(arg_values, arg_count, instance_handle);
	}
	catch (std::bad_alloc& e) {
		std::cerr << "Out of memory" << std::endl;
		std::cerr << e.what() << std::endl;
		return_code = -3;
	}
	catch (std::exception& e) {
		std::cerr << "Generic exception" << std::endl;
		std::cerr << e.what() << std::endl;
		return_code = -2;
	}
	catch (...) {
		std::cerr << "Unknown exception" << std::endl;
		return_code = -1;
	}

	// Done
	LocalFree(arg_values);
	return return_code;
}



Window* main_window = nullptr;
BOOL WINAPI ctrl_c_handler(DWORD dwCtrlType) {
	if (main_window == nullptr) return FALSE;

	main_window->close();

	return TRUE;
}



void string_to_upper(std::wstring& str) {
	for (size_t i = 0; i < str.length(); ++i) {
		str[i] = std::toupper(str[i]);
	}
}

bool compare_strings_no_case(const std::wstring& s1, const std::wstring& s2) {
	size_t min_len = s1.length();
	if (s2.length() < min_len) min_len = s2.length();

	for (size_t i = 0; i < min_len; ++i) {
		if (std::tolower(s1[i]) < std::tolower(s2[i])) return true;
		else if (std::tolower(s1[i]) > std::tolower(s2[i])) return false;
	}

	return (s1.length() < s2.length());
}

void usage_print_keys(std::basic_ostream<wchar_t>& stream, const std::list<std::wstring>& list, size_t line_maxlen, const wchar_t* line_start) {
	size_t line_len = 0;
	for (auto it = list.cbegin(); it != list.cend(); ++it) {
		if (it != list.cbegin()) {
			line_len += 1;
			if (line_len + it->length() >= line_maxlen) {
				stream << line_start;
				line_len = 0;
			}
			else {
				stream << L" ";
			}
		}
		stream << *it;
		line_len += it->length();
	}

	stream << L"\n";
}

void usage(std::basic_ostream<wchar_t>& stream, const wchar_t* exe_name, const std::wstring& exe_cscript, const std::wstring& exe_voice) {
	size_t slash = 0;
	for (size_t i = 0; exe_name[i] != L'\x00'; ++i) {
		if (exe_name[i] == L'\\') slash = i + 1;
	}
	exe_name += slash;

	stream << L"Usage:\n" <<
		L"  " << exe_name << L" [options]\n\n\n" <<
		L"Available options:\n"
		L"  -v, --voice <voice>\n"
		L"  -r, --rate <rate>\n"
		L"  -c, --channels <channels>\n"
		L"  -s, --sample-rate <sample rate>\n"
		L"      --volume <volume>\n"
		L"  -d, --device <device>\n"
		L"  -f, --filters [filters...]\n"
		L"  -x, --left <center | x-coordinate>\n"
		L"  -y, --top <center | y-coordinate>\n"
		L"      --right <x-coordinate>\n"
		L"      --bottom <y-coordinate>\n"
		L"  -w, --width <width>\n"
		L"  -h, --height <height>\n"
		L"      --white\n"
		L"  -o, --opacity <opacity>\n"
		L"      --opacity-active <opacity>\n"
		L"  -t, --temp, --use-temp\n"
		L"      --message <message>\n"
		L"      --hotkey-focus <hotkey>\n"
		L"      --hotkey-replay <hotkey>\n"
		L"      --hotkey-stop <hotkey>\n"
		L"      --hotkey-clear <hotkey>\n"
		L"      --hotkey-exit <hotkey>\n"
		L"      --exe-cscript <exe>\n"
		L"      --exe-voice <exe>\n"
		L"      --exe-sox <exe>\n"
		L"      --dll <dll-file>\n"
		L"  -?, --help, --usage\n"
		L"      --\n\n\n"
		L"Default controls:\n"
		L"  F1 - text input\n"
		L"  F2 - replay last\n"
		L"  F3 - stop playing\n"
		L"  F3+Shift - stop playing and clear queue\n"
		L"  F4 - close\n"
		L"  Enter - say text (when focused)\n"
		L"  Enter+Shift - say text and retain focus\n"
		L"  Escape - remove focus (when focused)\n"
		L"\n\n";

	// Playback devices
	std::list<std::wstring> devices;
/*	WAVEINCAPS device_info;
	size_t device_count = waveInGetNumDevs();
	for (size_t i = 0; i < device_count; ++i) {
		if (waveInGetDevCaps(i, &device_info, sizeof(WAVEINCAPS)) == MMSYSERR_NOERROR) {
			devices.emplace(devices.end(), device_info.szPname);
		}
	}*/
	WAVEOUTCAPS device_info;
	size_t device_count = waveOutGetNumDevs();
	for (size_t i = 0; i < device_count; ++i) {
		if (waveOutGetDevCaps(i, &device_info, sizeof(WAVEOUTCAPS)) == MMSYSERR_NOERROR) {
			devices.emplace(devices.end(), device_info.szPname);
		}
	}

	devices.sort(compare_strings_no_case);

	stream << L"Available devices:\n";
	for (auto it = devices.begin(); it != devices.end(); ++it) {
		stream << L"  " << *it << L"\n";
	}

	// Voices
	std::stringstream out_stream;
	SubProcess::NullOStream null_out;
	SubProcess::NullIStream null_in;
	std::list<std::wstring> cmd;
	cmd.emplace(cmd.end(), exe_cscript);
	cmd.emplace(cmd.end(), L"//Nologo");
	cmd.emplace(cmd.end(), exe_voice);
	cmd.emplace(cmd.end(), L"list-voices");

	SubProcess p(cmd);
	p.pipeStdout(&out_stream);
	p.pipeStderr(&null_out);
	p.pipeStdin(&null_in);
	p.start();
	p.communicate();
	p.join();

	stream << L"\n\nAvailable voices:\n";
	std::string line;
	while (!out_stream.eof()) {
		std::getline(out_stream, line);
		if (line.length() > 0) {
			std::wcout << L"  " << line.c_str() << L"\n";
		}
	}

	// Key codes
	stream << L"\n\nHotkeys:\n"
		L"  Hotkeys are formed by using a key name and any\n"
		L"  combination of modifiers, separated by spaces.\n\n"
		L"  Note that the \"block\" modifier makes the hotkey not\n"
		L"  pass any input to other applications. If omitted, the\n"
		L"  key will trigger a keypress as usual.\n\n"
		L"  The following keys are available (some are repeats):\n    ";

	size_t line_maxlen = 72;
	std::list<std::wstring> key_names;
	Window::get_key_names(key_names);
	usage_print_keys(stream, key_names, line_maxlen, L"\n    ");

	stream << L"\n"
		L"  The following modifiers are available (some are repeats):\n    ";

	key_names.clear();
	Window::get_key_modifiers(key_names);
	usage_print_keys(stream, key_names, line_maxlen, L"\n    ");

}



double str_to_double(const wchar_t* str) {
	try {
		return std::stod(str, nullptr);
	}
	catch (...) {}
	return 0.0;
}
int str_to_int(const wchar_t* str) {
	try {
		return std::stoi(str, nullptr, 10);
	}
	catch (...) {}
	return 0;
}

template <typename T>
T clamp(T t, T t_min, T t_max) {
	if (t < t_min) return t_min;
	if (t > t_max) return t_max;
	return t;
}



int umain(wchar_t** arguments, int argument_count, HINSTANCE instance_handle) {
	// Ctrl+C
	SetConsoleCtrlHandler(ctrl_c_handler, TRUE);



	// Default settings
	std::wstring voice;
	int voice_volume = 100;
	int voice_rate = 0;
	int voice_channels = 2;
	int voice_sample_rate = 44100;
	bool use_temp = false;
	std::list<std::wstring> voice_filters;
	std::list<std::wstring> voice_devices;
	std::wstring exe_cscript = L"cscript";
	std::wstring exe_voice = L"voice.vbs";
	std::wstring exe_sox = L"sox";
	std::wstring dll_name = WSTRING(DLL_NAME);
	bool show_usage = false;
	Window::InitPosition window_pos;
	bool window_white = false;
	double window_opacity = 0.5;
	double window_opacity_active = 1.0;
	std::wstring window_title = L"Robot";
	std::wstringstream window_message;
	bool window_message_has = false;
	std::wstring hotkey_focus = L"F1";
	std::wstring hotkey_replay = L"F2";
	std::wstring hotkey_stop = L"F3";
	std::wstring hotkey_clear = L"F3 shift";
	std::wstring hotkey_exit = L"F4";



	// Console title
	SetConsoleTitle(window_title.c_str());



	// Command line
	int i = 1;
	for (; i < argument_count; ++i) {
		const wchar_t* arg = arguments[i];

		if (wcscmp(arg, L"-v") == 0 || wcscmp(arg, L"--voice") == 0) {
			if (++i < argument_count) {
				voice = arguments[i];
			}
		}
		else if (wcscmp(arg, L"-d") == 0 || wcscmp(arg, L"--device") == 0) {
			if (++i < argument_count) {
				voice_devices.emplace(voice_devices.end(), arguments[i]);
			}
		}
		else if (wcscmp(arg, L"--exe-cscript") == 0) {
			if (++i < argument_count) {
				exe_cscript = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--exe-voice") == 0) {
			if (++i < argument_count) {
				exe_voice = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--exe-sox") == 0) {
			if (++i < argument_count) {
				exe_sox = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--dll") == 0) {
			if (++i < argument_count) {
				dll_name = arguments[i];
			}
		}
		else if (wcscmp(arg, L"-r") == 0 || wcscmp(arg, L"--rate") == 0) {
			if (++i < argument_count) {
				voice_rate = clamp(str_to_int(arguments[i]), -10, 10);
			}
		}
		else if (wcscmp(arg, L"-s") == 0 || wcscmp(arg, L"--sample-rate") == 0) {
			if (++i < argument_count) {
				voice_sample_rate = clamp(str_to_int(arguments[i]), 8000, 48000);
			}
		}
		else if (wcscmp(arg, L"-c") == 0 || wcscmp(arg, L"--channels") == 0) {
			if (++i < argument_count) {
				voice_channels = clamp(str_to_int(arguments[i]), 1, 2);
			}
		}
		else if (wcscmp(arg, L"--volume") == 0) {
			if (++i < argument_count) {
				voice_volume = clamp(str_to_int(arguments[i]), 0, 100);
			}
		}
		else if (wcscmp(arg, L"-t") == 0 || wcscmp(arg, L"--temp") == 0 || wcscmp(arg, L"--use-temp") == 0) {
			use_temp = true;
		}
		else if (wcscmp(arg, L"-f") == 0 || wcscmp(arg, L"--filters") == 0) {
			while (++i < argument_count) {
				voice_filters.emplace(voice_filters.cend(), arguments[i]);
			}
			break;
		}
		else if (wcscmp(arg, L"--white") == 0) {
			window_white = true;
		}
		else if (wcscmp(arg, L"-x") == 0 || wcscmp(arg, L"--left") == 0) {
			if (++i < argument_count) {
				if (wcscmp(arguments[i], L"center") == 0) {
					window_pos.set_center_horizontal(true);
				}
				else {
					window_pos.set_left(str_to_int(arguments[i]));
				}
			}
		}
		else if (wcscmp(arg, L"-y") == 0 || wcscmp(arg, L"--top") == 0) {
			if (++i < argument_count) {
				if (wcscmp(arguments[i], L"center") == 0) {
					window_pos.set_center_vertical(true);
				}
				else {
					window_pos.set_top(str_to_int(arguments[i]));
				}
			}
		}
		else if (wcscmp(arg, L"--right") == 0) {
			if (++i < argument_count) {
				window_pos.set_right(str_to_int(arguments[i]));
			}
		}
		else if (wcscmp(arg, L"--bottom") == 0) {
			if (++i < argument_count) {
				window_pos.set_bottom(str_to_int(arguments[i]));
			}
		}
		else if (wcscmp(arg, L"-w") == 0 || wcscmp(arg, L"--width") == 0) {
			if (++i < argument_count) {
				window_pos.set_width(str_to_int(arguments[i]));
			}
		}
		else if (wcscmp(arg, L"-h") == 0 || wcscmp(arg, L"--height") == 0) {
			if (++i < argument_count) {
				window_pos.set_height(str_to_int(arguments[i]));
			}
		}
		else if (wcscmp(arg, L"-o") == 0 || wcscmp(arg, L"--opacity") == 0) {
			if (++i < argument_count) {
				window_opacity = clamp(str_to_double(arguments[i]), 0.0, 1.0);
			}
		}
		else if (wcscmp(arg, L"--opacity-active") == 0) {
			if (++i < argument_count) {
				window_opacity_active = clamp(str_to_double(arguments[i]), 0.0, 1.0);
			}
		}
		else if (wcscmp(arg, L"--message") == 0) {
			if (++i < argument_count) {
				window_message.str(L"");
				window_message << arguments[i];
				window_message_has = true;
			}
		}
		else if (wcscmp(arg, L"--hotkey-focus") == 0) {
			if (++i < argument_count) {
				hotkey_focus = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--hotkey-replay") == 0) {
			if (++i < argument_count) {
				hotkey_replay = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--hotkey-stop") == 0) {
			if (++i < argument_count) {
				hotkey_stop = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--hotkey-clear") == 0) {
			if (++i < argument_count) {
				hotkey_clear = arguments[i];
			}
		}
		else if (wcscmp(arg, L"--hotkey-exit") == 0) {
			if (++i < argument_count) {
				hotkey_exit = arguments[i];
			}
		}
		else if (wcscmp(arg, L"-?") == 0 || wcscmp(arg, L"--help") == 0 || wcscmp(arg, L"--usage") == 0) {
			show_usage = true;
		}
		else if (wcscmp(arg, L"--") == 0) {
			// No-op
		}
		else {
			std::wcerr << L"Unknown option " << arg << L"\n";
		}
	}
	if (i > argument_count) {
		std::wcerr << L"Missing option value for " << arguments[argument_count - 1] << L"\n";
	}



	// Usage
	if (show_usage) {
		usage(std::wcout, arguments[0], exe_cscript, exe_voice);
		return 0;
	}



	// Load dll
	HINSTANCE robot_lib = Window::setup_dll(dll_name.c_str());
	if (robot_lib == nullptr) {
		std::wcerr << L"Could not load " << dll_name << L"\n";
		return -1;
	}


	// Create window
	Window w;
	main_window = &w;



	// Hotkey setup
	int hotkey_focus_key = VK_F1;
	int hotkey_focus_mod = 0;
	int hotkey_replay_key = VK_F2;
	int hotkey_replay_mod = 0;
	int hotkey_stop_key = VK_F3;
	int hotkey_stop_mod = 0;
	int hotkey_clear_key = VK_F3;
	int hotkey_clear_mod = MOD_SHIFT;
	int hotkey_exit_key = VK_F4;
	int hotkey_exit_mod = 0;

	std::wstring key1 = hotkey_focus;
	std::wstring key2 = hotkey_exit;

	if (!w.get_key_info(hotkey_focus, hotkey_focus_key, hotkey_focus_mod, &key1)) {
		std::wcerr << L"Focus hotkey was invalid\n";
	}
	if (!w.get_key_info(hotkey_replay, hotkey_replay_key, hotkey_replay_mod, nullptr)) {
		std::wcerr << L"Replay hotkey was invalid\n";
	}
	if (!w.get_key_info(hotkey_stop, hotkey_stop_key, hotkey_stop_mod, nullptr)) {
		std::wcerr << L"Stop hotkey was invalid\n";
	}
	if (!w.get_key_info(hotkey_clear, hotkey_clear_key, hotkey_clear_mod, nullptr)) {
		std::wcerr << L"Clear hotkey was invalid\n";
	}
	if (!w.get_key_info(hotkey_exit, hotkey_exit_key, hotkey_exit_mod, &key2)) {
		std::wcerr << L"Exit hotkey was invalid\n";
	}

	if (!window_message_has) {
		string_to_upper(key1);
		string_to_upper(key2);
		window_message << key1 << L" for focus; " << key2 << " to close";
	}

	w.set_strings(window_title, window_message.str());


	// Execute
	SoundThread st(voice, voice_volume, voice_rate, voice_channels, voice_sample_rate, voice_filters, use_temp, w, exe_cscript, exe_voice, exe_sox);

	for (auto it = voice_devices.cbegin(); it != voice_devices.cend(); ++it) {
		st.add_audio_device(*it);
	}
	voice_devices.clear();

	st.start();

	w.set_settings(window_white, window_opacity, window_opacity_active);
	if (w.init(instance_handle, robot_lib, window_pos)) {
		w.add_text_input_listener([&st](Window& w, const std::wstring& text, int hotkey_mod) {
			st.queue(text);
			if ((hotkey_mod & MOD_SHIFT) == 0) {
				w.set_focus(false);
			}
		});
		w.register_hotkey(VK_ESCAPE, 0, [](Window& w) {
			if (w.has_focus()) {
				w.set_focus(false);
			}
		});
		w.register_hotkey(hotkey_focus_key, hotkey_focus_mod, [](Window& w) {
			w.set_focus(!w.has_focus());
		});
		w.register_hotkey(hotkey_replay_key, hotkey_replay_mod, [&st](Window& w) {
			st.replay_last();
		});
		w.register_hotkey(hotkey_stop_key, hotkey_stop_mod, [&st](Window& w) {
			st.stop_playing();
		});
		w.register_hotkey(hotkey_clear_key, hotkey_clear_mod, [&st](Window& w) {
			st.clear_queue();
			st.stop_playing();
		});
		w.register_hotkey(hotkey_exit_key, hotkey_exit_mod, [](Window& w) {
			w.close();
		});

		w.message_loop();
	}

	w.destroy();
	main_window = nullptr;

	st.stop();
	st.stop_playing();
	st.join();


	return 0;
}


