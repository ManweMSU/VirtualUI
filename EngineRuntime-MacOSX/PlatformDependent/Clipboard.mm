#include "Clipboard.h"

#include "CocoaInterop.h"

#include "AppKit/AppKit.h"

namespace Engine
{
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format)
        {
            bool result = false;
            NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
            NSArray<NSPasteboardType> * types = [pasteboard types];
            for (int i = 0; i < [types count]; i++) {
                if ([types[i] isEqualToString: NSPasteboardTypeString] && format == Format::Text) result = true;
            }
            return result;
        }
		bool GetData(string & value)
        {
            NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
            NSString * text = [pasteboard stringForType: NSPasteboardTypeString];
            if (text) {
                value = Cocoa::EngineString(text);
                return true;
            } else return false;
        }
		bool SetData(const string & value)
        {
            NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
            NSString * text = Cocoa::CocoaString(value);
            [pasteboard clearContents];
            bool result = [pasteboard setString: text forType: NSPasteboardTypeString];
            [text release];
            return result;
        }
	}
}