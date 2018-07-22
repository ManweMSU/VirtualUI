#include <EngineRuntime.h>
#include <PlatformDependent/CocoaInterop.h>
#include <PlatformDependent/KeyCodes.h>
#include <PlatformDependent/QuartzDevice.h>
#include <PlatformDependent/AppleCodec.h>

using namespace Engine;
using namespace Engine::UI;

#include <stdio.h>
#include <unistd.h>

@import Foundation;
@import AppKit;

UI::InterfaceTemplate interface;
Cocoa::QuartzRenderingDevice device;
UI::Shape * shape = 0;
UI::ITexture * tex = 0;
UI::TextureShape * shape2 = 0;
UI::IFont * font = 0;
UI::Shape * button = 0;
UI::ITextRenderingInfo * tr = 0;

class Loader : public Engine::UI::IResourceLoader
{
public:
    virtual UI::ITexture * LoadTexture(Streaming::Stream * Source) override
    {
        return device.LoadTexture(Source);
    }
    virtual UI::ITexture * LoadTexture(const string & Name) override
    {
        Streaming::FileStream source(Name, Streaming::AccessRead, Streaming::OpenExisting);
        return LoadTexture(&source);
    }
    virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override
    {
        return device.LoadFont(FaceName, Height, Weight, IsItalic, IsUnderline, IsStrikeout);
    }
    virtual void ReloadTexture(UI::ITexture * Texture, Streaming::Stream * Source) override
    {

    }
    virtual void ReloadTexture(UI::ITexture * Texture, const string & Name) override
    {

    }
    virtual void ReloadFont(UI::IFont * Font) override
    {

    }
};
class Arguments : public Engine::UI::IArgumentProvider
{
public:
    Arguments(void) {}
    virtual void GetArgument(const string & name, int * value) override
    {
        *value = 1;
    }
    virtual void GetArgument(const string & name, double * value) override
    {
        *value = 0.666;
    }
    virtual void GetArgument(const string & name, UI::Color * value) override
    {
        *value = UI::Color(0);
    }
    virtual void GetArgument(const string & name, string * value) override
    {
        *value = L"pidor ыыыы";
    }
    virtual void GetArgument(const string & name, UI::ITexture ** value) override
    {
        *value = 0;
    }
    virtual void GetArgument(const string & name, UI::IFont ** value) override
    {
        font->Retain();
        *value = font;
    }
};

@interface TestView : NSView
{
}
- (void) drawRect : (NSRect) dirtyRect;
- (void) keyDown: (NSEvent *) event;
- (void) mouseDown: (NSEvent *) event;
@property(readonly) BOOL acceptsFirstResponder;
@end

@implementation TestView
- (BOOL) acceptsFirstResponder
{
    return YES;
}
- (void) drawRect : (NSRect) dirtyRect
{
    auto wnd = [self window];
    auto rect = [self frame];
    float scale = [wnd backingScaleFactor];
    UI::Box box = UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    device.SetContext(context, box.Right, box.Bottom, (scale > 1.5f) ? 2 : 1);
    device.SetTimerValue(GetTimerValue());

    shape2->Render(&device, box);
    shape->Render(&device, UI::Box(50, 50, 550, 110));
    button->Render(&device, UI::Box(600, 50, 900, 110));
    device.RenderText(tr, UI::Box(50, 130, 550, 200), true);
}
- (void) mouseDown: (NSEvent *) event
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    Console << string(L"Left button down") + IO::NewLineChar;

    [self lockFocusIfCanDraw];
}
- (void) keyDown: (NSEvent *) event
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);

    uint32 key = [event keyCode];
    Console << L"Key Down: " + string(key, L"0123456789ABCDEF") + IO::NewLineChar;
}
@end

