#pragma once

#include "../ImageCodec/CodecBase.h"

#include <wincodec.h>

namespace Engine
{
	namespace WIC
	{
		extern IWICImagingFactory * WICFactory;

		Codec::ICodec * CreateWICodec(void);
	}
}