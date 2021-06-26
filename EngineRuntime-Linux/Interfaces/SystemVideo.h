#pragma once

#include "../Media/Video.h"

namespace Engine
{
	namespace Video
	{
		IVideoCodec * InitializeSystemCodec(void);
		IVideoFactory * CreateSystemVideoFactory(void);
	}
}