class _cb2 : public Windows::IWindowEventCallback
{
public:
    virtual void OnInitialized(UI::Window * window) override {}
    virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
    {
        if (ID == 1) {
            auto group1 = window->FindChild(101);
            auto group2 = window->FindChild(102);
            if (group1->IsVisible()) {
                group1->HideAnimated(Animation::SlideSide::Left, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
                group2->ShowAnimated(Animation::SlideSide::Right, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
            } else {
                group2->HideAnimated(Animation::SlideSide::Left, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
                group1->ShowAnimated(Animation::SlideSide::Right, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
            }
        } else if (ID == 2) {
            auto group1 = window->FindChild(101);
            auto group2 = window->FindChild(102);
            if (group1->IsVisible()) {
                group1->HideAnimated(Animation::SlideSide::Right, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
                group2->ShowAnimated(Animation::SlideSide::Left, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
            } else {
                group2->HideAnimated(Animation::SlideSide::Right, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
                group1->ShowAnimated(Animation::SlideSide::Left, 500,
                    Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
            }
        }
    }
    virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
    {
        // SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
        // Streaming::TextWriter Console(ConsoleOutStream);
        // if (event == Windows::FrameEvent::Close) Console << string(L"Close window") + IO::NewLineChar;
        // else if (event == Windows::FrameEvent::Move) Console << string(L"Move window") + IO::NewLineChar;
        if (event == Windows::FrameEvent::Close) window->Destroy();
    }
};

int Main(void)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    SafePointer<Streaming::FileStream> ConsoleInStream = new Streaming::FileStream(IO::GetStandartInput());
    Streaming::TextReader Input(ConsoleInStream, Encoding::UTF8);

    Codec::CreateIconCodec();
    Cocoa::CreateAppleCodec();
    UI::Zoom = NativeWindows::GetScreenScale();

    string src = IO::Path::GetDirectory(IO::Path::GetDirectory(IO::Path::GetDirectory(IO::Path::GetDirectory(
        IO::Path::GetDirectory(IO::GetExecutablePath()))))) + L"/../Tests/test.eui";
    Console << src << IO::NewLineChar;
    Streaming::FileStream source(src, Streaming::AccessRead, Streaming::OpenExisting);
    ::Loader loader;
    UI::Loader::LoadUserInterfaceFromBinary(interface, &source, &loader, 0);
    tex = interface.Texture[L"Wallpaper"];
    Arguments args;
    auto temp = interface.Application[L"Progress"];
    auto temp2 = interface.Application[L"ButtonFocused"];
    font = device.LoadFont(L"Tahoma", 38, 400, false, true, false);
    tr = device.CreateTextRenderingInfo(font, L"Пидор 🍄🍄🍄 ыыыы", 1, 1, UI::Color(32, 32, 128, 196));
    tr->SetHighlightColor(UI::Color(255, 160, 160, 128));
    tr->HighlightText(1, 4);
    {
        Array<UI::Color> hl;
        hl << UI::Color(255, 0, 0);
        hl << UI::Color(0, 0, 255);
        tr->SetCharPalette(hl);
        Array<uint8> ind;
        ind.SetLength(14);
        ZeroMemory(ind.GetBuffer(), 14);
        ind[0] = 1;
        ind[1] = 2;
        ind[2] = 1;
        ind[3] = 2;
        ind[4] = 1;
        tr->SetCharColors(ind);
    }
    Console << L"Template = " << string(reinterpret_cast<void *>(temp)) << IO::NewLineChar;
    shape = temp->Initialize(&args);
    button = temp2->Initialize(&args);
    shape2 = new UI::TextureShape(UI::Rectangle::Entire(), tex, UI::Rectangle::Entire(), UI::TextureShape::TextureRenderMode::Fit);

    Console << L"Shape = " << string(reinterpret_cast<void *>(shape)) << IO::NewLineChar;

    NSUserDefaults * def = [[NSUserDefaults alloc] init];
    NSDictionary<NSString *,id> * dict = [def dictionaryRepresentation];
    NSArray<NSString *> * keys = [dict allKeys];
    int c = [keys count];
    Console << L"Dictionary length: " << c << IO::NewLineChar;
    for (int i = 0; i < c; i++) {
        Console << L"   - " << Cocoa::EngineString([keys objectAtIndex: i]) << IO::NewLineChar;
    }
    [keys release];
    [dict release];
    [def release];

    Console << L"Keyboard delay " + string(Keyboard::GetKeyboardDelay()) + L" ms" + IO::NewLineChar;
    Console << L"Keyboard speed " + string(Keyboard::GetKeyboardSpeed()) + L" ms" + IO::NewLineChar;

    auto sbox = NativeWindows::GetScreenDimensions();
    Console << L"Screen " + string(sbox.Left) + L", " + string(sbox.Top) + L", " + string(sbox.Right) + L", " + string(sbox.Bottom) + IO::NewLineChar;
    Console << L"Scale  " + string(NativeWindows::GetScreenScale()) + IO::NewLineChar;
    
    NativeWindows::InitializeWindowSystem();

    NSView * view = [[TestView alloc] init];

    NSWindow * window = [[NSWindow alloc]
        initWithContentRect: NSMakeRect(100, 100, 700, 400)
        styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
            NSWindowStyleMaskResizable
        backing: NSBackingStoreBuffered
        defer: NO];
    [window setContentView: view];
    [window setTitle: @"Test Window"];
    [window orderFrontRegardless];
    [window display];
    [window makeKeyWindow];
    [view lockFocus];
    Console << L"Window: " << string((__bridge void*) window) << IO::NewLineChar;
    [view release];

    auto Callback2 = new _cb2;
    auto w4 = Windows::CreateFramedDialog(interface.Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid(), 0);
    if (w4) w4->Show(true);

    {
        Codec::Frame frame(64, 64, -1, Codec::PixelFormat::B8G8R8A8, Codec::AlphaFormat::Normal, Codec::LineDirection::TopDown);
        for (int y = 0; y < 64; y++) for (int x = 0; x < 64; x++) {
            frame.SetPixel(x, y, (y == 20 || x == 20) ? 0xFF444488 : 0x22FFFFFF);
        }
        frame.HotPointX = 20;
        frame.HotPointY = 20;
        ICursor * cursor = w4->GetStation()->LoadCursor(&frame);
        w4->GetStation()->SetSystemCursor(SystemCursor::Arrow, cursor);
        cursor->Release();
    }
    
    NativeWindows::RunMainMessageLoop();

    return 0;
}
