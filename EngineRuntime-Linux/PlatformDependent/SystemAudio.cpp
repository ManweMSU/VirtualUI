#include "../Interfaces/SystemAudio.h"
#include "CoreX11.h"

namespace Engine
{
	namespace Audio
	{
		IAudioCodec * InitializeSystemCodec(void)
		{
			// TODO: IMPLEMENT
			return 0;
		}
		IAudioDeviceFactory * CreateSystemAudioDeviceFactory(void)
		{
			// TODO: IMPLEMENT
			return 0;
		}
		void SystemBeep(void) { auto ws = Windows::GetWindowSystem(); if (ws) static_cast<X11::IXWindowSystem *>(ws)->Beep(); }
	}
}