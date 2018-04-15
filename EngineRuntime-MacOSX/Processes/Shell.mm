#include "Shell.h"

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
            string path = IO::ExpandPath(working_directory).Replace(L' ', L"\\\\ ");
            string script =
                L"tell application \"Terminal\"\n"
	            L"if it is running then\n"
		        L"if ((count of windows) is 0) then\n"
			    L"activate\n"
			    L"do script \"cd " + path + "\"\n"
		        L"else\n"
			    L"do script \"cd " + path + "\"\n"
			    L"activate\n"
		        L"end if\n"
	            L"else\n"
		        L"activate\n"
		        L"do script \"cd " + path + "\" in window 1\n"
	            L"end if\n"
                L"end tell\n";
            NSString * Script = Cocoa::CocoaString(script);
            NSAppleScript * AppleScript = [[NSAppleScript alloc] initWithSource: Script];
            [AppleScript executeAndReturnError: NULL];
            [Script release];
            [AppleScript release];
        }
    }
}