#pragma once

@import Foundation;
@import AppKit;

#include "../UserInterface/Menus.h"

@interface EngineRuntimeApplicationDelegate : NSObject<NSApplicationDelegate>
- (void) close_all: (id) sender;
- (void) applicationDidFinishLaunching: (NSNotification *) notification;
- (void) application: (NSApplication *) application openURLs: (NSArray<NSURL *> *) urls;
- (BOOL) applicationOpenUntitledFile: (NSApplication *) sender;
- (void) new_file: (id) sender;
- (void) open_file: (id) sender;
- (void) show_props: (id) sender;
- (void) show_help: (id) sender;
- (NSApplicationTerminateReply) applicationShouldTerminate: (NSApplication *) sender;
@end

namespace Engine
{
	namespace NativeWindows
	{
        void InternalShowWindow(UI::WindowStation * Station, bool Show);
        void SetTouchBarObject(UI::WindowStation * Station, Object * Bar);
        NSWindow * GetWindowObject(UI::WindowStation * station);
		NSMenu * CreateCocoaMenu(UI::Menus::Menu * menu, int * result, id target, SEL action, UI::IRenderingDevice * quartz_device);
		void DestroyCocoaMenu(UI::Menus::Menu * menu, NSMenu * cocoa_menu);
    }
}