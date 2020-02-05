#include "NativeStation.h"

#include "Direct2D.h"
#include "Direct3D.h"
#include "WindowStation.h"
#include "../UserInterface/OverlappedWindows.h"
#include "../Miscellaneous/DynamicString.h"
#include "../ImageCodec/IconCodec.h"
#include "../Storage/ImageVolume.h"
#include "../PlatformSpecific/WindowsEffects.h"

#include <Windows.h>

#undef ZeroMemory

using namespace Engine::UI;

#define ENGINE_MAIN_WINDOW_CLASS	L"engine_runtime_main_class"
#define ENGINE_POPUP_WINDOW_CLASS	L"engine_runtime_popup_class"

namespace Engine
{
	namespace NativeWindows
	{
		LRESULT WINAPI WindowCallbackProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);
		void FillWinapiMenu(HMENU Menu, ObjectArray<Menus::MenuElement> & Elements)
		{
			for (int i = 0; i < Elements.Length(); i++) {
				if (Elements[i].IsSeparator()) {
					AppendMenuW(Menu, MF_OWNERDRAW | MF_DISABLED | MF_GRAYED, 0, reinterpret_cast<LPCWSTR>(Elements.ElementAt(i)));
				} else {
					Menus::MenuItem * item = static_cast<Menus::MenuItem *>(Elements.ElementAt(i));
					uint flags = MF_OWNERDRAW;
					if (item->Checked) flags |= MF_CHECKED;
					if (item->Disabled) flags |= MF_DISABLED | MF_GRAYED;
					if (item->Children.Length()) {
						HMENU Submenu = CreatePopupMenu();
						FillWinapiMenu(Submenu, item->Children);
						AppendMenuW(Menu, flags | MF_POPUP, reinterpret_cast<UINT_PTR>(Submenu), reinterpret_cast<LPCWSTR>(Elements.ElementAt(i)));
					} else {
						AppendMenuW(Menu, flags, static_cast<UINT_PTR>(item->ID), reinterpret_cast<LPCWSTR>(Elements.ElementAt(i)));
					}
				}
			}
		}
		void DestroyWinapiMenu(HMENU Menu)
		{
			int count = GetMenuItemCount(Menu);
			for (int i = count - 1; i >= 0; i--) {
				HMENU Submenu = GetSubMenu(Menu, i);
				DeleteMenu(Menu, i, MF_BYPOSITION);
				if (Submenu) DestroyWinapiMenu(Submenu);
			}
			DestroyMenu(Menu);
		}

		bool SystemInitialized = false;
		void InitializeWindowSystem(void)
		{
			if (!SystemInitialized) {
				InitializeCodecCollection();
				Direct3D::CreateDevices();
				WNDCLASSEXW Cls;
				ZeroMemory(&Cls, sizeof(Cls));
				Cls.cbSize = sizeof(Cls);
				Cls.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_GLOBALCLASS;
				Cls.lpfnWndProc = WindowCallbackProc;
				Cls.cbWndExtra = sizeof(void*);
				Cls.hInstance = GetModuleHandleW(0);
				Cls.hIcon = reinterpret_cast<HICON>(LoadImageW(Cls.hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
					GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0));
				Cls.hIconSm = reinterpret_cast<HICON>(LoadImageW(Cls.hInstance, MAKEINTRESOURCEW(1), IMAGE_ICON,
					GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0));
				Cls.lpszClassName = ENGINE_MAIN_WINDOW_CLASS;
				RegisterClassExW(&Cls);
				Cls.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_GLOBALCLASS | CS_DROPSHADOW;
				Cls.hIcon = 0;
				Cls.hIconSm = 0;
				Cls.lpszClassName = ENGINE_POPUP_WINDOW_CLASS;
				RegisterClassExW(&Cls);
				SystemInitialized = true;
			}
		}
		void InitializeCodecCollection(void)
		{
			Direct2D::CreateWicCodec();
			Codec::CreateIconCodec();
			Storage::CreateVolumeCodec();
		}
		class NativeResourceLoader : public IResourceLoader
		{
		public:
			NativeResourceLoader(void) {}
			~NativeResourceLoader(void) override {}

