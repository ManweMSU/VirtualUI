#include "NativeStation.h"

@import Foundation;
@import AppKit;

#include "CocoaInterop.h"
#include "QuartzDevice.h"

using namespace Engine::UI;

static void RenderStationContent(Engine::UI::WindowStation * station);
static NSWindow * GetStationWindow(Engine::UI::WindowStation * station);

@interface EngineRuntimeContentView : NSView
{
    Engine::UI::WindowStation * station;
}
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station;
- (void) drawRect : (NSRect) dirtyRect;
- (BOOL) acceptsFirstMouse: (NSEvent *) event;
- (void) setFrame : (NSRect) frame;
- (void) setFrameSize: (NSSize) newSize;

- (void) mouseMoved: (NSEvent *) event;
- (void) mouseDragged: (NSEvent *) event;
- (void) mouseDown: (NSEvent *) event;
- (void) mouseUp: (NSEvent *) event;
- (void) rightMouseDragged: (NSEvent *) event;
- (void) rightMouseDown: (NSEvent *) event;
- (void) rightMouseUp: (NSEvent *) event;
//- (void) keyDown: (NSEvent *) event;
// wheel and keyboard !
@property(readonly) BOOL acceptsFirstResponder;
@end
@interface EngineRuntimeWindowDelegate : NSObject<NSWindowDelegate>
{
    Engine::UI::WindowStation * station;
}
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station;
- (BOOL) windowShouldClose: (NSWindow *) sender;
- (void) windowDidBecomeKey: (NSNotification *) notification;
- (void) windowDidResignKey: (NSNotification *) notification;
@end

@implementation EngineRuntimeContentView
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station
{
    [self init];
    station = _station;
    return self;
}
- (void) drawRect : (NSRect) dirtyRect
{
    if (station) {
        RenderStationContent(station);
    }
}
- (BOOL) acceptsFirstMouse: (NSEvent *) event
{
    return YES;
}
- (void) setFrame : (NSRect) frame
{
    [super setFrame: frame];
    double scale = [[self window] backingScaleFactor];
    station->SetBox(Engine::UI::Box(0, 0, double(frame.size.width * scale), double(frame.size.height * scale)));
    station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Move);
    [self setNeedsDisplay: YES];
}
- (void) setFrameSize: (NSSize) newSize
{
    [super setFrameSize: newSize];
    double scale = [[self window] backingScaleFactor];
    station->SetBox(Engine::UI::Box(0, 0, double(newSize.width * scale), double(newSize.height * scale)));
    station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Move);
    [self setNeedsDisplay: YES];
}
- (void) mouseMoved: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"MOUSE MOVE: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (void) mouseDragged: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"MOUSE DRAG: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (void) mouseDown: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"MOUSE DOWN: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (void) mouseUp: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"MOUSE UP: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (void) rightMouseDragged: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"RIGHT MOUSE DRAG: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (void) rightMouseDown: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"RIGHT MOUSE DOWN: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (void) rightMouseUp: (NSEvent *) event
{
    auto pos = [NSEvent mouseLocation];
    Engine::SafePointer<Engine::Streaming::FileStream> ConsoleOutStream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
    Engine::Streaming::TextWriter Console(ConsoleOutStream);
    Console << Engine::string(L"RIGHT MOUSE UP: ") + Engine::string(pos.x) + L" : " + Engine::string(pos.y) + Engine::IO::NewLineChar;
}
- (BOOL) acceptsFirstResponder
{
    return YES;
}
@end
@implementation EngineRuntimeWindowDelegate
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station
{
    [super init];
    station = _station;
    return self;
}
- (BOOL) windowShouldClose: (NSWindow *) sender
{
    station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Close);
    return NO;
}
- (void) windowDidBecomeKey: (NSNotification *) notification
{
    station->FocusChanged(true);
    station->CaptureChanged(true);
    [[GetStationWindow(station) contentView] setNeedsDisplay: YES];
}
- (void) windowDidResignKey: (NSNotification *) notification
{
    station->FocusChanged(false);
    station->CaptureChanged(false);
    [[GetStationWindow(station) contentView] setNeedsDisplay: YES];
}
@end

