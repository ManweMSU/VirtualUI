#pragma once

#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Cocoa
	{
        Codec::ICodec * CreateAppleCodec(void);
    }
}