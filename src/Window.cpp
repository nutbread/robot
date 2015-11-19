#include <cctype>
#include <sstream>
#include "Window.hpp"
#include "Resources.h"



#define WM_HOOK (WM_USER + 0x0001)
#define WM_STATUS_COLOR (WM_USER + 0x0002)
#define MOD_ALL_VALID (MOD_SHIFT | MOD_CONTROL | MOD_ALT | MOD_WIN)

Window::hook_input_fn Window::hook_input = nullptr;
Window::unhook_input_fn Window::unhook_input = nullptr;
Window* Window::on_active_window_change_obj = nullptr;



struct KeyName {
	const char* name;
	int vk_code;
};
struct KeyModifier {
	const char* name;
	int modifier;
};

const KeyName key_names[] = {
	{ "backspace", VK_BACK },
	{ "tab", VK_TAB },
	{ "clear", VK_CLEAR },
	{ "return", VK_RETURN },
	{ "enter", VK_RETURN },
	{ "pause", VK_PAUSE },
	{ "capslock", VK_CAPITAL },
	{ "caps", VK_CAPITAL },
	{ "escape", VK_ESCAPE },
	{ "esc", VK_ESCAPE },
	{ "spacebar", VK_SPACE },
	{ "space", VK_SPACE },
	{ "pageup", VK_PRIOR },
	{ "pagedown", VK_NEXT },
	{ "pagedn", VK_NEXT },
	{ "end", VK_NEXT },
	{ "home", VK_NEXT },
	{ "left", VK_LEFT },
	{ "up", VK_UP },
	{ "right", VK_RIGHT },
	{ "down", VK_DOWN },
	{ "select", VK_SELECT },
	{ "print", VK_PRINT },
	{ "execute", VK_EXECUTE },
	{ "printscreen", VK_SNAPSHOT },
	{ "insert", VK_INSERT },
	{ "ins", VK_INSERT },
	{ "delete", VK_DELETE },
	{ "del", VK_DELETE },
	{ "help", VK_HELP },
	{ "0", '0' },
	{ "1", '1' },
	{ "2", '2' },
	{ "3", '3' },
	{ "4", '4' },
	{ "5", '5' },
	{ "6", '6' },
	{ "7", '7' },
	{ "8", '8' },
	{ "9", '9' },
	{ "a", 'A' },
	{ "b", 'B' },
	{ "c", 'C' },
	{ "d", 'D' },
	{ "e", 'E' },
	{ "f", 'F' },
	{ "g", 'G' },
	{ "h", 'H' },
	{ "i", 'I' },
	{ "j", 'J' },
	{ "k", 'K' },
	{ "l", 'L' },
	{ "m", 'M' },
	{ "n", 'N' },
	{ "o", 'O' },
	{ "p", 'P' },
	{ "q", 'Q' },
	{ "r", 'R' },
	{ "s", 'S' },
	{ "t", 'T' },
	{ "u", 'U' },
	{ "v", 'V' },
	{ "w", 'W' },
	{ "x", 'X' },
	{ "y", 'Y' },
	{ "z", 'Z' },
	{ "sleep", VK_SLEEP },
	{ "numpad0", VK_NUMPAD0 },
	{ "num0", VK_NUMPAD0 },
	{ "numpad1", VK_NUMPAD1 },
	{ "num1", VK_NUMPAD1 },
	{ "numpad2", VK_NUMPAD2 },
	{ "num2", VK_NUMPAD2 },
	{ "numpad3", VK_NUMPAD3 },
	{ "num3", VK_NUMPAD3 },
	{ "numpad4", VK_NUMPAD4 },
	{ "num4", VK_NUMPAD4 },
	{ "numpad5", VK_NUMPAD5 },
	{ "num5", VK_NUMPAD5 },
	{ "numpad6", VK_NUMPAD6 },
	{ "num6", VK_NUMPAD6 },
	{ "numpad7", VK_NUMPAD7 },
	{ "num7", VK_NUMPAD7 },
	{ "numpad8", VK_NUMPAD8 },
	{ "num8", VK_NUMPAD8 },
	{ "numpad9", VK_NUMPAD9 },
	{ "num9", VK_NUMPAD9 },
	{ "numpad*", VK_MULTIPLY },
	{ "num*", VK_MULTIPLY },
	{ "multiply", VK_MULTIPLY },
	{ "numpad+", VK_ADD },
	{ "num+", VK_ADD },
	{ "add", VK_ADD },
	{ "numpad,", VK_SEPARATOR },
	{ "num,", VK_SEPARATOR },
	{ "separator", VK_SEPARATOR },
	{ "numpad-", VK_SUBTRACT },
	{ "num-", VK_SUBTRACT },
	{ "subtract", VK_SUBTRACT },
	{ "numpad.", VK_DECIMAL },
	{ "num.", VK_DECIMAL },
	{ "decimal", VK_DECIMAL },
	{ "dot", VK_DECIMAL },
	{ "numpad/", VK_DIVIDE },
	{ "num/", VK_DIVIDE },
	{ "divide", VK_DIVIDE },
	{ "f1", VK_F1 },
	{ "f2", VK_F2 },
	{ "f3", VK_F3 },
	{ "f4", VK_F4 },
	{ "f5", VK_F5 },
	{ "f6", VK_F6 },
	{ "f7", VK_F7 },
	{ "f8", VK_F8 },
	{ "f9", VK_F9 },
	{ "f10", VK_F10 },
	{ "f11", VK_F11 },
	{ "f12", VK_F12 },
	{ "f13", VK_F13 },
	{ "f14", VK_F14 },
	{ "f15", VK_F15 },
	{ "f16", VK_F16 },
	{ "f17", VK_F17 },
	{ "f18", VK_F18 },
	{ "f19", VK_F19 },
	{ "f20", VK_F20 },
	{ "f21", VK_F21 },
	{ "f22", VK_F22 },
	{ "f23", VK_F23 },
	{ "f24", VK_F24 },
	{ "numlock", VK_NUMLOCK },
	{ "scrolllock", VK_SCROLL },
	{ "browserback", VK_BROWSER_BACK },
	{ "browserforward", VK_BROWSER_FORWARD },
	{ "browserrefresh", VK_BROWSER_REFRESH },
	{ "browserstop", VK_BROWSER_STOP },
	{ "browsersearch", VK_BROWSER_SEARCH },
	{ "browserfavorites", VK_BROWSER_FAVORITES },
	{ "browserhome", VK_BROWSER_HOME },
	{ "volumemute", VK_VOLUME_MUTE },
	{ "volumedown", VK_VOLUME_DOWN },
	{ "volumeup", VK_VOLUME_UP },
	{ "medianext", VK_MEDIA_NEXT_TRACK },
	{ "mediaprevious", VK_MEDIA_PREV_TRACK },
	{ "mediaprev", VK_MEDIA_PREV_TRACK },
	{ "mediastop", VK_MEDIA_STOP },
	{ "mediaplay", VK_MEDIA_PLAY_PAUSE },
	{ "launchmail", VK_LAUNCH_MAIL },
	{ "launchmediaselect", VK_LAUNCH_MEDIA_SELECT },
	{ "launchapp1", VK_LAUNCH_APP1 },
	{ "launchapp2", VK_LAUNCH_APP2 },
	{ ";", VK_OEM_1 },
	{ ":", VK_OEM_1 },
	{ "+", VK_OEM_PLUS },
	{ "=", VK_OEM_PLUS },
	{ ",", VK_OEM_COMMA },
	{ "<", VK_OEM_COMMA },
	{ "-", VK_OEM_MINUS },
	{ "_", VK_OEM_MINUS },
	{ ".", VK_OEM_PERIOD },
	{ ">", VK_OEM_PERIOD },
	{ "/", VK_OEM_2 },
	{ "?", VK_OEM_2 },
	{ "`", VK_OEM_3 },
	{ "~", VK_OEM_3 },
	{ "[", VK_OEM_4 },
	{ "{", VK_OEM_4 },
	{ "\\", VK_OEM_5 },
	{ "|", VK_OEM_5 },
	{ "]", VK_OEM_6 },
	{ "}", VK_OEM_6 },
	{ "\"", VK_OEM_7 },
	{ "'", VK_OEM_7 },
	{ "attn", VK_ATTN },
	{ "crsel", VK_CRSEL },
	{ "exsel", VK_EXSEL },
	{ "eraseeof", VK_EREOF },
	{ "play", VK_PLAY },
	{ "zoom", VK_ZOOM },
	{ nullptr, 0 },
};

