#pragma once

#include "../Graphics/GraphicsBase.h"
#include "../Graphics/Graphics.h"

namespace Engine
{
	namespace Graphics
	{
		void InitializeSystemCodecCollection(void);
		IDeviceFactory * CreateSystemDeviceFactory(void);
		IDevice * GetSystemCommonDevice(void);
		void ResetSystemCommonDevice(void);
		UI::IObjectFactory * CreateSystemDrawingObjectFactory(void);
	}
}