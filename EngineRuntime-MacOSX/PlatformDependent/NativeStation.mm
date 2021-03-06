#include "NativeStation.h"

@import Foundation;
@import AppKit;

#include "CocoaInterop.h"
#include "QuartzDevice.h"
#include "MetalDevice.h"
#include "MetalGraphics.h"
#include "CocoaKeyCodes.h"
#include "KeyCodes.h"
#include "AppleCodec.h"
#include "NativeStationBackdoors.h"
#include "Application.h"
#include "ApplicationBackdoors.h"
#include "../ImageCodec/IconCodec.h"
#include "../Storage/ImageVolume.h"
#include "Assembly.h"
#include "../PlatformSpecific/MacWindowEffects.h"
#include "../PlatformSpecific/MacTouchBar.h"

using namespace Engine::UI;

Engine::Dictionary::PlainDictionary<Engine::KeyCodes::Key, bool> __engine_keyboard_status(0x100);

static void RenderStationContent(Engine::UI::WindowStation * station);
static Engine::MetalGraphics::MetalPresentationInterface * GetStationPresentationInterface(Engine::UI::WindowStation * station);
static NSWindow * GetStationWindow(Engine::UI::WindowStation * station);
static void ScreenToView(double sx, double sy, NSView * view, int & ox, int & oy)
{
	double scale = [[view window] backingScaleFactor];
	CGRect rect = NSMakeRect(sx, sy, 0.0, 0.0);
	CGRect frame = [view frame];
	rect = [view convertRect: [[view window] convertRectFromScreen: rect] fromView: nil];
	ox = int(rect.origin.x * scale);
	oy = int((frame.size.height - rect.origin.y) * scale);
}
static void ViewToScreen(int ox, int oy, NSView * view, double & sx, double & sy)
{
	double scale = [[view window] backingScaleFactor];
	CGRect frame = [view frame];
	CGRect rect = NSMakeRect(double(ox) / scale, frame.size.height - double(oy) / scale, 0.0, 0.0);
	rect = [[view window] convertRectToScreen: [view convertRect: rect toView: nil]];
	sx = rect.origin.x;
	sy = rect.origin.y;
}

@interface PopupWindow : NSPanel
@property(readonly) BOOL canBecomeKeyWindow;
- (void) resignKeyWindow;
@end
@interface EngineRuntimeTimerTarget : NSObject
{
@public
	Engine::UI::Window * target;
	NSTimer * timer;
}
- (instancetype) init;
@end
@interface EngineRuntimeEvent : NSObject
{
@public
	Engine::UI::Window * target;
	int operation;
	int identifier;
	Engine::Tasks::ThreadJob * job;
}
- (void) queue_event: (NSView *) view;
@end
@interface EngineRuntimeContentView : NSView
{
@public
	Engine::UI::WindowStation * station;
	id<NSWindowDelegate> window_delegate;
	uint32 last_ldown;
	uint32 last_rdown;
	uint32 dbl;
	int last_x, last_y, fkeys, last_px, last_py;
	bool lwas, rwas, mwas;
	NSObject<NSTextInputClient> * input_client;
	NSTextInputContext * input_context;
}
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station;
- (void) dealloc;
- (void) drawRect: (NSRect) dirtyRect;
- (BOOL) acceptsFirstMouse: (NSEvent *) event;
- (void) setFrame: (NSRect) frame;
- (void) setFrameSize: (NSSize) newSize;
- (void) keyboardStateInactivate;
- (void) require_contents_update;

- (void) mouseMoved: (NSEvent *) event;
- (void) mouseDragged: (NSEvent *) event;
- (void) mouseDown: (NSEvent *) event;
- (void) mouseUp: (NSEvent *) event;
- (void) rightMouseDragged: (NSEvent *) event;
- (void) rightMouseDown: (NSEvent *) event;
- (void) rightMouseUp: (NSEvent *) event;
- (void) keyDown: (NSEvent *) event;
- (void) keyUp: (NSEvent *) event;
- (void) flagsChanged: (NSEvent *) event;
- (void) scrollWheel: (NSEvent *) event;
- (void) timerFireMethod: (NSTimer *) timer;

- (void) engineEvent: (EngineRuntimeEvent *) event;
@property(readonly) BOOL acceptsFirstResponder;
@property(readonly, strong) NSObject * inputContext;
@end
@interface EngineRuntimeInputContext : NSObject<NSTextInputClient>
{
@public
	EngineRuntimeContentView * view;
}
- (BOOL) hasMarkedText;
- (NSRange) markedRange;
- (NSRange) selectedRange;
- (void) setMarkedText: (id) string selectedRange: (NSRange) selectedRange replacementRange: (NSRange) replacementRange;
- (void) unmarkText;
- (NSArray<NSAttributedStringKey> *) validAttributesForMarkedText;
- (NSAttributedString *) attributedSubstringForProposedRange: (NSRange) range actualRange: (NSRangePointer) actualRange;
- (void) insertText: (id) string replacementRange: (NSRange) replacementRange;
- (NSUInteger) characterIndexForPoint: (NSPoint) point;
- (NSRect) firstRectForCharacterRange: (NSRange) range actualRange: (NSRangePointer) actualRange;
- (void) doCommandBySelector: (SEL) selector;
@end
@interface EngineRuntimeWindowDelegate : NSObject<NSWindowDelegate>
{
@public
	Engine::UI::WindowStation * station;
	EngineRuntimeContentView * view;
}
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station;
- (BOOL) windowShouldClose: (NSWindow *) sender;
- (void) windowDidBecomeKey: (NSNotification *) notification;
- (void) windowDidResignKey: (NSNotification *) notification;
- (void) windowWillBeginSheet: (NSNotification *) notification;
- (void) windowDidEndSheet: (NSNotification *) notification;
- (void) windowDidMiniaturize: (NSNotification *) notification;
- (void) windowDidDeminiaturize: (NSNotification *) notification;
- (void) windowDidEnterFullScreen: (NSNotification *) notification;
- (void) windowDidExitFullScreen: (NSNotification *) notification;
- (void) windowDidBecomeMain: (NSNotification *) notification;
- (void) windowDidResignMain: (NSNotification *) notification;
- (void) windowDidMove: (NSNotification *) notification;
@end
EngineRuntimeContentView * __GetEngineViewFromWindow(NSWindow * window)
{
	return ((EngineRuntimeWindowDelegate *) [window delegate])->view;
}
void __SetEngineWindowBackgroundColor(Engine::UI::Window * window, Engine::UI::Color color)
{
	@autoreleasepool {
		[Engine::NativeWindows::GetWindowObject(window->GetStation()) setBackgroundColor: [NSColor colorWithDeviceRed: color.r / 255.0
			green: color.g / 255.0 blue: color.b / 255.0 alpha: color.a / 255.0]];
	}
}
void __SetEngineWindowAlpha(Engine::UI::Window * window, double value)
{
	[Engine::NativeWindows::GetWindowObject(window->GetStation()) setAlphaValue: value];
}
void __SetEngineWindowEffectBackgroundMaterial(Engine::UI::Window * window, long material)
{
	[[Engine::NativeWindows::GetWindowObject(window->GetStation()) contentView] setMaterial: (NSVisualEffectMaterial) material];
}

