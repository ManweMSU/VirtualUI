#include "MetalGraphics.h"

#include "NativeStationBackdoors.h"

using namespace Engine;
using namespace Engine::Graphics;

// Begin Interface Declaration
	@interface EngineMetalView : NSView<CALayerDelegate>
	{
	@public
		Engine::UI::WindowStation * station_ref;
		Engine::MetalGraphics::PresentationRenderContentsCallback callback_ref;
		Engine::MetalGraphics::MetalPresentationInterface * presentation_interface;
		CAMetalLayer * layer;
		double w, h;
		bool autosize;
	}
	- (instancetype) init;
	- (void) dealloc;
	- (void) displayLayer: (CALayer *) layer;
	- (void) setFrame: (NSRect) frame;
	- (void) setFrameSize: (NSSize) newSize;
	@end
	@implementation EngineMetalView
	- (instancetype) init
	{
		[super init];
		layer = [CAMetalLayer layer];
		[layer setDelegate: self];
		[self setLayer: layer];
		[self setWantsLayer: YES];
		w = h = 1;
		autosize = false;
		return self;
	}
	- (void) dealloc
	{
		if (presentation_interface) presentation_interface->metal_view = 0;
		[super dealloc];
	}
	- (void) displayLayer: (CALayer *) sender { @autoreleasepool { if (callback_ref) callback_ref(station_ref); } }
	- (void) setFrame: (NSRect) frame
	{
		[super setFrame: frame];
		if (autosize) {
			double scale = [[self window] backingScaleFactor];
			w = max(frame.size.width * scale, 1.0); h = max(frame.size.height * scale, 1.0);
			[layer setDrawableSize: NSMakeSize(w, h)]; [layer setNeedsDisplay];
		}
	}
	- (void) setFrameSize: (NSSize) newSize
	{
		[super setFrameSize: newSize];
		if (autosize) {
			double scale = [[self window] backingScaleFactor];
			w = max(newSize.width * scale, 1.0); h = max(newSize.height * scale, 1.0);
			[layer setDrawableSize: NSMakeSize(w, h)]; [layer setNeedsDisplay];
		}
	}
	@end
// End Interface Declaration

namespace Engine
{
	namespace MetalGraphics
	{
		MetalPresentationInterface::MetalPresentationInterface(NSView * parent_view, UI::WindowStation * station, PresentationRenderContentsCallback callback)
		{
			EngineMetalView * _metal_view = [[EngineMetalView alloc] init];
			_metal_view->station_ref = station;
			_metal_view->callback_ref = callback;
			_metal_view->presentation_interface = this;
			[parent_view addSubview: _metal_view];
			[parent_view setWantsLayer: YES];
			[_metal_view setFrameSize: [parent_view frame].size];
			[_metal_view setAutoresizingMask: NSViewWidthSizable | NSViewHeightSizable];
			metal_view = _metal_view;
		}
		MetalPresentationInterface::~MetalPresentationInterface(void) { if (metal_view) { reinterpret_cast<EngineMetalView *>(metal_view)->presentation_interface = 0; [metal_view release]; } }
		id<MTLDevice> MetalPresentationInterface::GetLayerDevice(void) { if (metal_view) return [reinterpret_cast<EngineMetalView *>(metal_view)->layer device]; else return 0; }
		void MetalPresentationInterface::SetLayerDevice(id<MTLDevice> device) { if (metal_view) [reinterpret_cast<EngineMetalView *>(metal_view)->layer setDevice: device]; }
		MTLPixelFormat MetalPresentationInterface::GetPixelFormat(void) { if (metal_view) return [reinterpret_cast<EngineMetalView *>(metal_view)->layer pixelFormat]; else return MTLPixelFormatInvalid; }
		void MetalPresentationInterface::SetPixelFormat(MTLPixelFormat format) { if (metal_view) [reinterpret_cast<EngineMetalView *>(metal_view)->layer setPixelFormat: format]; }
		UI::Point MetalPresentationInterface::GetLayerSize(void)
		{
			if (metal_view) return UI::Point(reinterpret_cast<EngineMetalView *>(metal_view)->w, reinterpret_cast<EngineMetalView *>(metal_view)->h);
			else return UI::Point(1, 1);
		}
		void MetalPresentationInterface::SetLayerSize(UI::Point size) { if (metal_view) [reinterpret_cast<EngineMetalView *>(metal_view)->layer setDrawableSize: NSMakeSize(size.x, size.y)]; }
		void MetalPresentationInterface::SetLayerAutosize(bool set)
		{
			if (metal_view) {
				auto view = reinterpret_cast<EngineMetalView *>(metal_view);
				auto size = [metal_view frame].size;
				view->autosize = set;
				if (set) {
					double scale = [[view window] backingScaleFactor];
					view->w = max(size.width * scale, 1.0);
					view->h = max(size.height * scale, 1.0);
					[view->layer setDrawableSize: NSMakeSize(view->w, view->h)];
					[view->layer setNeedsDisplay];
				}
			}
		}
		void MetalPresentationInterface::SetLayerOpaque(bool set) { if (metal_view) [reinterpret_cast<EngineMetalView *>(metal_view)->layer setOpaque: set]; }
		void MetalPresentationInterface::SetLayerFramebufferOnly(bool set) { if (metal_view) [reinterpret_cast<EngineMetalView *>(metal_view)->layer setFramebufferOnly: set]; }
		id<CAMetalDrawable> MetalPresentationInterface::GetLayerDrawable(void) { if (metal_view) return [reinterpret_cast<EngineMetalView *>(metal_view)->layer nextDrawable]; else return 0; }
		void MetalPresentationInterface::InvalidateContents(void) { if (metal_view) [reinterpret_cast<EngineMetalView *>(metal_view)->layer setNeedsDisplay]; }

		// Engine::NativeWindows::InitWindowPresentationInterface(station)  - to create layer

		Graphics::IDeviceFactory * CreateMetalDeviceFactory(void)
		{
			// TODO: IMPLEMENT
			return 0;
		}
		Graphics::IDevice * GetMetalCommonDevice(void)
		{
			// TODO: IMPLEMENT
			return 0;
		}
	}
}