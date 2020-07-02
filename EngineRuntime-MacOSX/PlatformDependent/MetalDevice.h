#pragma once

#include "../UserInterface/ControlBase.h"

#import "Foundation/Foundation.h"
#import "AppKit/AppKit.h"
#import "Metal/Metal.h"

namespace Engine
{
	namespace Cocoa
	{
		typedef void (* RenderContentsCallback) (UI::WindowStation * station);
        UI::IRenderingDevice * CreateMetalDeviceAndView(NSView * parent_view, UI::WindowStation * station, RenderContentsCallback callback);
		void SetMetalRenderingDeviceHandle(UI::IRenderingDevice * device, id<MTLDevice> mtl_device);
		void SetMetalRenderingDeviceShaderLibrary(UI::IRenderingDevice * device, id<MTLLibrary> mtl_shaders, MTLPixelFormat format);
		void SetMetalRenderingDeviceQueue(UI::IRenderingDevice * device, id<MTLCommandQueue> mtl_queue);
		void SetMetalRenderingDeviceState(UI::IRenderingDevice * device, int view_width, int view_height);
		void MetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, MTLRenderPassDescriptor * pass_descriptor);
		void MetalRenderingDeviceEndDraw(UI::IRenderingDevice * device, id<MTLDrawable> drawable, bool wait);
		void InvalidateMetalDeviceContents(UI::IRenderingDevice * device);
		
		id<MTLTexture> QueryTextureMetalSurface(Graphics::ITexture * texture);
		int QueryTextureWidth(Graphics::ITexture * texture);
		int QueryTextureHeight(Graphics::ITexture * texture);
		id<MTLDevice> GetMetalSharedDevice(void);
		id<MTLCommandQueue> GetMetalSharedQueue(void);
	}
}