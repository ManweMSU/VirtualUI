#pragma once

#include "../Graphics/GraphicsBase.h"
#include "../Graphics/Graphics.h"

namespace Engine
{
	namespace Graphics
	{
		I2DDeviceContextFactory * CreateDeviceContextFactory(void);
		IDeviceFactory * CreateDeviceFactory(void);
		IDevice * GetCommonDevice(void);
		void ResetCommonDevice(void);
	}
	namespace Codec
	{
		void InitializeDefaultCodecs(void);
	}
}