const KeyModifier key_modifiers[] = {
	{ "shift", MOD_SHIFT },
	{ "control", MOD_CONTROL },
	{ "ctrl", MOD_CONTROL },
	{ "alt", MOD_ALT },
	{ "windows", MOD_WIN },
	{ "win", MOD_WIN },
	{ "block", MOD_BLOCKING },
	{ nullptr, 0 },
};



Window::InitPosition :: InitPosition() :
	flags(0),
	left(0),
	top(0),
	right(0),
	bottom(0),
	width(0),
	height(0)
{}
Window::InitPosition :: ~InitPosition() {}

void Window::InitPosition :: set_center_horizontal(bool enabled) {
	if (enabled) this->flags |= Window::InitPosition::CenterH;
	else this->flags &= ~Window::InitPosition::CenterH;
}
void Window::InitPosition :: set_center_vertical(bool enabled) {
	if (enabled) this->flags |= Window::InitPosition::CenterV;
	else this->flags &= ~Window::InitPosition::CenterV;
}
void Window::InitPosition :: set_left(int x) {
	this->left = x;
	this->flags |= Window::InitPosition::LeftSet;
}
void Window::InitPosition :: set_top(int y) {
	this->top = y;
	this->flags |= Window::InitPosition::TopSet;
}
void Window::InitPosition :: set_right(int x) {
	this->right = x;
	this->flags |= Window::InitPosition::RightSet;
}
void Window::InitPosition :: set_bottom(int y) {
	this->bottom = y;
	this->flags |= Window::InitPosition::BottomSet;
}
void Window::InitPosition :: set_width(int width) {
	this->width = width;
	this->flags |= Window::InitPosition::WidthSet;
}
void Window::InitPosition :: set_height(int height) {
	this->height = height;
	this->flags |= Window::InitPosition::HeightSet;
}



