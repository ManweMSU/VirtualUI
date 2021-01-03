#include "Graphics.h"

#include "../PlatformDependent/NativeStation.h"

namespace Engine
{
	namespace Graphics
	{
		VolumeIndex::VolumeIndex(void) {}
		VolumeIndex::VolumeIndex(uint32 sx) : x(sx), y(0), z(0) {}
		VolumeIndex::VolumeIndex(uint32 sx, uint32 sy) : x(sx), y(sy), z(0) {}
		VolumeIndex::VolumeIndex(uint32 sx, uint32 sy, uint32 sz) : x(sx), y(sy), z(sz) {}
		SubresourceIndex::SubresourceIndex(void) {}
		SubresourceIndex::SubresourceIndex(uint32 mip, uint32 index) : mip_level(mip), array_index(index) {}
		IDeviceFactory * CreateDeviceFactory(void) { return NativeWindows::CreateDeviceFactory(); }
		IDevice * GetCommonDevice(void) { return NativeWindows::GetCommonDevice(); }
	}
}