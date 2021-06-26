#pragma once

#include "../Graphics/GraphicsBase.h"
#include "../Miscellaneous/ThreadPool.h"

namespace Engine
{
	namespace Windows
	{
		class IScreen;
		class IWindowCallback;
		class IWindow;
		class IStatusBarIcon;
		class IMenuItem;
		class IMenu;

		enum class ThemeColor {
			Accent,
			WindowBackgroup, WindowText, SelectedBackground, SelectedText,
			MenuBackground, MenuText, MenuHotBackground, MenuHotText,
			GrayedText, Hyperlink
		};
		enum class ThemeClass : uint { Light, Dark };
		enum class ProgressDisplayMode { Hide, Normal, Paused, Error, Indeterminated };
		enum class CocoaEffectMaterial : uint { Titlebar, Selection, Menu, Popover, Sidebar, HeaderView, Sheet, WindowBackground, HUD, FullScreenUI, ToolTip };
		enum class CloseButtonState { Enabled, Alert, Disabled };
		enum class ImageRenderMode { CoverKeepAspectRatio, FitKeepAspectRatio, Stretch, Blit };
		enum class DeviceClass { DontCare, Hardware, Basic, Null };
		enum class SystemCursorClass { Null = 0, Arrow = 1, Beam = 2, Link = 3, SizeLeftRight = 4, SizeUpDown = 5, SizeLeftUpRightDown = 6, SizeLeftDownRightUp = 7, SizeAll = 8 };
		enum class MessageBoxButtonSet { Ok, OkCancel, YesNo, YesNoCancel };
		enum class MessageBoxStyle { Error, Warning, Information };
		enum class MessageBoxResult { Ok, Cancel, Yes, No };
		enum class ApplicationHandler { CreateFile, OpenSomeFile, OpenExactFile, ShowHelp, ShowAbout, ShowProperties, Terminate };
		enum class WindowHandler { Save, SaveAs, Export, Print, Undo, Redo, Cut, Copy, Paste, Duplicate, Delete, Find, Replace, SelectAll };
		enum class StatusBarIconColorUsage { Colourfull, Monochromic };
		enum class IPCStatus { Unknown, Accepted, Discarded, ServerClosed, InternalError };
		enum WindowFlags : uint {
			WindowFlagHasTitle			= 0x00000001,
			WindowFlagSizeble			= 0x00000002,
			WindowFlagCloseButton		= 0x00000004,
			WindowFlagMinimizeButton	= 0x00000008,
			WindowFlagMaximizeButton	= 0x00000010,
			WindowFlagHelpButton		= 0x00000020,
			WindowFlagToolWindow		= 0x00000040,
			WindowFlagPopup				= 0x00000080,
			WindowFlagNonOpaque			= 0x00000100,
			WindowFlagOverrideTheme		= 0x00000200,
			WindowFlagTransparent		= 0x00000400,
			WindowFlagBlurBehind		= 0x00000800,
			WindowFlagBlurFactor		= 0x00001000,

			WindowFlagWindowsExtendedFrame		= 0x00010000,
			WindowFlagCocoaTransparentTitle		= 0x00020000,
			WindowFlagCocoaEffectBackground		= 0x00040000,
			WindowFlagCocoaShadowless			= 0x00080000,
			WindowFlagCocoaContentUnderTitle	= 0x00100000,
			WindowFlagCocoaCustomBackground		= 0x00200000,
		};
		enum HotKeyFlags : uint {
			HotKeyShift			= 0x01,
			HotKeyControl		= 0x02,
			HotKeyAlternative	= 0x04,
			HotKeySystem		= 0x08,
		};
		struct CreateWindowDesc
		{
			uint Flags;
			string Title;
			UI::Box Position;
			UI::Point MinimalConstraints;
			UI::Point MaximalConstraints;
			UI::Box FrameMargins;
			double Opacity, BlurFactor;
			ThemeClass Theme;
			UI::Color BackgroundColor;
			IWindowCallback * Callback;
			IWindow * ParentWindow;
			IScreen * Screen;
		};
		struct FileFormat
		{
			string Description;
			Array<string> Extensions = Array<string>(0x10);
		};
		struct OpenFileInfo
		{
			bool MultiChoose = false;
			Array<FileFormat> Formats;
			int DefaultFormat = -1;
			string Title;
			Array<string> Files = Array<string>(0x10);
		};
		struct SaveFileInfo
		{
			Array<FileFormat> Formats = Array<FileFormat>(0x10);
			int Format = -1;
			string Title;
			string File;
			bool AppendExtension = true;
		};
		struct ChooseDirectoryInfo
		{
			string Title;
			string Directory;
		};

