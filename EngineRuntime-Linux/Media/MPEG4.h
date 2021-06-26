#pragma once

#include "Media.h"
#include "Subtitles.h"

namespace Engine
{
	namespace Media
	{
		IMediaContainerCodec * InitializeMPEG4Codec(void);
	}
	namespace Subtitles
	{
		ISubtitleCodec * Initialize3GPPTTCodec(void);
	}
}