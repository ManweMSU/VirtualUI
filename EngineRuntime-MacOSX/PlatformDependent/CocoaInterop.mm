#include "CocoaInterop.h"

namespace Engine
{
    namespace Cocoa
    {
        NSString * CocoaString(const string & str) __attribute((ns_returns_retained))
        {
            Array<uint16> utf16(0x100);
            utf16.SetLength(str.GetEncodedLength(Encoding::UTF16));
            str.Encode(utf16.GetBuffer(), Encoding::UTF16, false);
            NSString * New = [[NSString alloc] initWithBytes: utf16.GetBuffer() length: utf16.Length() * 2 encoding: NSUTF16LittleEndianStringEncoding];
            return New;
        }
        string EngineString(NSString * str)
        {
            Array<uint16> utf16(0x100);
            utf16.SetLength([str length]);
            [str getBytes: (utf16.GetBuffer()) maxLength: 2 * utf16.Length() usedLength: NULL encoding: NSUTF16LittleEndianStringEncoding
                options: 0 range: NSMakeRange(0, utf16.Length()) remainingRange: NULL];
            return string(utf16.GetBuffer(), utf16.Length(), Engine::Encoding::UTF16);
        }
    }
}