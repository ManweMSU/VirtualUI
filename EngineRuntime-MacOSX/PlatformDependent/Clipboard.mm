#include "Clipboard.h"

#include "CocoaInterop.h"

#include "AppKit/AppKit.h"

namespace Engine
{
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format)
        {
            @autoreleasepool {
                if (format == Format::Text) {
                    bool result = false;
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<NSPasteboardType> * types = [pasteboard types];
                    for (int i = 0; i < [types count]; i++) {
                        if ([types[i] isEqualToString: NSPasteboardTypeString] && format == Format::Text) result = true;
                    }
                    return result;
                } else if (format == Format::Image) {
                    NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                    NSArray<Class> * clss = [NSArray arrayWithObject: [NSImage class]];
                    NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                    if ([pasteboard canReadObjectForClasses: clss options: dict]) return true; else return false;
                } else return false;
            }
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
        bool GetData(Codec::Frame ** value)
        {
            @autoreleasepool {
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                NSArray<Class> * clss = [NSArray arrayWithObject: [NSImage class]];
                NSDictionary<NSPasteboardReadingOptionKey, id> * dict = [NSDictionary dictionary];
                NSArray<NSImage *> * images = [pasteboard readObjectsForClasses: clss options: dict];
                if (!images || ![images count]) return false;
                *value = Cocoa::EngineImage([images firstObject]);
                return true;
            }
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
        bool SetData(Codec::Frame * value)
        {
            @autoreleasepool {
                NSImage * image = Cocoa::CocoaImage(value);
                NSArray<NSImage *> * objs = [NSArray arrayWithObject: image];
                NSPasteboard * pasteboard = [NSPasteboard generalPasteboard];
                [pasteboard clearContents];
                BOOL result = [pasteboard writeObjects: objs];
                [image release];
                return result ? true : false;
            }
        }
	}
}