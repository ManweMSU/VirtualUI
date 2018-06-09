#include <EngineRuntime.h>

using namespace Engine;

#include <stdio.h>
#include <unistd.h>

@import Foundation;
@import AppKit;

int main(int argc, char ** argv)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    SafePointer<Streaming::FileStream> ConsoleInStream = new Streaming::FileStream(IO::GetStandartInput());
    Streaming::TextReader Input(ConsoleInStream, Encoding::UTF8);

    Codec::CreateIconCodec();
    Cocoa::CreateAppleCodec();
    SafePointer<Streaming::FileStream> file = new Streaming::FileStream(L"Unix Tests/ah.jpg", Streaming::AccessRead, Streaming::OpenExisting);
    SafePointer<Codec::Image> img1 = Codec::DecodeImage(file);
    file = new Streaming::FileStream(L"Unix Tests/rar.png", Streaming::AccessRead, Streaming::OpenExisting);
    SafePointer<Codec::Image> img2 = Codec::DecodeImage(file);
    file = new Streaming::FileStream(L"Unix Tests/sad.gif", Streaming::AccessRead, Streaming::OpenExisting);
    SafePointer<Codec::Image> img3 = Codec::DecodeImage(file);

    SafePointer<Streaming::FileStream> file2 = new Streaming::FileStream(L"Unix Tests/raw.ico", Streaming::AccessReadWrite, Streaming::CreateAlways);
    SafePointer<Codec::Image> img5 = new Codec::Image;
    SafePointer<Codec::Frame> frame = new Codec::Frame(128, 128, -1, Codec::PixelFormat::R8G8B8A8, Codec::AlphaFormat::Premultiplied, Codec::LineDirection::TopDown);
    for (int x = 0; x < 128; x++) for (int y = 0; y < 128; y++) {
        frame->SetPixel(x, y, img2->Frames[0].GetPixel(400 + x, 200 + y));
    }
    img5->Frames.Append(frame);
    frame = new Codec::Frame(32, 32, 128, Codec::PixelFormat::B8G8R8A8, Codec::AlphaFormat::Normal, Codec::LineDirection::TopDown);
    for (int x = 0; x < 32; x++) for (int y = 0; y < 32; y++) {
        if (x < 16) {
            if (y < 16) {
                frame->SetPixel(x, y, 0xFF0000FF);
            } else {
                frame->SetPixel(x, y, 0xFFFF0000);
            }
        } else {
            if (y < 16) {
                frame->SetPixel(x, y, 0xFF00FF00);
            } else {
                frame->SetPixel(x, y, 0xFF00FFFF);
            }
        }
    }
    img5->Frames.Append(frame);
    Codec::EncodeImage(file2, img5, L"ICO");

    string in;
    // do {
    //     Input >> in;
    //     if (in.Length()) {
    //         try {
    //             Console << Syntax::Math::CalculateExpressionDouble(in) << IO::NewLineChar;
    //         }
    //         catch (...) { Console << L"pizdets" << IO::NewLineChar; }
    //     }
    // } while (in.Length());
    
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

    NSWindow * window = [[NSWindow alloc]
        initWithContentRect: NSMakeRect(100, 100, 700, 400)
        styleMask: NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable |
            NSWindowStyleMaskResizable
        backing: NSBackingStoreBuffered
        defer: NO];
    [window setTitle: @"Test Window"];
    [window orderFrontRegardless];
    [window display];
    Console << L"Window: " << string((__bridge void*) window) << IO::NewLineChar;
    [NSApp run];

    return 0;
}
