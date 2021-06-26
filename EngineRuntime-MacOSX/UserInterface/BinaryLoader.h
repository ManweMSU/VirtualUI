#pragma once

#include "Templates.h"
#include "../Streaming.h"

namespace Engine
{
	namespace UI
	{
		namespace Loader
		{
			void LoadUserInterfaceFromBinary(InterfaceTemplate & Template, Streaming::Stream * Source, Graphics::I2DDeviceContextFactory * ResourceLoader = 0, IResourceResolver * ResourceResolver = 0);
			void LoadUserInterfaceWithStyleSet(InterfaceTemplate & Template, InterfaceTemplate & Styles, Streaming::Stream * Source, Graphics::I2DDeviceContextFactory * ResourceLoader = 0, IResourceResolver * ResourceResolver = 0);
		}
	}
}