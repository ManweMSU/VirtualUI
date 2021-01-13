#include "MetalGraphics.h"

#include "NativeStationBackdoors.h"
#include "CocoaInterop.h"

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

		class MTL_Device : public IDevice
		{
		public:
			id<MTLDevice> device;
			id<MTLCommandQueue> queue;

			MTL_Device(id<MTLDevice> _device) : device(_device)
			{
				[device retain];
				queue = [device newCommandQueue];
			}
			virtual ~MTL_Device(void) override
			{
				[device release];
				[queue release];
			}
			virtual string GetDeviceName(void) noexcept override { return Cocoa::EngineString([device name]); }
			virtual uint64 GetDeviceIdentifier(void) noexcept override { return [device registryID]; }
			virtual bool DeviceIsValid(void) noexcept override { return true; }
			virtual void GetImplementationInfo(string & tech, uint32 & version) noexcept override
			{
				tech = L"Metal";
				if ([device supportsFamily: MTLGPUFamilyMac2]) version = 2;
				else if ([device supportsFamily: MTLGPUFamilyMac1]) version = 2;
				else version = 0;
			}
			virtual IShaderLibrary * LoadShaderLibrary(const void * data, int length) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IShaderLibrary * LoadShaderLibrary(const DataBlock * data) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IShaderLibrary * LoadShaderLibrary(Streaming::Stream * stream) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IDeviceContext * GetDeviceContext(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IPipelineState * CreateRenderingPipelineState(const PipelineStateDesc & desc) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual ISamplerState * CreateSamplerState(const SamplerDesc & desc) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IBuffer * CreateBuffer(const BufferDesc & desc) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IBuffer * CreateBuffer(const BufferDesc & desc, const ResourceInitDesc & init) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual ITexture * CreateTexture(const TextureDesc & desc) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual ITexture * CreateTexture(const TextureDesc & desc, const ResourceInitDesc * init) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual IWindowLayer * CreateWindowLayer(UI::Window * window, const WindowLayerDesc & desc) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual string ToString(void) const override { return L"MTL_Device"; }
		};
		class MTL_Factory : public IDeviceFactory
		{
		public:
			virtual Dictionary::PlainDictionary<uint64, string> * GetAvailableDevices(void) noexcept override
			{
				SafePointer< Dictionary::PlainDictionary<uint64, string> > result = new (std::nothrow) Dictionary::PlainDictionary<uint64, string>(0x10);
				NSArray< id<MTLDevice> > * devs = MTLCopyAllDevices();
				try {
					for (int i = 0; i < [devs count]; i++) {
						auto dev = [devs objectAtIndex: i];
						result->Append([dev registryID], Cocoa::EngineString([dev name]));
					}
				} catch (...) {
					[devs release];
					return 0;
				}
				[devs release];
				result->Retain();
				return result;
			}
			virtual IDevice * CreateDevice(uint64 identifier) noexcept override
			{
				NSArray< id<MTLDevice> > * devs = MTLCopyAllDevices();
				id<MTLDevice> selected = 0;
				for (int i = 0; i < [devs count]; i++) {
					auto dev = [devs objectAtIndex: i];
					if ([dev registryID] == identifier) { selected = dev; break; }
				}
				if (selected) {
					SafePointer<MTL_Device> device = new (std::nothrow) MTL_Device(selected);
					[devs release];
					if (!device) return 0;
					device->Retain();
					return device;
				} else {
					[devs release];
					return 0;
				}
			}
			virtual IDevice * CreateDefaultDevice(void) noexcept override
			{
				id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
				SafePointer<MTL_Device> device = new (std::nothrow) MTL_Device(dev);
				[dev release];
				if (!device) return 0;
				device->Retain();
				return device;
			}
			virtual string ToString(void) const override { return L"MTL_Factory"; }
		};

		SafePointer<MTL_Device> common_device;

		Graphics::IDeviceFactory * CreateMetalDeviceFactory(void)
		{
			SafePointer<MTL_Factory> factory = new (std::nothrow) MTL_Factory;
			if (!factory) return 0;
			factory->Retain();
			return factory;
		}
		Graphics::IDevice * GetMetalCommonDevice(void)
		{
			if (!common_device) {
				id<MTLDevice> dev = MTLCreateSystemDefaultDevice();
				common_device = new (std::nothrow) MTL_Device(dev);
				[dev release];
			}
			return common_device;
		}

		id<MTLDevice> GetInnerMetalDevice(Graphics::IDevice * wrapper) { return static_cast<MTL_Device *>(wrapper)->device; }
		id<MTLCommandQueue> GetInnerMetalQueue(Graphics::IDevice * wrapper) { return static_cast<MTL_Device *>(wrapper)->queue; }
	}
}