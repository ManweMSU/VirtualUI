#pragma once

#include "Audio.h"

namespace Engine
{
	namespace Audio
	{
		IAudioCodec * InitializeWaveCodec(void);
	}
	namespace Media
	{
		IMediaContainerCodec * InitializeRIFFCodec(void);
	}
}