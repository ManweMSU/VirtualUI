#pragma once

#include "Templates.h"
#include "../Streaming.h"

namespace Engine
{
	namespace UI
	{
		namespace Loader
		{
			void LoadUserInterfaceFromBinaryLegacy(InterfaceTemplate & Template, Streaming::Stream * Source, Graphics::I2DDeviceContextFactory * ResourceLoader, IResourceResolver * ResourceResolver);
		}
	}
}