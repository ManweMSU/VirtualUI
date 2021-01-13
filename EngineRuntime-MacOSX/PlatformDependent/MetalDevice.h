#pragma once

#include "MetalGraphics.h"

namespace Engine
{
	namespace Cocoa
	{
		UI::IRenderingDevice * CreateMetalRenderingDevice(MetalGraphics::MetalPresentationInterface * presentation);
		id<MTLLibrary> CreateMetalRenderingDeviceShaders(id<MTLDevice> device);
		id<MTLDrawable> CoreMetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, MetalGraphics::MetalPresentationInterface * presentation);
		void CoreMetalRenderingDeviceEndDraw(UI::IRenderingDevice * device, id<MTLDrawable> drawable, bool wait);

		id<MTLDevice> GetInnerMetalDevice(UI::IRenderingDevice * device);
	}
}