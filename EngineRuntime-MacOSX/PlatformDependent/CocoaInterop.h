#pragma once

#include "../EngineBase.h"
#include "../ImageCodec/CodecBase.h"

#include "Foundation/Foundation.h"
#include <ImageIO/ImageIO.h>

namespace Engine
{
    namespace Cocoa
    {
        NSString * CocoaString(const string & str) __attribute((ns_returns_retained));
        string EngineString(NSString * str);
        NSImage * CocoaImage(Codec::Frame * frame, double scale_factor = 1.0) __attribute((ns_returns_retained));
        Codec::Frame * EngineImage(NSImage * image);
		void NormalizeUnicodeString(string & text);
        template <class O> class CocoaPointer
        {
            O object;
        public:
            CocoaPointer(void) { object = 0; }
            CocoaPointer(O src) { object = src; }
            CocoaPointer(const CocoaPointer & ptr) { object = ptr.object; if (object) [object retain]; }
            ~CocoaPointer(void) { if (object) [object release]; }
            CocoaPointer & operator = (const CocoaPointer & ptr)
            {
                if (this == &ptr) return *this;
                if (object) [object release];
                object = ptr.object; if (object) [object retain];
                return *this;
            }

            operator O (void) { return object; }
            operator bool (void) { return object != 0; }
        };
    }
}