@implementation PopupWindow : NSPanel
- (BOOL) canBecomeKeyWindow
{
	return YES;
}
- (void) resignKeyWindow
{
	[[self delegate] windowDidResignKey: nil];
}
@end
@implementation EngineRuntimeInputContext
- (BOOL) hasMarkedText { return NO; }
- (NSRange) markedRange { return { NSNotFound, 0 }; }
- (NSRange) selectedRange { return { NSNotFound, 0 }; }
- (void) setMarkedText: (id) string selectedRange: (NSRange) selectedRange replacementRange: (NSRange) replacementRange {}
- (void) unmarkText {}
- (NSArray<NSAttributedStringKey> *) validAttributesForMarkedText { return [NSArray<NSAttributedStringKey> array]; }
- (NSAttributedString *) attributedSubstringForProposedRange: (NSRange) range actualRange: (NSRangePointer) actualRange
{
	auto str = [[NSAttributedString alloc] initWithString: [NSString string]];
	[str autorelease];
	return str;
}
- (void) insertText: (id) string replacementRange: (NSRange) replacementRange
{
	Engine::string text;
	if ([string respondsToSelector: @selector(isEqualToAttributedString:)]) {
		NSString * str = [string string];
		text = Engine::Cocoa::EngineString(str);
	} else {
		NSString * str = string;
		text = Engine::Cocoa::EngineString(str);
	}
	if (text.Length() && view->station) {
		for (int i = 0; i < text.Length(); i++) view->station->CharDown(text[i]);
		[view require_contents_update];
	}
}
- (NSUInteger) characterIndexForPoint: (NSPoint) point { return NSNotFound; }
- (NSRect) firstRectForCharacterRange: (NSRange) range actualRange: (NSRangePointer) actualRange { return NSMakeRect(0.0, 0.0, 0.0, 0.0); }
- (void) doCommandBySelector: (SEL) selector {}
@end
@implementation EngineRuntimeTimerTarget : NSObject
- (instancetype) init
{
	[super init];
	target = 0;
	timer = 0;
	return self;
}
@end
@implementation EngineRuntimeContentView
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station
{
	[self init];
	station = _station;
	last_ldown = 0;
	last_rdown = 0;
	last_x = 0;
	last_y = 0;
	last_px = -1;
	last_py = -1;
	fkeys = 0;
	lwas = false;
	rwas = false;
	mwas = false;
	dbl = uint32([NSEvent doubleClickInterval] * 1000.0);
	auto ic = [[EngineRuntimeInputContext alloc] init];
	input_client = ic;
	input_context = [[NSTextInputContext alloc] initWithClient: ic];
	ic->view = self;
	return self;
}
- (void) dealloc
{
	[window_delegate release];
	[input_context release];
	[input_client release];
	[super dealloc];
}
- (void) drawRect : (NSRect) dirtyRect
{
	if (station && !GetStationPresentationInterface(station)) {
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
	if (!station) return;
	double scale = [[self window] backingScaleFactor];
	station->SetBox(Engine::UI::Box(0, 0, double(frame.size.width * scale), double(frame.size.height * scale)));
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Move);
	[self require_contents_update];
}
- (void) setFrameSize: (NSSize) newSize
{
	[super setFrameSize: newSize];
	if (!station) return;
	double scale = [[self window] backingScaleFactor];
	station->SetBox(Engine::UI::Box(0, 0, double(newSize.width * scale), double(newSize.height * scale)));
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Move);
	[self require_contents_update];
}
- (void) keyboardStateInactivate
{
	fkeys = 0;
}
- (void) require_contents_update
{
	if (!station) return;
	if (GetStationPresentationInterface(station)) GetStationPresentationInterface(station)->InvalidateContents();
	else [self setNeedsDisplay: YES];
}
- (void) mouseMoved: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		if (x != last_x || y != last_y || !mwas) {
			last_x = x;
			last_y = y;
			mwas = true;
			station->MouseMove(Engine::UI::Point(x, y));
			[self require_contents_update];
		}
	}
}
- (void) mouseDragged: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		if (x != last_x || y != last_y || !mwas) {
			last_x = x;
			last_y = y;
			mwas = true;
			station->MouseMove(Engine::UI::Point(x, y));
			[self require_contents_update];
		}
	}
}
- (void) mouseDown: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		if (x != last_x || y != last_y) [self mouseMoved: event];
		auto time = Engine::GetTimerValue();
		if (lwas && (time - last_ldown) < dbl && x == last_px && y == last_py) {
			lwas = false;
			station->LeftButtonDoubleClick(Engine::UI::Point(x, y));
			[self require_contents_update];
		} else {
			lwas = true;
			last_ldown = time;
			station->LeftButtonDown(Engine::UI::Point(x, y));
			[self require_contents_update];
		}
		last_px = x; last_py = y;
	}
}
- (void) mouseUp: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		station->LeftButtonUp(Engine::UI::Point(x, y));
		[self require_contents_update];
	}
}
- (void) rightMouseDragged: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		if (x != last_x || y != last_y || !mwas) {
			last_x = x;
			last_y = y;
			mwas = true;
			station->MouseMove(Engine::UI::Point(x, y));
			[self require_contents_update];
		}
	}
}
- (void) rightMouseDown: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		if (x != last_x || y != last_y) [self mouseMoved: event];
		auto time = Engine::GetTimerValue();
		if (rwas && (time - last_rdown) < dbl && x == last_px && y == last_py) {
			rwas = false;
			station->RightButtonDoubleClick(Engine::UI::Point(x, y));
			[self require_contents_update];
		} else {
			rwas = true;
			last_rdown = time;
			station->RightButtonDown(Engine::UI::Point(x, y));
			[self require_contents_update];
		}
		last_px = x; last_py = y;
	}
}
- (void) rightMouseUp: (NSEvent *) event
{
	if ([[self window] isKeyWindow] && station) {
		auto pos = [NSEvent mouseLocation];
		int x, y;
		ScreenToView(pos.x, pos.y, self, x, y);
		station->RightButtonUp(Engine::UI::Point(x, y));
		[self require_contents_update];
	}
}
- (void) keyDown: (NSEvent *) event
{
	bool dead;
	Engine::KeyCodes::Key key = static_cast<Engine::KeyCodes::Key>(Engine::Cocoa::EngineKeyCode([event keyCode], dead));
	if (Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::Control) || Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::Alternative) ||
		Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::System)) dead = true;
	if (!__engine_keyboard_status.ElementByKey(key)) __engine_keyboard_status.Append(key, true);
	if (key && station) {
		if (!station->KeyDown(key)) {
			if (!dead) {
				Engine::string etext;
				if (key == Engine::KeyCodes::Tab) {
					etext = L"\t";
				} else if (key == Engine::KeyCodes::Space) {
					etext = L" ";
				} else {
					NSString * text = [event characters];
					etext = Engine::Cocoa::EngineString(text);
				}        
				for (int i = 0; i < etext.Length(); i++) station->CharDown(etext[i]);
			}
		}
		[self require_contents_update];
	}
}
- (void) keyUp: (NSEvent *) event
{
	bool dead;
	Engine::KeyCodes::Key key = static_cast<Engine::KeyCodes::Key>(Engine::Cocoa::EngineKeyCode([event keyCode], dead));
	__engine_keyboard_status.RemoveByKey(key);
	if (key && station) {
		station->KeyUp(key);
		[self require_contents_update];
	}
}
- (void) flagsChanged: (NSEvent *) event
{
	if (!station) return;
	bool shift = Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::Shift);
	bool control = Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::Control);
	bool alternative = Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::Alternative);
	bool system = Engine::Keyboard::IsKeyPressed(Engine::KeyCodes::System);
	bool redraw = false;
	if ((shift && !(fkeys & 1)) || (!shift && (fkeys & 1))) {
		redraw = true;
		if (shift) station->KeyDown(Engine::KeyCodes::Shift);
		else station->KeyUp(Engine::KeyCodes::Shift);
	}
	if ((control && !(fkeys & 2)) || (!control && (fkeys & 2))) {
		redraw = true;
		if (control) station->KeyDown(Engine::KeyCodes::Control);
		else station->KeyUp(Engine::KeyCodes::Control);
	}
	if ((alternative && !(fkeys & 4)) || (!alternative && (fkeys & 4))) {
		redraw = true;
		if (alternative) station->KeyDown(Engine::KeyCodes::Alternative);
		else station->KeyUp(Engine::KeyCodes::Alternative);
	}
	if ((system && !(fkeys & 8)) || (!system && (fkeys & 8))) {
		redraw = true;
		if (system) station->KeyDown(Engine::KeyCodes::System);
		else station->KeyUp(Engine::KeyCodes::System);
	}
	fkeys = (shift ? 1 : 0) | (control ? 2 : 0) | (alternative ? 4 : 0) | (system ? 8 : 0);
	if (redraw) { [self require_contents_update]; }
}
- (void) scrollWheel: (NSEvent *) event
{
	if (!station) return;
	double dx = [event deltaX];
	double dy = [event deltaY];
	station->ScrollHorizontally(-dx);
	station->ScrollVertically(-dy);
	[self require_contents_update];
}
- (void) timerFireMethod: (NSTimer *) timer
{
	auto target = ((EngineRuntimeTimerTarget *) [timer userInfo])->target;
	if (target && station) target->Timer();
	[self require_contents_update];
}
- (void) engineEvent: (EngineRuntimeEvent *) event
{
	if (event->operation == 0) {
		if (!station) return;
		if (event->target) event->target->Destroy();
	} else if (event->operation == 1) {
		if (!station) return;
		if (event->target) event->target->RaiseEvent(event->identifier, Engine::UI::Window::Event::Deferred, 0);
	} else if (event->operation == 2) {
		event->job->DoJob(0);
		event->job->Release();
	}
}
- (BOOL) acceptsFirstResponder { return YES; }
- (NSObject *) inputContext { return input_context; }
@end
@implementation EngineRuntimeApplicationDelegate
- (void) close_all: (id) sender
{
	NSRunLoop * loop = [NSRunLoop currentRunLoop];
	NSArray<NSWindow *> * windows = [NSApp windows];
	NSArray<NSRunLoopMode> * modes = [NSArray<NSRunLoopMode> arrayWithObject: NSDefaultRunLoopMode];
	Engine::Array<NSWindow *> asked(0x10);
	while (true) {
		bool exit = true;
		for (int i = 0; i < [windows count]; i++) {
			NSWindow * target = [windows objectAtIndex: i];
			bool was_asked = false;
			for (int j = 0; j < asked.Length(); j++) if (asked[j] == target) { was_asked = true; break; }
			if (was_asked) continue;
			asked << target;
			exit = false;
			if (![target parentWindow]) {
				[loop performSelector: @selector(performClose:) target: target argument: self order: 0 modes: modes];
			}
			break;
		}
		if (exit) break;
	}
}
- (void) applicationDidFinishLaunching: (NSNotification *) notification
{
	Engine::Application::ApplicationLaunched();
}
- (void) application: (NSApplication *) application openURLs: (NSArray<NSURL *> *) urls
{
	auto controller = Engine::Application::GetController();
	auto callback = controller ? controller->GetCallback() : 0;
	if (callback) {
		for (int i = 0; i < [urls count]; i++) {
			NSURL * url = [urls objectAtIndex: i];
			if ([[url scheme] compare: @"file"] == NSOrderedSame) callback->OpenExactFile(Engine::Cocoa::EngineString([url path]));
			else callback->OpenExactFile(Engine::Cocoa::EngineString([url absoluteString]));
		}
	}
}
- (BOOL) applicationOpenUntitledFile: (NSApplication *) sender
{
	auto controller = Engine::Application::GetController();
	auto callback = controller ? controller->GetCallback() : 0;
	if (callback) {
		callback->CreateNewFile();
		return YES;
	} return NO;
}
- (void) new_file: (id) sender
{
	auto controller = Engine::Application::GetController();
	auto callback = controller ? controller->GetCallback() : 0;
	if (callback) callback->CreateNewFile();
}
- (void) open_file: (id) sender
{
	auto controller = Engine::Application::GetController();
	auto callback = controller ? controller->GetCallback() : 0;
	if (callback) callback->OpenSomeFile();
}
- (void) show_props: (id) sender
{
	auto controller = Engine::Application::GetController();
	auto callback = controller ? controller->GetCallback() : 0;
	if (callback) callback->ShowProperties();
}
- (void) show_help: (id) sender
{
	auto controller = Engine::Application::GetController();
	auto callback = controller ? controller->GetCallback() : 0;
	if (callback) callback->InvokeHelp();
}
- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) sender
{
	NSRunLoop * loop = [NSRunLoop currentRunLoop];
	NSArray<NSWindow *> * windows = [NSApp windows];
	NSArray<NSRunLoopMode> * modes = [NSArray<NSRunLoopMode> arrayWithObject: NSDefaultRunLoopMode];
	Engine::Array<NSWindow *> asked(0x10);
	while (true) {
		bool exit = true;
		for (int i = 0; i < [windows count]; i++) {
			NSWindow * target = [windows objectAtIndex: i];
			bool was_asked = false;
			for (int j = 0; j < asked.Length(); j++) if (asked[j] == target) { was_asked = true; break; }
			if (was_asked) continue;
			asked << target;
			exit = false;
			if (![target parentWindow]) {
				[loop performSelector: @selector(performClose:) target: target argument: self order: 0 modes: modes];
			}
			break;
		}
		if (exit) break;
	}
	return NSTerminateCancel;
}
@end
@implementation EngineRuntimeEvent
- (void) queue_event: (NSView *) view;
{
	NSRunLoop * loop = [NSRunLoop currentRunLoop];
	NSArray<NSRunLoopMode> * modes = [NSArray<NSRunLoopMode> arrayWithObject: NSRunLoopCommonModes];
	[loop performSelector: @selector(engineEvent:) target: view argument: self order: 0 modes: modes];
	[self release];
}
@end