// LRESULT WINAPI WindowCallbackProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam)
// {
//     NativeStation * station = reinterpret_cast<NativeStation *>(GetWindowLongPtrW(Wnd, 0));
//     LRESULT Result = 0;
//     if (Msg == WM_TIMER) {
//         Result = station->ProcessWindowEvents(Msg, WParam, LParam);
//         InvalidateRect(Wnd, 0, FALSE);
//         return Result;
//     } else if (Msg == WM_MEASUREITEM) {
//         LPMEASUREITEMSTRUCT mis = reinterpret_cast<LPMEASUREITEMSTRUCT>(LParam);
//         if (mis->CtlType == ODT_MENU) {
//             auto item = reinterpret_cast<Menues::MenuElement *>(mis->itemData);
//             mis->itemWidth = item->GetWidth();
//             mis->itemHeight = item->GetHeight();
//         }
//         Result = 1;
//     } else if (Msg == WM_DRAWITEM) {
//         LPDRAWITEMSTRUCT dis = reinterpret_cast<LPDRAWITEMSTRUCT>(LParam);
//         if (dis->CtlType == ODT_MENU) {
//             auto item = reinterpret_cast<Menues::MenuElement *>(dis->itemData);
//             auto box = Box(0, 0, dis->rcItem.right - dis->rcItem.left, dis->rcItem.bottom - dis->rcItem.top);
//             auto target = static_cast<ID2D1DCRenderTarget *>(static_cast<Direct2D::D2DRenderDevice *>(item->GetRenderingDevice())->GetRenderTarget());
//             target->BindDC(dis->hDC, &dis->rcItem);
//             target->BeginDraw();
//             if (dis->itemState & ODS_SELECTED) {
//                 item->Render(box, true);
//             } else {
//                 item->Render(box, false);
//             }
//             target->EndDraw();
//         }
//         Result = 1;
//     } else {
//         if (station) Result = station->ProcessWindowEvents(Msg, WParam, LParam);
//         else Result = DefWindowProcW(Wnd, Msg, WParam, LParam);
//     }
//     if (Msg == WM_MOUSEMOVE || Msg == WM_LBUTTONDOWN || Msg == WM_LBUTTONUP || Msg == WM_LBUTTONDBLCLK ||
//         Msg == WM_RBUTTONDOWN || Msg == WM_RBUTTONUP || Msg == WM_RBUTTONDBLCLK ||
//         Msg == WM_MOUSEWHEEL || Msg == WM_MOUSEHWHEEL || Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN ||
//         Msg == WM_KEYUP || Msg == WM_SYSKEYUP || Msg == WM_CHAR) {
//         InvalidateRect(Wnd, 0, FALSE);
//     }
//     return Result;
// }
// eint HandleWindowStation::ProcessWindowEvents(uint32 Msg, eint WParam, eint LParam)
// {
//     if (Msg == WM_KEYDOWN || Msg == WM_SYSKEYDOWN) {
//         if (WParam == VK_SHIFT) {
//             if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyDown(KeyCodes::LeftShift);
//             else KeyDown(KeyCodes::RightShift);
//         } else if (WParam == VK_CONTROL) {
//             if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyDown(KeyCodes::LeftControl);
//             else KeyDown(KeyCodes::RightControl);
//         } else if (WParam == VK_MENU) {
//             if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyDown(KeyCodes::LeftAlternative);
//             else KeyDown(KeyCodes::RightAlternative);
//         } else KeyDown(int32(WParam));
//     } else if (Msg == WM_KEYUP || Msg == WM_SYSKEYUP) {
//         if (WParam == VK_SHIFT) {
//             if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyUp(KeyCodes::LeftShift);
//             else KeyUp(KeyCodes::RightShift);
//         } else if (WParam == VK_CONTROL) {
//             if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyUp(KeyCodes::LeftControl);
//             else KeyUp(KeyCodes::RightControl);
//         } else if (WParam == VK_MENU) {
//             if (MapVirtualKeyW((LParam & 0xFF0000) >> 16, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT) KeyUp(KeyCodes::LeftAlternative);
//             else KeyUp(KeyCodes::RightAlternative);
//         } else KeyUp(int32(WParam));
//     } else if (Msg == WM_CHAR) {
//         if ((WParam & 0xFC00) == 0xD800) {
//             _surrogate = ((WParam & 0x3FF) << 10) + 0x10000;
//         } else if ((WParam & 0xFC00) == 0xDC00) {
//             _surrogate |= (WParam & 0x3FF) + 0x10000;
//             CharDown(_surrogate);
//             _surrogate = 0;
//         } else {
//             _surrogate = 0;
//             CharDown(uint32(WParam));
//         }
//         return FALSE;
//     } else if (Msg == WM_MOUSEMOVE) {
//         POINTS p = MAKEPOINTS(LParam);
//         MouseMove(Point(p.x, p.y));
//     } else if (Msg == WM_LBUTTONDOWN) {
//         POINTS p = MAKEPOINTS(LParam);
//         LeftButtonDown(Point(p.x, p.y));
//     } else if (Msg == WM_LBUTTONUP) {
//         POINTS p = MAKEPOINTS(LParam);
//         LeftButtonUp(Point(p.x, p.y));
//     } else if (Msg == WM_LBUTTONDBLCLK) {
//         POINTS p = MAKEPOINTS(LParam);
//         LeftButtonDoubleClick(Point(p.x, p.y));
//     } else if (Msg == WM_RBUTTONDOWN) {
//         POINTS p = MAKEPOINTS(LParam);
//         RightButtonDown(Point(p.x, p.y));
//     } else if (Msg == WM_RBUTTONUP) {
//         POINTS p = MAKEPOINTS(LParam);
//         RightButtonUp(Point(p.x, p.y));
//     } else if (Msg == WM_RBUTTONDBLCLK) {
//         POINTS p = MAKEPOINTS(LParam);
//         RightButtonDoubleClick(Point(p.x, p.y));
//     } else if (Msg == WM_MOUSEWHEEL) {
//         ScrollVertically(-double(GET_WHEEL_DELTA_WPARAM(WParam)) * 3.0 / double(WHEEL_DELTA));
//     } else if (Msg == WM_MOUSEHWHEEL) {
//         ScrollHorizontally(double(GET_WHEEL_DELTA_WPARAM(WParam)) * 3.0 / double(WHEEL_DELTA));
//     } else if (Msg == WM_TIMER) {
//         int index = WParam - 2;
//         if (index >= 0 && index < _timers.Length() && _timers[index]) _timers[index]->Timer();
//     }
//     return DefWindowProcW(_window, Msg, WParam, LParam);
// }