LRESULT CALLBACK
Window :: window_proc(
	HWND window_handle,
	UINT msg,
	WPARAM w_param,
	LPARAM l_param
) {
	Window* self = reinterpret_cast<Window*>(GetWindowLongPtr(window_handle, GWLP_USERDATA));

	// Custom handlers
	switch (msg) {
		case WM_CLOSE:
		{
			self->active = false;
		}
		break;
		case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		break;
		case WM_SYSCOMMAND:
		{
			WPARAM w_param2 = LOWORD(w_param);
			switch (w_param2) {
				case SC_CLOSE:
				{
					return 0;
				}
				break;
				case SC_CONTEXTHELP:
				{
				}
				break;
				case SC_DEFAULT:
				{
				}
				break;
				case SC_HOTKEY:
				{
				}
				break;
				case SC_HSCROLL:
				{
				}
				break;
				case SC_KEYMENU:
				{
					// Alt key
					if ((l_param >> 16) <= 0) {
						// Block alt key menu
						return 0;
					}
				}
				break;
				case SC_MAXIMIZE:
				{
				}
				break;
				case SC_MINIMIZE:
				{
				}
				break;
				case SC_MONITORPOWER:
				{
					// block power saving
					return 0;
				}
				break;
				case SC_MOUSEMENU:
				{
					// block right icon menu
					return 0;
				}
				break;
				case SC_MOVE:
				{
				}
				break;
				case SC_NEXTWINDOW:
				{
				}
				break;
				case SC_PREVWINDOW:
				{
				}
				break;
				case SC_RESTORE:
				{
				}
				break;
				case SC_SCREENSAVE:
				{
				}
				break;
				case SC_SIZE:
				{
				}
				break;
				case SC_TASKLIST:
				{
				}
				break;
				case SC_VSCROLL:
				{
				}
				break;
				default:
				{
				}
				break;
			}
		}
		break;
		case WM_NCCREATE:
		{
			return TRUE;
		}
		break;
		case WM_CREATE:
		{
			// Move the custom data to the right place
			SetWindowLongPtr(window_handle, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCT*>(l_param)->lpCreateParams));
			return 0;
		}
		break;
		case WM_NCRBUTTONDOWN:
		{
			// block right-click on title bar
			return 0;
		}
		break;
		case WM_DISPLAYCHANGE:
		{
			// A display may have been added or removed;
		}
		break;
		case WM_CTLCOLOREDIT:
		{
			HDC hdc = (HDC) w_param;

			if ((HWND) l_param == self->text_input) {
				SetTextColor(hdc, self->font_color);
				SetBkColor(hdc, self->bg_color);
				return (LRESULT) self->bg_color_brush; // GetStockObject(NULL_BRUSH);
			}
		}
		break;
		case WM_CTLCOLORSTATIC:
		{
			HDC hdc = (HDC) w_param;

			if ((HWND) l_param == self->text_label) {
				SetTextColor(hdc, self->font_color);
				SetBkColor(hdc, self->bg_color);
				return (LRESULT) self->bg_color_brush; // GetStockObject(NULL_BRUSH);
			}
			else if ((HWND) l_param == self->status_bar) {
				SetTextColor(hdc, self->bg_color);
				SetBkColor(hdc, self->bg_color);
				if (self->status_bar_bg == nullptr) {
					return (LRESULT) self->bg_color_brush; // GetStockObject(NULL_BRUSH);
				}
				else {
					return (LRESULT) self->status_bar_bg;
				}
			}
		}
		break;
		case WM_ACTIVATEAPP:
		{
		}
		break;
		case WM_HOTKEY:
		{
			//int mod = LOWORD(l_param);
			int key = HIWORD(l_param);
			self->window_proc_handle_input(WM_KEYDOWN, key);
			return 0;
		}
		break;
		case WM_INPUT:
		{
			static BYTE lpb[40];
			UINT dwSize = sizeof(lpb);

			GetRawInputData((HRAWINPUT) l_param, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER));

			RAWINPUT* raw = ((RAWINPUT*) lpb);

			if (raw->header.dwType == RIM_TYPEKEYBOARD) {
				self->window_proc_handle_input(raw->data.keyboard.Message, raw->data.keyboard.VKey);
			}
		}
		break;
		case WM_HOOK:
		{
			// Process
			KBDLLHOOKSTRUCT* kb = (KBDLLHOOKSTRUCT*) l_param;
			self->window_proc_handle_input(static_cast<UINT>(w_param), kb->vkCode);

			// Block
			return TRUE;
		}
		break;
		case WM_STATUS_COLOR:
		{
			if (self->status_bar_bg != nullptr) {
				DeleteObject(self->status_bar_bg);
			}

			self->status_bar_bg = CreateSolidBrush(static_cast<COLORREF>(w_param));

			SendMessage(self->window_handle, WM_CTLCOLORSTATIC, (WPARAM) GetDC(self->status_bar), (LPARAM) self->status_bar);
			InvalidateRect(self->status_bar, nullptr, TRUE);
			return 0;
		}
		break;
	}

	// Default handler
	return DefWindowProc(window_handle, msg, w_param, l_param);
}

