#include <EngineRuntime.h>

using namespace Engine;

#include <stdio.h>
#include <unistd.h>

@import Foundation;
@import AppKit;

UI::InterfaceTemplate interface;
Cocoa::QuartzRenderingDevice device;
UI::Shape * shape = 0;
UI::ITexture * tex = 0;
UI::ITextureRenderingInfo * tri = 0;
UI::TextureShape * shape2 = 0;

class Loader : public Engine::UI::IResourceLoader
{
public:
    class Font : public UI::IFont
    {
    public:
        Font(void) {}
        virtual int GetWidth(void) const override { return 1; }
        virtual int GetHeight(void) const override { return 1; }
        virtual void Reload(UI::IRenderingDevice * Device) override {}
    };

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
        return new Font();
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
        *value = L"";
    }
    virtual void GetArgument(const string & name, UI::ITexture ** value) override
    {
        *value = 0;
    }
    virtual void GetArgument(const string & name, UI::IFont ** value) override
    {
        *value = 0;
    }
};

@interface TestView : NSView
{
}
- (void) drawRect : (NSRect) dirtyRect;
@end

@implementation TestView
- (void) drawRect : (NSRect) dirtyRect
{
    auto wnd = [self window];
    auto rect = [self frame];
    float scale = [wnd backingScaleFactor];
    UI::Box box = UI::Box(0, 0, int(rect.size.width * scale), int(rect.size.height * scale));
    CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
    device.SetContext(context, box.Right, box.Bottom, (scale > 1.5f) ? 2 : 1);

    shape2->Render(&device, box);
    shape->Render(&device, UI::Box(50, 50, 550, 110));
}
@end

int main(int argc, char ** argv)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    SafePointer<Streaming::FileStream> ConsoleInStream = new Streaming::FileStream(IO::GetStandartInput());
    Streaming::TextReader Input(ConsoleInStream, Encoding::UTF8);

    Codec::CreateIconCodec();
    Cocoa::CreateAppleCodec();

    string src = IO::Path::GetDirectory(IO::Path::GetDirectory(IO::Path::GetDirectory(IO::Path::GetDirectory(
        IO::Path::GetDirectory(IO::GetExecutablePath()))))) + L"/Tests/test.eui";
    Console << src << IO::NewLineChar;
    Streaming::FileStream source(src, Streaming::AccessRead, Streaming::OpenExisting);
    Loader loader;
    UI::Loader::LoadUserInterfaceFromBinary(interface, &source, &loader, 0);
    tex = interface.Texture[L"Wallpaper"];
    tri = device.CreateTextureRenderingInfo(tex, UI::Box(1000, 400, 1300, 700), true);
    Arguments args;
    auto temp = interface.Application[L"Progress"];
    Console << L"Template = " << string(reinterpret_cast<void *>(temp)) << IO::NewLineChar;
    shape = temp->Initialize(&args);
    shape2 = new UI::TextureShape(UI::Rectangle::Entire(), tex, UI::Rectangle::Entire(), UI::TextureShape::TextureRenderMode::Fit);

    Console << L"Shape = " << string(reinterpret_cast<void *>(shape)) << IO::NewLineChar;
    
    [NSApplication sharedApplication];

    NSMenu * menu = [[NSMenu alloc] initWithTitle: @"Main Menu"];
    NSMenuItem * main_item = [[NSMenuItem alloc] initWithTitle: @"Test Application Menu" action: NULL keyEquivalent: @""];
    NSMenu * main_menu = [[NSMenu alloc] initWithTitle: @"Main Menu"];
    [main_item setSubmenu: main_menu];
    [main_menu addItem: [[NSMenuItem alloc] initWithTitle: @"О программе" action: @selector(orderFrontStandardAboutPanel:) keyEquivalent: @""]];
    [main_menu addItem: [NSMenuItem separatorItem]];
    NSMenuItem * services_items = [[NSMenuItem alloc] initWithTitle: @"Службы" action: NULL keyEquivalent: @""];
    NSMenu * services_menu = [[NSMenu alloc] initWithTitle: @"Services Menu"];
    [services_items setSubmenu: services_menu];
    [main_menu addItem: services_items];
    [main_menu addItem: [NSMenuItem separatorItem]];
    [main_menu addItem: [[NSMenuItem alloc] initWithTitle: @"Скрыть программу" action: @selector(hide:) keyEquivalent: @"h"]];
    NSMenuItem * hide_others;
    [main_menu addItem: hide_others = [[NSMenuItem alloc] initWithTitle: @"Скрыть другие программы" action: @selector(hideOtherApplications:) keyEquivalent: @"h"]];
    [hide_others setKeyEquivalentModifierMask: NSEventModifierFlagOption | NSEventModifierFlagCommand];
    [main_menu addItem: [[NSMenuItem alloc] initWithTitle: @"Показать все" action: @selector(unhideAllApplications:) keyEquivalent: @""]];
    [main_menu addItem: [NSMenuItem separatorItem]];
    [main_menu addItem: [[NSMenuItem alloc] initWithTitle: @"Выход" action: @selector(terminate:) keyEquivalent: @"q"]];

    [menu addItem: main_item];
    [NSApp setMainMenu: menu];
    [NSApp setServicesMenu: services_menu];

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
    Console << L"Window: " << string((__bridge void*) window) << IO::NewLineChar;
    [view release];
    [NSApp run];

    return 0;
}