@interface EngineRuntimeMenuItem : NSView
{
@public
	Engine::Cocoa::QuartzRenderingDevice * device;
	Engine::UI::Menus::MenuItem * item;
	int * code;
	NSMenuItem * server;
	NSMenu * owner;
	NSUInteger rect_tag;
	id target;
	SEL action;
	bool state;
	bool status_bar;
}
- (instancetype) init;
- (void) setFrame : (NSRect) frame;
- (void) setFrameSize: (NSSize) newSize;
- (void) drawRect : (NSRect) dirtyRect;
- (void) mouseMoved: (NSEvent *) event;
- (void) mouseDown: (NSEvent *) event;
- (void) mouseUp: (NSEvent *) event;
- (void) mouseEntered: (NSEvent *) event;
- (void) mouseExited: (NSEvent *) event;
- (void) viewDidMoveToWindow;
@end
@interface EngineRuntimeMenuSeparator : NSView
{
@public
	Engine::Cocoa::QuartzRenderingDevice * device;
	Engine::UI::Menus::MenuSeparator * item;
}
- (void) drawRect : (NSRect) dirtyRect;
@end

@implementation EngineRuntimeMenuItem
- (instancetype) init
{
	[super init];
	state = 0;
	rect_tag = [self addTrackingRect: NSMakeRect(0.0, 0.0, 0.0, 0.0) owner: self userData: 0 assumeInside: NO];
	return self;
}
- (void) setFrame : (NSRect) frame
{
	[super setFrame: frame];
	[self removeTrackingRect: rect_tag];
	rect_tag = [self addTrackingRect: NSMakeRect(0.0, 0.0, frame.size.width, frame.size.height) owner: self userData: 0 assumeInside: NO];
}
- (void) setFrameSize: (NSSize) newSize
{
	[super setFrameSize: newSize];
	[self removeTrackingRect: rect_tag];
	rect_tag = [self addTrackingRect: NSMakeRect(0.0, 0.0, newSize.width, newSize.height) owner: self userData: 0 assumeInside: NO];
}
- (void) drawRect : (NSRect) dirtyRect
{
	auto rect = [self frame];
	double scale = [[self window] backingScaleFactor];
	Engine::UI::Box box = Engine::UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
	CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
	device->SetContext(context, box.Right, box.Bottom, (scale > 1.5f) ? 2 : 1);
	item->Render(box, state != 0);
}
- (void) mouseMoved: (NSEvent *) event {}
- (void) mouseDown: (NSEvent *) event {}
- (void) mouseUp: (NSEvent *) event
{
	if (!item->Disabled) {
		*code = item->ID;
		[owner cancelTracking];
		state = 0;
		if (target) [target performSelector: action withObject: nil];
	}
}
- (void) mouseEntered: (NSEvent *) event
{
	state = 1;
	[self setNeedsDisplay: YES];
}
- (void) mouseExited: (NSEvent *) event
{
	state = 0;
	[self setNeedsDisplay: YES];
}
- (void) viewDidMoveToWindow
{
	state = 0;
	if (status_bar && [self window]) [[self window] becomeKeyWindow];
}
@end
@implementation EngineRuntimeMenuSeparator
- (void) drawRect : (NSRect) dirtyRect
{
	auto rect = [self frame];
	double scale = [[self window] backingScaleFactor];
	Engine::UI::Box box = Engine::UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
	CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
	device->SetContext(context, box.Right, box.Bottom, (scale > 1.5f) ? 2 : 1);
	item->Render(box, false);
}
@end

NSView * create_engine_menu_item(NSMenu * owner, int width, int * code, id target, SEL action, Engine::Cocoa::QuartzRenderingDevice * device, Engine::UI::Menus::MenuItem * item, NSMenuItem * server, bool status_bar)
{
	double scale = Engine::UI::Windows::GetScreenScale();
	EngineRuntimeMenuItem * obj = [[EngineRuntimeMenuItem alloc] init];
	[obj setFrameSize: NSMakeSize(double(width) / scale, double(item->GetHeight()) / scale)];
	obj->device = device;
	obj->item = item;
	obj->code = code;
	obj->target = target;
	obj->action = action;
	obj->server = server;
	obj->owner = owner;
	obj->status_bar = status_bar;
	return obj;
}
NSView * create_engine_menu_separator(int width, Engine::Cocoa::QuartzRenderingDevice * device, Engine::UI::Menus::MenuSeparator * item)
{
	double scale = Engine::UI::Windows::GetScreenScale();
	EngineRuntimeMenuSeparator * obj = [[EngineRuntimeMenuSeparator alloc] init];
	[obj setFrameSize: NSMakeSize(double(width) / scale, double(item->GetHeight()) / scale)];
	obj->device = device;
	obj->item = item;
	return obj;
}
NSMenu * build_engine_menu(NSMenu * owner, int * code, id target, SEL action, Engine::Cocoa::QuartzRenderingDevice * device, Engine::ObjectArray<Engine::UI::Menus::MenuElement> & elements, bool status_bar)
{
	NSMenu * result = [[NSMenu alloc] initWithTitle: @""];
	[result setAutoenablesItems: NO];
	int width = 0;
	for (int i = 0; i < elements.Length(); i++) {
		int w = elements[i].GetWidth();
		if (w > width) width = w;
	}
	for (int i = 0; i < elements.Length(); i++) {
		Engine::UI::Menus::MenuItem * src_item = 0;
		Engine::UI::Menus::MenuSeparator * src_sep = 0;
		if (elements[i].IsSeparator()) src_sep = static_cast<Engine::UI::Menus::MenuSeparator *>(elements.ElementAt(i));
		else src_item = static_cast<Engine::UI::Menus::MenuItem *>(elements.ElementAt(i));
		NSMenuItem * item = [[NSMenuItem alloc] initWithTitle: @"" action: nil keyEquivalent: @""];
		if (src_sep || src_item->Disabled) [item setEnabled: NO];
		if (src_item && src_item->Checked) [item setState: NSControlStateValueOn];
		NSView * view = 0;
		if (src_item) {
			view = create_engine_menu_item(owner ? owner : result, width, code, target, action, device, src_item, item, status_bar);
		} else if (src_sep) {
			view = create_engine_menu_separator(width, device, src_sep);
		}
		[item setView: view];
		[view release];
		if (src_item && src_item->Children.Length()) {
			NSMenu * sub = build_engine_menu(owner ? owner : result, code, target, action, device, src_item->Children, status_bar);
			[item setSubmenu: sub];
			[sub release];
		}
		[result addItem: item];
		[item release];
	}
	return result;
}

