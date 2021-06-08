#include "../Interfaces/Shell.h"

#include "../PlatformDependent/FileApi.h"
#include "../PlatformDependent/CocoaInterop.h"

@import Foundation;
@import AppKit;

namespace Engine
{
    namespace Shell
    {
        bool OpenFile(const string & file)
        {
            NSString * path = Cocoa::CocoaString(IO::ExpandPath(file));
            auto result = [[NSWorkspace sharedWorkspace] openFile: path];
            [path release];
            return result;
        }
        void ShowInBrowser(const string & path, bool directory)
        {
            if (directory) {
                OpenFile(IO::ExpandPath(path));
            } else {
                string full_path = IO::ExpandPath(path);;
                string directory = IO::Path::GetDirectory(full_path);
                NSString * FileName = Cocoa::CocoaString(full_path);
                NSString * Directory = Cocoa::CocoaString(directory);
                [[NSWorkspace sharedWorkspace] selectFile: FileName inFileViewerRootedAtPath: Directory ];
                [FileName release];
                [Directory release];
            }
        }
        void OpenCommandPrompt(const string & working_directory)
        {
			@autoreleasepool {
				NSArray<NSURL *> * apps = (NSArray<NSURL *> *) LSCopyApplicationURLsForBundleIdentifier((CFStringRef) @"com.apple.Terminal", 0);
				if (![apps count]) return;
				NSString * path = Cocoa::CocoaString(working_directory);
				LSLaunchURLSpec spec;
				spec.appURL = (CFURLRef) apps[0];
				spec.asyncRefCon = 0;
				spec.itemURLs = (CFArrayRef) [NSArray arrayWithObject: [NSURL fileURLWithPath: path]];
				spec.launchFlags = kLSLaunchDefaults;
				spec.passThruParams = 0;
				LSOpenFromURLSpec(&spec, 0);
				[apps release];
				[path release];
			}
        }
    }
}