		class IScreen : public Object
		{
		public:
			virtual string GetName(void) = 0;
			virtual UI::Box GetScreenRectangle(void) noexcept = 0;
			virtual UI::Box GetUserRectangle(void) noexcept = 0;
			virtual UI::Point GetResolution(void) noexcept = 0;
			virtual double GetDpiScale(void) noexcept = 0;
			virtual Codec::Frame * Capture(void) noexcept = 0;
		};
		class ITheme : public Object
		{
		public:
			virtual ThemeClass GetClass(void) noexcept = 0;
			virtual UI::Color GetColor(ThemeColor color) noexcept = 0;
		};
		class ICursor : public Object
		{
		public:
			virtual handle GetOSHandle(void) noexcept = 0;
		};

		class IWindowCallback
		{
		public:
			// Fundamental callback
			virtual void Created(IWindow * window);
			virtual void Destroyed(IWindow * window);
			virtual void Shown(IWindow * window, bool show);
			// Presentation callback
			virtual void RenderWindow(IWindow * window);
			// Frame event callback
			virtual void WindowClose(IWindow * window);
			virtual void WindowMaximize(IWindow * window);
			virtual void WindowMinimize(IWindow * window);
			virtual void WindowRestore(IWindow * window);
			virtual void WindowHelp(IWindow * window);
			virtual void WindowActivate(IWindow * window);
			virtual void WindowDeactivate(IWindow * window);
			// Position callback
			virtual void WindowMove(IWindow * window);
			virtual void WindowSize(IWindow * window);
			// Keyboard events
			virtual void FocusChanged(IWindow * window, bool got);
			virtual bool KeyDown(IWindow * window, int key_code);
			virtual void KeyUp(IWindow * window, int key_code);
			virtual void CharDown(IWindow * window, uint32 ucs_code);
			// Mouse events
			virtual void CaptureChanged(IWindow * window, bool got);
			virtual void SetCursor(IWindow * window, UI::Point at);
			virtual void MouseMove(IWindow * window, UI::Point at);
			virtual void LeftButtonDown(IWindow * window, UI::Point at);
			virtual void LeftButtonUp(IWindow * window, UI::Point at);
			virtual void LeftButtonDoubleClick(IWindow * window, UI::Point at);
			virtual void RightButtonDown(IWindow * window, UI::Point at);
			virtual void RightButtonUp(IWindow * window, UI::Point at);
			virtual void RightButtonDoubleClick(IWindow * window, UI::Point at);
			// Scroll events
			virtual void ScrollVertically(IWindow * window, double delta);
			virtual void ScrollHorizontally(IWindow * window, double delta);
			// Miscellaneous event
			virtual void Timer(IWindow * window, int timer_id);
			virtual void ThemeChanged(IWindow * window);
			virtual bool IsWindowEventEnabled(IWindow * window, WindowHandler handler);
			virtual void HandleWindowEvent(IWindow * window, WindowHandler handler);
		};
		class IApplicationCallback
		{
		public:
			virtual bool IsHandlerEnabled(ApplicationHandler event);
			virtual bool IsWindowEventAccessible(WindowHandler handler);
			virtual void CreateNewFile(void);
			virtual void OpenSomeFile(void);
			virtual bool OpenExactFile(const string & path);
			virtual void ShowHelp(void);
			virtual void ShowAbout(void);
			virtual void ShowProperties(void);
			virtual void HotKeyEvent(int event_id);
			virtual bool DataExchangeReceive(handle client, const string & verb, const DataBlock * data);
			virtual DataBlock * DataExchangeRespond(handle client, const string & verb);
			virtual void DataExchangeDisconnect(handle client);
			virtual bool Terminate(void);
		};
		class IMenuItemCallback
		{
		public:
			virtual UI::Point MeasureMenuItem(IMenuItem * item, UI::IRenderingDevice * device) = 0;
			virtual void RenderMenuItem(IMenuItem * item, UI::IRenderingDevice * device, const UI::Box & at, bool hot_state) = 0;
			virtual void MenuClosed(IMenuItem * item) = 0;
			virtual void MenuItemDisposed(IMenuItem * item);
		};
		class IStatusCallback
		{
		public:
			virtual void StatusIconCommand(IStatusBarIcon * icon, int id);
		};

