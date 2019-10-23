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
        NSImage * CocoaImage(Codec::Frame * frame) __attribute((ns_returns_retained));
        Codec::Frame * EngineImage(NSImage * image);
    }
}