void Window :: window_proc_handle_input(UINT msg, int key) {
	if (msg == WM_KEYDOWN) {
		bool allow_forward = true;
		switch (key) {
			case VK_SHIFT:
			case VK_LSHIFT:
			case VK_RSHIFT:
				this->hotkey_mod |= MOD_SHIFT;
			break;
			case VK_CONTROL:
			case VK_LCONTROL:
			case VK_RCONTROL:
				this->hotkey_mod |= MOD_CONTROL;
			break;
			case VK_MENU:
			case VK_LMENU:
			case VK_RMENU:
				this->hotkey_mod |= MOD_ALT;
			break;
			case VK_LWIN:
			case VK_RWIN:
				this->hotkey_mod |= MOD_WIN;
			break;
			default:
				allow_forward = !this->trigger_hotkey(key);
			break;
		}

		if (this->focused && allow_forward) {
			unsigned int flags = 0;
			SetFocus(this->text_input);
			PostMessage(this->text_input, msg, (WPARAM) key, (LPARAM) flags);
		}
	}
	else if (msg == WM_KEYUP) {
		switch (key) {
			case VK_SHIFT:
			case VK_LSHIFT:
			case VK_RSHIFT:
				this->hotkey_mod &= ~MOD_SHIFT;
			break;
			case VK_CONTROL:
			case VK_LCONTROL:
			case VK_RCONTROL:
				this->hotkey_mod &= ~MOD_CONTROL;
			break;
			case VK_MENU:
			case VK_LMENU:
			case VK_RMENU:
				this->hotkey_mod &= ~MOD_ALT;
			break;
			case VK_LWIN:
			case VK_RWIN:
				this->hotkey_mod &= ~MOD_WIN;
			break;
		}
	}
}



Window :: Window() :
	monitor(nullptr),
	monitor_rect{ 0, 0, 0, 0 },
	window_rect{ 0, 0, 0, 0 },
	window_style(WS_POPUP | WS_VISIBLE | WS_SYSMENU | WS_CLIPCHILDREN | WS_CLIPSIBLINGS),
	window_style_extended(WS_EX_LAYERED | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW),
	wc_atom(),
	instance_handle(nullptr),
	lib_handle(nullptr),
	window_handle(nullptr),
	default_font(nullptr),
	active(false),
	focused(false),
	text_input(nullptr),
	text_label(nullptr),
	status_bar(nullptr),
	status_bar_bg(nullptr),
	active_window_switch_hook(nullptr),
	font_color(RGB(255,255,255)),
	bg_color(RGB(0,0,0)),
	font_color_brush(nullptr),
	bg_color_brush(nullptr),
	opacity(0.5),
	opacity_active(1.0),
	key_code_mapping(),
	key_mod_mapping(),
	hotkeys(),
	hotkey_mod(0),
	hotkey_id(1),
	text_input_listeners(),
	window_title(L""),
	window_message(L"")
{
	this->font_color_brush = CreateSolidBrush(this->font_color);
	this->bg_color_brush = CreateSolidBrush(this->bg_color);
	this->setup_key_code_mapping();
}
Window :: ~Window() {
	this->destroy();
}

void Window :: set_settings(bool window_white, double window_opacity, double window_opacity_active) {
	if (window_white) {
		font_color = RGB(0,0,0);
		bg_color = RGB(255,255,255);
	}
	else {
		font_color = RGB(255,255,255);
		bg_color = RGB(0,0,0);
	}

	DeleteObject(this->font_color_brush);
	DeleteObject(this->bg_color_brush);
	this->font_color_brush = CreateSolidBrush(this->font_color);
	this->bg_color_brush = CreateSolidBrush(this->bg_color);

	this->opacity = window_opacity;
	this->opacity_active = window_opacity_active;
}

