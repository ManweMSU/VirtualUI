#include "../EngineRuntime-MacOSX/EngineRuntime.h"

using namespace Engine;

#include <stdio.h>
#include <unistd.h>

@import Foundation;
@import AppKit;

SafePointer<Semaphore> write_sem;

int task(void * arg)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    for (int i = 0; i < 20; i++) {
        write_sem->Wait();
        Console << string(reinterpret_cast<widechar *>(arg)) << IO::NewLineChar;
        write_sem->Open();
        Sleep(100);
    }
    return 666;
}

int main(int argc, char ** argv)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    SafePointer<Streaming::FileStream> ConsoleInStream = new Streaming::FileStream(IO::GetStandartInput());
    Streaming::TextReader Input(ConsoleInStream, Encoding::UTF8);

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

    write_sem.SetReference(CreateSemaphore(1));

    SafePointer<Thread> t1 = CreateThread(task, (void*) L"text A");
    SafePointer<Thread> t2 = CreateThread(task, (void*) L"text B");

    for (int i = 0; i < 30; i++) {
        write_sem->Wait();
        if (t1->Exited()) {
            Console << L"Thread A: exited with " + string(t1->GetExitCode()) << IO::NewLineChar;
        } else {
            Console << L"Thread A: working " << IO::NewLineChar;
        }
        if (t2->Exited()) {
            Console << L"Thread B: exited with " + string(t2->GetExitCode()) << IO::NewLineChar;
        } else {
            Console << L"Thread B: working " << IO::NewLineChar;
        }
        write_sem->Open();
        Sleep(100);
    }

    t1->Wait();
    t1->Release();
    t2->Release();
    
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