namespace Engine
{
	namespace NativeWindows
	{
        class NativeStation : public WindowStation
		{
			NSWindow * _window;
			SafePointer<ICursor> _null;
			SafePointer<ICursor> _arrow;
			SafePointer<ICursor> _beam;
			SafePointer<ICursor> _link;
			SafePointer<ICursor> _size_left_right;
			SafePointer<ICursor> _size_up_down;
			SafePointer<ICursor> _size_left_up_right_down;
			SafePointer<ICursor> _size_left_down_right_up;
			SafePointer<ICursor> _size_all;
			Array<Window *> _timers;
			SafePointer<Cocoa::QuartzRenderingDevice> RenderingDevice;
			Window::RefreshPeriod InternalRate = Window::RefreshPeriod::None;
        public:
            class DesktopWindowFactory : public WindowStation::IDesktopWindowFactory
			{
				Template::ControlTemplate * _template = 0;
			public:
				DesktopWindowFactory(Template::ControlTemplate * Template) : _template(Template) {}
				virtual Window * CreateDesktopWindow(WindowStation * Station) override
				{
					return new Controls::OverlappedWindow(0, Station, _template);
				}
			};
		
            NativeStation(NSWindow * window, WindowStation::IDesktopWindowFactory * factory) : _window(window), WindowStation(factory)
            {
                RenderingDevice = new Cocoa::QuartzRenderingDevice;
                SetRenderingDevice(RenderingDevice);
                // cursors and another shit
            }
			virtual ~NativeStation(void) override
            {
                // release cocoa objects
            }
			virtual bool IsNativeStationWrapper(void) const override { return true; }
			virtual void SetFocus(Window * window) override { if ([_window isKeyWindow]) WindowStation::SetFocus(window); }
			virtual Window * GetFocus(void) override { if ([_window isKeyWindow]) return WindowStation::GetFocus(); else return 0; }
			virtual void SetCapture(Window * window) override { if ([_window isKeyWindow]) WindowStation::SetCapture(window); }
			virtual Window * GetCapture(void) override { if ([_window isKeyWindow]) return WindowStation::GetCapture(); else return 0; }
			virtual void ReleaseCapture(void) override { WindowStation::SetCapture(0); }
			virtual void SetExclusiveWindow(Window * window) override { if ([_window isKeyWindow]) WindowStation::SetExclusiveWindow(window); }
			virtual Window * GetExclusiveWindow(void) override { if ([_window isKeyWindow]) return WindowStation::GetExclusiveWindow(); else return 0; }
			virtual UI::Point GetCursorPos(void) override
            {
                return UI::Point(0, 0);
            }
			virtual bool NativeHitTest(const UI::Point & at) override
            {
                return false;
            }
			virtual ICursor * LoadCursor(Streaming::Stream * Source) override
            {
                return 0;
            }
			virtual ICursor * LoadCursor(Codec::Image * Source) override
            {
                return 0;
            }
			virtual ICursor * LoadCursor(Codec::Frame * Source) override
            {
                return 0;
            }
			virtual ICursor * GetSystemCursor(SystemCursor cursor) override
            {
                return 0;
            }
			virtual void SetSystemCursor(SystemCursor entity, ICursor * cursor) override
            {

            }
			virtual void SetCursor(ICursor * cursor) override
            {

            }
			virtual void SetTimer(Window * window, uint32 period) override
            {
                
            }

