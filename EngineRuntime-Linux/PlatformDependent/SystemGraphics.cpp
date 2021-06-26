#include "../Interfaces/SystemGraphics.h"
#include "../ImageCodec/IconCodec.h"
#include "../Storage/ImageVolume.h"
#include "LinuxImageCodecs.h"
#include "DeviceX11.h"

namespace Engine
{
	namespace Graphics
	{
		SafePointer<IDevice> _common_device;

		void InitializeSystemCodecCollection(void)
		{
			Linux::InitLinuxStandardCodecs();
			Codec::CreateIconCodec();
			Storage::CreateVolumeCodec();
		}
		IDeviceFactory * CreateSystemDeviceFactory(void)
		{
			// TODO: IMPLEMENT
			return 0;
		}
		IDevice * GetSystemCommonDevice(void)
		{
			if (!_common_device) {
				SafePointer<IDeviceFactory> factory = CreateSystemDeviceFactory();
				if (!factory) return 0;
				_common_device = factory->CreateDefaultDevice();
			}
			return _common_device;
		}
		void ResetSystemCommonDevice(void)
		{
			_common_device.SetReference(0);
			GetSystemCommonDevice();
		}
		UI::IObjectFactory * CreateSystemDrawingObjectFactory(void) { return X11::CreateGraphicsObjectFactory(); }
	}
}