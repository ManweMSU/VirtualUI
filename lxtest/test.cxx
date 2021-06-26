#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::IO;
using namespace Engine::Network;
using namespace Engine::Windows;
using namespace Engine::Clipboard;

Console console;
SafePointer<Codec::Frame> frame;
SafePointer<ICursor> cursor;

class TestWndCallback : public Windows::IWindowCallback
{
	bool modal = false;
public:
	// Fundamental callback
	virtual void Created(IWindow * window) {
		console.WriteLine(FormatString(L"Created: %0", string((void*) window)));
		console.WriteLine(FormatString(L"Background flags: %0", window->GetBackgroundFlags()));
		window->SetBackbufferedRenderingDevice(frame, ImageRenderMode::FitKeepAspectRatio, 0x40FF0080);
	}
	virtual void Destroyed(IWindow * window) {
		console.WriteLine(FormatString(L"Destroyed: %0", string((void*) window)));
		delete this;
	}
	virtual void Shown(IWindow * window, bool show) { console.WriteLine(FormatString(L"Shown: %0, %1", string((void*) window), show)); }
	// // Frame event callback
	virtual void WindowClose(IWindow * window) {
		console.WriteLine(FormatString(L"Close: %0", string((void*) window)));
		if (modal) GetWindowSystem()->ExitModalSession(window);
		else {
			GetWindowSystem()->UnregisterMainWindow(window);
			window->Destroy();
		}
	}
	virtual void WindowHelp(IWindow * window) override {
		console.WriteLine(FormatString(L"Help: %0", string((void*) window)));
	}
	// // Keyboard events
	virtual void FocusChanged(IWindow * window, bool got) { console.WriteLine(FormatString(L"Focus changed: %0, %1", string((void*) window), got)); }
	virtual bool KeyDown(IWindow * window, int key_code)
	{
		console.WriteLine(FormatString(L"Key down: %0, %1", string((void*) window), string(uint(key_code), HexadecimalBase, 4)));
		if (key_code == KeyCodes::V) {
			if (IsFormatAvailable(Format::Image)) {
				SafePointer<Codec::Frame> frame;
				GetData(frame.InnerRef());
				if (frame && window) {
					window->SetBackbufferedRenderingDevice(frame, ImageRenderMode::FitKeepAspectRatio, 0x40FF0080);
				}
				console.WriteLine(L"Clipboard image exported");
			}
		} else if (key_code == KeyCodes::C) {
			auto callback = new TestWndCallback;
			CreateWindowDesc desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Flags = WindowFlagCloseButton | WindowFlagSizeble | WindowFlagHasTitle | WindowFlagBlurBehind;
			desc.Title = L"A child window";
			desc.Position = UI::Box(100, 100, 300, 200);
			desc.MinimalConstraints = UI::Point(200, 100);
			desc.ParentWindow = window;
			desc.Callback = callback;
			auto window = GetWindowSystem()->CreateWindow(desc);
			window->Show(true);
		} else if (key_code == KeyCodes::M) {
			auto callback = new TestWndCallback;
			callback->modal = true;
			CreateWindowDesc desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Flags = WindowFlagCloseButton | WindowFlagSizeble | WindowFlagHasTitle;
			desc.Title = L"A locally modal window";
			desc.Position = UI::Box(100, 100, 300, 200);
			desc.MinimalConstraints = UI::Point(200, 100);
			desc.ParentWindow = window;
			desc.Callback = callback;
			auto window = GetWindowSystem()->CreateModalWindow(desc);
		} else if (key_code == KeyCodes::G) {
			auto callback = new TestWndCallback;
			callback->modal = true;
			CreateWindowDesc desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.Flags = WindowFlagCloseButton | WindowFlagSizeble | WindowFlagHasTitle | WindowFlagBlurBehind;
			desc.Title = L"A globally modal window";
			desc.Position = UI::Box(100, 100, 300, 200);
			desc.MinimalConstraints = UI::Point(200, 100);
			desc.Callback = callback;
			auto window = GetWindowSystem()->CreateModalWindow(desc);
		} else if (key_code == KeyCodes::T) {
			window->SetOpacity(0.6);
		} else if (key_code == KeyCodes::N) {
			window->SetOpacity(1.0);
		} else if (key_code == KeyCodes::D) {
			window->SetPresentationEngine(0);
		}
		return false;
	}
	virtual void KeyUp(IWindow * window, int key_code)
	{
		console.WriteLine(FormatString(L"Key up: %0, %1", string((void*) window), string(uint(key_code), HexadecimalBase, 4)));
	}
	virtual void CharDown(IWindow * window, uint32 ucs_code)
	{
		string input = string(&ucs_code, 1, Encoding::UTF32);
		console.WriteLine(FormatString(L"Character input: %0, %1", string((void*) window), input));
	}
	// // Mouse events
	virtual void SetCursor(IWindow * window, UI::Point at)
	{
		console.WriteLine(FormatString(L"Set cursor: %0, %1, %2", string((void*) window), at.x, at.y));
		if (at.x < 100 && at.y < 100) {
			GetWindowSystem()->SetCursor(GetWindowSystem()->GetSystemCursor(SystemCursorClass::Null));
		} else if (at.x >= 100 && at.y < 100) {
			GetWindowSystem()->SetCursor(GetWindowSystem()->GetSystemCursor(SystemCursorClass::SizeLeftDownRightUp));
		} else if (at.x < 100 && at.y >= 100) {
			GetWindowSystem()->SetCursor(GetWindowSystem()->GetSystemCursor(SystemCursorClass::SizeLeftUpRightDown));
		} else {
			GetWindowSystem()->SetCursor(cursor);
		}
	}
	virtual void MouseMove(IWindow * window, UI::Point at)
	{
		console.WriteLine(FormatString(L"Mouse move: %0, %1, %2", string((void*) window), at.x, at.y));
		auto global = window->PointClientToGlobal(at);
		auto local = window->PointGlobalToClient(global);
		auto alt_global = GetWindowSystem()->GetCursorPosition();
		console.WriteLine(FormatString(L"[%4, %5] (%0, %1) --> (%2, %3)", global.x, global.y, local.x, local.y, alt_global.x, alt_global.y));
		console.WriteLine(FormatString(L"Hit test: %0", window->PointHitTest(UI::Point(100, 100))));
	}
};
class TestAppCallback : public Windows::IApplicationCallback
{
public:
	virtual bool IsHandlerEnabled(ApplicationHandler event) { return event == ApplicationHandler::CreateFile; }
	virtual void CreateNewFile(void) {
		auto callback = new TestWndCallback;
		CreateWindowDesc desc;
		ZeroMemory(&desc, sizeof(desc));
		desc.Flags = WindowFlagCloseButton | WindowFlagHelpButton | WindowFlagSizeble | WindowFlagHasTitle | WindowFlagBlurBehind;
		desc.Title = L"Сука привет";
		desc.Position = UI::Box(100, 100, 500, 400);
		desc.MinimalConstraints = UI::Point(200, 100);
		desc.Callback = callback;
		auto window = GetWindowSystem()->CreateWindow(desc);
		window->Show(true);
		GetWindowSystem()->RegisterMainWindow(window);
	}
};