namespace Engine
{
	namespace NativeWindows
	{
		class NativeStation : public WindowStation
		{
			friend void InternalShowWindow(UI::WindowStation * Station, bool Show, bool Activate);
			friend void ShowWindow(UI::WindowStation * Station, bool Show);
			friend void ShowSlaves(UI::WindowStation * Station, bool Show);

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
			Array<NativeStation *> _slaves;
			bool _visible = false;
			NativeStation * _parent = 0;
			Array<EngineRuntimeTimerTarget *> _timers;
			Window::RefreshPeriod InternalRate = Window::RefreshPeriod::None;
		public:
			NSView * main_window_view;
			SafePointer<Object> TouchBar;
			SafePointer<Cocoa::QuartzRenderingDevice> RenderingDevice;
			SafePointer<UI::IRenderingDevice> MetalRenderingDevice;
			SafePointer<MetalGraphics::MetalPresentationInterface> MetalInterface;
			Controls::OverlappedWindow * top_level_window;
			class DesktopWindowFactory : public WindowStation::IDesktopWindowFactory
			{
				Template::ControlTemplate * _template = 0;
			public:
				DesktopWindowFactory(Template::ControlTemplate * Template) : _template(Template) {}
				virtual Window * CreateDesktopWindow(WindowStation * Station) override
				{
					return _template ? new Controls::OverlappedWindow(0, Station, _template) : new Controls::OverlappedWindow(0, Station);
				}
			};
			class CocoaCursor : public ICursor
			{
				NSCursor * _cursor;
				bool _release;
			public:
				CocoaCursor(NSCursor * cursor, bool take_control) : _cursor(cursor), _release(take_control) {}
				~CocoaCursor(void) override { if (_release) { [_cursor release]; } }
				void Set(void) { [_cursor set]; }
			};
			static void FreeDataCallback(void * info, const void * data, size_t size) { free(info); }
		
			NativeStation(NSWindow * window, WindowStation::IDesktopWindowFactory * factory, bool create_dev = true) : _window(window), WindowStation(factory), _slaves(0x10), _timers(0x10)
			{
				if (create_dev) {
					auto feature_class = MacOSXSpecific::GetRenderingDeviceFeatureClass();
					if (feature_class == MacOSXSpecific::RenderingDeviceFeatureClass::DontCare) feature_class = MacOSXSpecific::RenderingDeviceFeatureClass::Metal;
					if (feature_class == MacOSXSpecific::RenderingDeviceFeatureClass::Quartz) {
						RenderingDevice = new Cocoa::QuartzRenderingDevice;
						SetRenderingDevice(RenderingDevice);
					} else if (feature_class == MacOSXSpecific::RenderingDeviceFeatureClass::Metal) {}
				}
				_arrow.SetReference(new CocoaCursor([NSCursor arrowCursor], false));
				_beam.SetReference(new CocoaCursor([NSCursor IBeamCursor], false));
				_link.SetReference(new CocoaCursor([NSCursor pointingHandCursor], false));
				_size_left_right.SetReference(new CocoaCursor([NSCursor resizeLeftRightCursor], false));
				_size_up_down.SetReference(new CocoaCursor([NSCursor resizeUpDownCursor], false));
				_size_left_up_right_down.SetReference(new CocoaCursor([NSCursor resizeLeftRightCursor], false));
				_size_left_down_right_up.SetReference(new CocoaCursor([NSCursor resizeLeftRightCursor], false));
				_size_all.SetReference(new CocoaCursor([NSCursor openHandCursor], false));
				top_level_window = GetDesktop()->As<Controls::OverlappedWindow>();
			}
			virtual ~NativeStation(void) override
			{
				for (int i = 0; i < _timers.Length(); i++) {
					[_timers[i]->timer invalidate];
					[_timers[i] release];
				}
			}
			virtual bool IsNativeStationWrapper(void) const override { return true; }
			virtual void SetFocus(Window * window) override { if (window) [_window makeKeyWindow]; if ([_window isKeyWindow]) WindowStation::SetFocus(window); else WindowStation::SetFocus(0); }
			virtual Window * GetFocus(void) override { if ([_window isKeyWindow]) return WindowStation::GetFocus(); else return 0; }
			virtual void SetCapture(Window * window) override { if ([_window isKeyWindow]) WindowStation::SetCapture(window); else WindowStation::SetCapture(0); }
			virtual Window * GetCapture(void) override { if ([_window isKeyWindow]) return WindowStation::GetCapture(); else return 0; }
			virtual void ReleaseCapture(void) override { WindowStation::SetCapture(0); }
			virtual void SetExclusiveWindow(Window * window) override
			{
				if (window) [_window makeKeyWindow];
				if ([_window isKeyWindow]) {
					WindowStation::SetExclusiveWindow(window);
					WindowStation::SetFocus(window);
				}
			}
			virtual Window * GetExclusiveWindow(void) override { if ([_window isKeyWindow]) return WindowStation::GetExclusiveWindow(); else return 0; }
			virtual UI::Point GetCursorPos(void) override
			{
				auto pos = [NSEvent mouseLocation];
				int x, y;
				ScreenToView(pos.x, pos.y, __GetEngineViewFromWindow(_window), x, y);
				return UI::Point(x, y);
			}
			virtual void SetCursorPos(UI::Point pos) override
			{
				double x, y;
				ViewToScreen(pos.x, pos.y, __GetEngineViewFromWindow(_window), x, y);
				CGDirectDisplayID display = [[[[_window screen] deviceDescription] objectForKey: @"NSScreenNumber"] unsignedIntValue];
				auto screen_rect = [[_window screen] frame];
				CGDisplayMoveCursorToPoint(display, CGPointMake(x, screen_rect.size.height - y));
			}
			virtual bool NativeHitTest(const UI::Point & at) override
			{
				auto box = GetBox();

				double scale = [_window backingScaleFactor];
				CGRect frame = [__GetEngineViewFromWindow(_window) frame];
				CGRect rect = NSMakeRect(double(at.x) / scale, frame.size.height - double(at.y) / scale, 0.0, 0.0);
				rect = [_window convertRectToScreen: [__GetEngineViewFromWindow(_window) convertRect: rect toView: nil]];
				if ([_window isKeyWindow] && at.x >= 0 && at.y >= 0 && at.x < box.Right && at.y < box.Bottom
					&& [NSWindow windowNumberAtPoint: rect.origin belowWindowWithWindowNumber: 0] == [_window windowNumber]) return true;
				return false;
			}
			virtual ICursor * LoadCursor(Streaming::Stream * Source) override
			{
				SafePointer<Codec::Image> Image = Codec::DecodeImage(Source);
				if (Image) { return LoadCursor(Image); } else throw InvalidArgumentException();
			}
			virtual ICursor * LoadCursor(Codec::Image * Source) override { return LoadCursor(Source->GetFrameBestDpiFit(UI::Zoom)); }
			virtual ICursor * LoadCursor(Codec::Frame * Source) override
			{
				SafePointer<Codec::Frame> conv = Source->ConvertFormat(
					Engine::Codec::PixelFormat::R8G8B8A8, Engine::Codec::AlphaMode::Normal, Engine::Codec::ScanOrigin::TopDown);
				CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
				uint len = conv->GetScanLineLength() * conv->GetHeight();
				void * data = malloc(len);
				MemoryCopy(data, conv->GetData(), len);
				if (!data) throw OutOfMemoryException();
				CGDataProviderRef provider = CGDataProviderCreateWithData(data, data, len, FreeDataCallback);
				CGImageRef frame = CGImageCreate(conv->GetWidth(), conv->GetHeight(), 8, 32, conv->GetScanLineLength(),
					rgb, kCGImageAlphaLast, provider, 0, false, kCGRenderingIntentDefault);
				CGColorSpaceRelease(rgb);
				CGDataProviderRelease(provider);
				NSScreen * screen = [NSScreen mainScreen];
				double scale = [screen backingScaleFactor];
				NSImage * image = [[NSImage alloc] initWithCGImage: frame
					size: NSMakeSize(double(Source->GetWidth()) / scale, double(Source->GetHeight()) / scale)];
				CGImageRelease(frame);
				CocoaCursor * cursor = new CocoaCursor([[NSCursor alloc] initWithImage: image
					hotSpot: NSMakePoint(double(Source->HotPointX) / scale, double(Source->HotPointY) / scale)], true);
				[image release];
				return cursor;
			}
			virtual ICursor * GetSystemCursor(SystemCursor cursor) override
			{
				if (cursor == SystemCursor::Null) return _null;
				else if (cursor == SystemCursor::Arrow) return _arrow;
				else if (cursor == SystemCursor::Beam) return _beam;
				else if (cursor == SystemCursor::Link) return _link;
				else if (cursor == SystemCursor::SizeLeftRight) return _size_left_right;
				else if (cursor == SystemCursor::SizeUpDown) return _size_up_down;
				else if (cursor == SystemCursor::SizeLeftUpRightDown) return _size_left_up_right_down;
				else if (cursor == SystemCursor::SizeLeftDownRightUp) return _size_left_down_right_up;
				else if (cursor == SystemCursor::SizeAll) return _size_all;
				else return 0;
			}
			virtual void SetSystemCursor(SystemCursor entity, ICursor * cursor) override
			{
				if (entity == SystemCursor::Null) _null.SetRetain(cursor);
				else if (entity == SystemCursor::Arrow) _arrow.SetRetain(cursor);
				else if (entity == SystemCursor::Beam) _beam.SetRetain(cursor);
				else if (entity == SystemCursor::Link) _link.SetRetain(cursor);
				else if (entity == SystemCursor::SizeLeftRight) _size_left_right.SetRetain(cursor);
				else if (entity == SystemCursor::SizeUpDown) _size_up_down.SetRetain(cursor);
				else if (entity == SystemCursor::SizeLeftUpRightDown) _size_left_up_right_down.SetRetain(cursor);
				else if (entity == SystemCursor::SizeLeftDownRightUp) _size_left_down_right_up.SetRetain(cursor);
				else if (entity == SystemCursor::SizeAll) _size_all.SetRetain(cursor);
			}
			virtual void SetCursor(ICursor * cursor) override { static_cast<CocoaCursor *>(cursor)->Set(); }
			virtual void SetTimer(Window * window, uint32 period) override { LowLevelSetTimer(window, period); }