bool Window :: init(HINSTANCE instance_handle, HINSTANCE lib_handle, InitPosition& pos) {
	// Default monitor
	this->monitor = MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO monitor_info;
	monitor_info.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(monitor, &monitor_info)) {
		this->monitor_rect = monitor_info.rcMonitor;
	}

	// Default settings
	HMENU menu_handle = nullptr;

	this->window_rect.left = 0;
	this->window_rect.top = 80;
	this->window_rect.right = 180; // width
	this->window_rect.bottom = 44; // height

	if ((pos.flags & InitPosition::WidthSet) != 0) this->window_rect.right = pos.width;
	if ((pos.flags & InitPosition::HeightSet) != 0) this->window_rect.bottom = pos.height;

	if ((pos.flags & InitPosition::LeftSet) != 0) {
		this->window_rect.left = pos.left;
		if ((pos.flags & InitPosition::RightSet) != 0) this->window_rect.right = pos.right - pos.left;
	}
	else if ((pos.flags & InitPosition::RightSet) != 0) {
		this->window_rect.left = (monitor_rect.right - monitor_rect.left) - pos.right - this->window_rect.right;
	}
	else {
		pos.flags |= InitPosition::CenterH;
	}

	if ((pos.flags & InitPosition::TopSet) != 0) {
		this->window_rect.top = pos.top;
		if ((pos.flags & InitPosition::BottomSet) != 0) this->window_rect.bottom = pos.bottom - pos.top;
	}
	else if ((pos.flags & InitPosition::BottomSet) != 0) {
		this->window_rect.top = (monitor_rect.bottom - monitor_rect.top) - pos.bottom - this->window_rect.bottom;
	}

	if ((pos.flags & InitPosition::CenterH) != 0) {
		this->window_rect.left = (monitor_rect.left + monitor_rect.right - this->window_rect.right) / 2;
	}
	if ((pos.flags & InitPosition::CenterV) != 0) {
		this->window_rect.top = (monitor_rect.top + monitor_rect.bottom - this->window_rect.bottom) / 2;
	}

	this->window_rect.right += this->window_rect.left;
	this->window_rect.bottom += this->window_rect.top;

	// Window class
	WNDCLASSEXW wc;

	wc.cbSize = sizeof(wc);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = Window::window_proc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = instance_handle;
	wc.hIcon = nullptr;
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = this->bg_color_brush;
	wc.lpszMenuName = nullptr;
	wc.lpszClassName = L"Robot";
	wc.hIconSm = nullptr;

	this->wc_atom = RegisterClassExW(&wc);
	if (this->wc_atom == 0) {
		// Error
		return false;
	}



	// Adjust window size
	AdjustWindowRectEx(
		&window_rect,
		this->window_style,
		menu_handle != nullptr,
		this->window_style_extended
	);



	// Create
	this->instance_handle = instance_handle;
	this->lib_handle = lib_handle;
	this->window_handle = CreateWindowEx(
		this->window_style_extended,
		MAKEINTATOM(this->wc_atom), // wc.lpszClassName,
		L"",
		this->window_style,
		window_rect.left,
		window_rect.top,
		window_rect.right - window_rect.left,
		window_rect.bottom - window_rect.top,
		HWND_DESKTOP,
		menu_handle,
		this->instance_handle,
		this
	);
	if (this->window_handle == nullptr) {
		// Error
		UnregisterClass(MAKEINTATOM(this->wc_atom), this->instance_handle);
		return false;
	}

	// Make window visible
	ShowWindow(this->window_handle, SW_NORMAL | SW_SHOWNOACTIVATE);
	SetWindowText(this->window_handle, this->window_title.c_str());
	SetWindowPos(
		this->window_handle,
		HWND_TOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE
	);


	// Icon
	HICON icon = LoadIcon(this->instance_handle, MAKEINTRESOURCE(RES_ICON_WINDOW));
	if (icon != nullptr) {
		SendMessage(this->window_handle, WM_SETICON, ICON_BIG, (LPARAM) icon);
		SendMessage(this->window_handle, WM_SETICON, ICON_SMALL, (LPARAM) icon);
		DestroyIcon(icon);
	}


	// Raw mouse input
	RAWINPUTDEVICE rawInputDevices[1];
	rawInputDevices[0].usUsagePage = static_cast<USHORT>(0x01); // HID_USAGE_PAGE_GENERIC;
	rawInputDevices[0].usUsage = static_cast<USHORT>(0x06); // HID_USAGE_GENERIC_KEYBOARD;
	rawInputDevices[0].dwFlags = RIDEV_INPUTSINK;
	rawInputDevices[0].hwndTarget = this->window_handle;
	RegisterRawInputDevices(rawInputDevices, 1, sizeof(rawInputDevices[0]));


	// Hook active window switch
	Window::on_active_window_change_obj = this;
	this->active_window_switch_hook = SetWinEventHook(
		EVENT_SYSTEM_FOREGROUND,
		EVENT_SYSTEM_FOREGROUND,
		nullptr,
		Window::on_active_window_change_callback,
		0,
		0,
		WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
	);

	// Okay
	this->active = true;
	if (!this->setup_gui()) {
		this->destroy();
		return false;
	}
	this->set_opacity(this->opacity);
	return true;
}

