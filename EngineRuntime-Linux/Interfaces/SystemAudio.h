#pragma once

#include "../Media/Audio.h"

namespace Engine
{
	namespace Audio
	{
		IAudioCodec * InitializeSystemCodec(void);
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void);
		void SystemBeep(void);
	}
}