#pragma once

#include "MetalGraphics.h"

namespace Engine
{
	namespace Cocoa
	{
		UI::IRenderingDevice * CreateMetalDevice(MetalGraphics::MetalPresentationInterface * presentation); // TODO: REWORK
		void SetMetalRenderingDeviceHandle(UI::IRenderingDevice * device, id<MTLDevice> mtl_device);
		void SetMetalRenderingDeviceShaderLibrary(UI::IRenderingDevice * device, id<MTLLibrary> mtl_shaders, MTLPixelFormat format);
		void SetMetalRenderingDeviceQueue(UI::IRenderingDevice * device, id<MTLCommandQueue> mtl_queue);
		void SetMetalRenderingDeviceState(UI::IRenderingDevice * device, int view_width, int view_height);
		void MetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, MTLRenderPassDescriptor * pass_descriptor); // TODO: REWORK
		void MetalRenderingDeviceEndDraw(UI::IRenderingDevice * device, id<MTLDrawable> drawable, bool wait); // TODO: REWORK
		
		// id<MTLTexture> QueryTextureMetalSurface(Graphics::ITexture * texture);
		// int QueryTextureWidth(Graphics::ITexture * texture);
		// int QueryTextureHeight(Graphics::ITexture * texture);
		// id<MTLDevice> GetMetalSharedDevice(void);
		// id<MTLCommandQueue> GetMetalSharedQueue(void);
	}
}