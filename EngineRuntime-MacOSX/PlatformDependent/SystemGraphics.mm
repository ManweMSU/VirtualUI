#include "../Interfaces/SystemGraphics.h"

#include "QuartzDevice.h"
#include "MetalGraphics.h"
#include "AppleCodec.h"
#include "../ImageCodec/IconCodec.h"
#include "../Storage/ImageVolume.h"

namespace Engine
{
	namespace Graphics
	{
		I2DDeviceContextFactory * CreateDeviceContextFactory(void) { auto dcf = Cocoa::GetCommonDeviceContextFactory(); if (dcf) dcf->Retain(); return dcf; }
		IDeviceFactory * CreateDeviceFactory(void) { return MetalGraphics::CreateMetalDeviceFactory(); }
		IDevice * GetCommonDevice(void) { return MetalGraphics::GetMetalCommonDevice(); }
		void ResetCommonDevice(void) {}
	}
	namespace Codec
	{
		void InitializeDefaultCodecs(void)
		{
			Cocoa::CreateAppleCodec();
			Codec::CreateIconCodec();
			Storage::CreateVolumeCodec();
		}
	}
}