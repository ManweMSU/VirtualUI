#pragma once

#include "ShapeBase.h"
#include "Templates.h"
#include "ControlClasses.h"
#include "../Streaming.h"

namespace Engine
{
	namespace UI
	{
		namespace Loader
		{
			void LoadUserInterfaceFromBinaryLegacy(InterfaceTemplate & Template, Streaming::Stream * Source, IResourceLoader * ResourceLoader, IResourceResolver * ResourceResolver);
		}
	}
}