			virtual void OnDesktopDestroy(void) override { [_window close]; DestroyStation(); }
			virtual void RequireRefreshRate(Window::RefreshPeriod period) override { InternalRate = period; AnimationStateChanged(); }
			virtual Window::RefreshPeriod GetRefreshRate(void) override { return InternalRate; }
			virtual void AnimationStateChanged(void) override
			{
				// int fr = int(GetFocus() ? GetFocus()->FocusedRefreshPeriod() : Window::RefreshPeriod::None);
				// int ar = int(IsPlayingAnimation() ? Window::RefreshPeriod::Cinematic : Window::RefreshPeriod::None);
				// int ur = int(InternalRate);
				// int mr = max(max(fr, ar), ur);
				// if (!mr) KillTimer(GetHandle(), 1); else {
				// 	if (mr == 1) ::SetTimer(GetHandle(), 1, GetRenderingDevice()->GetCaretBlinkHalfTime(), 0);
				// 	else if (mr == 2) {
				// 		if (::GetActiveWindow() == GetHandle()) ::SetTimer(GetHandle(), 1, 25, 0);
				// 		else ::SetTimer(GetHandle(), 1, 100, 0);
				// 	}
				// }
			}
			virtual void FocusWindowChanged(void) override { [[_window contentView] setNeedsDisplay: YES]; AnimationStateChanged(); }

			void RenderContent(void)
			{
				if (RenderingDevice) {
                    auto rect = [[_window contentView] frame];
                    double scale = [_window backingScaleFactor];
                    UI::Box box = UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
                    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
                    RenderingDevice->SetContext(context, box.Right, box.Bottom, (scale > 1.5f) ? 2 : 1);
                    RenderingDevice->SetTimerValue(GetTimerValue());
                    Render();
				}
			}
			NSWindow * GetWindow(void) const { return _window; }
		};