		class I2DPresentationEngine : public IPresentationEngine
		{
		public:
			// Extended presentation control
			virtual UI::IRenderingDevice * GetRenderingDevice(void) noexcept = 0;
			virtual bool BeginRenderingPass(void) noexcept = 0;
			virtual bool EndRenderingPass(void) noexcept = 0;
		};
		class IWindow : public ICoreWindow
		{
		public:
			// Frame properties control
			virtual void Show(bool show) = 0;
			virtual bool IsVisible(void) = 0;
			virtual void SetText(const string & text) = 0;
			virtual string GetText(void) = 0;
			virtual void SetPosition(const UI::Box & box) = 0;
			virtual UI::Box GetPosition(void) = 0;
			virtual UI::Point GetClientSize(void) = 0;
			virtual void SetMinimalConstraints(UI::Point size) = 0;
			virtual UI::Point GetMinimalConstraints(void) = 0;
			virtual void SetMaximalConstraints(UI::Point size) = 0;
			virtual UI::Point GetMaximalConstraints(void) = 0;
			virtual void Activate(void) = 0;
			virtual bool IsActive(void) = 0;
			virtual void Maximize(void) = 0;
			virtual bool IsMaximized(void) = 0;
			virtual void Minimize(void) = 0;
			virtual bool IsMinimized(void) = 0;
			virtual void Restore(void) = 0;
			virtual void RequireAttention(void) = 0;
			virtual void SetOpacity(double opacity) = 0;
			virtual void SetCloseButtonState(CloseButtonState state) = 0;
			// Children control
			virtual IWindow * GetParentWindow(void) = 0;
			virtual IWindow * GetChildWindow(int index) = 0;
			virtual int GetChildrenCount(void) = 0;
			// Frame system-dependent properties control
			virtual void SetProgressMode(ProgressDisplayMode mode) = 0;
			virtual void SetProgressValue(double value) = 0;
			virtual void SetCocoaEffectMaterial(CocoaEffectMaterial material) = 0;
			// Event handling
			virtual void SetCallback(IWindowCallback * callback) = 0;
			virtual IWindowCallback * GetCallback(void) = 0;
			virtual bool PointHitTest(UI::Point at) = 0;
			virtual UI::Point PointClientToGlobal(UI::Point at) = 0;
			virtual UI::Point PointGlobalToClient(UI::Point at) = 0;
			virtual void SetFocus(void) = 0;
			virtual bool IsFocused(void) = 0;
			virtual void SetCapture(void) = 0;
			virtual void ReleaseCapture(void) = 0;
			virtual bool IsCaptured(void) = 0;
			virtual void SetTimer(uint32 id, uint32 period) = 0;
			// Setting special presentation settings
			virtual void SetBackbufferedRenderingDevice(Codec::Frame * image, ImageRenderMode mode = ImageRenderMode::Stretch, UI::Color filling = 0xFF000000) noexcept = 0;
			virtual I2DPresentationEngine * Set2DRenderingDevice(DeviceClass device_class = DeviceClass::DontCare) noexcept = 0;
			// Getting visual appearance
			virtual double GetDpiScale(void) = 0;
			virtual IScreen * GetCurrentScreen(void) = 0;
			virtual ITheme * GetCurrentTheme(void) = 0;
			virtual uint GetBackgroundFlags(void) = 0;
			// Finilizing
			virtual void Destroy(void) = 0;
		};
		class IMenuItem : public Object
		{
		public:
			virtual void SetCallback(IMenuItemCallback * callback) = 0;
			virtual IMenuItemCallback * GetCallback(void) = 0;
			virtual void SetUserData(void * data) = 0;
			virtual void * GetUserData(void) = 0;
			virtual void SetSubmenu(IMenu * menu) = 0;
			virtual IMenu * GetSubmenu(void) = 0;
			virtual void SetID(int id) = 0;
			virtual int GetID(void) = 0;
			virtual void SetText(const string & text) = 0;
			virtual string GetText(void) = 0;
			virtual void SetSideText(const string & text) = 0;
			virtual string GetSideText(void) = 0;
			virtual void SetIsSeparator(bool separator) = 0;
			virtual bool IsSeparator(void) = 0;
			virtual void Enable(bool enable) = 0;
			virtual bool IsEnabled(void) = 0;
			virtual void Check(bool check) = 0;
			virtual bool IsChecked(void) = 0;
		};
		class IMenu : public Object
		{
		public:
			virtual void AppendMenuItem(IMenuItem * item) noexcept = 0;
			virtual void InsertMenuItem(IMenuItem * item, int at) noexcept = 0;
			virtual void RemoveMenuItem(int at) noexcept = 0;
			virtual IMenuItem * ElementAt(int at) noexcept = 0;
			virtual int Length(void) noexcept = 0;
			virtual IMenuItem * FindMenuItem(int id) noexcept = 0;
			virtual int Run(IWindow * owner, UI::Point at) noexcept = 0;
			virtual handle GetOSHandle(void) noexcept = 0;
		};
		class IStatusBarIcon : public Object
		{
		public:
			virtual void SetCallback(IStatusCallback * callback) noexcept = 0;
			virtual IStatusCallback * GetCallback(void) noexcept = 0;
			virtual UI::Point GetIconSize(void) noexcept = 0;
			virtual void SetIcon(Codec::Image * image) noexcept = 0;
			virtual Codec::Image * GetIcon(void) noexcept = 0;
			virtual void SetIconColorUsage(StatusBarIconColorUsage color_usage) noexcept = 0;
			virtual StatusBarIconColorUsage GetIconColorUsage(void) noexcept = 0;
			virtual void SetTooltip(const string & text) noexcept = 0;
			virtual string GetTooltip(void) noexcept = 0;
			virtual void SetEventID(int ID) noexcept = 0;
			virtual int GetEventID(void) noexcept = 0;
			virtual void SetMenu(IMenu * menu) noexcept = 0;
			virtual IMenu * GetMenu(void) noexcept = 0;
			virtual bool PresentIcon(bool present) noexcept = 0;
			virtual bool IsVisible(void) noexcept = 0;
		};
		class IIPCClient : public Object
		{
		public:
			virtual bool SendData(const string & verb, const DataBlock * data, IDispatchTask * on_responce, IPCStatus * result) noexcept = 0;
			virtual bool RequestData(const string & verb, IDispatchTask * on_responce, IPCStatus * result, DataBlock ** data) noexcept = 0;
			virtual IPCStatus GetStatus(void) noexcept = 0;
		};
		class IWindowSystem : public IDispatchQueue
		{
		public:
			// Creating windows
			virtual IWindow * CreateWindow(const CreateWindowDesc & desc) noexcept = 0;
			virtual IWindow * CreateModalWindow(const CreateWindowDesc & desc) noexcept = 0;
			// Measuring windows
			virtual UI::Box ConvertClientToWindow(const UI::Box & box, uint flags) noexcept = 0;
			virtual UI::Point ConvertClientToWindow(const UI::Point & size, uint flags) noexcept = 0;
			// Event handling
			virtual void SetFilesToOpen(const string * files, int num_files) noexcept = 0;
			virtual IApplicationCallback * GetCallback(void) noexcept = 0;
			virtual void SetCallback(IApplicationCallback * callback) noexcept = 0;
			// Application loop control
			virtual void RunMainLoop(void) noexcept = 0;
			virtual void ExitMainLoop(void) noexcept = 0;
			virtual void ExitModalSession(IWindow * window) noexcept = 0;
			// Main window registry maintanance
			virtual void RegisterMainWindow(IWindow * window) noexcept = 0;
			virtual void UnregisterMainWindow(IWindow * window) noexcept = 0;
			// Global manupulation with cursor
			virtual UI::Point GetCursorPosition(void) noexcept = 0;
			virtual void SetCursorPosition(UI::Point position) noexcept = 0;
			virtual ICursor * LoadCursor(Codec::Frame * source) noexcept = 0;
			virtual ICursor * GetSystemCursor(SystemCursorClass cursor) noexcept = 0;
			virtual void SetCursor(ICursor * cursor) noexcept = 0;
			// Miscellaneous
			virtual Array<UI::Point> * GetApplicationIconSizes(void) noexcept = 0;
			virtual void SetApplicationIcon(Codec::Image * icon) noexcept = 0;
			virtual void SetApplicationBadge(const string & text) noexcept = 0;
			virtual void SetApplicationIconVisibility(bool visible) noexcept = 0;
			// Starting system dialogs
			virtual bool OpenFileDialog(OpenFileInfo * info, IWindow * parent, IDispatchTask * on_exit) noexcept = 0;
			virtual bool SaveFileDialog(SaveFileInfo * info, IWindow * parent, IDispatchTask * on_exit) noexcept = 0;
			virtual bool ChooseDirectoryDialog(ChooseDirectoryInfo * info, IWindow * parent, IDispatchTask * on_exit) noexcept = 0;
			virtual bool MessageBox(MessageBoxResult * result, const string & text, const string & title, IWindow * parent, MessageBoxButtonSet buttons, MessageBoxStyle style, IDispatchTask * on_exit) noexcept = 0;
			// Menu control
			virtual IMenu * CreateMenu(void) noexcept = 0;
			virtual IMenuItem * CreateMenuItem(void) noexcept = 0;
			// Notification control
			virtual UI::Point GetUserNotificationIconSize(void) noexcept = 0;
			virtual void PushUserNotification(const string & title, const string & text, Codec::Image * icon = 0) noexcept = 0;
			// Status icon control
			virtual IStatusBarIcon * CreateStatusBarIcon(void) noexcept = 0;
			// Global hot key control
			virtual bool CreateHotKey(int event_id, int key_code, uint key_flags) noexcept = 0;
			virtual void RemoveHotKey(int event_id) noexcept = 0;
			// Interprocess communication / Dynamic data exchange
			virtual bool LaunchIPCServer(const string & app_id, const string & auth_id) noexcept = 0;
			virtual IIPCClient * CreateIPCClient(const string & server_app_id, const string & server_auth_id) noexcept = 0;
		};

		ObjectArray<IScreen> * GetActiveScreens(void);
		IScreen * GetDefaultScreen(void);
		ITheme * GetCurrentTheme(void);
		IWindowSystem * GetWindowSystem(void);
	}
}