bool Window :: setup_gui() {
	// Font
	int font_height = 16;
	this->default_font = CreateFont(
		font_height, 0, 0, 0,
		FW_NORMAL,
		FALSE,
		FALSE,
		FALSE,
		DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS,
		CLIP_DEFAULT_PRECIS,
		CLEARTYPE_QUALITY,
		DEFAULT_PITCH,
		L"Verdana"
	);
	SendMessage(this->window_handle, WM_SETFONT, (WPARAM) this->default_font, (LPARAM) TRUE);

	int status_bar_height = 4;

	// The text may disappear at length 4094+
	this->text_input = CreateWindow(
		L"EDIT", nullptr,
		WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL,
		0,
		2,
		this->window_rect.right - this->window_rect.left,
		20,
		this->window_handle,
		nullptr,
		this->instance_handle,
		nullptr
	);
	if (this->text_input == nullptr) return false;
	SendMessage(this->text_input, WM_SETFONT, (WPARAM) this->default_font, (LPARAM) TRUE);
	SetWindowText(this->text_input, L"");
	this->replace_control_proc(this->text_input, [](Window* self, bool& used, HWND wnd, UINT msg, WPARAM w_param, LPARAM l_param) -> LRESULT {
		switch (msg) {
			case WM_KEYDOWN:
			case WM_CHAR:
			{
				if (w_param == VK_RETURN) {
					if (msg == WM_KEYDOWN) {
						self->on_text_input_submit();
					}
					return 0;
				}
			}
			break;
			case WM_PASTE:
			{
				OpenClipboard(nullptr);
				HANDLE clip = GetClipboardData(CF_UNICODETEXT); // CF_TEXT
				if (clip != nullptr) {
					wchar_t* clip_text = static_cast<wchar_t*>(GlobalLock(clip));
					if (clip_text != nullptr) {
						std::wstring clip_string = clip_text;
						GlobalUnlock(clip_text);

						self->make_singleline_string(clip_string);

						SendMessage(wnd, EM_REPLACESEL, (WPARAM) TRUE, (LPARAM) static_cast<const void*>(clip_string.c_str()));
					}
				}

				CloseClipboard();

				return 0;
			}
			break;
		}

		used = false;
		return 0;
	});

	if (this->window_message.length() > 0) {
		this->text_label = CreateWindow(
			L"STATIC", this->window_message.c_str(),
			WS_CHILD | WS_VISIBLE | SS_CENTER,
			0,
			static_cast<int>((this->window_rect.bottom - this->window_rect.top - status_bar_height) / 2),
			this->window_rect.right - this->window_rect.left,
			20,
			this->window_handle,
			nullptr,
			this->instance_handle,
			nullptr
		);
		if (this->text_label == nullptr) return false;
		SendMessage(this->text_label, WM_SETFONT, (WPARAM) this->default_font, (LPARAM) TRUE);
	}

	this->status_bar = CreateWindow(
		L"STATIC", nullptr,
		WS_CHILD | WS_VISIBLE,
		0,
		(this->window_rect.bottom - this->window_rect.top - status_bar_height),
		this->window_rect.right - this->window_rect.left,
		status_bar_height,
		this->window_handle,
		nullptr,
		this->instance_handle,
		nullptr
	);
	if (this->status_bar == nullptr) return false;

	// CTRL+A = select all
	this->register_hotkey('A', MOD_CONTROL, [](Window& self) {
		HWND focus = GetFocus();
		if (focus != nullptr && Window::hwnd_is_textbox(focus)) {
			PostMessage(focus, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);
		}
	});

	return true;
}

bool Window :: destroy() {
	this->active = false;

	if (this->active_window_switch_hook != nullptr) {
		UnhookWinEvent(this->active_window_switch_hook);
		this->active_window_switch_hook = nullptr;
		if (Window::on_active_window_change_obj == this) {
			Window::on_active_window_change_obj = nullptr;
		}
	}
	if (this->status_bar_bg != nullptr) {
		DeleteObject(this->status_bar_bg);
		this->status_bar_bg = nullptr;
	}
	if (this->font_color_brush != nullptr) {
		DeleteObject(this->font_color_brush);
		this->font_color_brush = nullptr;
	}
	if (this->bg_color_brush != nullptr) {
		DeleteObject(this->bg_color_brush);
		this->bg_color_brush = nullptr;
	}
	if (this->default_font != nullptr) {
		DeleteObject(this->default_font);
		this->default_font = nullptr;
	}
	if (this->window_handle != nullptr) {
		if (!DestroyWindow(this->window_handle)) {
			return false;
		}
		this->window_handle = nullptr;
	}
	if (this->instance_handle != nullptr) {
		if (!UnregisterClass(MAKEINTATOM(this->wc_atom), this->instance_handle)) {
			return false;
		}
		this->instance_handle = nullptr;
	}

	this->clear_hotkeys();
	this->clear_listeners();

	return true;
}

void Window :: message_loop() {
	// Message loop
	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0) != 0) {
		// Send to the WindowProc
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (!this->active) break;
	}
}

void Window :: deactivate() {
	this->active = false;
}

void Window :: set_opacity(double opacity) {
	if (opacity < 0.0) opacity = 0.0;
	else if (opacity > 1.0) opacity = 1.0;

	int opacity_int = static_cast<int>(opacity * 255 + 0.5);

	if (opacity_int < 0) opacity_int = 0;
	else if (opacity_int > 255) opacity_int = 255;

	SetLayeredWindowAttributes(this->window_handle, 0, opacity_int, LWA_ALPHA);
}

void Window :: set_status_bar_color(int r, int g, int b) {
	PostMessage(this->window_handle, WM_STATUS_COLOR, (WPARAM) (r < 0 ? TRANSPARENT : RGB(r,g,b)), 0);
}

