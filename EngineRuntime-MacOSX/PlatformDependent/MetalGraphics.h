#pragma once

#include "../UserInterface/ControlBase.h"
#include "../Graphics/Graphics.h"

#import "Foundation/Foundation.h"
#import "QuartzCore/QuartzCore.h"
#import "AppKit/AppKit.h"
#import "Metal/Metal.h"

namespace Engine
{
	namespace MetalGraphics
	{
		typedef void (* PresentationRenderContentsCallback) (UI::WindowStation * station);

		class MetalPresentationInterface : public Object
		{
		public:
			NSView * metal_view;

			MetalPresentationInterface(NSView * parent_view, UI::WindowStation * station, PresentationRenderContentsCallback callback);
			virtual ~MetalPresentationInterface(void) override;

			id<MTLDevice> GetLayerDevice(void);
			void SetLayerDevice(id<MTLDevice> device);
			MTLPixelFormat GetPixelFormat(void);
			void SetPixelFormat(MTLPixelFormat format);
			UI::Point GetLayerSize(void);
			void SetLayerSize(UI::Point size);
			void SetLayerAutosize(bool set);
			void SetLayerOpaque(bool set);
			void SetLayerFramebufferOnly(bool set);
			id<CAMetalDrawable> GetLayerDrawable(void);
			void InvalidateContents(void);
		};

		Graphics::IDeviceFactory * CreateMetalDeviceFactory(void);
		Graphics::IDevice * GetMetalCommonDevice(void);

		id<MTLDevice> GetInnerMetalDevice(Graphics::IDevice * wrapper);
		id<MTLCommandQueue> GetInnerMetalQueue(Graphics::IDevice * wrapper);
	}
}