        bool _Initialized = false;
		void InitializeWindowSystem(void)
        {
            if (!_Initialized) {
                [NSApplication sharedApplication];
                NSMenu * menu = [[NSMenu alloc] initWithTitle: @"Main Menu"];
                NSMenuItem * main_item = [[NSMenuItem alloc] initWithTitle: @"Application Menu" action: NULL keyEquivalent: @""];
                NSMenu * main_menu = [[NSMenu alloc] initWithTitle: @"Main Menu"];
                NSMenuItem * about = [[NSMenuItem alloc] initWithTitle: @"About" action: @selector(orderFrontStandardAboutPanel:) keyEquivalent: @""];
                NSMenuItem * about_sep = [NSMenuItem separatorItem];
                [main_item setSubmenu: main_menu];
                [main_menu addItem: about];
                [main_menu addItem: about_sep];
                NSMenuItem * services_items = [[NSMenuItem alloc] initWithTitle: @"Services" action: NULL keyEquivalent: @""];
                NSMenu * services_menu = [[NSMenu alloc] initWithTitle: @"Services Menu"];
                NSMenuItem * services_sep = [NSMenuItem separatorItem];
                NSMenuItem * hide_app = [[NSMenuItem alloc] initWithTitle: @"Hide" action: @selector(hide:) keyEquivalent: @"h"];
                [services_items setSubmenu: services_menu];
                [main_menu addItem: services_items];
                [main_menu addItem: services_sep];
                [main_menu addItem: hide_app];
                NSMenuItem * hide_others = [[NSMenuItem alloc] initWithTitle: @"Hide others" action: @selector(hideOtherApplications:) keyEquivalent: @"h"];
                [main_menu addItem: hide_others];
                [hide_others setKeyEquivalentModifierMask: NSEventModifierFlagOption | NSEventModifierFlagCommand];
                NSMenuItem * show_all = [[NSMenuItem alloc] initWithTitle: @"Show all" action: @selector(unhideAllApplications:) keyEquivalent: @""];
                NSMenuItem * show_sep = [NSMenuItem separatorItem];
                NSMenuItem * item_exit = [[NSMenuItem alloc] initWithTitle: @"Exit" action: @selector(terminate:) keyEquivalent: @"q"]; // another selector?
                [main_menu addItem: show_all];
                [main_menu addItem: show_sep];
                [main_menu addItem: item_exit];

                [menu addItem: main_item];
                [NSApp setMainMenu: menu];
                [NSApp setServicesMenu: services_menu];

                [menu release];
                [main_item release];
                [main_menu release];
                [about release];
                [about_sep release];
                [services_items release];
                [services_menu release];
                [services_sep release];
                [hide_app release];
                [hide_others release];
                [show_all release];
                [show_sep release];
                [item_exit release];

                _Initialized = true;
            }
        }