int Main(void)
{
	console.WriteLine(Engine::IO::GetExecutablePath());

	Codec::InitializeDefaultCodecs();

	SafePointer<Streaming::Stream> saryu = new Streaming::FileStream(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/saryu.png", Streaming::AccessRead, Streaming::OpenExisting);
	frame = Codec::DecodeFrame(saryu);

	SafePointer<Codec::Frame> src = new Codec::Frame(64, 64, Codec::PixelFormat::R8G8B8A8);
	for (int j = 0; j < 64; j++) for (int i = 0; i < 64; i++) {
		src->SetPixel(i, j, (i + j) & 1 ? 0xFFFF0080 : 0x4000FF00);
	}

	src->HotPointX = 32;
	src->HotPointY = 16;
	cursor = Windows::GetWindowSystem()->LoadCursor(src);
	TestAppCallback callback;
	Windows::GetWindowSystem()->SetCallback(&callback);
	Windows::GetWindowSystem()->RunMainLoop();

	// TODO: IMPLEMENT

	// SystemWindows.cpp - windows
	// SystemGraphics.cpp - hardware graphics
	// SystemVideo.cpp
	// SystemAudio.cpp
	// SystemPower.cpp

	// TODO: TRY IMPLEMENT

	// Progress bar in task bar

	return 0;
}