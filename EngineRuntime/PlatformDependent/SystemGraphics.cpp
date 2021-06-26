#include "../Interfaces/SystemGraphics.h"

#include "../PlatformDependent/SystemCodecs.h"
#include "../PlatformDependent/Direct2D.h"
#include "../PlatformDependent/Direct3D.h"
#include "../ImageCodec/IconCodec.h"
#include "../Storage/ImageVolume.h"

namespace Engine
{
	namespace Graphics
	{
		I2DDeviceContextFactory * CreateDeviceContextFactory(void)
		{
			Codec::InitializeDefaultCodecs();
			Direct2D::InitializeFactory();
			Direct2D::CommonFactory->Retain();
			return Direct2D::CommonFactory;
		}
		IDeviceFactory * CreateDeviceFactory(void) { return Direct3D::CreateDeviceFactoryD3D11(); }
		IDevice * GetCommonDevice(void) { Direct3D::CreateDevices(); return Direct3D::WrappedDevice; }
		void ResetCommonDevice(void) { Direct3D::ReleaseDevices(); Direct3D::CreateDevices(); }
	}
	namespace Codec
	{
		void InitializeDefaultCodecs(void)
		{
			WIC::CreateWICodec();
			Codec::CreateIconCodec();
			Storage::CreateVolumeCodec();
		}
	}
}