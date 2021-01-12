#include "Graphics.h"

#include "../PlatformDependent/NativeStation.h"

namespace Engine
{
	namespace Graphics
	{
		VolumeIndex::VolumeIndex(void) : x(0), y(0), z(0) {}
		VolumeIndex::VolumeIndex(uint32 sx) : x(sx), y(0), z(0) {}
		VolumeIndex::VolumeIndex(uint32 sx, uint32 sy) : x(sx), y(sy), z(0) {}
		VolumeIndex::VolumeIndex(uint32 sx, uint32 sy, uint32 sz) : x(sx), y(sy), z(sz) {}
		SubresourceIndex::SubresourceIndex(void) : mip_level(0), array_index(0) {}
		SubresourceIndex::SubresourceIndex(uint32 mip, uint32 index) : mip_level(mip), array_index(index) {}
		IDeviceFactory * CreateDeviceFactory(void) { return NativeWindows::CreateDeviceFactory(); }
		IDevice * GetCommonDevice(void) { return NativeWindows::GetCommonDevice(); }
		bool IsColorFormat(PixelFormat format) { return (uint32(format) & 0xF0000000) == 0x80000000; }
		bool IsDepthStencilFormat(PixelFormat format) { return (uint32(format) & 0xF0000000) == 0x40000000; }
		int GetFormatChannelCount(PixelFormat format) { return (uint32(format) & 0x00F00000) >> 20; }
		int GetFormatBitsPerPixel(PixelFormat format)
		{
			auto bpp = (uint32(format) & 0x0F000000);
			if (bpp == 0x01000000) return 8;
			else if (bpp == 0x02000000) return 16;
			else if (bpp == 0x03000000) return 32;
			else if (bpp == 0x04000000) return 64;
			else if (bpp == 0x05000000) return 128;
			else return 0;
		}
	}
}