			virtual void OnDesktopDestroy(void) override
			{
				RenderingDevice.SetReference(0);
				MetalRenderingDevice.SetReference(0);
				SetRenderingDevice(0);
				EngineRuntimeWindowDelegate * dlg = (EngineRuntimeWindowDelegate *) [_window delegate];
				((EngineRuntimeContentView *) __GetEngineViewFromWindow(_window))->station = 0;
				if (dlg) dlg->station = 0;
				for (int i = 0; i < _slaves.Length(); i++) _slaves[i]->GetDesktop()->Destroy();
				if (_parent) {
					for (int i = 0; i < _parent->_slaves.Length(); i++) if (_parent->_slaves[i] == this) {
						_parent->_slaves.Remove(i);
						break;
					}
				}
				[_window close];
				DestroyStation();
			}
			virtual void RequireRefreshRate(Window::RefreshPeriod period) override { InternalRate = period; AnimationStateChanged(); }
			virtual Window::RefreshPeriod GetRefreshRate(void) override { return InternalRate; }
			virtual void AnimationStateChanged(void) override
			{
				int fr = int(GetFocus() ? GetFocus()->FocusedRefreshPeriod() : Window::RefreshPeriod::None);
				int ar = int(IsPlayingAnimation() ? Window::RefreshPeriod::Cinematic : Window::RefreshPeriod::None);
				int ur = int(InternalRate);
				int mr = max(max(fr, ar), ur);
				if (!mr) LowLevelSetTimer(0, 0); else {
					if (mr == 1) LowLevelSetTimer(0, 500);
					else if (mr == 2) LowLevelSetTimer(0, 16);
				}
			}
			virtual void FocusWindowChanged(void) override { [__GetEngineViewFromWindow(_window) require_contents_update]; AnimationStateChanged(); }
			virtual Box GetDesktopBox(void) override
			{
				NSScreen * screen = [_window screen];
				CGRect rect = [screen frame];
				CGRect vrect = [screen visibleFrame];
				double scale = [screen backingScaleFactor];
				return UI::Box(int(vrect.origin.x * scale), int((rect.size.height - vrect.origin.y - vrect.size.height) * scale),
					int((vrect.origin.x + vrect.size.width) * scale), int((rect.size.height - vrect.origin.y) * scale));
			}
			virtual Box GetAbsoluteDesktopBox(const Box & box) override
			{
				double scale = [_window backingScaleFactor];
				CGRect frame = [__GetEngineViewFromWindow(_window) frame];
				CGRect rect = NSMakeRect(double(box.Left) / scale, frame.size.height - double(box.Bottom) / scale,
					double(box.Right - box.Left) / scale, double(box.Bottom - box.Top) / scale);
				rect = [_window convertRectToScreen: [__GetEngineViewFromWindow(_window) convertRect: rect toView: nil]];
				NSScreen * screen = [NSScreen mainScreen];
				CGRect srect = [screen frame];
				Box result(int(rect.origin.x * scale), int((srect.size.height - rect.origin.y - rect.size.height) * scale), 0, 0);
				result.Right = result.Left + int(rect.size.width * scale);
				result.Bottom = result.Top + int(rect.size.height * scale);
				return result;
			}
			virtual void RequireRedraw(void) override { [__GetEngineViewFromWindow(_window) require_contents_update]; }
			virtual void DeferredDestroy(Window * window) override
			{
				EngineRuntimeEvent * event = [[EngineRuntimeEvent alloc] init];
				event->target = window;
				event->operation = 0;
				event->identifier = 0;
				[event performSelectorOnMainThread: @selector(queue_event:) withObject: __GetEngineViewFromWindow(_window) waitUntilDone: NO];
			}
			virtual void DeferredRaiseEvent(Window * window, int ID) override
			{
				EngineRuntimeEvent * event = [[EngineRuntimeEvent alloc] init];
				event->target = window;
				event->operation = 1;
				event->identifier = ID;
				[event performSelectorOnMainThread: @selector(queue_event:) withObject: __GetEngineViewFromWindow(_window) waitUntilDone: NO];
			}
			virtual void PostJob(Tasks::ThreadJob * job) override
			{
				EngineRuntimeEvent * event = [[EngineRuntimeEvent alloc] init];
				event->operation = 2;
				event->job = job;
				job->Retain();
				[event performSelectorOnMainThread: @selector(queue_event:) withObject: __GetEngineViewFromWindow(_window) waitUntilDone: NO];
			}
			virtual handle GetOSHandle(void) override { return _window; }

			bool IsOnScreen(void) const { if (_parent) return _parent->IsOnScreen() && _visible; else return _visible; }
			void SetParent(NativeStation * new_parent) { _parent = new_parent; new_parent->_slaves << this; }
			void LowLevelSetTimer(Window * window, uint32 period)
			{
				if (period) {
					EngineRuntimeTimerTarget * timer = 0;
					for (int i = 0; i < _timers.Length(); i++) {
						if (_timers[i]->target == window) { timer = _timers[i]; break; }
					}
					if (!timer) {
						timer = [[EngineRuntimeTimerTarget alloc] init];
						timer->target = window;
						_timers << timer;
					}
					if (timer->timer) {
						[timer->timer invalidate];
						timer->timer = 0;
					}
					NSTimer * sys_timer = [NSTimer timerWithTimeInterval: double(period) / 1000.0
						target: __GetEngineViewFromWindow(_window) selector: @selector(timerFireMethod:) userInfo: timer repeats: YES];
					timer->timer = sys_timer;
					[[NSRunLoop currentRunLoop] addTimer: sys_timer forMode: NSRunLoopCommonModes];
				} else {
					for (int i = _timers.Length() - 1; i >= 0; i--) {
						if (_timers[i]->target == window) {
							[_timers[i]->timer invalidate];
							[_timers[i] release];
							_timers.Remove(i);
						}
					}
				}
			}
			void RenderContent(void)
			{
				if (RenderingDevice) {
					RenderingDevice->SetTimerValue(GetTimerValue());
					Animate();
					auto rect = [__GetEngineViewFromWindow(_window) frame];
					double scale = [_window backingScaleFactor];
					UI::Box box = UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
					CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
					RenderingDevice->SetContext(context, box.Right, box.Bottom, (scale > 1.5f) ? 2 : 1);
					Render();
				} else if (MetalRenderingDevice) {
					auto drawable = Cocoa::CoreMetalRenderingDeviceBeginDraw(MetalRenderingDevice, MetalInterface);
					MetalRenderingDevice->SetTimerValue(GetTimerValue());
					Animate();
					Render();
					Cocoa::CoreMetalRenderingDeviceEndDraw(MetalRenderingDevice, drawable, true);
				} else if (top_level_window->GetCallback()) top_level_window->GetCallback()->OnFrameEvent(top_level_window, Windows::FrameEvent::Draw);
			}
			void CreateMetalInterface(void) { if (!MetalInterface) MetalInterface = new MetalGraphics::MetalPresentationInterface(main_window_view, this, RenderStationContent); }
			NSWindow * GetWindow(void) const { return _window; }
		};

