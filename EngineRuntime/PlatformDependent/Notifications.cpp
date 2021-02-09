#include "Notifications.h"

#include "WindowStation.h"
#include "NativeStation.h"
#include "../PlatformSpecific/WindowsRegistry.h"

#include <ShObjIdl.h>

#define ENGINE_TRAY_WINDOW_CLASS	L"engine_runtime_tray_class"
#define ERTM_TRAYEVENT				(WM_USER + 1)

namespace Engine
{
	namespace NativeWindows {
		LRESULT WINAPI HandleEngineMenuMessages(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	}
	namespace UI
	{
		namespace Windows
		{
			bool _tray_initialized = false;
			LRESULT WINAPI TrayWindowProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);
			void InitializeTraySubsystem(void)
			{
				if (!_tray_initialized) {
					NativeWindows::InitializeWindowSystem();
					WNDCLASSEXW cls;
					cls.cbSize = sizeof(cls);
					cls.style = CS_GLOBALCLASS;
					cls.lpfnWndProc = TrayWindowProc;
					cls.cbClsExtra = 0;
					cls.cbWndExtra = sizeof(void *);
					cls.hInstance = GetModuleHandleW(0);
					cls.hIcon = 0;
					cls.hCursor = 0;
					cls.hbrBackground = 0;
					cls.lpszMenuName = 0;
					cls.lpszClassName = ENGINE_TRAY_WINDOW_CLASS;
					cls.hIconSm = 0;
					RegisterClassExW(&cls);
					_tray_initialized = true;
				}
			}
			class WindowsTrayIcon : public StatusBarIcon
			{
				IWindowEventCallback * _callback;
				SafePointer<Codec::Image> _icon;
				StatusBarIconColorUsage _icon_usage;
				string _tooltip;
				int _id;
				SafePointer<Menus::Menu> _menu;
				bool _present;
				HWND _tray_window;
				HICON _tray_icon;
				void _update_tray(void)
				{
					NOTIFYICONDATAW data;
					ZeroMemory(&data, sizeof(data));
					data.cbSize = sizeof(data);
					data.hWnd = _tray_window;
					data.uID = 1;
					data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
					data.uCallbackMessage = ERTM_TRAYEVENT;
					data.hIcon = _tray_icon;
					string tip = _tooltip.Fragment(0, 127);
					MemoryCopy(data.szTip, static_cast<const widechar *>(tip), tip.Length() * 2);
					Shell_NotifyIconW(NIM_MODIFY, &data);
				}
				bool _is_light_theme(void)
				{
					try {
						SafePointer<WindowsSpecific::RegistryKey> root = WindowsSpecific::OpenRootRegistryKey(WindowsSpecific::RegistryRootKey::CurrentUser);
						if (!root) return false;
						SafePointer<WindowsSpecific::RegistryKey> personalize =
							root->OpenKey(L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize", WindowsSpecific::RegistryKeyAccess::ReadOnly);
						if (!personalize) return false;
						uint32 value = personalize->GetValueUInt32(L"SystemUsesLightTheme");
						return value != 0;
					} catch (...) { return false; }
				}
				void _update_icon(void)
				{
					if (_tray_icon) DestroyIcon(_tray_icon);
					Codec::Frame * frame = _icon ? _icon->GetFrameBestSizeFit(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON)) : 0;
					if (frame) {
						SafePointer<Codec::Frame> use_frame;
						if (_icon_usage == StatusBarIconColorUsage::Monochromic) {
							use_frame = frame->ConvertFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::BottomUp);
							uint32 color = _is_light_theme() ? 0x00000000 : 0x00FFFFFF;
							for (int y = 0; y < use_frame->GetHeight(); y++) for (int x = 0; x < use_frame->GetWidth(); x++) {
								uint32 pixel = use_frame->GetPixel(x, y);
								use_frame->SetPixel(x, y, (pixel & 0xFF000000) | color);
							}
						} else use_frame.SetRetain(frame);
						_tray_icon = CreateWinIcon(use_frame);
					} else _tray_icon = 0;
					if (_present) _update_tray();
				}
			public:
				WindowsTrayIcon(void)
				{
					InitializeTraySubsystem();
					_callback = 0; _icon_usage = StatusBarIconColorUsage::Colourfull;
					_id = 0; _present = false;
					_tray_window = CreateWindowExW(0, ENGINE_TRAY_WINDOW_CLASS, L"", WS_OVERLAPPED, 0, 0, 0, 0, 0, 0, 0, 0);
					_tray_icon = 0;
					SetWindowLongPtrW(_tray_window, 0, reinterpret_cast<LONG_PTR>(this));
				}
				virtual ~WindowsTrayIcon(void) override
				{
					if (_present) PresentIcon(false);
					DestroyWindow(_tray_window);
					if (_tray_icon) DestroyIcon(_tray_icon);
				}
				virtual void SetCallback(IWindowEventCallback * callback) override { _callback = callback; }
				virtual IWindowEventCallback * GetCallback(void) override { return _callback; }
				virtual void SetIcon(Codec::Image * image) override { _icon.SetRetain(image); _update_icon(); }
				virtual Codec::Image * GetIcon(void) override { return _icon; }
				virtual void SetIconColorUsage(StatusBarIconColorUsage color_usage) override { _icon_usage = color_usage; _update_icon(); }
				virtual StatusBarIconColorUsage GetIconColorUsage(void) override { return _icon_usage; }
				virtual void SetTooltip(const string & text) override { _tooltip = text; if (_present) _update_tray(); }
				virtual string GetTooltip(void) override { return _tooltip; }
				virtual void SetEventID(int ID) override { _id = ID; }
				virtual int GetEventID(void) override { return _id; }
				virtual void SetMenu(Menus::Menu * menu) override { _menu.SetRetain(menu); }
				virtual Menus::Menu * GetMenu(void) override { return _menu; }
				virtual void PresentIcon(bool present) override
				{
					if (present == _present) return;
					if (!_tray_icon) throw InvalidArgumentException();
					if (present) {
						NOTIFYICONDATAW data;
						ZeroMemory(&data, sizeof(data));
						data.cbSize = sizeof(data);
						data.hWnd = _tray_window;
						data.uID = 1;
						data.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
						data.uCallbackMessage = ERTM_TRAYEVENT;
						data.hIcon = _tray_icon;
						string tip = _tooltip.Fragment(0, 127);
						MemoryCopy(data.szTip, static_cast<const widechar *>(tip), tip.Length() * 2);
						if (!Shell_NotifyIconW(NIM_ADD, &data)) throw Exception();
						_present = true;
					} else {
						NOTIFYICONDATAW data;
						ZeroMemory(&data, sizeof(data));
						data.cbSize = sizeof(data);
						data.hWnd = _tray_window;
						data.uID = 1;
						if (!Shell_NotifyIconW(NIM_DELETE, &data)) throw Exception();
						_present = false;
					}
				}
				virtual bool IsVisible(void) override { return _present; }
			};
			LRESULT WINAPI TrayWindowProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam)
			{
				if (Msg == ERTM_TRAYEVENT) {
					if (LParam == WM_LBUTTONUP) {
						WindowsTrayIcon * info = reinterpret_cast<WindowsTrayIcon *>(GetWindowLongPtrW(Wnd, 0));
						if (info) {
							auto callback = info->GetCallback();
							if (callback) {
								if (info->GetEventID()) {
									callback->OnControlEvent(0, info->GetEventID(), Window::Event::MenuCommand, 0);
								} else if (info->GetMenu()) {
									POINT pos;
									GetCursorPos(&pos);
									int result = NativeWindows::RunMenuPopup(info->GetMenu(), Wnd, Point(pos.x, pos.y), true);
									if (result) callback->OnControlEvent(0, result, Window::Event::MenuCommand, 0);
								}
							}
						}
					}
					return 0;
				} else if (Msg == WM_MEASUREITEM) {
					return NativeWindows::HandleEngineMenuMessages(Wnd, Msg, WParam, LParam);
				} else if (Msg == WM_DRAWITEM) {
					return NativeWindows::HandleEngineMenuMessages(Wnd, Msg, WParam, LParam);
				}
				return DefWindowProcW(Wnd, Msg, WParam, LParam);
			}
			StatusBarIcon * CreateStatusBarIcon(void) { return new WindowsTrayIcon(); }
			void PushUserNotification(const string & title, const string & text, Codec::Image * icon)
			{
				InitializeTraySubsystem();
				HWND notify_wnd = CreateWindowExW(0, ENGINE_TRAY_WINDOW_CLASS, L"", WS_OVERLAPPED, 0, 0, 0, 0, 0, 0, 0, 0);
				string text_cut = text.Fragment(0, 255);
				string title_cut = title.Fragment(0, 63);
				NOTIFYICONDATAW data;
				ZeroMemory(&data, sizeof(data));
				data.cbSize = sizeof(data);
				data.hWnd = notify_wnd;
				data.uID = 1;
				data.uFlags = NIF_INFO;
				MemoryCopy(data.szInfo, static_cast<const widechar *>(text_cut), text_cut.Length() * 2);
				MemoryCopy(data.szInfoTitle, static_cast<const widechar *>(title_cut), text_cut.Length() * 2);
				HICON icon_handle = 0;
				if (icon) {
					Codec::Frame * frame = icon ? icon->GetFrameBestSizeFit(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON)) : 0;
					if (frame) icon_handle = CreateWinIcon(frame);
				} else icon_handle = reinterpret_cast<HICON>(LoadImageW(GetModuleHandleW(0), MAKEINTRESOURCEW(1), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
				if (icon_handle) {
					data.hBalloonIcon = icon_handle;
					data.dwInfoFlags = NIIF_USER | NIIF_LARGE_ICON;
				} else data.dwInfoFlags = NIIF_NONE;
				Shell_NotifyIconW(NIM_ADD, &data);
				Shell_NotifyIconW(NIM_DELETE, &data);
				DestroyWindow(notify_wnd);
				if (icon_handle) DestroyIcon(icon_handle);
			}
		}
	}
}