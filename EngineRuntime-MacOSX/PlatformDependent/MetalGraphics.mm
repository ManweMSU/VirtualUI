#include "MetalGraphics.h"

#include "NativeStationBackdoors.h"
#include "MetalDevice.h"
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

		class MTL_Buffer : public IBuffer
		{
			IDevice * wrapper;
		public:
			id<MTLBuffer> buffer;
			uint32 size, usage;

			MTL_Buffer(IDevice * _wrapper) : wrapper(_wrapper), buffer(0), size(0), usage(0) {}
			virtual ~MTL_Buffer(void) override { [buffer release]; }
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual ResourceType GetResourceType(void) noexcept override { return ResourceType::Buffer; }
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept override { return ResourceMemoryPool::Default; }
			virtual uint32 GetResourceUsage(void) noexcept override { return usage; }
			virtual uint32 GetLength(void) noexcept override { return size; }
			virtual string ToString(void) const override { return L"MTL_Buffer"; }
		};
		class MTL_Texture : public ITexture
		{
			IDevice * wrapper;
		public:
			id<MTLTexture> texture;
			TextureType type;
			PixelFormat format;
			uint32 usage, width, height, depth, size;

			MTL_Texture(IDevice * _wrapper) : wrapper(_wrapper), texture(0), format(PixelFormat::Invalid), usage(0), width(0), height(0), depth(0), size(0) {}
			virtual ~MTL_Texture(void) override { [texture release]; }
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual ResourceType GetResourceType(void) noexcept override { return ResourceType::Texture; }
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept override { return ResourceMemoryPool::Default; }
			virtual uint32 GetResourceUsage(void) noexcept override { return usage; }
			virtual TextureType GetTextureType(void) noexcept override { return type; }
			virtual PixelFormat GetPixelFormat(void) noexcept override { return format; }
			virtual uint32 GetWidth(void) noexcept override { return width; }
			virtual uint32 GetHeight(void) noexcept override { return height; }
			virtual uint32 GetDepth(void) noexcept override { return depth; }
			virtual uint32 GetMipmapCount(void) noexcept override { return [texture mipmapLevelCount]; }
			virtual uint32 GetArraySize(void) noexcept override { return size; }
			virtual string ToString(void) const override { return L"MTL_Texture"; }
		};
		class MTL_DeviceContext : public IDeviceContext
		{
			SafePointer<UI::IRenderingDevice> device_2d;
			IDevice * wrapper;
		public:
			id<MTLDevice> device;
			id<MTLCommandQueue> queue;
			id<MTLCommandBuffer> current_command;
			id<MTLRenderCommandEncoder> render_command_encoder;
			id<MTLBlitCommandEncoder> blit_command_encoder;
			NSAutoreleasePool * autorelease_pool;
			uint32 state;
			bool error_state;

			MTL_DeviceContext(IDevice * _wrapper, id<MTLDevice> _device, id<MTLCommandQueue> _queue) : wrapper(_wrapper), device(_device), queue(_queue),
				current_command(0), state(0), autorelease_pool(0), render_command_encoder(0), blit_command_encoder(0)
			{
				[device retain];
				[queue retain];
			}
			virtual ~MTL_DeviceContext(void) override
			{
				[autorelease_pool release];
				[device release];
				[queue release];
				[current_command release];
			}
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept override
			{
				if (state) return false;
				if (!rtc || !rtv) return false;
				autorelease_pool = [[NSAutoreleasePool alloc] init];
				if (!current_command) {
					current_command = [queue commandBuffer];
					[current_command retain];
				}
				MTLRenderPassDescriptor * desc = [MTLRenderPassDescriptor renderPassDescriptor];
				for (uint i = 0; i < rtc; i++) {
					if (!rtv[i].Texture || !(rtv[i].Texture->GetResourceUsage() & ResourceUsageRenderTarget)) {
						[autorelease_pool release];
						autorelease_pool = 0;
						return false;
					}
					MTLRenderPassColorAttachmentDescriptor * ca = [[MTLRenderPassColorAttachmentDescriptor alloc] init];
					[ca autorelease];
					ca.texture = GetInnerMetalTexture(rtv[i].Texture);
					if (rtv[i].LoadAction == TextureLoadAction::Clear) {
						ca.loadAction = MTLLoadActionClear;
						MTLClearColor clr;
						clr.red = rtv[i].ClearValue[0];
						clr.green = rtv[i].ClearValue[1];
						clr.blue = rtv[i].ClearValue[2];
						clr.alpha = rtv[i].ClearValue[3];
						ca.clearColor = clr;
					} else if (rtv[i].LoadAction == TextureLoadAction::Load) ca.loadAction = MTLLoadActionLoad;
					else if (rtv[i].LoadAction == TextureLoadAction::DontCare) ca.loadAction = MTLLoadActionDontCare;
					ca.storeAction = MTLStoreActionStore;
					[desc.colorAttachments setObject: ca atIndexedSubscript: i];
				}
				if (dsv) {
					if (!dsv->Texture || !(dsv->Texture->GetResourceUsage() & ResourceUsageDepthStencil)) {
						[autorelease_pool release];
						autorelease_pool = 0;
						return false;
					}
					auto format = dsv->Texture->GetPixelFormat();
					MTLRenderPassDepthAttachmentDescriptor * da = [[MTLRenderPassDepthAttachmentDescriptor alloc] init];
					[da autorelease];
					da.texture = GetInnerMetalTexture(dsv->Texture);
					if (dsv->DepthLoadAction == TextureLoadAction::Clear) {
						da.loadAction = MTLLoadActionClear;
						da.clearDepth = dsv->DepthClearValue;
					} else if (dsv->DepthLoadAction == TextureLoadAction::Load) da.loadAction = MTLLoadActionLoad;
					else if (dsv->DepthLoadAction == TextureLoadAction::DontCare) da.loadAction = MTLLoadActionDontCare;
					da.storeAction = MTLStoreActionStore;
					desc.depthAttachment = da;
					if (format == PixelFormat::D24S8_unorm || format == PixelFormat::D32S8_float) {
						MTLRenderPassStencilAttachmentDescriptor * sa = [[MTLRenderPassStencilAttachmentDescriptor alloc] init];
						[sa autorelease];
						sa.texture = GetInnerMetalTexture(dsv->Texture);
						if (dsv->StencilLoadAction == TextureLoadAction::Clear) {
							sa.loadAction = MTLLoadActionClear;
							sa.clearStencil = dsv->StencilClearValue;
						} else if (dsv->StencilLoadAction == TextureLoadAction::Load) sa.loadAction = MTLLoadActionLoad;
						else if (dsv->StencilLoadAction == TextureLoadAction::DontCare) sa.loadAction = MTLLoadActionDontCare;
						sa.storeAction = MTLStoreActionStore;
						desc.stencilAttachment = sa;
					}
				}
				render_command_encoder = [current_command renderCommandEncoderWithDescriptor: desc];
				state = 1;
				error_state = true;
				return true;
			}
			virtual bool Begin2DRenderingPass(ITexture * rt) noexcept override
			{
				if (state) return false;
				if (!device_2d) {
					device_2d = Cocoa::CreateMetalRenderingDevice(wrapper);
					if (!device_2d) return false;
				}
				if (!(rt->GetResourceUsage() & ResourceUsageRenderTarget)) return false;
				if (rt->GetTextureType() != TextureType::Type2D) return false;
				if (rt->GetPixelFormat() != PixelFormat::B8G8R8A8_unorm) return false;
				if (rt->GetMipmapCount() != 1) return false;
				autorelease_pool = [[NSAutoreleasePool alloc] init];
				if (!current_command) {
					current_command = [queue commandBuffer];
					[current_command retain];
				}
				Cocoa::PureMetalRenderingDeviceBeginDraw(device_2d, current_command, GetInnerMetalTexture(rt), rt->GetWidth(), rt->GetHeight());
				state = 2;
				error_state = true;
				return true;
			}
			virtual bool BeginMemoryManagementPass(void) noexcept override
			{
				if (state) return false;
				autorelease_pool = [[NSAutoreleasePool alloc] init];
				if (!current_command) {
					current_command = [queue commandBuffer];
					[current_command retain];
				}
				state = 3;
				error_state = true;
				return true;
			}
			virtual bool EndCurrentPass(void) noexcept override
			{
				if (!state) return false;
				if (state == 1) {
					[render_command_encoder endEncoding];
					render_command_encoder = 0;
				} else if (state == 2) {
					Cocoa::PureMetalRenderingDeviceEndDraw(device_2d);
				} else if (state == 3) {
					if (blit_command_encoder) [blit_command_encoder endEncoding];
					blit_command_encoder = 0;
				}
				[autorelease_pool release];
				autorelease_pool = 0;
				state = 0;
				return error_state;
			}
			virtual void Flush(void) noexcept override
			{
				if (!state && current_command) {
					[current_command commit];
					[current_command release];
					current_command = 0;
				}
			}
			virtual void SetRenderingPipelineState(IPipelineState * state) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetViewport(float top_left_x, float top_left_y, float width, float height, float min_depth, float max_depth) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetVertexShaderResource(uint32 at, IDeviceResource * resource) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetVertexShaderConstant(uint32 at, IBuffer * buffer) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetVertexShaderConstant(uint32 at, const void * data, int length) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetPixelShaderConstant(uint32 at, IBuffer * buffer) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetPixelShaderConstant(uint32 at, const void * data, int length) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetPixelShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetIndexBuffer(IBuffer * index, IndexBufferFormat format) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetStencilReferenceValue(uint8 ref) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void DrawPrimitives(uint32 vertex_count, uint32 first_vertex) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void DrawInstancedPrimitives(uint32 vertex_count, uint32 first_vertex, uint32 instance_count, uint32 first_instance) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void DrawIndexedInstancedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex, uint32 instance_count, uint32 first_instance) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual UI::IRenderingDevice * Get2DRenderingDevice(void) noexcept override { return device_2d; }
			virtual void GenerateMipmaps(ITexture * texture) noexcept override
			{
				if (state != 3) { error_state = false; return; }
				if (!blit_command_encoder) blit_command_encoder = [current_command blitCommandEncoder];
				[blit_command_encoder generateMipmapsForTexture: GetInnerMetalTexture(texture)];
			}
			virtual void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept override
			{
				if (state != 3) { error_state = false; return; }
				if (!blit_command_encoder) blit_command_encoder = [current_command blitCommandEncoder];
				if (dest->GetResourceType() == ResourceType::Buffer && src->GetResourceType() == ResourceType::Buffer) {
					auto src_rsrc = static_cast<MTL_Buffer *>(src);
					auto dest_rsrc = static_cast<MTL_Buffer *>(dest);
					if (src_rsrc->size != dest_rsrc->size) { error_state = false; return; }
					[blit_command_encoder copyFromBuffer: src_rsrc->buffer sourceOffset: 0 toBuffer: dest_rsrc->buffer destinationOffset: 0 size: src_rsrc->size];
				} else if (dest->GetResourceType() == ResourceType::Texture && src->GetResourceType() == ResourceType::Texture) {
					auto src_rsrc = static_cast<MTL_Texture *>(src);
					auto dest_rsrc = static_cast<MTL_Texture *>(dest);
					[blit_command_encoder copyFromTexture: src_rsrc->texture toTexture: dest_rsrc->texture];
				} else error_state = false;
			}
			virtual void CopySubresourceData(IDeviceResource * dest, SubresourceIndex dest_subres, VolumeIndex dest_origin, IDeviceResource * src, SubresourceIndex src_subres, VolumeIndex src_origin, VolumeIndex size) noexcept override
			{
				if (state != 3) { error_state = false; return; }
				if (!blit_command_encoder) blit_command_encoder = [current_command blitCommandEncoder];
				if (dest->GetResourceType() == ResourceType::Buffer && src->GetResourceType() == ResourceType::Buffer) {
					auto src_rsrc = static_cast<MTL_Buffer *>(src);
					auto dest_rsrc = static_cast<MTL_Buffer *>(dest);
					[blit_command_encoder copyFromBuffer: src_rsrc->buffer sourceOffset: src_origin.x
						toBuffer: dest_rsrc->buffer destinationOffset: dest_origin.x size: size.x];
				} else if (dest->GetResourceType() == ResourceType::Texture && src->GetResourceType() == ResourceType::Texture) {
					auto src_rsrc = static_cast<MTL_Texture *>(src);
					auto dest_rsrc = static_cast<MTL_Texture *>(dest);
					[blit_command_encoder
						copyFromTexture: src_rsrc->texture sourceSlice: src_subres.array_index sourceLevel: src_subres.mip_level
						sourceOrigin: MTLOriginMake(src_origin.x, src_origin.y, src_origin.z) sourceSize: MTLSizeMake(size.x, size.y, size.z)
						toTexture: dest_rsrc->texture destinationSlice: dest_subres.array_index destinationLevel: dest_subres.mip_level
						destinationOrigin: MTLOriginMake(dest_origin.x, dest_origin.y, dest_origin.z)];
				} else error_state = false;
			}
			virtual void UpdateResourceData(IDeviceResource * dest, SubresourceIndex subres, VolumeIndex origin, VolumeIndex size, const ResourceInitDesc & src) noexcept override
			{
				if (state != 3) { error_state = false; return; }
				if (!(dest->GetResourceUsage() & ResourceUsageCPUWrite)) { error_state = false; return; }
				if (!blit_command_encoder) blit_command_encoder = [current_command blitCommandEncoder];
				id<MTLResource> rsrc = 0;
				if (dest->GetResourceType() == ResourceType::Buffer) rsrc = static_cast<MTL_Buffer *>(dest)->buffer;
				else if (dest->GetResourceType() == ResourceType::Texture) rsrc = static_cast<MTL_Texture *>(dest)->texture;
				[blit_command_encoder synchronizeResource: rsrc];
				[blit_command_encoder endEncoding];
				blit_command_encoder = 0;
				[current_command commit];
				[current_command waitUntilCompleted];
				[current_command release];
				current_command = 0;
				if (dest->GetResourceType() == ResourceType::Buffer) {
					auto buffer = static_cast<MTL_Buffer *>(dest)->buffer;
					auto dest_data = reinterpret_cast<uint8 *>([buffer contents]);
					auto src_data = reinterpret_cast<const uint8 *>(src.Data);
					MemoryCopy(dest_data + origin.x, src_data, size.x);
					[buffer didModifyRange: NSMakeRange(origin.x, size.x)];
				} else if (dest->GetResourceType() == ResourceType::Texture) {
					auto tex_wrapper = static_cast<MTL_Texture *>(dest);
					auto texture = tex_wrapper->texture;
					if (tex_wrapper->GetTextureType() == TextureType::Type1D || tex_wrapper->GetTextureType() == TextureType::TypeArray1D) {
						[texture replaceRegion: MTLRegionMake1D(origin.x, size.x)
							mipmapLevel: subres.mip_level slice: subres.array_index withBytes: src.Data bytesPerRow: 0 bytesPerImage: 0];
					} else if (tex_wrapper->GetTextureType() == TextureType::Type2D || tex_wrapper->GetTextureType() == TextureType::TypeArray2D) {
						[texture replaceRegion: MTLRegionMake2D(origin.x, origin.y, size.x, size.y)
							mipmapLevel: subres.mip_level slice: subres.array_index withBytes: src.Data bytesPerRow: src.DataPitch bytesPerImage: 0];
					} else if (tex_wrapper->GetTextureType() == TextureType::TypeCube || tex_wrapper->GetTextureType() == TextureType::TypeArrayCube) {
						[texture replaceRegion: MTLRegionMake2D(origin.x, origin.y, size.x, size.y)
							mipmapLevel: subres.mip_level slice: subres.array_index withBytes: src.Data bytesPerRow: src.DataPitch bytesPerImage: 0];
					} else if (tex_wrapper->GetTextureType() == TextureType::Type3D) {
						[texture replaceRegion: MTLRegionMake3D(origin.x, origin.y, origin.z, size.x, size.y, size.z)
							mipmapLevel: subres.mip_level slice: subres.array_index
							withBytes: src.Data bytesPerRow: src.DataPitch bytesPerImage: src.DataSlicePitch];
					}
				}
				current_command = [queue commandBuffer];
				[current_command retain];
			}
			virtual void QueryResourceData(const ResourceDataDesc & dest, IDeviceResource * src, SubresourceIndex subres, VolumeIndex origin, VolumeIndex size) noexcept override
			{
				if (state != 3) { error_state = false; return; }
				if (!(src->GetResourceUsage() & ResourceUsageCPURead)) { error_state = false; return; }
				if (!blit_command_encoder) blit_command_encoder = [current_command blitCommandEncoder];
				id<MTLResource> rsrc = 0;
				if (src->GetResourceType() == ResourceType::Buffer) rsrc = static_cast<MTL_Buffer *>(src)->buffer;
				else if (src->GetResourceType() == ResourceType::Texture) rsrc = static_cast<MTL_Texture *>(src)->texture;
				[blit_command_encoder synchronizeResource: rsrc];
				[blit_command_encoder endEncoding];
				[current_command commit];
				[current_command waitUntilCompleted];
				[current_command release];
				blit_command_encoder = 0;
				current_command = 0;
				if (src->GetResourceType() == ResourceType::Buffer) {
					auto buffer = static_cast<MTL_Buffer *>(src)->buffer;
					auto dest_data = reinterpret_cast<uint8 *>(dest.Data);
					auto src_data = reinterpret_cast<const uint8 *>([buffer contents]);
					MemoryCopy(dest_data, src_data + origin.x, size.x);
				} else if (src->GetResourceType() == ResourceType::Texture) {
					auto tex_wrapper = static_cast<MTL_Texture *>(src);
					auto texture = tex_wrapper->texture;
					if (tex_wrapper->GetTextureType() == TextureType::Type1D || tex_wrapper->GetTextureType() == TextureType::TypeArray1D) {
						[texture getBytes: dest.Data bytesPerRow: 0 bytesPerImage: 0
							fromRegion: MTLRegionMake1D(origin.x, size.x) mipmapLevel: subres.mip_level slice: subres.array_index];
					} else if (tex_wrapper->GetTextureType() == TextureType::Type2D || tex_wrapper->GetTextureType() == TextureType::TypeArray2D) {
						[texture getBytes: dest.Data bytesPerRow: dest.DataPitch bytesPerImage: 0
							fromRegion: MTLRegionMake2D(origin.x, origin.y, size.x, size.y) mipmapLevel: subres.mip_level slice: subres.array_index];
					} else if (tex_wrapper->GetTextureType() == TextureType::TypeCube || tex_wrapper->GetTextureType() == TextureType::TypeArrayCube) {
						[texture getBytes: dest.Data bytesPerRow: dest.DataPitch bytesPerImage: 0
							fromRegion: MTLRegionMake2D(origin.x, origin.y, size.x, size.y) mipmapLevel: subres.mip_level slice: subres.array_index];
					} else if (tex_wrapper->GetTextureType() == TextureType::Type3D) {
						[texture getBytes: dest.Data bytesPerRow: dest.DataPitch bytesPerImage: dest.DataSlicePitch
							fromRegion: MTLRegionMake3D(origin.x, origin.y, origin.z, size.x, size.y, size.z) mipmapLevel: subres.mip_level slice: subres.array_index];
					}
				}
				current_command = [queue commandBuffer];
				[current_command retain];
			}
			virtual string ToString(void) const override { return L"MTL_DeviceContext"; }
		};
		class MTL_WindowLayer : public IWindowLayer
		{
			SafePointer<MTL_DeviceContext> context;
			SafePointer<MTL_Texture> texture;
			SafePointer<MetalPresentationInterface> presentation;
			IDevice * wrapper;
			NSWindow * window;
			PixelFormat format;
			uint32 width, height, usage;
			id<CAMetalDrawable> last_drawable;
			bool fullscreen;
		public:
			MTL_WindowLayer(IDevice * _wrapper, MTL_DeviceContext * _context, const WindowLayerDesc & desc, UI::WindowStation * station) : wrapper(_wrapper), last_drawable(0)
			{
				presentation.SetRetain(NativeWindows::InitWindowPresentationInterface(station));
				context.SetRetain(_context);
				fullscreen = false;
				window = NativeWindows::GetWindowObject(station);
				presentation->SetLayerDevice(GetInnerMetalDevice(wrapper));
				if (!(desc.Usage & ResourceUsageRenderTarget)) throw InvalidArgumentException();
				if (desc.Usage & ResourceUsageShaderRead) presentation->SetLayerFramebufferOnly(false);
				else presentation->SetLayerFramebufferOnly(true);
				presentation->SetLayerOpaque(true);
				presentation->SetLayerSize(UI::Point(max(desc.Width, 1U), max(desc.Height, 1U)));
				if (desc.Format != PixelFormat::Invalid) {
					if (desc.Format == PixelFormat::B8G8R8A8_unorm) presentation->SetPixelFormat(MTLPixelFormatBGRA8Unorm);
					else if (desc.Format == PixelFormat::R16G16B16A16_float) presentation->SetPixelFormat(MTLPixelFormatRGBA16Float);
					else if (desc.Format == PixelFormat::R10G10B10A2_unorm) presentation->SetPixelFormat(MTLPixelFormatRGB10A2Unorm);
					else throw InvalidArgumentException();
					format = desc.Format;
				} else {
					presentation->SetPixelFormat(MTLPixelFormatBGRA8Unorm);
					format = PixelFormat::B8G8R8A8_unorm;
				}
				width = desc.Width;
				height = desc.Height;
				usage = ResourceUsageRenderTarget;
				if (desc.Usage & ResourceUsageShaderRead) usage |= ResourceUsageShaderRead;
			}
			virtual ~MTL_WindowLayer(void) override { [last_drawable release]; }
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual bool Present(void) noexcept override
			{
				if (last_drawable) {
					if (context->state) return false;
					if (!context->current_command) {
						@autoreleasepool {
							context->current_command = [context->queue commandBuffer];
							[context->current_command retain];
						}
					}
					[context->current_command presentDrawable: last_drawable];
					[context->current_command commit];
					[context->current_command release];
					context->current_command = 0;
					return true;
				} else return false;
			}
			virtual ITexture * QuerySurface(void) noexcept override
			{
				if (!texture) {
					texture = new (std::nothrow) MTL_Texture(wrapper);
					if (!texture) return 0;
					texture->type = TextureType::Type2D;
					texture->format = format;
					texture->width = width;
					texture->height = height;
					texture->depth = 1;
					texture->size = 1;
					texture->usage = usage;
				}
				if (last_drawable) {
					[last_drawable release];
					last_drawable = 0;
				}
				if (texture->texture) {
					[texture->texture release];
					texture->texture = 0;
				}
				last_drawable = presentation->GetLayerDrawable();
				if (!last_drawable) return 0;
				[last_drawable retain];
				texture->texture = [last_drawable texture];
				[texture->texture retain];
				texture->Retain();
				return texture;
			}
			virtual bool ResizeSurface(uint32 _width, uint32 _height) noexcept override
			{
				width = max(_width, 1U);
				height = max(_height, 1U);
				presentation->SetLayerSize(UI::Point(width, height));
				texture.SetReference(0);
				return true;
			}
			virtual bool SwitchToFullscreen(void) noexcept override
			{
				if (!fullscreen) {
					[window toggleFullScreen: nil];
					[NSApp setPresentationOptions: NSApplicationPresentationHideDock | NSApplicationPresentationHideMenuBar];
					fullscreen = true;
				}
				return true;
			}
			virtual bool SwitchToWindow(void) noexcept override
			{
				if (fullscreen) {
					[window toggleFullScreen: nil];
					[NSApp setPresentationOptions: NSApplicationPresentationDefault];
					fullscreen = false;
				}
				return true;
			}
			virtual bool IsFullscreen(void) noexcept override { return fullscreen; }
			virtual string ToString(void) const override { return L"MTL_WindowLayer"; }
		};
		class MTL_Device : public IDevice
		{
			SafePointer<MTL_DeviceContext> context;
		public:
			id<MTLDevice> device;
			id<MTLCommandQueue> queue;

			MTL_Device(id<MTLDevice> _device) : device(_device)
			{
				[device retain];
				queue = [device newCommandQueue];
				context = new MTL_DeviceContext(this, device, queue);
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
			virtual IDeviceContext * GetDeviceContext(void) noexcept override { return context; }
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
				if (desc.Usage & ~ResourceUsageBufferMask) return 0;
				if (!desc.Length) return 0;
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) return 0;
				SafePointer<MTL_Buffer> buffer = new (std::nothrow) MTL_Buffer(this);
				if (!buffer) return 0;
				if ((desc.Usage & ResourceUsageShaderAll) || (desc.Usage & ResourceUsageConstantBuffer) || (desc.Usage & ResourceUsageIndexBuffer)) {
					buffer->usage |= ResourceUsageShaderAll;
					buffer->usage |= ResourceUsageConstantBuffer;
					buffer->usage |= ResourceUsageIndexBuffer;
				}
				if (desc.Usage & ResourceUsageCPUAll) {
					buffer->usage |= ResourceUsageCPUAll;
				}
				MTLResourceOptions options;
				if (buffer->usage & ResourceUsageCPUAll) options = MTLResourceStorageModePrivate;
				else options = MTLResourceStorageModeManaged;
				buffer->buffer = [device newBufferWithLength: desc.Length options: options];
				if (!buffer->buffer) return 0;
				buffer->size = desc.Length;
				buffer->Retain();
				return buffer;
			}
			virtual IBuffer * CreateBuffer(const BufferDesc & desc, const ResourceInitDesc & init) noexcept override
			{
				if (desc.Usage & ~ResourceUsageBufferMask) return 0;
				if (!desc.Length) return 0;
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) {
					if (desc.Usage & ResourceUsageShaderWrite) return 0;
					if (desc.Usage & ResourceUsageCPUAll) return 0;
				}
				SafePointer<MTL_Buffer> buffer = new (std::nothrow) MTL_Buffer(this);
				if (!buffer) return 0;
				if ((desc.Usage & ResourceUsageShaderAll) || (desc.Usage & ResourceUsageConstantBuffer) || (desc.Usage & ResourceUsageIndexBuffer)) {
					buffer->usage |= ResourceUsageShaderAll;
					buffer->usage |= ResourceUsageConstantBuffer;
					buffer->usage |= ResourceUsageIndexBuffer;
				}
				buffer->usage |= ResourceUsageCPUAll;
				buffer->buffer = [device newBufferWithBytes: init.Data length: desc.Length options: MTLResourceStorageModeManaged];
				if (!buffer->buffer) return 0;
				buffer->size = desc.Length;
				buffer->Retain();
				return buffer;
			}
			virtual ITexture * CreateTexture(const TextureDesc & desc) noexcept override
			{
				if (desc.Usage & ~ResourceUsageTextureMask) return 0;
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) return 0;
				if (desc.Usage & ResourceUsageRenderTarget) {
					if (!Graphics::IsColorFormat(desc.Format)) return 0;
				}
				if (desc.Usage & ResourceUsageDepthStencil) {
					if (!Graphics::IsDepthStencilFormat(desc.Format)) return 0;
				}
				SafePointer<MTL_Texture> texture = new (std::nothrow) MTL_Texture(this);
				if (!texture) return 0;
				MTLTextureDescriptor * descriptor = [[MTLTextureDescriptor alloc] init];
				descriptor.width = desc.Width;
				texture->type = desc.Type;
				texture->format = desc.Format;
				texture->width = desc.Width;
				texture->height = texture->depth = texture->size = 1;
				if (desc.Type == TextureType::Type1D) {
					descriptor.textureType = MTLTextureType1D;
				} else if (desc.Type == TextureType::TypeArray1D) {
					descriptor.arrayLength = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureType1DArray;
					texture->size = desc.DepthOrArraySize;
				} else if (desc.Type == TextureType::Type2D) {
					descriptor.height = desc.Height;
					descriptor.textureType = MTLTextureType2D;
					texture->height = desc.Height;
				} else if (desc.Type == TextureType::TypeArray2D) {
					descriptor.height = desc.Height;
					descriptor.arrayLength = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureType2DArray;
					texture->height = desc.Height;
					texture->size = desc.DepthOrArraySize;
				} else if (desc.Type == TextureType::TypeCube) {
					descriptor.height = desc.Height;
					descriptor.textureType = MTLTextureTypeCube;
					texture->height = desc.Height;
				} else if (desc.Type == TextureType::TypeArrayCube) {
					descriptor.height = desc.Height;
					descriptor.arrayLength = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureTypeCubeArray;
					texture->height = desc.Height;
					texture->size = desc.DepthOrArraySize;
				} else if (desc.Type == TextureType::Type3D) {
					descriptor.height = desc.Height;
					descriptor.depth = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureType3D;
					texture->height = desc.Height;
					texture->depth = desc.DepthOrArraySize;
				} else { [descriptor release]; return 0; }
				uint32 mips = desc.MipmapCount;
				if (!mips) {
					uint32 mx_size = max(max(texture->width, texture->height), texture->depth);
					while (mx_size) { mips++; mx_size /= 2; }
				}
				descriptor.mipmapLevelCount = mips;
				descriptor.pixelFormat = MakeMetalPixelFormat(desc.Format);
				descriptor.usage = 0;
				if (desc.Usage & ResourceUsageShaderRead) {
					descriptor.usage |= MTLTextureUsageShaderRead;
					texture->usage |= ResourceUsageShaderRead;
				}
				if (desc.Usage & ResourceUsageShaderWrite) {
					descriptor.usage |= MTLTextureUsageShaderWrite;
					texture->usage |= ResourceUsageShaderWrite;
				}
				if ((desc.Usage & ResourceUsageRenderTarget) || (desc.Usage & ResourceUsageDepthStencil)) {
					descriptor.usage |= MTLTextureUsageRenderTarget;
					if (Graphics::IsColorFormat(desc.Format)) texture->usage |= ResourceUsageRenderTarget;
					if (Graphics::IsDepthStencilFormat(desc.Format)) texture->usage |= ResourceUsageDepthStencil;
				}
				if (desc.Usage & ResourceUsageCPUAll) texture->usage |= ResourceUsageCPUAll;
				if (texture->usage & ResourceUsageCPUAll) descriptor.storageMode = MTLStorageModeManaged;
				else descriptor.storageMode = MTLStorageModePrivate;
				texture->texture = [device newTextureWithDescriptor: descriptor];
				[descriptor release];
				if (!texture->texture) return 0;
				texture->Retain();
				return texture;
			}
			virtual ITexture * CreateTexture(const TextureDesc & desc, const ResourceInitDesc * init) noexcept override
			{
				if (!init) return 0;
				if (desc.Usage & ~ResourceUsageTextureMask) return 0;
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) {
					if (desc.Usage & ResourceUsageShaderWrite) return 0;
					if (desc.Usage & ResourceUsageCPUAll) return 0;
					if (desc.Usage & ResourceUsageRenderTarget) return 0;
					if (desc.Usage & ResourceUsageDepthStencil) return 0;
				}
				if (desc.Usage & ResourceUsageRenderTarget) {
					if (!Graphics::IsColorFormat(desc.Format)) return 0;
				}
				if (desc.Usage & ResourceUsageDepthStencil) {
					if (!Graphics::IsDepthStencilFormat(desc.Format)) return 0;
				}
				SafePointer<MTL_Texture> texture = new (std::nothrow) MTL_Texture(this);
				if (!texture) return 0;
				MTLTextureDescriptor * descriptor = [[MTLTextureDescriptor alloc] init];
				descriptor.width = desc.Width;
				texture->type = desc.Type;
				texture->format = desc.Format;
				texture->width = desc.Width;
				texture->height = texture->depth = texture->size = 1;
				if (desc.Type == TextureType::Type1D) {
					descriptor.textureType = MTLTextureType1D;
				} else if (desc.Type == TextureType::TypeArray1D) {
					descriptor.arrayLength = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureType1DArray;
					texture->size = desc.DepthOrArraySize;
				} else if (desc.Type == TextureType::Type2D) {
					descriptor.height = desc.Height;
					descriptor.textureType = MTLTextureType2D;
					texture->height = desc.Height;
				} else if (desc.Type == TextureType::TypeArray2D) {
					descriptor.height = desc.Height;
					descriptor.arrayLength = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureType2DArray;
					texture->height = desc.Height;
					texture->size = desc.DepthOrArraySize;
				} else if (desc.Type == TextureType::TypeCube) {
					descriptor.height = desc.Height;
					descriptor.textureType = MTLTextureTypeCube;
					texture->height = desc.Height;
				} else if (desc.Type == TextureType::TypeArrayCube) {
					descriptor.height = desc.Height;
					descriptor.arrayLength = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureTypeCubeArray;
					texture->height = desc.Height;
					texture->size = desc.DepthOrArraySize;
				} else if (desc.Type == TextureType::Type3D) {
					descriptor.height = desc.Height;
					descriptor.depth = desc.DepthOrArraySize;
					descriptor.textureType = MTLTextureType3D;
					texture->height = desc.Height;
					texture->depth = desc.DepthOrArraySize;
				} else { [descriptor release]; return 0; }
				uint32 mips = desc.MipmapCount;
				if (!mips) {
					uint32 mx_size = max(max(texture->width, texture->height), texture->depth);
					while (mx_size) { mips++; mx_size /= 2; }
				}
				descriptor.mipmapLevelCount = mips;
				descriptor.pixelFormat = MakeMetalPixelFormat(desc.Format);
				descriptor.usage = 0;
				if (desc.Usage & ResourceUsageShaderRead) {
					descriptor.usage |= MTLTextureUsageShaderRead;
					texture->usage |= ResourceUsageShaderRead;
				}
				if (desc.Usage & ResourceUsageShaderWrite) {
					descriptor.usage |= MTLTextureUsageShaderWrite;
					texture->usage |= ResourceUsageShaderWrite;
				}
				if ((desc.Usage & ResourceUsageRenderTarget) || (desc.Usage & ResourceUsageDepthStencil)) {
					descriptor.usage |= MTLTextureUsageRenderTarget;
					if (Graphics::IsColorFormat(desc.Format)) texture->usage |= ResourceUsageRenderTarget;
					if (Graphics::IsDepthStencilFormat(desc.Format)) texture->usage |= ResourceUsageDepthStencil;
				}
				texture->usage |= ResourceUsageCPUAll;
				descriptor.storageMode = MTLStorageModeManaged;
				texture->texture = [device newTextureWithDescriptor: descriptor];
				[descriptor release];
				if (!texture->texture) return 0;
				if (desc.Type == TextureType::Type1D || desc.Type == TextureType::TypeArray1D) {
					for (uint j = 0; j < texture->size; j++) for (uint i = 0; i < mips; i++) {
						uint subres = i + j * mips;
						[texture->texture replaceRegion: MTLRegionMake1D(0, texture->width) mipmapLevel: i slice: j
							withBytes: init[subres].Data bytesPerRow: 0 bytesPerImage: 0];
					}
				} else if (desc.Type == TextureType::Type2D || desc.Type == TextureType::TypeArray2D) {
					for (uint j = 0; j < texture->size; j++) for (uint i = 0; i < mips; i++) {
						uint subres = i + j * mips;
						[texture->texture replaceRegion: MTLRegionMake2D(0, 0, texture->width, texture->height) mipmapLevel: i slice: j
							withBytes: init[subres].Data bytesPerRow: init[subres].DataPitch bytesPerImage: 0];
					}
				} else if (desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) {
					for (uint j = 0; j < texture->size * 6; j++) for (uint i = 0; i < mips; i++) {
						uint subres = i + j * mips;
						[texture->texture replaceRegion: MTLRegionMake2D(0, 0, texture->width, texture->height) mipmapLevel: i slice: j
							withBytes: init[subres].Data bytesPerRow: init[subres].DataPitch bytesPerImage: 0];
					}
				} else if (desc.Type == TextureType::Type3D) {
					for (uint i = 0; i < mips; i++) {
						[texture->texture replaceRegion: MTLRegionMake3D(0, 0, 0, texture->width, texture->height, texture->depth)
							mipmapLevel: i slice: 0 withBytes: init[i].Data bytesPerRow: init[i].DataPitch bytesPerImage: init[i].DataSlicePitch];
					}
				}
				texture->Retain();
				return texture;
			}
			virtual IWindowLayer * CreateWindowLayer(UI::Window * window, const WindowLayerDesc & desc) noexcept override
			{
				try {
					if (!window->GetStation()->IsNativeStationWrapper()) return 0;
					SafePointer<MTL_WindowLayer> layer = new MTL_WindowLayer(this, context, desc, window->GetStation());
					layer->Retain();
					return layer;
				} catch (...) { return 0; }
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
		id<MTLBuffer> GetInnerMetalBuffer(Graphics::IBuffer * buffer) { return static_cast<MTL_Buffer *>(buffer)->buffer; }
		id<MTLTexture> GetInnerMetalTexture(Graphics::ITexture * texture) { return static_cast<MTL_Texture *>(texture)->texture; }
		MTLPixelFormat MakeMetalPixelFormat(Graphics::PixelFormat format)
		{
			if (IsColorFormat(format)) {
				auto bpp = GetFormatBitsPerPixel(format);
				if (bpp == 8) {
					if (format == PixelFormat::A8_unorm) return MTLPixelFormatA8Unorm;
					else if (format == PixelFormat::R8_unorm) return MTLPixelFormatR8Unorm;
					else if (format == PixelFormat::R8_snorm) return MTLPixelFormatR8Snorm;
					else if (format == PixelFormat::R8_uint) return MTLPixelFormatR8Uint;
					else if (format == PixelFormat::R8_sint) return MTLPixelFormatR8Sint;
					else return MTLPixelFormatInvalid;
				} else if (bpp == 16) {
					if (format == PixelFormat::R16_unorm) return MTLPixelFormatR16Unorm;
					else if (format == PixelFormat::R16_snorm) return MTLPixelFormatR16Snorm;
					else if (format == PixelFormat::R16_uint) return MTLPixelFormatR16Uint;
					else if (format == PixelFormat::R16_sint) return MTLPixelFormatR16Sint;
					else if (format == PixelFormat::R16_float) return MTLPixelFormatR16Float;
					else if (format == PixelFormat::R8G8_unorm) return MTLPixelFormatRG8Unorm;
					else if (format == PixelFormat::R8G8_snorm) return MTLPixelFormatRG8Snorm;
					else if (format == PixelFormat::R8G8_uint) return MTLPixelFormatRG8Uint;
					else if (format == PixelFormat::R8G8_sint) return MTLPixelFormatRG8Sint;
					else if (format == PixelFormat::B5G6R5_unorm) return MTLPixelFormatB5G6R5Unorm;
					else if (format == PixelFormat::B5G5R5A1_unorm) return MTLPixelFormatBGR5A1Unorm;
					else if (format == PixelFormat::B4G4R4A4_unorm) return MTLPixelFormatABGR4Unorm;
					else return MTLPixelFormatInvalid;
				} else if (bpp == 32) {
					if (format == PixelFormat::R32_uint) return MTLPixelFormatR32Uint;
					else if (format == PixelFormat::R32_sint) return MTLPixelFormatR32Sint;
					else if (format == PixelFormat::R32_float) return MTLPixelFormatR32Float;
					else if (format == PixelFormat::R16G16_unorm) return MTLPixelFormatRG16Unorm;
					else if (format == PixelFormat::R16G16_snorm) return MTLPixelFormatRG16Snorm;
					else if (format == PixelFormat::R16G16_uint) return MTLPixelFormatRG16Uint;
					else if (format == PixelFormat::R16G16_sint) return MTLPixelFormatRG16Sint;
					else if (format == PixelFormat::R16G16_float) return MTLPixelFormatRG16Float;
					else if (format == PixelFormat::B8G8R8A8_unorm) return MTLPixelFormatBGRA8Unorm;
					else if (format == PixelFormat::R8G8B8A8_unorm) return MTLPixelFormatRGBA8Unorm;
					else if (format == PixelFormat::R8G8B8A8_snorm) return MTLPixelFormatRGBA8Snorm;
					else if (format == PixelFormat::R8G8B8A8_uint) return MTLPixelFormatRGBA8Uint;
					else if (format == PixelFormat::R8G8B8A8_sint) return MTLPixelFormatRGBA8Sint;
					else if (format == PixelFormat::R10G10B10A2_unorm) return MTLPixelFormatRGB10A2Unorm;
					else if (format == PixelFormat::R10G10B10A2_uint) return MTLPixelFormatRGB10A2Uint;
					else if (format == PixelFormat::R11G11B10_float) return MTLPixelFormatRG11B10Float;
					else if (format == PixelFormat::R9G9B9E5_float) return MTLPixelFormatRGB9E5Float;
					else return MTLPixelFormatInvalid;
				} else if (bpp == 64) {
					if (format == PixelFormat::R32G32_uint) return MTLPixelFormatRG32Uint;
					else if (format == PixelFormat::R32G32_sint) return MTLPixelFormatRG32Sint;
					else if (format == PixelFormat::R32G32_float) return MTLPixelFormatRG32Float;
					else if (format == PixelFormat::R16G16B16A16_unorm) return MTLPixelFormatRGBA16Unorm;
					else if (format == PixelFormat::R16G16B16A16_snorm) return MTLPixelFormatRGBA16Snorm;
					else if (format == PixelFormat::R16G16B16A16_uint) return MTLPixelFormatRGBA16Uint;
					else if (format == PixelFormat::R16G16B16A16_sint) return MTLPixelFormatRGBA16Sint;
					else if (format == PixelFormat::R16G16B16A16_float) return MTLPixelFormatRGBA16Float;
					else return MTLPixelFormatInvalid;
				} else if (bpp == 128) {
					if (format == PixelFormat::R32G32B32A32_uint) return MTLPixelFormatRGBA32Uint;
					else if (format == PixelFormat::R32G32B32A32_sint) return MTLPixelFormatRGBA32Sint;
					else if (format == PixelFormat::R32G32B32A32_float) return MTLPixelFormatRGBA32Float;
					else return MTLPixelFormatInvalid;
				} else return MTLPixelFormatInvalid;
			} else if (IsDepthStencilFormat(format)) {
				if (format == PixelFormat::D16_unorm) return MTLPixelFormatDepth16Unorm;
				else if (format == PixelFormat::D32_float) return MTLPixelFormatDepth32Float;
				else if (format == PixelFormat::D24S8_unorm) return MTLPixelFormatDepth24Unorm_Stencil8;
				else if (format == PixelFormat::D32S8_float) return MTLPixelFormatDepth32Float_Stencil8;
				else return MTLPixelFormatInvalid;
			} else return MTLPixelFormatInvalid;
		}
	}
}