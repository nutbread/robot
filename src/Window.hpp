#ifndef ___H_WINDOW
#define ___H_WINDOW

#define WIN32_LEAN_AND_MEAN
#define VC_EXTRALEAN
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <functional>
#include <string>
#include <list>
#include <unordered_map>



#define MOD_BLOCKING 0x0010



class Window final {
private:
	typedef bool (*hook_input_fn)(HINSTANCE instance_handle, HWND main_window);
	typedef bool (*unhook_input_fn)();
	static hook_input_fn hook_input;
	static unhook_input_fn unhook_input;

	HMONITOR monitor;
	RECT monitor_rect;
	RECT window_rect;
	DWORD window_style;
	DWORD window_style_extended;
	ATOM wc_atom;
	HINSTANCE instance_handle;
	HINSTANCE lib_handle;
	HWND window_handle;
	HFONT default_font;
	bool active;
	bool focused;

	HWND text_input;
	HWND text_label;
	HWND status_bar;
	HBRUSH status_bar_bg;

	HWINEVENTHOOK active_window_switch_hook;
	static Window* on_active_window_change_obj;

	int font_color;
	int bg_color;
	HBRUSH font_color_brush;
	HBRUSH bg_color_brush;
	double opacity;
	double opacity_active;

	typedef std::function<LRESULT(Window*, bool&, HWND, UINT, WPARAM, LPARAM)> WindowProcReplacement;
	struct WindowProcInfo {
		Window* self;
		WNDPROC old_proc;
		WindowProcReplacement new_proc;
	};

public:
	typedef std::function<void(Window&)> HotkeyCallback;
	struct HotkeyEntry final {
	private:
		friend class Window;

		int mod;
		HotkeyCallback callback;

		HotkeyEntry(int mod, HotkeyCallback callback) :
			mod(mod),
			callback(callback)
		{}
		~HotkeyEntry() {}
	};
	typedef std::list<HotkeyEntry*> HotkeyCallbackList;

	class InitPosition final {
	private:
		friend class Window;
		enum : int {
			LeftSet = 0x1,
			TopSet = 0x2,
			BottomSet = 0x4,
			RightSet = 0x8,
			WidthSet = 0x10,
			HeightSet = 0x20,
			CenterH = 0x40,
			CenterV = 0x80,
		};

		int flags;
		int left;
		int top;
		int right;
		int bottom;
		int width;
		int height;

	public:
		InitPosition();
		~InitPosition();

		void set_center_horizontal(bool enabled);
		void set_center_vertical(bool enabled);
		void set_left(int x);
		void set_top(int y);
		void set_right(int x);
		void set_bottom(int y);
		void set_width(int width);
		void set_height(int height);

	};

private:
	std::unordered_map<std::string, int> key_code_mapping;
	std::unordered_map<std::string, int> key_mod_mapping;
	std::unordered_map<int, HotkeyCallbackList> hotkeys;
	int hotkey_mod;
	int hotkey_id;

	typedef std::function<void(Window&, const std::wstring&, int)> TextInputCallback;
	std::list<TextInputCallback> text_input_listeners;

	std::wstring window_title;
	std::wstring window_message;


	static
	LRESULT CALLBACK
	window_proc(
		HWND window_handle,
		UINT msg,
		WPARAM w_param,
		LPARAM l_param
	);

	void window_proc_handle_input(
		UINT msg,
		int key
	);

public:
	Window();
	~Window();

	void set_settings(bool window_white, double window_opacity, double window_opacity_active);

	bool init(HINSTANCE instance_handle, HINSTANCE lib_handle, InitPosition& pos);

	bool destroy();

	void message_loop();

	void deactivate();

	void set_opacity(double opacity);

	void set_status_bar_color(int r, int g, int b);

	HotkeyEntry* register_hotkey(int key, int mod, HotkeyCallback callback);

	bool unregister_hotkey(int key, HotkeyEntry* entry);

	void set_focus(bool focused);

	bool has_focus() const;

	void add_text_input_listener(TextInputCallback callback);

	static HINSTANCE setup_dll(const wchar_t* filename);

	void close();

	bool get_key_info(const std::wstring& str, int& key, int& mod, std::wstring* key_name) const;

	void set_strings(
		const std::wstring& window_title,
		const std::wstring& window_message
	);

	static void get_key_names(std::list<std::wstring>& list);
	static void get_key_modifiers(std::list<std::wstring>& list);

private:
	bool setup_gui();

	bool trigger_hotkey(int key);

	void clear_hotkeys();

	static bool hwnd_is_textbox(HWND window_handle);

	static bool char_array_icompare(const wchar_t* a1, const wchar_t* a2, size_t length);

	void replace_control_proc(HWND window_handle, WindowProcReplacement new_proc);

	void set_keyboard_input_blocked(bool blocked);

	static
	LRESULT CALLBACK
	window_proc_replacement(
		HWND window_handle,
		UINT msg,
		WPARAM w_param,
		LPARAM l_param
	);

	void on_text_input_submit();

	void clear_listeners();

	static VOID CALLBACK on_active_window_change_callback(
		HWINEVENTHOOK win_event_hook,
		DWORD event,
		HWND hwnd,
		LONG object,
		LONG child,
		DWORD event_thread,
		DWORD event_time
	);

	void on_active_window_change();

	void setup_key_code_mapping();

	void make_singleline_string(std::wstring& text);
	
};



#endif // ___H_WINDOW


