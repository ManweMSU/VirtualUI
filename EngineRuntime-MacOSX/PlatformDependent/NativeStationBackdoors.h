#pragma once

@import Foundation;
@import AppKit;

@interface EngineRuntimeApplicationDelegate : NSObject<NSApplicationDelegate>
- (void) close_all: (id) sender;
- (void) applicationDidFinishLaunching: (NSNotification *) notification;
- (BOOL) application: (NSApplication *) sender openFile: (NSString *) filename;
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
    }
}