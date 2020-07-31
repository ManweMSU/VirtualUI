#pragma once

#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Storage
	{
		Codec::ICodec * CreateVolumeCodec(void);
		void SetVolumeCodecLoadBestDpiOnly(bool only);
		bool IsVolumeCodecLoadsBestDpiOnly(void);
	}
}