		UI::WindowStation * CreateOverlappedWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation)
        {
            InitializeWindowSystem();
            NSScreen * screen = [NSScreen mainScreen];
            CGRect desktop_rect = [screen frame];
            Box parent_box = ParentStation ? GetWindowPosition(ParentStation) : GetScreenDimensions();
            if (Position.IsValid()) parent_box = Box(Position, parent_box);
            Box client_box(Template->Properties->ControlPosition, parent_box);
            auto props = static_cast<Template::Controls::DialogFrame *>(Template->Properties);
            NSWindowStyleMask style = 0;
            if (props->Captioned) style |= NSWindowStyleMaskTitled;
            if (props->CloseButton) style |= NSWindowStyleMaskClosable;
			if (props->MinimizeButton) style |= NSWindowStyleMaskMiniaturizable;
			if (props->ToolWindow) style |= NSWindowStyleMaskUtilityWindow;
			if (props->Sizeble) style |= NSWindowStyleMaskResizable;
            double scale = GetScreenScale();
            CGRect window_rect = NSMakeRect(0, 0, double(client_box.Right - client_box.Left) / scale, double(client_box.Bottom - client_box.Top) / scale);
            CGRect minimal_rect = NSMakeRect(0, 0, double(props->MinimalWidth) / scale, double(props->MinimalHeight) / scale);
            CGRect parent_rect = NSMakeRect(double(parent_box.Left) / scale, desktop_rect.size.height - double(parent_box.Bottom) / scale,
                double(parent_box.Right - parent_box.Left) / scale, double(parent_box.Bottom - parent_box.Top) / scale);
            window_rect.origin.x = parent_rect.origin.x + (parent_rect.size.width - window_rect.size.width) / 2.0;
            window_rect.origin.y = parent_rect.origin.y + (parent_rect.size.height - window_rect.size.height) / 2.0;
            NSWindow * window = [[NSWindow alloc] initWithContentRect: window_rect styleMask: style backing: NSBackingStoreBuffered defer: NO];
            // set parent structure
            NSString * title = Cocoa::CocoaString(props->Title);
            [window setTitle: title];
            [window setContentMinSize: minimal_rect.size];
            [title release];
            NativeStation::DesktopWindowFactory factory(Template);
            SafePointer<NativeStation> station = new NativeStation(window, &factory);
            EngineRuntimeContentView * view = [[EngineRuntimeContentView alloc] initWithStation: station.Inner()];
            EngineRuntimeWindowDelegate * delegate = [[EngineRuntimeWindowDelegate alloc] initWithStation: station.Inner()];
            [window setAcceptsMouseMovedEvents: YES];
            [window setDelegate: delegate];
            [window setContentView: view];
            [view release];
            [delegate release];
			Controls::OverlappedWindow * local_desktop = station->GetDesktop()->As<Controls::OverlappedWindow>();
			local_desktop->GetContentFrame()->SetRectangle(UI::Rectangle::Entire());
			if (!props->Background) {
				Color BackgroundColor = 0;
				if (!props->DefaultBackground) {
					BackgroundColor = props->BackgroundColor;
				}
				SafePointer<Template::BarShape> Background = new Template::BarShape;
				Background->Position = UI::Rectangle::Entire();
				Background->Gradient << Template::GradientPoint(Template::ColorTemplate(BackgroundColor), 0.0);
				local_desktop->SetBackground(Background);
			}
            station->SetBox(UI::Box(0, 0, client_box.Right - client_box.Left, client_box.Bottom - client_box.Top));
            station->Retain();
			return station;
        }
		void ShowWindow(UI::WindowStation * Station, bool Show)
        {
            if (Show) {
                [static_cast<NativeStation *>(Station)->GetWindow() orderFrontRegardless];
            } else {
                [static_cast<NativeStation *>(Station)->GetWindow() orderOut: nil];
            }     
        }
		void EnableWindow(UI::WindowStation * Station, bool Enable) {}
		void SetWindowTitle(UI::WindowStation * Station, const string & Title)
        {
            NSString * title = Cocoa::CocoaString(Title);
            [static_cast<NativeStation *>(Station)->GetWindow() setTitle: title];
            [title release];
        }
		void SetWindowPosition(UI::WindowStation * Station, const UI::Box & position)
        {
            NSScreen * screen = [NSScreen mainScreen];
            CGRect srect = [screen frame];
            double scale = [screen backingScaleFactor];
            [static_cast<NativeStation *>(Station)->GetWindow()
                setFrame: NSMakeRect(double(position.Left) / scale, srect.size.height - double(position.Bottom) / scale,
                double(position.Right - position.Left) / scale, double(position.Bottom - position.Top) / scale) display: YES];
        }
		bool IsWindowVisible(UI::WindowStation * Station)
        {
            return [static_cast<NativeStation *>(Station)->GetWindow() isVisible] != 0;
        }
		bool IsWindowEnabled(UI::WindowStation * Station) { return true; }
		string GetWindowTitle(UI::WindowStation * Station)
        {
            NSString * title = [static_cast<NativeStation *>(Station)->GetWindow() title];
            return Cocoa::EngineString(title);
        }
		UI::Box GetWindowPosition(UI::WindowStation * Station)
        {
            NSScreen * screen = [NSScreen mainScreen];
            CGRect wrect = [static_cast<NativeStation *>(Station)->GetWindow() frame];
            CGRect srect = [screen frame];
            double scale = [screen backingScaleFactor];
            int x = int(wrect.origin.x * scale);
            int y = int((srect.size.height - wrect.origin.y - wrect.size.height) * scale);
            return UI::Box(x, y, x + int(wrect.size.width * scale), y + int(wrect.size.height * scale));
        }
		int RunMenuPopup(UI::Menues::Menu * menu, UI::Window * owner, UI::Point at)
        {
            return 0;
        }
		UI::Box GetScreenDimensions(void)
        {
            NSScreen * screen = [NSScreen mainScreen];
            CGRect rect = [screen frame];
            double scale = [screen backingScaleFactor];
            return UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
        }
		double GetScreenScale(void)
        {
            NSScreen * screen = [NSScreen mainScreen];
            double scale = [screen backingScaleFactor];
            return scale;
        }
		void RunMainMessageLoop(void)
        {
            [NSApp run];
        }
		int RunModalDialog(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, UI::Window * Parent)
        {
            return 0;
        }
		void ExitModal(int code)
        {

        }
		void ExitMainLoop(void)
        {
            [NSApp stop: nil];
        }
	}
}

static void RenderStationContent(Engine::UI::WindowStation * station)
{
    static_cast<Engine::NativeWindows::NativeStation *>(station)->RenderContent();
}
static NSWindow * GetStationWindow(Engine::UI::WindowStation * station)
{
    return static_cast<Engine::NativeWindows::NativeStation *>(station)->GetWindow();
}