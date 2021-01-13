#pragma once

#include "MetalGraphics.h"

namespace Engine
{
	namespace Cocoa
	{
		UI::IRenderingDevice * CreateMetalRenderingDevice(MetalGraphics::MetalPresentationInterface * presentation);
		UI::IRenderingDevice * CreateMetalRenderingDevice(Graphics::IDevice * device);
		id<MTLLibrary> CreateMetalRenderingDeviceShaders(id<MTLDevice> device);
		id<MTLDrawable> CoreMetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, MetalGraphics::MetalPresentationInterface * presentation);
		void CoreMetalRenderingDeviceEndDraw(UI::IRenderingDevice * device, id<MTLDrawable> drawable, bool wait);
		void PureMetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, id<MTLCommandBuffer> command, id<MTLTexture> texture, uint width, uint height);
		void PureMetalRenderingDeviceEndDraw(UI::IRenderingDevice * device);

		id<MTLDevice> GetInnerMetalDevice(UI::IRenderingDevice * device);
	}
}