			virtual ITexture * LoadTexture(Streaming::Stream * Source) override { return Direct2D::StandaloneDevice::LoadTexture(Source); }
			virtual ITexture * LoadTexture(Codec::Image * Source) override { return Direct2D::StandaloneDevice::LoadTexture(Source); }
			virtual ITexture * LoadTexture(Codec::Frame * Source) override { return Direct2D::StandaloneDevice::LoadTexture(Source); }
			virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override { return Direct2D::StandaloneDevice::LoadFont(FaceName, Height, Weight, IsItalic, IsUnderline, IsStrikeout); }
			virtual void ReloadTexture(ITexture * Texture, Streaming::Stream * Source) override {}
			virtual void ReloadTexture(ITexture * Texture, Codec::Image * Source) override {}
			virtual void ReloadTexture(ITexture * Texture, Codec::Frame * Source) override {}
			virtual void ReloadFont(UI::IFont * Font) override {}
		};
		IResourceLoader * CreateCompatibleResourceLoader(void)
		{
			InitializeCodecCollection();
			return new NativeResourceLoader();
		}
		Drawing::ITextureRenderingDevice * CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color)
		{
			Direct2D::InitializeFactory();
			return Direct2D::D2DRenderDevice::CreateD2DCompatibleTextureRenderingDevice(width, height, color);
		}
		class NativeStation : public HandleWindowStation
		{
			friend LRESULT WINAPI WindowCallbackProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam);
			friend void EnableWindow(UI::WindowStation * Station, bool Enable);
		private:
			SafePointer<IDXGISwapChain1> SwapChain;
			SafePointer<ID2D1DeviceContext> DeviceContext;
			SafePointer<ID2D1HwndRenderTarget> RenderTarget;
			SafePointer<Direct2D::D2DRenderDevice> RenderingDevice;
			int MinWidth = 0, MinHeight = 0;
			Array<NativeStation *> _slaves;
			NativeStation * _parent = 0;
			int last_x = 0x80000000, last_y = 0x80000000;
			Window::RefreshPeriod InternalRate = Window::RefreshPeriod::None;
		public:
			class DesktopWindowFactory : public WindowStation::IDesktopWindowFactory
			{
				Template::ControlTemplate * _template = 0;
			public:
				DesktopWindowFactory(Template::ControlTemplate * Template) : _template(Template) {}
				virtual Window * CreateDesktopWindow(WindowStation * Station) override
				{
					return _template ? new Controls::OverlappedWindow(0, Station, _template) :
						new Controls::OverlappedWindow(0, Station);
				}
			};
		
			NativeStation(HWND Handle, DesktopWindowFactory * Factory) : HandleWindowStation(Handle, Factory), _slaves(0x10)
			{
				if (WindowsSpecific::GetRenderingDeviceFeatureClass() == WindowsSpecific::RenderingDeviceFeatureClass::D3DDevice11) {
					try {
						Direct3D::CreateD2DDeviceContextForWindow(Handle, DeviceContext.InnerRef(), SwapChain.InnerRef());
						if (!DeviceContext) throw Exception();
						RenderingDevice.SetReference(new Direct2D::D2DRenderDevice(DeviceContext));
					} catch (...) {
						RECT Rect;
						GetClientRect(Handle, &Rect);
						D2D1_RENDER_TARGET_PROPERTIES RenderTargetProps;
						D2D1_HWND_RENDER_TARGET_PROPERTIES WndRenderTargetProps;
						RenderTargetProps.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
						RenderTargetProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
						RenderTargetProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
						RenderTargetProps.dpiX = 0.0f;
						RenderTargetProps.dpiY = 0.0f;
						RenderTargetProps.usage = D2D1_RENDER_TARGET_USAGE_NONE;
						RenderTargetProps.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
						WndRenderTargetProps.hwnd = Handle;
						WndRenderTargetProps.pixelSize.width = max(Rect.right, 1);
						WndRenderTargetProps.pixelSize.height = max(Rect.bottom, 1);
						WndRenderTargetProps.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
						Direct2D::D2DFactory->CreateHwndRenderTarget(&RenderTargetProps, &WndRenderTargetProps, RenderTarget.InnerRef());
						RenderingDevice.SetReference(new Direct2D::D2DRenderDevice(RenderTarget));
					}
				} else {
					RECT Rect;
					GetClientRect(Handle, &Rect);
					D2D1_RENDER_TARGET_PROPERTIES RenderTargetProps;
					D2D1_HWND_RENDER_TARGET_PROPERTIES WndRenderTargetProps;
					RenderTargetProps.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
					RenderTargetProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
					RenderTargetProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
					RenderTargetProps.dpiX = 0.0f;
					RenderTargetProps.dpiY = 0.0f;
					RenderTargetProps.usage = D2D1_RENDER_TARGET_USAGE_NONE;
					RenderTargetProps.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
					WndRenderTargetProps.hwnd = Handle;
					WndRenderTargetProps.pixelSize.width = max(Rect.right, 1);
					WndRenderTargetProps.pixelSize.height = max(Rect.bottom, 1);
					WndRenderTargetProps.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;
					Direct2D::D2DFactory->CreateHwndRenderTarget(&RenderTargetProps, &WndRenderTargetProps, RenderTarget.InnerRef());
					RenderingDevice.SetReference(new Direct2D::D2DRenderDevice(RenderTarget));
				}
				SetRenderingDevice(RenderingDevice);
			}
			virtual ~NativeStation(void) override {}
			virtual bool IsNativeStationWrapper(void) const override { return true; }
			virtual void OnDesktopDestroy(void) override
			{
				HWND parent_window = _parent ? _parent->GetHandle() : 0;
				if (parent_window) {
					NativeStation * parent = _parent;
					for (int i = 0; i < parent->_slaves.Length(); i++) if (parent->_slaves[i] == this) { parent->_slaves.Remove(i); break; }
				}
				Array<NativeStation *> slaves = _slaves;
				for (int i = 0; i < slaves.Length(); i++) slaves[i]->GetDesktop()->Destroy();
				SetWindowLongPtrW(_window, 0, 0);
				::DestroyWindow(_window);
				DeviceContext.SetReference(0);
				RenderTarget.SetReference(0);
				SwapChain.SetReference(0);
				DestroyStation();
			}
			virtual void RequireRefreshRate(Window::RefreshPeriod period) override
			{
				InternalRate = period;
				AnimationStateChanged();
			}
			virtual Window::RefreshPeriod GetRefreshRate(void) override { return InternalRate; }
			virtual void AnimationStateChanged(void) override
			{
				int fr = int(GetFocus() ? GetFocus()->FocusedRefreshPeriod() : Window::RefreshPeriod::None);
				int ar = int(IsPlayingAnimation() ? Window::RefreshPeriod::Cinematic : Window::RefreshPeriod::None);
				int ur = int(InternalRate);
				int mr = max(max(fr, ar), ur);
				if (!mr) KillTimer(GetHandle(), 1); else {
					if (mr == 1) ::SetTimer(GetHandle(), 1, GetRenderingDevice()->GetCaretBlinkHalfTime(), 0);
					else if (mr == 2) {
						if (::GetActiveWindow() == GetHandle()) ::SetTimer(GetHandle(), 1, 25, 0);
						else ::SetTimer(GetHandle(), 1, 100, 0);
					}
				}
			}
			virtual void FocusWindowChanged(void) override { InvalidateRect(GetHandle(), 0, 0); AnimationStateChanged(); }
			virtual Box GetDesktopBox(void) override { return GetScreenDimensions(); }
			virtual Box GetAbsoluteDesktopBox(const Box & box) override
			{
				POINT a = POINT{ box.Left, box.Top };
				POINT b = POINT{ box.Right, box.Bottom };
				ClientToScreen(GetHandle(), &a);
				ClientToScreen(GetHandle(), &b);
				return Box(a.x, a.y, b.x, b.y);
			}
			virtual void RequireRedraw(void) override { InvalidateRect(GetHandle(), 0, 0); }

			void RenderContent(void)
			{
				if (DeviceContext) {
					RenderingDevice->SetTimerValue(GetTimerValue());
					Animate();
					DeviceContext->SetDpi(96.0f, 96.0f);
					DeviceContext->BeginDraw();
					if (_clear_background) DeviceContext->Clear(D2D1::ColorF(0.0, 0.0, 0.0, 0.0));
					Render();
					DeviceContext->EndDraw();
					SwapChain->Present(1, 0);
				} else if (RenderTarget) {
					RenderingDevice->SetTimerValue(GetTimerValue());
					Animate();
					RenderTarget->SetDpi(96.0f, 96.0f);
					RenderTarget->BeginDraw();
					if (_clear_background) RenderTarget->Clear(D2D1::ColorF(0.0, 0.0, 0.0, 0.0));
					Render();
					RenderTarget->EndDraw();
				}
			}
			void ResizeContent(void)
			{
				if (DeviceContext) {
					Direct3D::ResizeRenderBufferForD2DDevice(DeviceContext, SwapChain);
				} else if (RenderTarget) {
					RECT Rect;
					GetClientRect(GetHandle(), &Rect);
					RenderTarget->Resize(D2D1::SizeU(max(Rect.right, 1), max(Rect.bottom, 1)));
				}
			}
			HWND GetHandle(void) const { return _window; }
			int & GetMinWidth(void) { return MinWidth; }
			int & GetMinHeight(void) { return MinHeight; }
			void MakeParent(NativeStation * child) { _slaves << child; child->_parent = this; }
		};
		UI::WindowStation * CreateOverlappedWindow(Template::ControlTemplate * Template, const UI::Rectangle & Position, WindowStation * ParentStation)
		{
			InitializeWindowSystem();
			RECT pRect;
			GetWindowRect(ParentStation ? static_cast<NativeStation *>(ParentStation)->GetHandle() : GetDesktopWindow(), &pRect);
			Box ParentBox(pRect.left, pRect.top, pRect.right, pRect.bottom);
			if (Position.IsValid()) ParentBox = Box(Position, ParentBox);
			Box ClientBox(Template->Properties->ControlPosition, ParentBox);
			Box ClientArea(0, 0, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top);
			DWORD ExStyle = WS_EX_DLGMODALFRAME;
			DWORD Style = WS_OVERLAPPED | WS_SYSMENU;
			auto props = static_cast<Template::Controls::DialogFrame *>(Template->Properties);
			if (props->Captioned) Style |= WS_CAPTION; else Style |= WS_POPUP;
			if (props->HelpButton) ExStyle |= WS_EX_CONTEXTHELP;
			if (props->MaximizeButton) Style |= WS_MAXIMIZEBOX;
			if (props->MinimizeButton) Style |= WS_MINIMIZEBOX;
			if (props->ToolWindow) ExStyle |= WS_EX_TOOLWINDOW;
			if (props->Sizeble) Style |= WS_THICKFRAME;
			RECT wRect = { 0, 0, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top };
			RECT mRect = { 0, 0, props->MinimalWidth, props->MinimalHeight };
			AdjustWindowRectEx(&wRect, Style, 0, ExStyle);
			AdjustWindowRectEx(&mRect, Style, 0, ExStyle);
			ClientBox.Left = ParentBox.Left + (ParentBox.Right - ParentBox.Left - wRect.right + wRect.left) / 2;
			ClientBox.Right = ClientBox.Left + wRect.right - wRect.left;
			ClientBox.Top = ParentBox.Top + (ParentBox.Bottom - ParentBox.Top - wRect.bottom + wRect.top) / 2;
			ClientBox.Bottom = ClientBox.Top + wRect.bottom - wRect.top;
			HWND Handle = CreateWindowExW(ExStyle, ENGINE_MAIN_WINDOW_CLASS, props->Title, Style,
				ClientBox.Left, ClientBox.Top, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top,
				ParentStation ? static_cast<NativeStation *>(ParentStation)->GetHandle() : 0,
				0, 0, 0);
			if (!props->CloseButton) EnableMenuItem(GetSystemMenu(Handle, FALSE), SC_CLOSE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			if (!props->MaximizeButton) EnableMenuItem(GetSystemMenu(Handle, FALSE), SC_MAXIMIZE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			if (!props->MinimizeButton) EnableMenuItem(GetSystemMenu(Handle, FALSE), SC_MINIMIZE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			if (!props->Sizeble) EnableMenuItem(GetSystemMenu(Handle, FALSE), SC_SIZE, MF_BYCOMMAND | MF_DISABLED | MF_GRAYED);
			NativeStation::DesktopWindowFactory Factory(Template);
			SafePointer<NativeStation> Station = new NativeStation(Handle, &Factory);
			if (ParentStation) static_cast<NativeStation *>(ParentStation)->MakeParent(Station);
			SetWindowLongPtrW(Handle, 0, reinterpret_cast<LONG_PTR>(Station.Inner()));
			Station->GetMinWidth() = mRect.right - mRect.left;
			Station->GetMinHeight() = mRect.bottom - mRect.top;
			Controls::OverlappedWindow * Desktop = Station->GetDesktop()->As<Controls::OverlappedWindow>();
			Desktop->GetContentFrame()->SetRectangle(UI::Rectangle::Entire());
			if (!props->Background) {
				Color BackgroundColor = 0;
				if (props->DefaultBackground) {
					BackgroundColor = GetSysColor(COLOR_BTNFACE);
					BackgroundColor.a = 255;
				} else {
					BackgroundColor = props->BackgroundColor;
				}
				SafePointer<Template::BarShape> Background = new Template::BarShape;
				Background->Position = UI::Rectangle::Entire();
				Background->Gradient << Template::GradientPoint(Template::ColorTemplate(BackgroundColor), 0.0);
				Desktop->SetBackground(Background);
			}
			Station->SetBox(ClientArea);
			Station->Retain();
			return Station;
		}
		UI::WindowStation * CreatePopupWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation)
		{
			InitializeWindowSystem();
			Box ClientBox(Position, GetScreenDimensions());
			Box ClientArea(0, 0, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top);
			DWORD ExStyle = WS_EX_NOACTIVATE | WS_EX_TOPMOST;
			DWORD Style = WS_POPUP;
			RECT wRect = { 0, 0, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top };
			AdjustWindowRectEx(&wRect, Style, 0, ExStyle);
			ClientBox.Left = ClientBox.Left + wRect.left;
			ClientBox.Right = ClientBox.Left + wRect.right;
			ClientBox.Top = ClientBox.Top + wRect.top;
			ClientBox.Bottom = ClientBox.Top + wRect.bottom;
			HWND Handle = CreateWindowExW(ExStyle, ENGINE_POPUP_WINDOW_CLASS, L"", Style,
				ClientBox.Left, ClientBox.Top, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top,
				0, 0, 0, 0);
			NativeStation::DesktopWindowFactory Factory(Template);
			SafePointer<NativeStation> Station = new NativeStation(Handle, &Factory);
			SetWindowLongPtrW(Handle, 0, reinterpret_cast<LONG_PTR>(Station.Inner()));
			Controls::OverlappedWindow * Desktop = Station->GetDesktop()->As<Controls::OverlappedWindow>();
			Desktop->GetContentFrame()->SetRectangle(UI::Rectangle::Entire());
			{
				Color BackgroundColor = 0xFF000000;
				SafePointer<Template::BarShape> Background = new Template::BarShape;
				Background->Position = UI::Rectangle::Entire();
				Background->Gradient << Template::GradientPoint(Template::ColorTemplate(BackgroundColor), 0.0);
				Desktop->SetBackground(Background);
			}
			Station->SetBox(ClientArea);
			Station->Retain();
			return Station;
		}
		void ShowWindow(UI::WindowStation * Station, bool Show)
		{
			HWND handle = reinterpret_cast<NativeStation *>(Station)->GetHandle();
			ShowWindow(handle, Show ? ((GetWindowLongPtr(handle, GWL_EXSTYLE) & WS_EX_NOACTIVATE) ? SW_SHOWNOACTIVATE : SW_SHOW) : SW_HIDE);
		}
		void EnableWindow(UI::WindowStation * Station, bool Enable)
		{
			auto st = static_cast<NativeStation *>(Station);
			::EnableWindow(st->GetHandle(), Enable);
		}
		void SetWindowTitle(UI::WindowStation * Station, const string & Title)
		{
			SetWindowTextW(reinterpret_cast<NativeStation *>(Station)->GetHandle(), Title);
		}
		void SetWindowPosition(UI::WindowStation * Station, const UI::Box & position)
		{
			SetWindowPos(reinterpret_cast<NativeStation *>(Station)->GetHandle(), 0,
				position.Left, position.Top, position.Right - position.Left, position.Bottom - position.Top, SWP_NOZORDER);
		}
		bool IsWindowVisible(UI::WindowStation * Station)
		{
			return ::IsWindowVisible(reinterpret_cast<NativeStation *>(Station)->GetHandle()) != 0;
		}
		bool IsWindowEnabled(UI::WindowStation * Station)
		{
			return ::IsWindowEnabled(reinterpret_cast<NativeStation *>(Station)->GetHandle()) != 0;
		}
		string GetWindowTitle(UI::WindowStation * Station)
		{
			DynamicString text;
			text.ReserveLength(GetWindowTextLengthW(reinterpret_cast<NativeStation *>(Station)->GetHandle()) + 1);
			GetWindowTextW(reinterpret_cast<NativeStation *>(Station)->GetHandle(), text, text.ReservedLength());
			return text.ToString();
		}
		UI::Box GetWindowPosition(UI::WindowStation * Station)
		{
			RECT Rect;
			GetWindowRect(reinterpret_cast<NativeStation *>(Station)->GetHandle(), &Rect);
			return UI::Box(Rect.left, Rect.top, Rect.right, Rect.bottom);
		}
		int RunMenuPopup(UI::Menus::Menu * menu, UI::Window * owner, UI::Point at)
		{
			POINT p = { at.x, at.y };
			HWND server = static_cast<NativeStation *>(owner->GetStation())->GetHandle();
			ClientToScreen(server, &p);
			SafePointer<ID2D1DCRenderTarget> RenderTarget;
			D2D1_RENDER_TARGET_PROPERTIES RenderTargetProps;
			RenderTargetProps.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
			RenderTargetProps.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
			RenderTargetProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
			RenderTargetProps.dpiX = 0.0f;
			RenderTargetProps.dpiY = 0.0f;
			RenderTargetProps.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
			RenderTargetProps.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
			if (Direct2D::D2DFactory->CreateDCRenderTarget(&RenderTargetProps, RenderTarget.InnerRef()) != S_OK) throw Exception();
			SafePointer<Direct2D::D2DRenderDevice> LocalRenderingDevice = new Direct2D::D2DRenderDevice(RenderTarget);
			LocalRenderingDevice->SetTimerValue(0);
			for (int i = 0; i < menu->Children.Length(); i++) menu->Children[i].WakeUp(LocalRenderingDevice);
			HMENU Menu = CreatePopupMenu();
			FillWinapiMenu(Menu, menu->Children);
			int result = TrackPopupMenuEx(Menu, TPM_RETURNCMD, p.x, p.y, server, 0);
			DestroyWinapiMenu(Menu);
			for (int i = 0; i < menu->Children.Length(); i++) menu->Children[i].Shutdown();
			return result;
		}
		UI::Box GetScreenDimensions(void)
		{
			RECT Rect;
			GetWindowRect(GetDesktopWindow(), &Rect);
			return UI::Box(Rect.left, Rect.top, Rect.right, Rect.bottom);
		}
		double GetScreenScale(void)
		{
			HDC DC = GetDC(0);
			int dpi = GetDeviceCaps(DC, LOGPIXELSX);
			ReleaseDC(0, DC);
			return double(dpi) / 96.0;
		}
		void RunMainMessageLoop(void)
		{
			MSG Msg;
			while (GetMessageW(&Msg, 0, 0, 0)) {
				TranslateMessage(&Msg);
				DispatchMessageW(&Msg);
			}
		}
		void ExitMainLoop(void) { PostQuitMessage(0); }
		Array<string> * GetFontFamilies(void)
		{
			Direct2D::InitializeFactory();
			SafePointer<IDWriteFontCollection> collection;
			if (Direct2D::DWriteFactory && Direct2D::DWriteFactory->GetSystemFontCollection(collection.InnerRef()) == S_OK) {
				uint count = collection->GetFontFamilyCount();
				SafePointer< Array<string> > result = new Array<string>(int(count));
				for (uint i = 0; i < count; i++) {
					SafePointer<IDWriteFontFamily> family;
					if (collection->GetFontFamily(i, family.InnerRef()) == S_OK) {
						SafePointer<IDWriteLocalizedStrings> strings;
						if (family->GetFamilyNames(strings.InnerRef()) == S_OK) {
							UINT32 index, length;
							BOOL exists;
							if (strings->FindLocaleName(L"en-us", &index, &exists) != S_OK || !exists) index = 0;
							if (strings->GetStringLength(index, &length) == S_OK) {
								DynamicString str;
								str.ReserveLength(length + 1);
								strings->GetString(index, str, length + 1);
								result->Append(str.ToString());
							}
						}
					}
				}
				result->Retain();
				return result;
			}
			return 0;
		}
		void SetApplicationIcon(Codec::Image * icon)
		{
			InitializeWindowSystem();
			auto icon_sm_frame = icon->GetFrameBestSizeFit(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON));
			auto icon_nm_frame = icon->GetFrameBestSizeFit(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON));
			auto icon_sm_handle = UI::CreateWinIcon(icon_sm_frame);
			auto icon_nm_handle = UI::CreateWinIcon(icon_nm_frame);
			auto wnd = FindWindowExW(0, 0, ENGINE_MAIN_WINDOW_CLASS, 0);
			HICON icon_sm_old = icon_sm_handle;
			HICON icon_nm_old = icon_nm_handle;
			if (wnd) {
				icon_sm_old = (HICON) GetClassLongPtrW(wnd, GCLP_HICONSM);
				icon_nm_old = (HICON) GetClassLongPtrW(wnd, GCLP_HICON);
				SetClassLongPtrW(wnd, GCLP_HICONSM, (LONG_PTR) icon_sm_handle);
				SetClassLongPtrW(wnd, GCLP_HICON, (LONG_PTR) icon_nm_handle);
			} else {
				WNDCLASSEXW cls;
				cls.cbSize = sizeof(cls);
				if (GetClassInfoExW(GetModuleHandleW(0), ENGINE_MAIN_WINDOW_CLASS, &cls)) {
					if (UnregisterClassW(ENGINE_MAIN_WINDOW_CLASS, GetModuleHandleW(0))) {
						icon_sm_old = cls.hIconSm;
						icon_nm_old = cls.hIcon;
						cls.hIconSm = icon_sm_handle;
						cls.hIcon = icon_nm_handle;
						RegisterClassExW(&cls);
					}
				}
			}
			if (icon_sm_old) DestroyIcon(icon_sm_old);
			if (icon_nm_old) DestroyIcon(icon_nm_old);
		}
		LRESULT WINAPI WindowCallbackProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam)
		{
			NativeStation * station = reinterpret_cast<NativeStation *>(GetWindowLongPtrW(Wnd, 0));
			if (!station) return DefWindowProcW(Wnd, Msg, WParam, LParam);
			LRESULT Result = 0;
			if (Msg == WM_SIZE) {
				if (station) {
					station->ResizeContent();
					Result = station->ProcessWindowEvents(Msg, WParam, LParam);
					station->GetDesktop()->As<Controls::OverlappedWindow>()->RaiseFrameEvent(Windows::FrameEvent::Move);
				}
			} else if (Msg == WM_MOVE) {
				if (station) {
					Result = station->ProcessWindowEvents(Msg, WParam, LParam);
					auto p = MAKEPOINTS(LParam);
					if (station->last_x != 0x80000000 && station->last_y != 0x80000000) {
						int dx = p.x - station->last_x;
						int dy = p.y - station->last_y;
						for (int i = 0; i < station->_slaves.Length(); i++) {
							HWND child = station->_slaves[i]->GetHandle();
							RECT rect;
							GetWindowRect(child, &rect);
							SetWindowPos(child, 0, rect.left + dx, rect.top + dy, 0, 0, SWP_NOZORDER | SWP_NOSIZE | SWP_NOACTIVATE);
						}
					}
					station->last_x = p.x;
					station->last_y = p.y;
				}
			} else if (Msg == WM_ENABLE) {
				for (int i = 0; i < station->_slaves.Length(); i++) {
					::EnableWindow(station->_slaves[i]->GetHandle(), WParam ? true : false);
				}
			} else if (Msg == WM_SYSCOMMAND) {
				if (WParam == SC_CONTEXTHELP) {
					station->GetDesktop()->As<Controls::OverlappedWindow>()->RaiseFrameEvent(Windows::FrameEvent::Help);
				} else {
					Result = DefWindowProcW(Wnd, Msg, WParam, LParam);
				}
			} else if (Msg == WM_TIMER) {
				if (station) {
					Result = station->ProcessWindowEvents(Msg, WParam, LParam);
					InvalidateRect(Wnd, 0, FALSE);
					return Result;
				} else return DefWindowProcW(Wnd, Msg, WParam, LParam);
			} else if (Msg == WM_ACTIVATE) {
				if (station) {
					if (WParam == WA_INACTIVE) {
						station->AnimationStateChanged();
					} else {
						station->AnimationStateChanged();
					}
				}
			} else if (Msg == WM_PAINT) {
				if (station && ::IsWindowVisible(Wnd)) {
					station->RenderContent();
					ValidateRect(Wnd, 0);
				}
			} else if (Msg == WM_MOUSEACTIVATE) {
				if (GetWindowLongPtr(Wnd, GWL_EXSTYLE) & WS_EX_NOACTIVATE) return MA_NOACTIVATE;
				else return MA_ACTIVATE;
			} else if (Msg == WM_MEASUREITEM) {
				LPMEASUREITEMSTRUCT mis = reinterpret_cast<LPMEASUREITEMSTRUCT>(LParam);
				if (mis->CtlType == ODT_MENU) {
					auto item = reinterpret_cast<Menus::MenuElement *>(mis->itemData);
					mis->itemWidth = item->GetWidth();
					mis->itemHeight = item->GetHeight();
				}
				Result = 1;
			} else if (Msg == WM_DRAWITEM) {
				LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(LParam);
				if (dis->CtlType == ODT_MENU) {
					auto item = reinterpret_cast<Menus::MenuElement *>(dis->itemData);
					auto box = Box(0, 0, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top);
					auto target = static_cast<ID2D1DCRenderTarget *>(static_cast<Direct2D::D2DRenderDevice *>(item->GetRenderingDevice())->GetRenderTarget());
					target->BindDC(dis->hDC, &dis->rcItem);
					target->BeginDraw();
					if (dis->itemState & ODS_SELECTED) {
						item->Render(box, true);
					} else {
						item->Render(box, false);
					}
					target->EndDraw();
				}
				Result = 1;
			} else if (Msg == WM_GETMINMAXINFO) {
				if (station) {
					LPMINMAXINFO mmi = reinterpret_cast<LPMINMAXINFO>(LParam);
					mmi->ptMinTrackSize.x = station->GetMinWidth();
					mmi->ptMinTrackSize.y = station->GetMinHeight();
				}
			} else if (Msg == WM_CLOSE) {
				if (station) station->GetDesktop()->As<Controls::OverlappedWindow>()->RaiseFrameEvent(Windows::FrameEvent::Close);
				Result = 0;
			} else {
				if (station) Result = station->ProcessWindowEvents(Msg, WParam, LParam);
				else Result = DefWindowProcW(Wnd, Msg, WParam, LParam);
			}
			if (Msg == WM_MOUSEMOVE || Msg == WM_LBUTTONDOWN || Msg == WM_LBUTTONUP || Msg == WM_LBUTTONDBLCLK ||
				Msg == WM_RBUTTONDOWN || Msg == WM_RBUTTONUP || Msg == WM_RBUTTONDBLCLK ||
				Msg == WM_MOUSEWHEEL || Msg == WM_MOUSEHWHEEL || Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN ||
				Msg == WM_KEYUP || Msg == WM_SYSKEYUP || Msg == WM_CHAR) {
				InvalidateRect(Wnd, 0, FALSE);
			}
			return Result;
		}
	}
}