Window::HotkeyEntry* Window :: register_hotkey(int key, int mod, HotkeyCallback callback) {
	auto it = this->hotkeys.find(key);
	if (it == this->hotkeys.end()) {
		auto ins = this->hotkeys.emplace(std::pair<const int, HotkeyCallbackList>(key, HotkeyCallbackList()));
		if (!std::get<1>(ins)) return nullptr;
		it = std::get<0>(ins);
	}

	HotkeyEntry* entry = new HotkeyEntry(mod, callback);
	if ((mod & MOD_BLOCKING) != 0) {
		RegisterHotKey(this->window_handle, this->hotkey_id, mod & MOD_ALL_VALID, key);
		++this->hotkey_id;
	}
	it->second.push_back(entry);
	return entry;
}

bool Window :: unregister_hotkey(int key, HotkeyEntry* entry) {
	auto it = this->hotkeys.find(key);
	if (it != this->hotkeys.end()) {
		auto it2 = it->second.begin();
		auto it2_end = it->second.end();
		for (; it2 != it2_end; ++it2) {
			if (*it2 == entry) {
				it->second.erase(it2);
				if ((entry->mod & MOD_BLOCKING) != 0) {
					UnregisterHotKey(this->window_handle, this->hotkey_id);
				}
				delete entry;
				return true;
			}
		}
	}

	return false;
}

bool Window :: trigger_hotkey(int key) {
	bool ret = false;
	auto it = this->hotkeys.find(key);
	if (it != this->hotkeys.end()) {
		auto it2 = it->second.begin();
		auto it2_end = it->second.end();
		for (; it2 != it2_end; ++it2) {
			if (((*it2)->mod & MOD_ALL_VALID) == this->hotkey_mod) {
				((*it2)->callback)(*this);
				ret = true;
			}
		}
	}
	return ret;
}

void Window :: clear_hotkeys() {
	auto it = this->hotkeys.begin();
	auto it_end = this->hotkeys.end();
	for (; it != it_end; ++it) {
		auto it2 = it->second.begin();
		auto it2_end = it->second.end();
		for (; it2 != it2_end; ++it2) {
			if (((*it2)->mod & MOD_BLOCKING) != 0) {
				UnregisterHotKey(this->window_handle, this->hotkey_id);
			}
			delete *it2;
		}
	}

	this->hotkeys.erase(this->hotkeys.begin(), it_end);
}

bool Window :: char_array_icompare(const wchar_t* a1, const wchar_t* a2, size_t length) {
	for (size_t i = 0; i < length; ++i) {
		if (std::tolower(a1[i]) != std::tolower(a2[i])) return false;
		if (a1[i] == L'\x00') return true;
	}
	return true;
}

bool Window :: hwnd_is_textbox(HWND window_handle) {
	wchar_t class_name[6];
	return (
		::GetClassName(window_handle, class_name, sizeof(class_name)) &&
		Window::char_array_icompare(class_name, L"Edit", sizeof(class_name))
	);
}

void Window :: set_focus(bool focused) {
	this->focused = focused;

	if (focused) {
		SetFocus(this->text_input);
		this->set_keyboard_input_blocked(true);
		this->set_opacity(this->opacity_active);
		SetWindowPos(
			this->window_handle,
			HWND_TOPMOST,
			0, 0, 0, 0,
			SWP_NOMOVE | SWP_NOSIZE
		);
	}
	else {
		HWND focus = GetFocus();
		if (focus != nullptr) {
			SetFocus(this->window_handle);
		}
		this->set_keyboard_input_blocked(false);
		this->set_opacity(this->opacity);
	}
}

bool Window :: has_focus() const {
	return this->focused;
}

void Window :: set_keyboard_input_blocked(bool blocked) {
	if (blocked) {
		Window::hook_input(this->lib_handle, this->window_handle);
	}
	else {
		Window::unhook_input();
	}
}

void Window :: replace_control_proc(HWND window_handle, WindowProcReplacement new_proc) {
	WNDPROC old_proc = (WNDPROC) GetWindowLongPtr(window_handle, GWLP_WNDPROC);
	WindowProcInfo* pi = new WindowProcInfo{this, old_proc, new_proc}; // TODO : this is never deleted
	SetWindowLongPtr(window_handle, GWLP_USERDATA, (LONG_PTR) pi);
	SetWindowLongPtr(window_handle, GWLP_WNDPROC, (LONG_PTR) Window::window_proc_replacement);
}

LRESULT CALLBACK
Window :: window_proc_replacement(
	HWND window_handle,
	UINT msg,
	WPARAM w_param,
	LPARAM l_param
) {
	WindowProcInfo* pi = reinterpret_cast<WindowProcInfo*>(GetWindowLongPtr(window_handle, GWLP_USERDATA));
	bool used = true;
	LRESULT v = (pi->new_proc)(pi->self, used, window_handle, msg, w_param, l_param);
	if (!used) {
		v = (pi->old_proc)(window_handle, msg, w_param, l_param);
	}
	return v;
}

void Window :: on_text_input_submit() {
	int text_len = GetWindowTextLength(this->text_input);
	wchar_t* text_buf = new wchar_t[text_len + 1];
	GetWindowText(this->text_input, text_buf, text_len + 1);
	std::wstring text = text_buf;
	delete [] text_buf;

	SetWindowText(this->text_input, L"");

	auto it = this->text_input_listeners.begin();
	auto it_end = this->text_input_listeners.end();
	for (; it != it_end; ++it) {
		(*it)(*this, text, this->hotkey_mod);
	}
}

