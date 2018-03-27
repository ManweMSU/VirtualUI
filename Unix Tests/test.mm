#include "../EngineRuntime-MacOSX/EngineBase.h"
#include "../EngineRuntime-MacOSX/Streaming.h"
#include "../EngineRuntime-MacOSX/PlatformDependent/FileApi.h"

using namespace Engine;

#include <stdio.h>
#include <unistd.h>

@import Foundation;
@import AppKit;

int main(int argc, char ** argv)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    Console << IO::GetCurrentDirectory() << IO::NewLineChar;
    Console << IO::GetExecutablePath() << IO::NewLineChar;

    string path = IO::Path::GetDirectory(IO::GetExecutablePath()) + string(IO::PathChar) + L".." + string(IO::PathChar) + L".." + string(IO::PathChar) + L"..";
    Console << L"cd: " << path << IO::NewLineChar;

    IO::SetCurrentDirectory(path);

    Console << IO::GetCurrentDirectory() << IO::NewLineChar;

    SafePointer<Streaming::FileStream> Input = new Streaming::FileStream(L"in.txt", Streaming::AccessRead, Streaming::OpenExisting);
    SafePointer<Streaming::FileStream> LogOutput = new Streaming::FileStream(L"пидор 🌹.txt", Streaming::AccessWrite, Streaming::CreateAlways);
    Streaming::TextWriter Log(LogOutput);

    Console << int(sizeof(off_t)) << IO::NewLineChar;

    Array<uint8> chars;
    chars.SetLength(Input->Length());
    Input->Read(chars.GetBuffer(), Input->Length());
    string str = string(chars.GetBuffer(), chars.Length(), Encoding::UTF8);

    Console << str << IO::NewLineChar << L"корневген пидор" << IO::NewLineChar << IO::GetCurrentDirectory() << IO::NewLineChar;

    Log.WriteEncodingSignature();
    Log << str << IO::NewLineChar << L"Пидор 🌹" << IO::NewLineChar << IO::GetCurrentDirectory();
    
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
