#pragma once

#include "../EngineBase.h"

#include "Foundation/Foundation.h"

namespace Engine
{
    namespace Cocoa
    {
        NSString * CocoaString(const string & str) __attribute((ns_returns_retained));
        string EngineString(NSString * str);
    }
}