void Window :: add_text_input_listener(TextInputCallback callback) {
	this->text_input_listeners.push_back(callback);
}

void Window :: clear_listeners() {
	this->text_input_listeners.erase(this->text_input_listeners.begin(), this->text_input_listeners.end());
}

HINSTANCE Window :: setup_dll(const wchar_t* filename) {
	HINSTANCE lib = LoadLibrary(filename);
	if (lib == nullptr) return nullptr;

	hook_input = (hook_input_fn) GetProcAddress(lib, "hook_input");
	unhook_input = (unhook_input_fn) GetProcAddress(lib, "unhook_input");

	if (hook_input == nullptr || unhook_input == nullptr) {
		hook_input = nullptr;
		unhook_input = nullptr;
		FreeLibrary(lib);
		return nullptr;
	}

	return lib;
}

VOID CALLBACK Window :: on_active_window_change_callback(
	HWINEVENTHOOK win_event_hook,
	DWORD event,
	HWND window_handle,
	LONG object,
	LONG child,
	DWORD event_thread,
	DWORD event_time
) {
	Window::on_active_window_change_obj->on_active_window_change();
}

void Window :: on_active_window_change() {
	SetWindowPos(
		this->window_handle,
		HWND_TOPMOST,
		0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE
	);
}

void Window :: close() {
	PostMessage(this->window_handle, WM_DESTROY, 0, 0);
}

bool Window :: get_key_info(const std::wstring& str, int& key, int& mod, std::wstring* key_name) const {
	size_t temp_len = 0;
	std::stringstream temp;
	std::string temp_key;
	std::wstringstream new_key_name;
	bool key_found = false;
	decltype(this->key_code_mapping)::const_iterator it;
	int new_key = -1;
	int new_mod = 0;

	for (size_t i = 0; i < str.length(); ++i) {
		int c = static_cast<int>(static_cast<unsigned int>(str[i]));
		if (std::isspace(c)) {
			if (temp_len > 0) {
				// Check single
				temp_key = temp.str();
				if ((it = this->key_code_mapping.find(temp_key)) != this->key_code_mapping.cend()) {
					if (key_found) return false;
					new_key_name << it->first.c_str();
					new_key = it->second;
					key_found = true;
				}
				else if ((it = this->key_mod_mapping.find(temp_key)) != this->key_mod_mapping.cend()) {
					new_mod |= it->second;
				}
				else {
					return false;
				}
				temp_len = 0;
				temp.str("");
			}
		}
		else {
			temp << static_cast<char>(std::tolower(c));
			++temp_len;
		}
	}

	// Empty
	if (temp_len == 0) return false;

	// Check
	temp_key = temp.str();
	if ((it = this->key_code_mapping.find(temp_key)) != this->key_code_mapping.cend()) {
		if (key_found) return false;
		new_key_name << it->first.c_str();
		new_key = it->second;
		key_found = true;
	}
	else if ((it = this->key_mod_mapping.find(temp_key)) != this->key_mod_mapping.cend()) {
		new_mod |= it->second;
	}
	else {
		return false;
	}

	// Okay
	key = new_key;
	mod = new_mod;
	if (key_name != nullptr) *key_name = new_key_name.str();
	return true;
}

void Window :: setup_key_code_mapping() {
	for (const KeyName* knames = key_names; knames->name != nullptr; ++knames) {
		this->key_code_mapping.emplace(std::pair<std::string, int>(knames->name, knames->vk_code));
	}
	for (const KeyModifier* kmods = key_modifiers; kmods->name != nullptr; ++kmods) {
		this->key_mod_mapping.emplace(std::pair<std::string, int>(kmods->name, kmods->modifier));
	}
}

void Window :: set_strings(const std::wstring& window_title, const std::wstring& window_message) {
	this->window_title = window_title;
	this->window_message = window_message;
}

void Window :: get_key_names(std::list<std::wstring>& list) {
	std::wstringstream stream;
	for (const KeyName* knames = key_names; knames->name != nullptr; ++knames) {
		stream << knames->name;
		list.emplace(list.end(), stream.str());
		stream.str(L"");
	}
}

void Window :: get_key_modifiers(std::list<std::wstring>& list) {
	std::wstringstream stream;
	for (const KeyModifier* kmods = key_modifiers; kmods->name != nullptr; ++kmods) {
		stream << kmods->name;
		list.emplace(list.end(), stream.str());
		stream.str(L"");
	}
}

void Window :: make_singleline_string(std::wstring& text) {
	std::wstringstream out;

	size_t last = 0;
	size_t i;

	for (i = 0; i < text.length(); ++i) {
		if (text[i] == L'\n' || text[i] == L'\r' || text[i] == L'\t') {
			if (i > last) {
				if (last > 0) out << L' ';
				out << text.substr(last, i - last);
			}
			last = i + 1;
		}
	}

	if (i > last) {
		if (last > 0) out << L' ';
		out << text.substr(last, i - last);
	}

	text = out.str();
}