		bool _Initialized = false;
		void InitializeWindowSystem(void)
		{
			if (!_Initialized) {
				InitializeCodecCollection();

				NSString * about_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(301, L"About"));
				NSString * services_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(302, L"Services"));
				NSString * hide_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(303, L"Hide"));
				NSString * hide_all_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(304, L"Hide others"));
				NSString * show_all_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(305, L"Show all"));
				NSString * exit_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(306, L"Exit"));
				NSString * window_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(307, L"Window"));
				NSString * minimize_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(308, L"Minimize"));
				NSString * zoom_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(309, L"Zoom"));
				NSString * fullsreen_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(310, L"Toggle Full Screen"));
				NSString * bring_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(311, L"Bring All to Front"));
				NSString * help_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(312, L"Help"));

				[NSApplication sharedApplication];
				EngineRuntimeApplicationDelegate * delegate = [[EngineRuntimeApplicationDelegate alloc] init];
				[NSApp setDelegate: delegate];
				[NSApp disableRelaunchOnLogin];
				NSMenu * menu = [[NSMenu alloc] initWithTitle: @"Main Menu"];
				NSMenuItem * main_item = [[NSMenuItem alloc] initWithTitle: @"Application Menu" action: NULL keyEquivalent: @""];
				NSMenu * main_menu = [[NSMenu alloc] initWithTitle: @"Main Menu"];
				NSMenuItem * about = [[NSMenuItem alloc] initWithTitle: about_str action: @selector(orderFrontStandardAboutPanel:) keyEquivalent: @""];
				NSMenuItem * about_sep = [NSMenuItem separatorItem];
				[main_item setSubmenu: main_menu];
				[main_menu addItem: about];
				[main_menu addItem: about_sep];
				NSMenuItem * services_items = [[NSMenuItem alloc] initWithTitle: services_str action: NULL keyEquivalent: @""];
				NSMenu * services_menu = [[NSMenu alloc] initWithTitle: services_str];
				NSMenuItem * services_sep = [NSMenuItem separatorItem];
				NSMenuItem * hide_app = [[NSMenuItem alloc] initWithTitle: hide_str action: @selector(hide:) keyEquivalent: @"h"];
				[services_items setSubmenu: services_menu];
				[main_menu addItem: services_items];
				[main_menu addItem: services_sep];
				[main_menu addItem: hide_app];
				NSMenuItem * hide_others = [[NSMenuItem alloc] initWithTitle: hide_all_str action: @selector(hideOtherApplications:) keyEquivalent: @"h"];
				[main_menu addItem: hide_others];
				[hide_others setKeyEquivalentModifierMask: NSEventModifierFlagOption | NSEventModifierFlagCommand];
				NSMenuItem * show_all = [[NSMenuItem alloc] initWithTitle: show_all_str action: @selector(unhideAllApplications:) keyEquivalent: @""];
				NSMenuItem * show_sep = [NSMenuItem separatorItem];
				NSMenuItem * item_exit = [[NSMenuItem alloc] initWithTitle: exit_str action: @selector(close_all:) keyEquivalent: @"q"];
				[main_menu addItem: show_all];
				[main_menu addItem: show_sep];
				[main_menu addItem: item_exit];

				NSMenuItem * window_menu_item = [[NSMenuItem alloc] initWithTitle: window_str action: NULL keyEquivalent: @""];
				NSMenu * window_menu = [[NSMenu alloc] initWithTitle: window_str];
				[window_menu_item setSubmenu: window_menu];
				NSMenuItem * window_minimize = [[NSMenuItem alloc] initWithTitle: minimize_str action: @selector(performMiniaturize:) keyEquivalent: @"m"];
				NSMenuItem * window_maximize = [[NSMenuItem alloc] initWithTitle: zoom_str action: @selector(performZoom:) keyEquivalent: @""];
				NSMenuItem * window_fullscreen = [[NSMenuItem alloc] initWithTitle: fullsreen_str action: @selector(toggleFullScreen:) keyEquivalent: @"f"];
				[window_fullscreen setKeyEquivalentModifierMask: NSEventModifierFlagControl | NSEventModifierFlagCommand];
				NSMenuItem * window_sep = [NSMenuItem separatorItem];
				NSMenuItem * window_bring = [[NSMenuItem alloc] initWithTitle: bring_str action: @selector(arrangeInFront:) keyEquivalent: @""];
				[window_menu addItem: window_minimize];
				[window_menu addItem: window_maximize];
				[window_menu addItem: window_fullscreen];
				[window_menu addItem: window_sep];
				[window_menu addItem: window_bring];

				NSMenuItem * help_menu_item = [[NSMenuItem alloc] initWithTitle: help_str action: NULL keyEquivalent: @""];
				NSMenu * help_menu = [[NSMenu alloc] initWithTitle: help_str];
				[help_menu_item setSubmenu: help_menu];

				[menu addItem: main_item];
				[menu addItem: window_menu_item];
				[menu addItem: help_menu_item];
				[NSApp setMainMenu: menu];
				[NSApp setServicesMenu: services_menu];
				[NSApp setWindowsMenu: window_menu];
				[NSApp setHelpMenu: help_menu];

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
				[window_menu_item release];
				[window_menu release];
				[window_minimize release];
				[window_maximize release];
				[window_fullscreen release];
				[window_sep release];
				[window_bring release];
				[help_menu_item release];
				[help_menu release];

				[about_str release];
				[services_str release];
				[hide_str release];
				[hide_all_str release];
				[show_all_str release];
				[exit_str release];
				[window_str release];
				[minimize_str release];
				[zoom_str release];
				[fullsreen_str release];
				[bring_str release];
				[help_str release];

				_Initialized = true;
			}
		}
		void InitializeCodecCollection(void)
		{
			Cocoa::CreateAppleCodec();
			Codec::CreateIconCodec();
			Storage::CreateVolumeCodec();
		}
		class NativeResourceLoader : public Engine::UI::IResourceLoader
		{
			SafePointer<IRenderingDevice> _device;
		public:
			NativeResourceLoader(IRenderingDevice * device) { _device.SetRetain(device); }
			~NativeResourceLoader(void) override {}

			virtual UI::ITexture * LoadTexture(Codec::Frame * Source) override { return _device->LoadTexture(Source); }
			virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override
			{
				return _device->LoadFont(FaceName, Height, Weight, IsItalic, IsUnderline, IsStrikeout);
			}
		};
		UI::IResourceLoader * CreateCompatibleResourceLoader(void)
		{
			InitializeCodecCollection();
			SafePointer<Cocoa::QuartzRenderingDevice> device = new Cocoa::QuartzRenderingDevice;
			return new NativeResourceLoader(device);
		}
		Drawing::ITextureRenderingDevice * CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color)
		{
			return Cocoa::QuartzRenderingDevice::CreateQuartzCompatibleTextureRenderingDevice(width, height, color);
		}
		UI::WindowStation * CreateOverlappedWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation, bool NoDevice)
		{
			InitializeWindowSystem();
			UI::Template::Controls::FrameExtendedData * ex_data = 0;
			for (int i = 0; i < Template->Children.Length(); i++) {
				if (Template->Children[i].Properties->GetTemplateClass() == L"FrameExtendedData") {
					ex_data = static_cast<UI::Template::Controls::FrameExtendedData *>(Template->Children[i].Properties);
					break;
				}
			}
			auto prev_attr = MacOSXSpecific::GetWindowCreationAttribute();
			if (ex_data) {
				auto new_attr = 0;
				if (ex_data->MacTransparentTitle) new_attr |= MacOSXSpecific::CreationAttribute::TransparentTitle;
				else if (ex_data->MacEffectBackground) new_attr |= MacOSXSpecific::CreationAttribute::EffectBackground;
				else if (ex_data->MacTransparentWindow) new_attr |= MacOSXSpecific::CreationAttribute::Transparent;
				else if (ex_data->MacUseLightTheme) new_attr |= MacOSXSpecific::CreationAttribute::LightTheme;
				else if (ex_data->MacUseDarkTheme) new_attr |= MacOSXSpecific::CreationAttribute::DarkTheme;
				else if (ex_data->MacShadowlessWindow) new_attr |= MacOSXSpecific::CreationAttribute::Shadowless;
				if (ex_data->MacEffectBackgroundMaterial.Length()) new_attr |= MacOSXSpecific::CreationAttribute::EffectBackground;
				MacOSXSpecific::SetWindowCreationAttribute(new_attr);
			}
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
			if ((MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::TransparentTitle) &&
				(MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::EffectBackground) && props->Captioned) {
				style |= NSWindowStyleMaskFullSizeContentView;
			}
			double scale = GetScreenScale();
			CGRect window_rect = NSMakeRect(0, 0, double(client_box.Right - client_box.Left) / scale, double(client_box.Bottom - client_box.Top) / scale);
			CGRect minimal_rect = NSMakeRect(0, 0, double(props->MinimalWidth) / scale, double(props->MinimalHeight) / scale);
			CGRect parent_rect = NSMakeRect(double(parent_box.Left) / scale, desktop_rect.size.height - double(parent_box.Bottom) / scale,
				double(parent_box.Right - parent_box.Left) / scale, double(parent_box.Bottom - parent_box.Top) / scale);
			window_rect.origin.x = parent_rect.origin.x + (parent_rect.size.width - window_rect.size.width) / 2.0;
			window_rect.origin.y = parent_rect.origin.y + (parent_rect.size.height - window_rect.size.height) / 2.0;
			NSWindow * window = props->ToolWindow ? ([[NSPanel alloc] initWithContentRect: window_rect styleMask: style backing: NSBackingStoreBuffered defer: NO]) :
				([[NSWindow alloc] initWithContentRect: window_rect styleMask: style backing: NSBackingStoreBuffered defer: NO]);
			NSString * title = Cocoa::CocoaString(props->Title);
			[window setTitle: title];
			[window setContentMinSize: minimal_rect.size];
			[title release];
			NativeStation::DesktopWindowFactory factory(Template);
			SafePointer<NativeStation> station = new NativeStation(window, &factory, !NoDevice);
			if (ParentStation) station->SetParent(static_cast<NativeStation *>(ParentStation));
			EngineRuntimeContentView * view = [[EngineRuntimeContentView alloc] initWithStation: station.Inner()];
			EngineRuntimeWindowDelegate * delegate = [[EngineRuntimeWindowDelegate alloc] initWithStation: station.Inner()];
			station->main_window_view = view;
			view->window_delegate = delegate;
			delegate->view = view;
			[window setAcceptsMouseMovedEvents: YES];
			[window setTabbingMode: NSWindowTabbingModeDisallowed];
			[window setDelegate: delegate];
			if ((MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::TransparentTitle) &&
				(MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::EffectBackground) && props->Captioned) {
				NSVisualEffectView * fx_view = [[NSVisualEffectView alloc] init];
				NSRect client = [window contentRectForFrameRect: [window frame]];
				NSRect layout = [window contentLayoutRect];
				[fx_view setFrameSize: NSMakeSize(client.size.width, client.size.height)];
				[view setFrame: layout];
				[fx_view addSubview: view];
				[view setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
				[window setContentView: fx_view];
				[fx_view release];
			} else if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::EffectBackground) {
				NSVisualEffectView * fx_view = [[NSVisualEffectView alloc] init];
				[fx_view addSubview: view];
				[view setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
				[window setContentView: fx_view];
				[fx_view release];
			} else {
				[window setContentView: view];
			}
			if (!NoDevice && !station->RenderingDevice) {
				station->CreateMetalInterface();
				station->MetalRenderingDevice = Cocoa::CreateMetalRenderingDevice(station->MetalInterface);
				station->SetRenderingDevice(station->MetalRenderingDevice);
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::TransparentTitle) {
				[window setTitlebarAppearsTransparent: YES];
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::Transparent) {
				[window setOpaque: NO];
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::Shadowless) {
				[window setHasShadow: NO];
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::LightTheme) {
				@autoreleasepool {
					[window setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameAqua]];
				}
			} else if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::DarkTheme) {
				@autoreleasepool {
					[window setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameDarkAqua]];
				}
			}
			[view release];
			Controls::OverlappedWindow * local_desktop = station->GetDesktop()->As<Controls::OverlappedWindow>();
			local_desktop->GetContentFrame()->SetRectangle(UI::Rectangle::Entire());
			if (!NoDevice && !props->Background) {
				Color BackgroundColor = 0;
				if (!props->DefaultBackground) {
					BackgroundColor = props->BackgroundColor;
				}
				SafePointer<Template::BarShape> Background = new Template::BarShape;
				Background->Position = UI::Rectangle::Entire();
				Background->Gradient << Template::GradientPoint(Template::ColorTemplate(BackgroundColor), 0.0);
				local_desktop->SetBackground(Background);
			}
			NSRect actual = [view frame];
			station->SetBox(UI::Box(0, 0, int(actual.size.width * scale), int(actual.size.height * scale)));
			station->Retain();
			if (ex_data) {
				MacOSXSpecific::SetWindowCreationAttribute(prev_attr);
				if (ex_data->MacTouchBar) {
					SafePointer<MacOSXSpecific::TouchBar> touch_bar = MacOSXSpecific::SetTouchBarFromTemplate(station->GetDesktop(), ex_data->MacTouchBar);
				}
				if (ex_data->MacCustomBackgroundColor) MacOSXSpecific::SetWindowBackgroundColor(station->GetDesktop(), ex_data->MacBackgroundColor);
				if (ex_data->Transparentcy) MacOSXSpecific::SetWindowTransparentcy(station->GetDesktop(), 1.0 - ex_data->Transparentcy);
				if (ex_data->MacEffectBackgroundMaterial.Length()) {
					MacOSXSpecific::EffectBackgroundMaterial material = MacOSXSpecific::EffectBackgroundMaterial::WindowBackground;
					if (ex_data->MacEffectBackgroundMaterial == L"Titlebar") material = MacOSXSpecific::EffectBackgroundMaterial::Titlebar;
					else if (ex_data->MacEffectBackgroundMaterial == L"Selection") material = MacOSXSpecific::EffectBackgroundMaterial::Selection;
					else if (ex_data->MacEffectBackgroundMaterial == L"Menu") material = MacOSXSpecific::EffectBackgroundMaterial::Menu;
					else if (ex_data->MacEffectBackgroundMaterial == L"Popover") material = MacOSXSpecific::EffectBackgroundMaterial::Popover;
					else if (ex_data->MacEffectBackgroundMaterial == L"Sidebar") material = MacOSXSpecific::EffectBackgroundMaterial::Sidebar;
					else if (ex_data->MacEffectBackgroundMaterial == L"HeaderView") material = MacOSXSpecific::EffectBackgroundMaterial::HeaderView;
					else if (ex_data->MacEffectBackgroundMaterial == L"Sheet") material = MacOSXSpecific::EffectBackgroundMaterial::Sheet;
					else if (ex_data->MacEffectBackgroundMaterial == L"WindowBackground") material = MacOSXSpecific::EffectBackgroundMaterial::WindowBackground;
					else if (ex_data->MacEffectBackgroundMaterial == L"HUD") material = MacOSXSpecific::EffectBackgroundMaterial::HUD;
					else if (ex_data->MacEffectBackgroundMaterial == L"FullScreenUI") material = MacOSXSpecific::EffectBackgroundMaterial::FullScreenUI;
					else if (ex_data->MacEffectBackgroundMaterial == L"ToolTip") material = MacOSXSpecific::EffectBackgroundMaterial::ToolTip;
					MacOSXSpecific::SetEffectBackgroundMaterial(station->GetDesktop(), material);
				}
			}
			return station;
		}
		UI::WindowStation * CreatePopupWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation)
		{
			InitializeWindowSystem();
			Box ClientBox(Position, GetScreenDimensions());
			Box ClientArea(0, 0, ClientBox.Right - ClientBox.Left, ClientBox.Bottom - ClientBox.Top);
			NSScreen * screen = [NSScreen mainScreen];
			CGRect desktop_rect = [screen frame];
			double scale = GetScreenScale();
			NSWindowStyleMask style = NSWindowStyleMaskUtilityWindow;
			CGRect window_rect = NSMakeRect(0, 0, double(ClientArea.Right) / scale, double(ClientArea.Bottom) / scale);
			window_rect.origin.x = double(ClientBox.Left) / scale;
			window_rect.origin.y = desktop_rect.size.height - double(ClientBox.Bottom) / scale;
			PopupWindow * window = [[PopupWindow alloc] initWithContentRect: window_rect styleMask: style backing: NSBackingStoreBuffered defer: NO];
			[window setFloatingPanel: YES];
			[window setHidesOnDeactivate: NO];
			NativeStation::DesktopWindowFactory factory(Template);
			SafePointer<NativeStation> station = new NativeStation(window, &factory);
			EngineRuntimeContentView * view = [[EngineRuntimeContentView alloc] initWithStation: station.Inner()];
			EngineRuntimeWindowDelegate * delegate = [[EngineRuntimeWindowDelegate alloc] initWithStation: station.Inner()];
			station->main_window_view = view;
			view->window_delegate = delegate;
			delegate->view = view;
			[window setAcceptsMouseMovedEvents: YES];
			[window setTabbingMode: NSWindowTabbingModeDisallowed];
			[window setDelegate: delegate];
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::EffectBackground) {
				NSVisualEffectView * fx_view = [[NSVisualEffectView alloc] init];
				[fx_view addSubview: view];
				[view setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
				[window setContentView: fx_view];
				[fx_view release];
			} else {
				[window setContentView: view];
			}
			if (!station->RenderingDevice) {
				station->CreateMetalInterface();
				station->MetalRenderingDevice = Cocoa::CreateMetalRenderingDevice(station->MetalInterface);
				station->SetRenderingDevice(station->MetalRenderingDevice);
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::TransparentTitle) {
				[window setTitlebarAppearsTransparent: YES];
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::Transparent) {
				[window setOpaque: NO];
			}
			if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::LightTheme) {
				@autoreleasepool {
					[window setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameAqua]];
				}
			} else if (MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::DarkTheme) {
				@autoreleasepool {
					[window setAppearance: [NSAppearance appearanceNamed: NSAppearanceNameDarkAqua]];
				}
			}
			[view release];
			Controls::OverlappedWindow * local_desktop = station->GetDesktop()->As<Controls::OverlappedWindow>();
			local_desktop->GetContentFrame()->SetRectangle(UI::Rectangle::Entire());
			if (!(MacOSXSpecific::GetWindowCreationAttribute() & MacOSXSpecific::CreationAttribute::Transparent)) {
				Color BackgroundColor = 0xFF000000;
				SafePointer<Template::BarShape> Background = new Template::BarShape;
				Background->Position = UI::Rectangle::Entire();
				Background->Gradient << Template::GradientPoint(Template::ColorTemplate(BackgroundColor), 0.0);
				local_desktop->SetBackground(Background);
			}
			NSRect actual = [view frame];
			station->SetBox(UI::Box(0, 0, int(actual.size.width * scale), int(actual.size.height * scale)));
			station->Retain();
			return station;
		}
		void InternalShowWindow(UI::WindowStation * Station, bool Show, bool Activate)
		{
			NativeStation * st = static_cast<NativeStation *>(Station);
			if (Show) {
				NSWindow * obj = st->GetWindow();
				[obj orderFrontRegardless];
				if (Activate) [obj makeKeyWindow];
				if (st->_parent) {
					NSWindow * parent_obj = st->_parent->GetWindow();
					[parent_obj addChildWindow: obj ordered: NSWindowAbove];
				}
				for (int i = 0; i < st->_slaves.Length(); i++) if (st->_slaves[i]->_visible) {
					InternalShowWindow(st->_slaves[i], true, false);
				}
			} else {
				NSWindow * obj = st->GetWindow();
				for (int i = 0; i < st->_slaves.Length(); i++) {
					InternalShowWindow(st->_slaves[i], false, false);
				}
				if (st->_parent) {
					NSWindow * parent_obj = st->_parent->GetWindow();
					[parent_obj removeChildWindow: obj];
				}
				[obj orderOut: nil];
			}
		}
		void SetTouchBarObject(UI::WindowStation * Station, Object * Bar)
		{
			NativeStation * st = static_cast<NativeStation *>(Station);
			st->TouchBar.SetRetain(Bar);
		}
		Object * GetTouchBarObject(UI::WindowStation * Station)
		{
			NativeStation * st = static_cast<NativeStation *>(Station);
			return st->TouchBar;
		}
		void ShowWindow(UI::WindowStation * Station, bool Show)
		{
			NativeStation * st = static_cast<NativeStation *>(Station);
			if (!st->_parent || st->_parent->IsOnScreen()) InternalShowWindow(Station, Show, true);
			st->_visible = Show;
		}
		void ShowSlaves(UI::WindowStation * Station, bool Show)
		{
			NativeStation * st = static_cast<NativeStation *>(Station);
			if (!st->IsOnScreen()) return;
			for (int i = 0; i < st->_slaves.Length(); i++) if (st->_slaves[i]->_visible) {
				InternalShowWindow(st->_slaves[i], Show, false);
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
		void ActivateWindow(UI::WindowStation * Station)
		{
			[(NSWindow *) Station->GetOSHandle() orderFront: nil];
			[(NSWindow *) Station->GetOSHandle() makeMainWindow];
			[(NSWindow *) Station->GetOSHandle() makeKeyWindow];
		}
		void MaximizeWindow(UI::WindowStation * Station)
		{
			if (!IsWindowMaximized(Station)) [(NSWindow *) Station->GetOSHandle() toggleFullScreen: nil];
		}
		void MinimizeWindow(UI::WindowStation * Station)
		{
			[(NSWindow *) Station->GetOSHandle() miniaturize: nil];
		}
		void RestoreWindow(UI::WindowStation * Station)
		{
			if (IsWindowMaximized(Station)) [(NSWindow *) Station->GetOSHandle() toggleFullScreen: nil];
			if (IsWindowMinimized(Station)) [(NSWindow *) Station->GetOSHandle() deminiaturize: nil];
		}
		void RequestForAttention(UI::WindowStation * Station)
		{
			[NSApp requestUserAttention: NSInformationalRequest];
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
		bool IsWindowActive(UI::WindowStation * Station)
		{
			return [(NSWindow *) Station->GetOSHandle() isMainWindow] != 0;
		}
		bool IsWindowMinimized(UI::WindowStation * Station)
		{
			return [(NSWindow *) Station->GetOSHandle() isMiniaturized] != 0;
		}
		bool IsWindowMaximized(UI::WindowStation * Station)
		{
			return ([(NSWindow *) Station->GetOSHandle() styleMask] & NSFullScreenWindowMask) == NSFullScreenWindowMask;
		}
		NSMenu * CreateCocoaMenu(UI::Menus::Menu * menu, int * result, id target, SEL action, UI::IRenderingDevice * quartz_device)
		{
			auto device = static_cast<Cocoa::QuartzRenderingDevice *>(quartz_device);
			device->SetTimerValue(0);
			for (int i = 0; i < menu->Children.Length(); i++) menu->Children[i].WakeUp(device);
			return build_engine_menu(0, result, target, action, device, menu->Children, true);
		}
		void DestroyCocoaMenu(UI::Menus::Menu * menu, NSMenu * cocoa_menu)
		{
			[cocoa_menu release];
			for (int i = 0; i < menu->Children.Length(); i++) menu->Children[i].Shutdown();
		}
		int RunMenuPopup(UI::Menus::Menu * menu, UI::Window * owner, UI::Point at)
		{
			NSWindow * window = static_cast<NativeStation *>(owner->GetStation())->GetWindow();
			double scale = [window backingScaleFactor];
			NSView * server = __GetEngineViewFromWindow(window);
			NSRect rect = [server frame];
			NSPoint point = NSMakePoint(double(at.x / scale), rect.size.height - double(at.y / scale));
			SafePointer<Cocoa::QuartzRenderingDevice> device = new Cocoa::QuartzRenderingDevice;
			device->SetTimerValue(0);
			for (int i = 0; i < menu->Children.Length(); i++) menu->Children[i].WakeUp(device);
			int result = 0;
			NSMenu * popup = build_engine_menu(0, &result, 0, 0, device, menu->Children, false);
			[popup popUpMenuPositioningItem: nil atLocation: point inView: server];
			[popup release];
			for (int i = 0; i < menu->Children.Length(); i++) menu->Children[i].Shutdown();
			return result;
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
		void ExitMainLoop(void)
		{
			[NSApp stop: nil];
		}
		NSWindow * GetWindowObject(UI::WindowStation * station)
		{
			return static_cast<NativeStation *>(station)->GetWindow();
		}
		Array<string> * GetFontFamilies(void)
		{
			NSArray * fonts = (NSArray *) CTFontManagerCopyAvailableFontFamilyNames();
			SafePointer< Array<string> > result = new Array<string>([fonts count]);
			for (int i = 0; i < [fonts count]; i++) result->Append(Cocoa::EngineString([fonts objectAtIndex: i]));
			[fonts release];
			result->Retain();
			return result;
		}
		void SetApplicationIcon(Codec::Image * icon)
		{
			InitializeWindowSystem();
			NSScreen * screen = [NSScreen mainScreen];
			double scale = [screen backingScaleFactor];
			NSSize icon_size = [[NSApp dockTile] size];
			auto frame = icon->GetFrameBestSizeFit(icon_size.width * scale, icon_size.height * scale);
			NSImage * image = Cocoa::CocoaImage(frame);
			[NSApp setApplicationIconImage: image];
			[image release];
		}
		Codec::Frame * CaptureScreenState(void)
		{
			CGDirectDisplayID display = [[[[NSScreen mainScreen] deviceDescription] objectForKey: @"NSScreenNumber"] unsignedIntValue];
			CGImageRef image = CGDisplayCreateImage(display);
			if (!image) return 0;
			int width = CGImageGetWidth(image);
			int height = CGImageGetHeight(image);
			SafePointer<Codec::Frame> state = new Codec::Frame(width, height, -1, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::TopDown);
			CGColorSpaceRef rgb = CGColorSpaceCreateDeviceRGB();
			CGContextRef context = CGBitmapContextCreate(state->GetData(), width, height, 8, state->GetScanLineLength(), rgb, kCGImageAlphaPremultipliedLast);
			CGRect rect = CGRectMake(0.0f, 0.0f, float(width), float(height));
			CGContextSetBlendMode(context, kCGBlendModeCopy);
			CGContextDrawImage(context, rect, image);
			CGImageRelease(image);
			CGContextRelease(context);
			CGColorSpaceRelease(rgb);
			state->Retain();
			return state;
		}
		MetalGraphics::MetalPresentationInterface * InitWindowPresentationInterface(UI::WindowStation * Station)
		{
			static_cast<NativeStation *>(Station)->CreateMetalInterface();
			static_cast<NativeStation *>(Station)->MetalRenderingDevice.SetReference(0);
			static_cast<NativeStation *>(Station)->RenderingDevice.SetReference(0);
			return static_cast<NativeStation *>(Station)->MetalInterface;
		}
		Graphics::IDeviceFactory * CreateDeviceFactory(void) { return MetalGraphics::CreateMetalDeviceFactory(); }
		Graphics::IDevice * GetCommonDevice(void) { return MetalGraphics::GetMetalCommonDevice(); }
	}
}

static void RenderStationContent(Engine::UI::WindowStation * station)
{
	static_cast<Engine::NativeWindows::NativeStation *>(station)->RenderContent();
}
static Engine::MetalGraphics::MetalPresentationInterface * GetStationPresentationInterface(Engine::UI::WindowStation * station)
{
	return static_cast<Engine::NativeWindows::NativeStation *>(station)->MetalInterface;
}
static NSWindow * GetStationWindow(Engine::UI::WindowStation * station)
{
	return static_cast<Engine::NativeWindows::NativeStation *>(station)->GetWindow();
}

@implementation EngineRuntimeWindowDelegate
- (instancetype) initWithStation: (Engine::UI::WindowStation *) _station
{
	[super init];
	station = _station;
	return self;
}
- (BOOL) windowShouldClose: (NSWindow *) sender
{
	if (!station) return NO;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Close);
	return NO;
}
- (void) windowDidBecomeKey: (NSNotification *) notification
{
	if (!station) return;
	station->FocusChanged(true);
	station->CaptureChanged(true);
	[GetStationWindow(station) makeFirstResponder: __GetEngineViewFromWindow(GetStationWindow(station))];
	[__GetEngineViewFromWindow(GetStationWindow(station)) require_contents_update];
}
- (void) windowDidResignKey: (NSNotification *) notification
{
	__engine_keyboard_status.Clear();
	if (!station) return;
	station->FocusChanged(false);
	station->CaptureChanged(false);
	[__GetEngineViewFromWindow(GetStationWindow(station)) keyboardStateInactivate];
	[__GetEngineViewFromWindow(GetStationWindow(station)) require_contents_update];
}
- (void) windowWillBeginSheet: (NSNotification *) notification
{
	if (!station) return;
	Engine::NativeWindows::ShowSlaves(station, false);
}
- (void) windowDidEndSheet: (NSNotification *) notification
{
	if (!station) return;
	Engine::NativeWindows::ShowSlaves(station, true);
}
- (void) windowDidMiniaturize: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Minimize);
}
- (void) windowDidDeminiaturize: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Restore);
}
- (void) windowDidEnterFullScreen: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Maximize);
}
- (void) windowDidExitFullScreen: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Restore);
}
- (void) windowDidBecomeMain: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Activate);
}
- (void) windowDidResignMain: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Deactivate);
}
- (void) windowDidMove: (NSNotification *) notification
{
	if (!station) return;
	station->GetDesktop()->As<Engine::UI::Controls::OverlappedWindow>()->RaiseFrameEvent(Engine::UI::Windows::FrameEvent::Move);
}
@end