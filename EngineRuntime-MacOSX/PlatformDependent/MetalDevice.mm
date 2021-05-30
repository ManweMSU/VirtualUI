#include "MetalDevice.h"

#include "QuartzDevice.h"
#include "MetalDeviceShaders.h"
#include "CocoaInterop.h"

using namespace Engine;
using namespace Engine::Codec;
using namespace Engine::Streaming;
using namespace Engine::UI;

namespace Engine
{
	namespace Cocoa
	{
		CocoaPointer< id<MTLLibrary> > common_library;
		id<MTLDevice> GetInnerMetalDevice(UI::IRenderingDevice * device);

		struct MetalVertex {
			Math::Vector2f position;
			Math::Vector4f color;
			Math::Vector2f tex_coord;
		};
		struct TileShaderInfo {
			UI::Box draw_rect;
			UI::Point periods;
		};
		struct LayerInfo {
			UI::Point viewport_size;
			UI::Point viewport_offset;
		};
		struct LayerEndInfo {
			UI::Box render_at;
			UI::Point size;
			float opacity;
		};
		class MetalBarRenderingInfo : public IBarRenderingInfo
		{
		public:
			id<MTLBuffer> verticies;
			int vertex_count;
			bool use_clipping;
			virtual ~MetalBarRenderingInfo(void) override { [verticies release]; }
		};
		class MetalTexture : public UI::ITexture
		{
		public:
			UI::ITexture * base;
			UI::IRenderingDevice * factory_device;
			id<MTLTexture> frame;
			int w, h;

			MetalTexture(void) : base(0), factory_device(0), frame(0) {}
			virtual ~MetalTexture(void) override
			{
				[frame release];
				if (factory_device) factory_device->TextureWasDestroyed(this);
				if (base) base->VersionWasDestroyed(this);
			}
			virtual int GetWidth(void) const noexcept override { return w; }
			virtual int GetHeight(void) const noexcept override { return h; }

			virtual void VersionWasDestroyed(UI::ITexture * texture) noexcept override { if (texture == base) { base = 0; factory_device = 0; } }
			virtual void DeviceWasDestroyed(UI::IRenderingDevice * device) noexcept override { if (device == factory_device) { base = 0; factory_device = 0; } }
			virtual void AddDeviceVersion(UI::IRenderingDevice * device, UI::ITexture * texture) noexcept override {}
			virtual bool IsDeviceSpecific(void) const noexcept override { return true; }
			virtual UI::IRenderingDevice * GetParentDevice(void) const noexcept override { return factory_device; }
			virtual UI::ITexture * GetDeviceVersion(UI::IRenderingDevice * target_device) noexcept override
			{
				if (factory_device && target_device == factory_device) return this;
				else if (!target_device) return base;
				return 0;
			}

			virtual void Reload(Codec::Frame * source) override
			{
				if (!factory_device) return;
				w = source->GetWidth();
				h = source->GetHeight();
				MTLPixelFormat pf;
				SafePointer<Codec::Frame> conv;
				if (source->GetAlphaMode() != Codec::AlphaMode::Normal || source->GetScanOrigin() != Codec::ScanOrigin::TopDown ||
					(source->GetPixelFormat() != Codec::PixelFormat::B8G8R8A8 && source->GetPixelFormat() != Codec::PixelFormat::R8G8B8A8)) {
					conv = source->ConvertFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::TopDown);
				} else conv.SetRetain(source);
				if (conv->GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) pf = MTLPixelFormatRGBA8Unorm;
				else if (conv->GetPixelFormat() == Codec::PixelFormat::B8G8R8A8) pf = MTLPixelFormatBGRA8Unorm;
				auto dev = GetInnerMetalDevice(factory_device);
				[frame release];
				@autoreleasepool {
					frame = [dev newTextureWithDescriptor: [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: pf width: w height: h mipmapped: NO]];
					[frame replaceRegion: MTLRegionMake2D(0, 0, w, h) mipmapLevel: 0 withBytes: conv->GetData() bytesPerRow: 4 * w];
				}
			}
			virtual void Reload(UI::ITexture * device_independent) override
			{
				CGImageRef image = static_cast<CGImageRef>(GetCoreImageFromTexture(device_independent));
				CGDataProviderRef provider = CGImageGetDataProvider(image);
				int width = CGImageGetWidth(image);
				int height = CGImageGetHeight(image);
				CFDataRef data = CGDataProviderCopyData(provider);
				Codec::AlphaMode alpha = Codec::AlphaMode::Normal;
				if (CGImageGetAlphaInfo(image) == kCGImageAlphaPremultipliedLast) alpha = Codec::AlphaMode::Premultiplied;
				SafePointer<Codec::Frame> cover = new Codec::Frame(width, height, width * 4, Codec::PixelFormat::R8G8B8A8, alpha, Codec::ScanOrigin::TopDown);
				CFDataGetBytes(data, CFRangeMake(0, CFDataGetLength(data)), cover->GetData());
				CFRelease(data);
				Reload(cover);
			}
			virtual string ToString(void) const override { return L"Engine.Cocoa.MetalTexture"; }
		};
		class MetalTextureRenderingInfo : public ITextureRenderingInfo
		{
		public:
			id<MTLBuffer> verticies;
			int vertex_count;
			SafePointer<MetalTexture> texture;
			SafePointer<Graphics::ITexture> wrapped;
			bool tile_render;

			virtual ~MetalTextureRenderingInfo(void) override
			{
				[verticies release];
			}
		};
		class MetalTextRenderingInfo : public UI::ITextRenderingInfo
		{
		public:
			SafePointer<UI::ITextRenderingInfo> core_text_info;
			SafePointer<UI::IBarRenderingInfo> highlight_info;
			id<MTLTexture> texture;
			int width, height;
			int horz_align, vert_align;
			int highlight_left, highlight_right;
			int render_ofs_x, render_ofs_y;
			int real_width, real_height;
			bool dynamic_ofs;
			UI::Color highlight_color;

			MetalTextRenderingInfo(void) { texture = 0; width = height = highlight_left = highlight_right = -1; highlight_color = 0; render_ofs_x = render_ofs_y = 0; dynamic_ofs = false; }
			~MetalTextRenderingInfo(void) override { [texture release]; }

			void UpdateTexture(void) { if (texture) [texture release]; texture = 0; width = height = -1; }
			virtual void GetExtent(int & width, int & height) noexcept override { core_text_info->GetExtent(width, height); }
			virtual void SetHighlightColor(const UI::Color & color) noexcept override { if (color != highlight_color) { highlight_color = color; highlight_info.SetReference(0); } }
			virtual void HighlightText(int Start, int End) noexcept override
			{
				if (Start < 0 || Start == End) highlight_left = highlight_right = -1;
				else {
					if (Start == 0) highlight_left = 0; else highlight_left = EndOfChar(Start - 1);
					highlight_right = EndOfChar(End - 1);
				}
			}
			virtual int TestPosition(int point) noexcept override { return core_text_info->TestPosition(point); }
			virtual int EndOfChar(int Index) noexcept override { return core_text_info->EndOfChar(Index); }
			virtual void SetCharPalette(const Array<UI::Color> & colors) override { core_text_info->SetCharPalette(colors); UpdateTexture(); }
			virtual void SetCharColors(const Array<uint8> & indicies) override { core_text_info->SetCharColors(indicies); UpdateTexture(); }
		};
		class MetalInversionEffectRenderingInfo : public IInversionEffectRenderingInfo
		{
		public:
			id<MTLBuffer> verticies;
			int vertex_count;
			virtual ~MetalInversionEffectRenderingInfo(void) override { [verticies release]; }
		};
		class MetalBlurEffectRenderingInfo : public IBlurEffectRenderingInfo
		{
		public:
			float sigma;
		};
		class MetalDevice : public IRenderingDevice
		{
			struct tex_pair { ITexture * base; ITexture * spec; };
			SafePointer<Cocoa::QuartzRenderingDevice> LoaderDevice;
			Array<UI::Box> clipping;
			Array<LayerInfo> layers;
			id<MTLRenderPipelineState> main_state, tile_state, invert_state, layer_state, blur_state;
			id<MTLTexture> white_texture;
			id<MTLBuffer> viewport_data;
			id<MTLBuffer> box_data;
			id<MTLBuffer> layer_vertex;
			Dictionary::ObjectCache<UI::Color, UI::IBarRenderingInfo> brush_cache;
			SafePointer<UI::IInversionEffectRenderingInfo> inversion;
			Array<tex_pair> texture_cache;
		public:
			Graphics::IDevice * engine_device;
			id<MTLDevice> device;
			id<MTLLibrary> library;
			id<MTLCommandBuffer> command_buffer;
			id<MTLRenderCommandEncoder> encoder;
			MTLRenderPassDescriptor * base_descriptor;
			Array< id<MTLTexture> > layer_texture;
			Array<MTLRenderPassDescriptor *> desc;
			Array<double> layer_alpha;
			int width, height;
			MTLPixelFormat pixel_format;

			MetalDevice(void) : clipping(0x20), layers(0x20), brush_cache(0x20, Dictionary::ExcludePolicy::ExcludeLeastRefrenced), texture_cache(0x100),
				layer_texture(0x10), layer_alpha(0x10)
			{
				engine_device = 0;
				LoaderDevice = new Cocoa::QuartzRenderingDevice();
				main_state = tile_state = invert_state = layer_state = blur_state = 0;
				white_texture = 0;
				viewport_data = box_data = layer_vertex = 0;
				device = 0; library = 0; command_buffer = 0; encoder = 0;
				base_descriptor = 0; width = height = 1;
				pixel_format = MTLPixelFormatInvalid;
			}
			virtual ~MetalDevice(void) override
			{
				[main_state release];
				[tile_state release];
				[invert_state release];
				[layer_state release];
				[blur_state release];
				[white_texture release];
				[viewport_data release];
				[box_data release];
				[layer_vertex release];
				[library release];
				for (int i = 0; i < texture_cache.Length(); i++) {
					texture_cache[i].base->DeviceWasDestroyed(this);
					texture_cache[i].spec->DeviceWasDestroyed(this);
				}
			}
			virtual void TextureWasDestroyed(UI::ITexture * texture) noexcept override
			{
				for (int i = 0; i < texture_cache.Length(); i++) {
					if (texture_cache[i].base == texture || texture_cache[i].spec == texture) {
						texture_cache.Remove(i);
						break;
					}
				}
			}

			virtual void GetImplementationInfo(string & tech, uint32 & version) noexcept override { tech = L"Metal"; version = 1; }
			virtual uint32 GetFeatureList(void) noexcept override
			{
				return RenderingDeviceFeatureBlurCapable | RenderingDeviceFeatureInversionCapable |
					RenderingDeviceFeatureLayersCapable | RenderingDeviceFeatureHardware | RenderingDeviceFeatureGraphicsInteropCapable;
			}

			void CreateRenderingStates(void)
			{
				[main_state release];
				[tile_state release];
				[invert_state release];
				[layer_state release];
				[blur_state release];
				NSError * error;
				id<MTLFunction> main_vertex = [library newFunctionWithName: @"MetalDeviceMainVertexShader"];
				id<MTLFunction> main_pixel = [library newFunctionWithName: @"MetalDeviceMainPixelShader"];
				id<MTLFunction> tile_vertex = [library newFunctionWithName: @"MetalDeviceTileVertexShader"];
				id<MTLFunction> tile_pixel = [library newFunctionWithName: @"MetalDeviceTilePixelShader"];
				id<MTLFunction> layer_vertex_f = [library newFunctionWithName: @"MetalDeviceLayerVertexShader"];
				id<MTLFunction> layer_pixel_f = [library newFunctionWithName: @"MetalDeviceLayerPixelShader"];
				id<MTLFunction> blur_vertex_f = [library newFunctionWithName: @"MetalDeviceBlurVertexShader"];
				id<MTLFunction> blur_pixel_f = [library newFunctionWithName: @"MetalDeviceBlurPixelShader"];
				{
					MTLRenderPipelineDescriptor * descriptor = [[MTLRenderPipelineDescriptor alloc] init];
					descriptor.colorAttachments[0].pixelFormat = pixel_format;
					descriptor.colorAttachments[0].blendingEnabled = YES;
					descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOneMinusDestinationAlpha;
					descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
					descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
					descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					descriptor.vertexFunction = main_vertex;
					descriptor.fragmentFunction = main_pixel;
					main_state = [device newRenderPipelineStateWithDescriptor: descriptor error: &error];
					descriptor.vertexFunction = tile_vertex;
					descriptor.fragmentFunction = tile_pixel;
					tile_state = [device newRenderPipelineStateWithDescriptor: descriptor error: &error];
					descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationSubtract;
					descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorZero;
					descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
					descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorDestinationAlpha;
					descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOne;
					descriptor.vertexFunction = main_vertex;
					descriptor.fragmentFunction = main_pixel;
					invert_state = [device newRenderPipelineStateWithDescriptor: descriptor error: &error];
					descriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
					descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOneMinusDestinationAlpha;
					descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOne;
					descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
					descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
					descriptor.vertexFunction = layer_vertex_f;
					descriptor.fragmentFunction = layer_pixel_f;
					layer_state = [device newRenderPipelineStateWithDescriptor: descriptor error: &error];
					descriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
					descriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorZero;
					descriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorOne;
					descriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorZero;
					descriptor.vertexFunction = blur_vertex_f;
					descriptor.fragmentFunction = blur_pixel_f;
					blur_state = [device newRenderPipelineStateWithDescriptor: descriptor error: &error];
					[descriptor release];
				}
				[main_vertex release];
				[main_pixel release];
				[tile_vertex release];
				[tile_pixel release];
				[layer_vertex_f release];
				[layer_pixel_f release];
				[blur_vertex_f release];
				[blur_pixel_f release];
				if (viewport_data) [viewport_data release];
				if (box_data) [box_data release];
				if (white_texture) [white_texture release];
				if (layer_vertex) [layer_vertex release];
				viewport_data = [device newBufferWithLength: sizeof(LayerInfo) options: MTLResourceStorageModeShared];
				box_data = [device newBufferWithLength: sizeof(LayerEndInfo) options: MTLResourceStorageModeShared];
				@autoreleasepool {
					UI::Color data(255, 255, 255, 255);
					white_texture = [device newTextureWithDescriptor: [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: MTLPixelFormatBGRA8Unorm width: 1 height: 1 mipmapped: NO]];
					[white_texture replaceRegion: MTLRegionMake2D(0, 0, 1, 1) mipmapLevel: 0 withBytes: &data bytesPerRow: 4];
				}
				Array<MetalVertex> data(6);
				data.SetLength(6);
				data[0].position = Math::Vector2f(0.0f, 0.0f);
				data[0].color = Math::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
				data[0].tex_coord = Math::Vector2f(0.0f, 0.0f);
				data[1].position = Math::Vector2f(1.0f, 0.0f);
				data[1].color = data[0].color;
				data[1].tex_coord = Math::Vector2f(1.0f, 0.0f);
				data[2].position = Math::Vector2f(0.0f, 1.0f);
				data[2].color = data[0].color;
				data[2].tex_coord = Math::Vector2f(0.0f, 1.0f);
				data[3] = data[1];
				data[4] = data[2];
				data[5].position = Math::Vector2f(1.0f, 1.0f);
				data[5].color = data[0].color;
				data[5].tex_coord = Math::Vector2f(1.0f, 1.0f);
				layer_vertex = [device newBufferWithBytes: data.GetBuffer() length: sizeof(MetalVertex) * data.Length() options: MTLResourceStorageModeShared];
			}
			void InitDraw(void)
			{
				[encoder setVertexBuffer: viewport_data offset: 0 atIndex: 0];
				[encoder setVertexBuffer: box_data offset: 0 atIndex: 2];
				LayerInfo info;
				info.viewport_size = UI::Point(width, height);
				info.viewport_offset = UI::Point(0, 0);
				[encoder setVertexBytes: &info length: sizeof(info) atIndex: 0];
				clipping.SetLength(1);
				clipping.FirstElement() = UI::Box(0, 0, width, height);
				layers.SetLength(1);
				layers[0] = info;
				layer_texture.Clear();
				desc.SetLength(1);
				desc[0] = base_descriptor;
			}
			static Math::Vector4f ColorVectorMake(UI::Color color)
			{
				return Math::Vector4f(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
			}
			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint> & gradient, double angle) noexcept override
			{
				if (!gradient.Length()) return 0;
				if (gradient.Length() == 1) return CreateBarRenderingInfo(gradient.FirstElement().Color);
				Array<MetalVertex> data(0x10);
				float dx = cos(angle), dy = sin(angle);
				float mx = max(abs(dx), abs(dy));
				dx /= mx; dy /= mx;
				Math::Vector2f gradient_start(0.5f - dx / 2.0f, 0.5f + dy / 2.0f);
				Math::Vector2f gradient_direction(dx, -dy);
				Math::Vector2f gradient_normal = normalize(Math::Vector2f(dy, dx));
				Math::Vector2f gradient_uv(0.0f, 0.0f);
				data << MetalVertex{ gradient_start - gradient_direction + gradient_normal, ColorVectorMake(gradient.FirstElement().Color), gradient_uv };
				data << MetalVertex{ gradient_start - gradient_direction - gradient_normal, ColorVectorMake(gradient.FirstElement().Color), gradient_uv };
				for (int i = 0; i < gradient.Length(); i++) {
					Math::Vector4f clr = ColorVectorMake(gradient[i].Color);
					float pos = gradient[i].Position;
					data << MetalVertex{ gradient_start + pos * gradient_direction + gradient_normal, clr, gradient_uv };
					data << data[data.Length() - 2];
					data << data[data.Length() - 2];
					data << MetalVertex{ gradient_start + pos * gradient_direction - gradient_normal, clr, gradient_uv };
					data << MetalVertex{ gradient_start + pos * gradient_direction + gradient_normal, clr, gradient_uv };
					data << MetalVertex{ gradient_start + pos * gradient_direction - gradient_normal, clr, gradient_uv };
				}
				data << MetalVertex{ gradient_start + 2.0f * gradient_direction + gradient_normal, ColorVectorMake(gradient.LastElement().Color), gradient_uv };
				data << data[data.Length() - 2];
				data << data[data.Length() - 2];
				data << MetalVertex{ gradient_start + 2.0f * gradient_direction - gradient_normal, ColorVectorMake(gradient.LastElement().Color), gradient_uv };
				SafePointer<MetalBarRenderingInfo> info = new (std::nothrow) MetalBarRenderingInfo;
				if (!info) return 0;
				info->use_clipping = true;
				info->verticies = [device newBufferWithBytes: data.GetBuffer() length: sizeof(MetalVertex) * data.Length() options: MTLResourceStorageModeShared];
				info->vertex_count = data.Length();
				info->Retain();
				return info;
			}
			virtual IBarRenderingInfo * CreateBarRenderingInfo(Color color) noexcept override
			{
				auto cached_info = brush_cache.ElementByKey(color);
				if (cached_info) {
					cached_info->Retain();
					return cached_info;
				}
				Array<MetalVertex> data(6);
				data.SetLength(6);
				data[0].position = Math::Vector2f(0.0f, 0.0f);
				data[0].color = ColorVectorMake(color);
				data[0].tex_coord = Math::Vector2f(0.0f, 0.0f);
				data[1].position = Math::Vector2f(1.0f, 0.0f);
				data[1].color = data[0].color;
				data[1].tex_coord = Math::Vector2f(0.0f, 0.0f);
				data[2].position = Math::Vector2f(0.0f, 1.0f);
				data[2].color = data[0].color;
				data[2].tex_coord = Math::Vector2f(0.0f, 0.0f);
				data[3] = data[1];
				data[4] = data[2];
				data[5].position = Math::Vector2f(1.0f, 1.0f);
				data[5].color = data[0].color;
				data[5].tex_coord = Math::Vector2f(0.0f, 0.0f);
				SafePointer<MetalBarRenderingInfo> info = new (std::nothrow) MetalBarRenderingInfo;
				if (!info) return 0;
				info->use_clipping = false;
				info->verticies = [device newBufferWithBytes: data.GetBuffer() length: sizeof(MetalVertex) * data.Length() options: MTLResourceStorageModeShared];
				info->vertex_count = 6;
				brush_cache.Append(color, info);
				info->Retain();
				return info;
			}
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) noexcept override
			{
				SafePointer<MetalBlurEffectRenderingInfo> info = new (std::nothrow) MetalBlurEffectRenderingInfo;
				if (!info) return 0;
				info->sigma = float(power);
				info->Retain();
				return info;
			}
			virtual IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) noexcept override
			{
				if (inversion) {
					inversion->Retain();
					return inversion;
				}
				SafePointer<MetalInversionEffectRenderingInfo> info = new (std::nothrow) MetalInversionEffectRenderingInfo;
				if (!info) return 0;
				Array<MetalVertex> data(6);
				data.SetLength(6);
				data[0].position = Math::Vector2f(0.0f, 0.0f);
				data[0].color = Math::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
				data[0].tex_coord = Math::Vector2f(0.0f, 0.0f);
				data[1].position = Math::Vector2f(1.0f, 0.0f);
				data[1].color = data[0].color;
				data[1].tex_coord = data[0].tex_coord;
				data[2].position = Math::Vector2f(0.0f, 1.0f);
				data[2].color = data[0].color;
				data[2].tex_coord = data[0].tex_coord;
				data[3] = data[1];
				data[4] = data[2];
				data[5].position = Math::Vector2f(1.0f, 1.0f);
				data[5].color = data[0].color;
				data[5].tex_coord = data[0].tex_coord;
				info->verticies = [device newBufferWithBytes: data.GetBuffer() length: sizeof(MetalVertex) * data.Length() options: MTLResourceStorageModeShared];
				info->vertex_count = 6;
				info->Retain();
				inversion.SetRetain(info);
				return info;
			}
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) noexcept override
			{
				if (!texture) return 0;
				SafePointer<MetalTexture> device_texture;
				Box area = take_area;
				if (area.Left < 0) area.Left = 0;
				else if (area.Left > texture->GetWidth()) area.Left = texture->GetWidth();
				if (area.Top < 0) area.Top = 0;
				else if (area.Top > texture->GetHeight()) area.Top = texture->GetHeight();
				if (area.Right < 0) area.Right = 0;
				else if (area.Right > texture->GetWidth()) area.Right = texture->GetWidth();
				if (area.Bottom < 0) area.Bottom = 0;
				else if (area.Bottom > texture->GetHeight()) area.Bottom = texture->GetHeight();
				if (texture->IsDeviceSpecific()) {
					if (texture->GetParentDevice() == this) device_texture.SetRetain(static_cast<MetalTexture *>(texture));
					else return 0;
				} else {
					ITexture * cached = texture->GetDeviceVersion(this);
					if (!cached) {
						device_texture.SetReference(new (std::nothrow) MetalTexture);
						if (!device_texture) return 0;
						texture->AddDeviceVersion(this, device_texture);
						texture_cache << tex_pair{ texture, device_texture };
						device_texture->base = texture;
						device_texture->factory_device = this;
						device_texture->Reload(texture);
					} else device_texture.SetRetain(static_cast<MetalTexture *>(cached));
				}
				SafePointer<MetalTextureRenderingInfo> info = new (std::nothrow) MetalTextureRenderingInfo;
				if (!info) return 0;
				info->verticies = 0;
				if (fill_pattern) {
					if (area.Left == 0 && area.Top == 0 && area.Right == device_texture->w && area.Bottom == device_texture->h) {
						info->texture = device_texture;
					} else {
						SafePointer<MetalTexture> texture_cut = new (std::nothrow) MetalTexture;
						if (!texture_cut) return 0;
						MTLPixelFormat pixel_format = [device_texture->frame pixelFormat];
						texture_cut->w = area.Right - area.Left;
						texture_cut->h = area.Bottom - area.Top;
						if (texture_cut->w < 1 || texture_cut->h < 1 || texture_cut->w > 16384 || texture_cut->h > 16384) return 0;
						void * data_buffer = malloc(4 * texture_cut->w * texture_cut->h);
						if (!data_buffer) return 0;
						[device_texture->frame getBytes: data_buffer bytesPerRow: 4 * texture_cut->w fromRegion: MTLRegionMake2D(area.Left, area.Top, texture_cut->w, texture_cut->h) mipmapLevel: 0];
						@autoreleasepool {
							texture_cut->frame = [device newTextureWithDescriptor: [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: pixel_format width: texture_cut->w height: texture_cut->h mipmapped: NO]];
							[texture_cut->frame replaceRegion: MTLRegionMake2D(0, 0, texture_cut->w, texture_cut->h) mipmapLevel: 0 withBytes: data_buffer bytesPerRow: 4 * texture_cut->w];
						}
						free(data_buffer);
						info->texture = texture_cut;
					}
					Array<MetalVertex> data(6);
					data.SetLength(6);
					data[0].position = Math::Vector2f(0.0f, 0.0f);
					data[0].color = Math::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
					data[0].tex_coord = Math::Vector2f(0.0f, 0.0f);
					data[1].position = Math::Vector2f(1.0f, 0.0f);
					data[1].color = data[0].color;
					data[1].tex_coord = Math::Vector2f(0.0f, 0.0f);
					data[2].position = Math::Vector2f(0.0f, 1.0f);
					data[2].color = data[0].color;
					data[2].tex_coord = Math::Vector2f(0.0f, 0.0f);
					data[3] = data[1];
					data[4] = data[2];
					data[5].position = Math::Vector2f(1.0f, 1.0f);
					data[5].color = data[0].color;
					data[5].tex_coord = Math::Vector2f(0.0f, 0.0f);
					info->verticies = [device newBufferWithBytes: data.GetBuffer() length: sizeof(MetalVertex) * data.Length() options: MTLResourceStorageModeShared];
					info->vertex_count = 6;
					info->tile_render = true;
					info->Retain();
					return info;
				} else {
					info->texture = device_texture;
					Array<MetalVertex> data(6);
					data.SetLength(6);
					data[0].position = Math::Vector2f(0.0f, 0.0f);
					data[0].color = Math::Vector4f(1.0f, 1.0f, 1.0f, 1.0f);
					data[0].tex_coord = Math::Vector2f(area.Left, area.Top);
					data[1].position = Math::Vector2f(1.0f, 0.0f);
					data[1].color = data[0].color;
					data[1].tex_coord = Math::Vector2f(area.Right, area.Top);
					data[2].position = Math::Vector2f(0.0f, 1.0f);
					data[2].color = data[0].color;
					data[2].tex_coord = Math::Vector2f(area.Left, area.Bottom);
					data[3] = data[1];
					data[4] = data[2];
					data[5].position = Math::Vector2f(1.0f, 1.0f);
					data[5].color = data[0].color;
					data[5].tex_coord = Math::Vector2f(area.Right, area.Bottom);
					info->verticies = [device newBufferWithBytes: data.GetBuffer() length: sizeof(MetalVertex) * data.Length() options: MTLResourceStorageModeShared];
					info->vertex_count = 6;
					info->tile_render = false;
					info->Retain();
					return info;
				}
			}
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(Graphics::ITexture * texture) noexcept override
			{
				if (!texture) return 0;
				SafePointer<MetalTextureRenderingInfo> info = new (std::nothrow) MetalTextureRenderingInfo;
				if (!info) return 0;
				info->wrapped.SetRetain(texture);
				info->verticies = 0;
				info->vertex_count = 6;
				info->tile_render = false;
				info->Retain();
				return info;
			}
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) noexcept override
			{
				if (!font) return 0;
				SafePointer<ITextRenderingInfo> core_info = LoaderDevice->CreateTextRenderingInfo(font, text, 0, 0, color);
				SafePointer<MetalTextRenderingInfo> info = new MetalTextRenderingInfo;
				info->core_text_info = core_info;
				info->horz_align = horizontal_align;
				info->vert_align = vertical_align;
				info->Retain();
				return info;
			}
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const Color & color) noexcept override
			{
				if (!font) return 0;
				SafePointer<ITextRenderingInfo> core_info = LoaderDevice->CreateTextRenderingInfo(font, text, 0, 0, color);
				SafePointer<MetalTextRenderingInfo> info = new MetalTextRenderingInfo;
				info->core_text_info = core_info;
				info->horz_align = horizontal_align;
				info->vert_align = vertical_align;
				info->Retain();
				return info;
			}
			virtual Graphics::ITexture * CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height) override
			{
				if (width <= 0 || height <= 0) throw InvalidArgumentException();
				if (format != Graphics::PixelFormat::B8G8R8A8_unorm && format != Graphics::PixelFormat::R8G8B8A8_unorm) throw InvalidArgumentException();
				Graphics::TextureDesc desc;
				desc.Type = Graphics::TextureType::Type2D;
				desc.Format = format;
				desc.Width = width;
				desc.Height = height;
				desc.Depth = 0;
				desc.MipmapCount = 1;
				desc.Usage = Graphics::ResourceUsageShaderRead | Graphics::ResourceUsageRenderTarget;
				desc.MemoryPool = Graphics::ResourceMemoryPool::Default;
				return engine_device->CreateTexture(desc);
			}
			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) noexcept override
			{
				if (!desc.LastElement()) { return; }
				if (At.Right <= At.Left || At.Bottom <= At.Top) return;
				if (!Info) return;
				auto info = reinterpret_cast<MetalBarRenderingInfo *>(Info);
				if (info->use_clipping) PushClip(At);
				[encoder setRenderPipelineState: main_state];
				[encoder setFragmentTexture: white_texture atIndex: 0];
				[encoder setVertexBytes: &At length: sizeof(At) atIndex: 2];
				[encoder setVertexBuffer: info->verticies offset: 0 atIndex: 1];
				[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: info->vertex_count];
				if (info->use_clipping) PopClip();
			}
			virtual void RenderTexture(ITextureRenderingInfo * Info, const Box & At) noexcept override
			{
				if (!desc.LastElement()) { return; }
				if (At.Right <= At.Left || At.Bottom <= At.Top) return;
				if (!Info) return;
				auto info = reinterpret_cast<MetalTextureRenderingInfo *>(Info);
				if (info->tile_render) {
					TileShaderInfo tile_info;
					tile_info.draw_rect = At;
					tile_info.periods = UI::Point(info->texture->GetWidth(), info->texture->GetHeight());
					[encoder setRenderPipelineState: tile_state];
					[encoder setFragmentTexture: info->texture->frame atIndex: 0];
					[encoder setVertexBytes: &tile_info length: sizeof(tile_info) atIndex: 2];
					[encoder setVertexBuffer: info->verticies offset: 0 atIndex: 1];
					[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: info->vertex_count];
				} else if (info->texture) {
					[encoder setRenderPipelineState: main_state];
					[encoder setFragmentTexture: info->texture->frame atIndex: 0];
					[encoder setVertexBytes: &At length: sizeof(At) atIndex: 2];
					[encoder setVertexBuffer: info->verticies offset: 0 atIndex: 1];
					[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: info->vertex_count];
				} else if (info->wrapped) {
					engine_device->GetDeviceContext()->Flush();
					LayerEndInfo rinfo;
					rinfo.render_at = At;
					rinfo.size = UI::Point(info->wrapped->GetWidth(), info->wrapped->GetHeight());
					rinfo.opacity = 1.0f;
					[encoder setRenderPipelineState: layer_state];
					[encoder setFragmentTexture: MetalGraphics::GetInnerMetalTexture(info->wrapped) atIndex: 0];
					[encoder setVertexBytes: &rinfo length: sizeof(rinfo) atIndex: 2];
					[encoder setVertexBuffer: layer_vertex offset: 0 atIndex: 1];
					[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: info->vertex_count];
				}
			}
			virtual void RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) noexcept override
			{
				if (!desc.LastElement()) { return; }
				if (At.Right <= At.Left || At.Bottom <= At.Top) return;
				if (!Info) return;
				auto info = reinterpret_cast<MetalTextRenderingInfo *>(Info);
				int new_render_ofs_x = max(-At.Left, 0);
				int new_render_ofs_y = max(-At.Top, 0);
				if (!info->texture) {
					info->render_ofs_x = new_render_ofs_y = -1;
					info->core_text_info->GetExtent(info->real_width, info->real_height);
					info->width = info->real_width;
					info->height = info->real_height;
					if (info->width > 16384) { info->width = 16384; info->dynamic_ofs = true; }
					if (info->height > 16384) { info->height = 16384; info->dynamic_ofs = true; }
					if (info->width > 0 && info->height > 0) {
						@autoreleasepool {
							info->texture = [device newTextureWithDescriptor: [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: MTLPixelFormatRGBA8Unorm
								width: info->width height: info->height mipmapped: NO]];
						}
					}
				}
				if (!info->dynamic_ofs) { new_render_ofs_x = new_render_ofs_y = 0; }
				if (info->texture && (info->render_ofs_x != new_render_ofs_x || info->render_ofs_y != new_render_ofs_y)) {
					info->render_ofs_x = new_render_ofs_x;
					info->render_ofs_y = new_render_ofs_y;
					SafePointer<UI::ITextureRenderingDevice> inner_device = QuartzRenderingDevice::StaticCreateTextureRenderingDevice(info->width, info->height, 0);
					inner_device->BeginDraw();
					inner_device->RenderText(info->core_text_info, Box(-new_render_ofs_x, -new_render_ofs_y, info->width - new_render_ofs_x, info->height - new_render_ofs_y), false);
					inner_device->EndDraw();
					SafePointer<Codec::Frame> backbuffer = inner_device->GetRenderTargetAsFrame();
					[info->texture replaceRegion: MTLRegionMake2D(0, 0, info->width, info->height) mipmapLevel: 0 withBytes: backbuffer->GetData() bytesPerRow: backbuffer->GetScanLineLength()];
				}
				if (info->texture) {
					if (Clip) PushClip(At);
					LayerEndInfo rinfo;
					if (info->horz_align == 0) {
						rinfo.render_at.Left = At.Left + new_render_ofs_x;
						rinfo.render_at.Right = At.Left + new_render_ofs_x + info->width;
					} else if (info->horz_align == 1) {
						rinfo.render_at.Left = (At.Left + At.Right - info->real_width) / 2 + new_render_ofs_x;
						rinfo.render_at.Right = rinfo.render_at.Left + info->width;
					} else {
						rinfo.render_at.Right = At.Right + new_render_ofs_x;
						rinfo.render_at.Left = At.Right - info->real_width;
						rinfo.render_at.Right = rinfo.render_at.Left + info->width;
					}
					if (info->vert_align == 0) {
						rinfo.render_at.Top = At.Top + new_render_ofs_y;
						rinfo.render_at.Bottom = At.Top + new_render_ofs_y + info->height;
					} else if (info->vert_align == 1) {
						rinfo.render_at.Top = (At.Top + At.Bottom - info->real_height) / 2 + new_render_ofs_y;
						rinfo.render_at.Bottom = rinfo.render_at.Top + info->height;
					} else {
						rinfo.render_at.Bottom = At.Bottom + new_render_ofs_y;
						rinfo.render_at.Top = At.Bottom - info->real_height;
						rinfo.render_at.Bottom = rinfo.render_at.Top + info->height;
					}
					rinfo.size = UI::Point(info->width, info->height);
					rinfo.opacity = 1.0f;
					if (info->highlight_right > info->highlight_left) {
						if (!info->highlight_info) info->highlight_info.SetReference(CreateBarRenderingInfo(info->highlight_color));
						UI::Box highlight(rinfo.render_at.Left - new_render_ofs_x + info->highlight_left, At.Top, rinfo.render_at.Left - new_render_ofs_x + info->highlight_right, At.Bottom);
						RenderBar(info->highlight_info, highlight);
					}
					[encoder setRenderPipelineState: layer_state];
					[encoder setFragmentTexture: info->texture atIndex: 0];
					[encoder setVertexBytes: &rinfo length: sizeof(rinfo) atIndex: 2];
					[encoder setVertexBuffer: layer_vertex offset: 0 atIndex: 1];
					[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: 6];
					if (Clip) PopClip();
				}
			}
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) noexcept override
			{
				if (!desc.LastElement() || desc.Length() > 1 || !Info) return;
				if (At.Right <= At.Left || At.Bottom <= At.Top) return;
				auto info = reinterpret_cast<MetalBlurEffectRenderingInfo *>(Info);
				auto blur_box = UI::Box::Intersect(clipping.LastElement(), At);
				if (blur_box.Right <= blur_box.Left || blur_box.Bottom <= blur_box.Top) return;
				auto size = UI::Point(blur_box.Right - blur_box.Left, blur_box.Bottom - blur_box.Top);
				[encoder endEncoding];
				id<MTLTexture> blur_region;
				id<MTLTexture> blur_region_mip;
				uint lod = 1;
				float sigma = info->sigma;
				UI::Point mip_size = size;
				while (sigma > 4.0f) { sigma /= 2.0f; lod++; mip_size.x /= 2; mip_size.y /= 2; }
				@autoreleasepool {
					auto tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: pixel_format width: size.x height: size.y mipmapped: NO];
					tex_desc.mipmapLevelCount = lod;
					[tex_desc setUsage: MTLTextureUsageShaderRead];
					blur_region = [device newTextureWithDescriptor: tex_desc];
					blur_region_mip = [blur_region newTextureViewWithPixelFormat: pixel_format textureType: MTLTextureType2D
						levels: NSMakeRange(lod - 1, 1) slices: NSMakeRange(0, 1)];
				}
				id<MTLTexture> render_target = [[[base_descriptor colorAttachments] objectAtIndexedSubscript: 0] texture];
				id<MTLBlitCommandEncoder> blit_encoder = [command_buffer blitCommandEncoder];
				[blit_encoder copyFromTexture: render_target sourceSlice: 0 sourceLevel: 0 sourceOrigin: MTLOriginMake(blur_box.Left, blur_box.Top, 0)
					sourceSize: MTLSizeMake(size.x, size.y, 1) toTexture: blur_region destinationSlice: 0 destinationLevel: 0 destinationOrigin: MTLOriginMake(0, 0, 0)];
				[blit_encoder generateMipmapsForTexture: blur_region];
				[blit_encoder endEncoding];
				auto prev_desc = desc.LastElement();
				[[[prev_desc colorAttachments] objectAtIndexedSubscript: 0] setLoadAction: MTLLoadActionLoad];
				encoder = [command_buffer renderCommandEncoderWithDescriptor: prev_desc];
				[encoder setVertexBuffer: viewport_data offset: 0 atIndex: 0];
				[encoder setVertexBuffer: box_data offset: 0 atIndex: 2];
				[encoder setVertexBytes: &layers.LastElement() length: sizeof(LayerInfo) atIndex: 0];
				auto & box = clipping.LastElement();
				auto & layer = layers.LastElement();
				MTLScissorRect scissor;
				scissor.x = box.Left - layer.viewport_offset.x;
				scissor.y = box.Top - layer.viewport_offset.y;
				scissor.width = box.Right - box.Left;
				scissor.height = box.Bottom - box.Top;
				[encoder setScissorRect: scissor];
				LayerEndInfo rinfo;
				rinfo.render_at = blur_box;
				rinfo.size = mip_size;
				rinfo.opacity = sigma;
				[encoder setRenderPipelineState: blur_state];
				[encoder setFragmentTexture: blur_region_mip atIndex: 0];
				[encoder setVertexBytes: &rinfo length: sizeof(rinfo) atIndex: 2];
				[encoder setVertexBuffer: layer_vertex offset: 0 atIndex: 1];
				[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: 6];
				[blur_region release];
				[blur_region_mip release];
			}
			virtual void ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) noexcept override
			{
				if (!desc.LastElement()) { return; }
				if (At.Right <= At.Left || At.Bottom <= At.Top) return;
				if (Blink && !CaretShouldBeVisible()) return;
				if (!Info) return;
				auto info = reinterpret_cast<MetalInversionEffectRenderingInfo *>(Info);
				[encoder setRenderPipelineState: invert_state];
				[encoder setFragmentTexture: white_texture atIndex: 0];
				[encoder setVertexBytes: &At length: sizeof(At) atIndex: 2];
				[encoder setVertexBuffer: info->verticies offset: 0 atIndex: 1];
				[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: info->vertex_count];
			}
			virtual void DrawPolygon(const Math::Vector2 * points, int count, Color color, double width) noexcept override {}
			virtual void FillPolygon(const Math::Vector2 * points, int count, Color color) noexcept override {}
			virtual void PushClip(const Box & Rect) noexcept override
			{
				clipping << UI::Box::Intersect(clipping.LastElement(), Rect);
				auto & box = clipping.LastElement();
				auto & layer = layers.LastElement();
				MTLScissorRect scissor;
				scissor.x = box.Left - layer.viewport_offset.x;
				scissor.y = box.Top - layer.viewport_offset.y;
				scissor.width = box.Right - box.Left;
				scissor.height = box.Bottom - box.Top;
				[encoder setScissorRect: scissor];
			}
			virtual void PopClip(void) noexcept override
			{
				clipping.RemoveLast();
				auto & box = clipping.LastElement();
				auto & layer = layers.LastElement();
				MTLScissorRect scissor;
				scissor.x = box.Left - layer.viewport_offset.x;
				scissor.y = box.Top - layer.viewport_offset.y;
				scissor.width = box.Right - box.Left;
				scissor.height = box.Bottom - box.Top;
				[encoder setScissorRect: scissor];
			}
			virtual void BeginLayer(const Box & Rect, double Opacity) noexcept override
			{
				if (!desc.LastElement()) { desc << 0; return; }
				layer_alpha << Opacity;
				auto layer_box = UI::Box::Intersect(clipping.LastElement(), Rect);
				if (layer_box.Right <= layer_box.Left || layer_box.Bottom <= layer_box.Top) { desc << 0; return; }
				clipping << layer_box;
				LayerInfo info;
				info.viewport_size = UI::Point(layer_box.Right - layer_box.Left, layer_box.Bottom - layer_box.Top);
				info.viewport_offset = UI::Point(layer_box.Left, layer_box.Top);
				layers << info;
				id<MTLTexture> render_target;
				@autoreleasepool {
					auto tex_desc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat: pixel_format width: info.viewport_size.x height: info.viewport_size.y mipmapped: NO];
					[tex_desc setUsage: MTLTextureUsageShaderRead | MTLTextureUsageRenderTarget];
					render_target = [device newTextureWithDescriptor: tex_desc];
				}
				layer_texture << render_target;
				auto layer_desc = [MTLRenderPassDescriptor renderPassDescriptor];
				auto attachment = [[MTLRenderPassColorAttachmentDescriptor alloc] init];
				[attachment setTexture: render_target];
				[attachment setClearColor: MTLClearColorMake(0.0, 0.0, 0.0, 0.0)];
				[attachment setLoadAction: MTLLoadActionClear];
				[[layer_desc colorAttachments] setObject: attachment atIndexedSubscript: 0];
				[attachment release];
				[encoder endEncoding];
				desc << layer_desc;
				encoder = [command_buffer renderCommandEncoderWithDescriptor: layer_desc];
				[encoder setVertexBuffer: viewport_data offset: 0 atIndex: 0];
				[encoder setVertexBuffer: box_data offset: 0 atIndex: 2];
				[encoder setVertexBytes: &info length: sizeof(info) atIndex: 0];
			}
			virtual void EndLayer(void) noexcept override
			{
				if (!desc.LastElement()) { desc.RemoveLast(); return; }
				[encoder endEncoding];
				desc.RemoveLast();
				auto prev_desc = desc.LastElement();
				[[[prev_desc colorAttachments] objectAtIndexedSubscript: 0] setLoadAction: MTLLoadActionLoad];
				encoder = [command_buffer renderCommandEncoderWithDescriptor: prev_desc];
				[encoder setVertexBuffer: viewport_data offset: 0 atIndex: 0];
				[encoder setVertexBuffer: box_data offset: 0 atIndex: 2];
				[encoder setVertexBytes: &layers[layers.Length() - 2] length: sizeof(LayerInfo) atIndex: 0];
				auto & cinfo = layers.LastElement();
				LayerEndInfo info;
				info.render_at = UI::Box(cinfo.viewport_offset.x, cinfo.viewport_offset.y,
					cinfo.viewport_offset.x + cinfo.viewport_size.x, cinfo.viewport_offset.y + cinfo.viewport_size.y);
				info.size = UI::Point(cinfo.viewport_size.x, cinfo.viewport_size.y);
				info.opacity = layer_alpha.LastElement();
				[encoder setRenderPipelineState: layer_state];
				[encoder setFragmentTexture: layer_texture.LastElement() atIndex: 0];
				[encoder setVertexBytes: &info length: sizeof(info) atIndex: 2];
				[encoder setVertexBuffer: layer_vertex offset: 0 atIndex: 1];
				[encoder drawPrimitives: MTLPrimitiveTypeTriangle vertexStart: 0 vertexCount: 6];
				[layer_texture.LastElement() release];
				layer_texture.RemoveLast();
				layer_alpha.RemoveLast();
				layers.RemoveLast();
				PopClip();
			}
			virtual void SetTimerValue(uint32 time) noexcept override { LoaderDevice->SetTimerValue(time); }
			virtual uint32 GetCaretBlinkHalfTime(void) noexcept override { return LoaderDevice->GetCaretBlinkHalfTime(); }
			virtual bool CaretShouldBeVisible(void) noexcept override { return LoaderDevice->CaretShouldBeVisible(); }
			virtual void ClearCache(void) noexcept override { brush_cache.Clear(); inversion.SetReference(0); }
			virtual ITexture * LoadTexture(Codec::Frame * source) noexcept override
			{
				return LoaderDevice->LoadTexture(source);
			}
			virtual IFont * LoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept override
			{
				return LoaderDevice->LoadFont(face_name, height, weight, italic, underline, strikeout);
			}
			virtual ITextureRenderingDevice * CreateTextureRenderingDevice(int width, int height, Color color) noexcept override
			{
				return LoaderDevice->CreateTextureRenderingDevice(width, height, color);
			}
			virtual ITextureRenderingDevice * CreateTextureRenderingDevice(Codec::Frame * frame) noexcept override
			{
				return LoaderDevice->CreateTextureRenderingDevice(frame);
			}
		};

		id<MTLDevice> GetInnerMetalDevice(UI::IRenderingDevice * device) { return static_cast<MetalDevice *>(device)->device; }
		UI::IRenderingDevice * CreateMetalRenderingDevice(MetalGraphics::MetalPresentationInterface * presentation)
		{
			SafePointer<MetalDevice> engine_device = new (std::nothrow) MetalDevice;
			if (!engine_device) return 0;
			Graphics::IDevice * engine_graphics_device = MetalGraphics::GetMetalCommonDevice();
			engine_device->engine_device = engine_graphics_device;
			engine_device->device = MetalGraphics::GetInnerMetalDevice(engine_graphics_device);
			if (common_library) {
				engine_device->library = common_library;
				[engine_device->library retain];
			} else {
				engine_device->library = CreateMetalRenderingDeviceShaders(engine_device->device);
				common_library = engine_device->library;
				[engine_device->library retain];
			}
			presentation->SetLayerDevice(engine_device->device);
			presentation->SetLayerAutosize(true);
			presentation->SetLayerOpaque(false);
			presentation->SetLayerFramebufferOnly(false);
			presentation->InvalidateContents();
			engine_device->pixel_format = presentation->GetPixelFormat();
			engine_device->CreateRenderingStates();
			engine_device->Retain();
			return engine_device;
		}
		UI::IRenderingDevice * CreateMetalRenderingDevice(Graphics::IDevice * device)
		{
			SafePointer<MetalDevice> engine_device = new (std::nothrow) MetalDevice;
			if (!engine_device) return 0;
			engine_device->engine_device = device;
			engine_device->device = MetalGraphics::GetInnerMetalDevice(device);
			engine_device->library = CreateMetalRenderingDeviceShaders(engine_device->device);
			engine_device->pixel_format = MTLPixelFormatBGRA8Unorm;
			engine_device->CreateRenderingStates();
			engine_device->Retain();
			return engine_device;
		}
		id<MTLLibrary> CreateMetalRenderingDeviceShaders(id<MTLDevice> device)
		{
			id<MTLLibrary> library = 0;
			@autoreleasepool {
				const void * shaders_data;
				int shaders_size;
				NSError * error;
				GetMetalDeviceShaders(&shaders_data, &shaders_size);
				dispatch_data_t data_handle = dispatch_data_create(shaders_data, shaders_size, dispatch_get_main_queue(), DISPATCH_DATA_DESTRUCTOR_DEFAULT);
				library = [device newLibraryWithData: data_handle error: &error];
				[data_handle release];
				if (error) [error release];
			}
			return library;
		}
		id<MTLDrawable> CoreMetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, MetalGraphics::MetalPresentationInterface * presentation)
		{
			auto engine_device = static_cast<MetalDevice *>(device);
			auto drawable = presentation->GetLayerDrawable();
			auto size = presentation->GetLayerSize();
			MTLRenderPassDescriptor * descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
			descriptor.colorAttachments[0].texture = drawable.texture;
			descriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
			descriptor.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 0.0);
			engine_device->width = size.x;
			engine_device->height = size.y;
			engine_device->base_descriptor = descriptor;
			engine_device->command_buffer = [MetalGraphics::GetInnerMetalQueue(engine_device->engine_device) commandBuffer];
			engine_device->encoder = [engine_device->command_buffer renderCommandEncoderWithDescriptor: descriptor];
			[engine_device->encoder setViewport: (MTLViewport) { 0.0, 0.0, double(engine_device->width), double(engine_device->height), 0.0, 1.0 }];
			engine_device->InitDraw();
			return drawable;
		}
		void CoreMetalRenderingDeviceEndDraw(UI::IRenderingDevice * device, id<MTLDrawable> drawable, bool wait)
		{
			auto engine_device = static_cast<MetalDevice *>(device);
			[engine_device->encoder endEncoding];
			[engine_device->command_buffer presentDrawable: drawable];
			[engine_device->command_buffer commit];
			if (wait) [engine_device->command_buffer waitUntilCompleted];
			engine_device->encoder = 0;
			engine_device->command_buffer = 0;
		}
		void PureMetalRenderingDeviceBeginDraw(UI::IRenderingDevice * device, id<MTLCommandBuffer> command, id<MTLTexture> texture, uint width, uint height)
		{
			auto engine_device = static_cast<MetalDevice *>(device);
			MTLRenderPassDescriptor * descriptor = [MTLRenderPassDescriptor renderPassDescriptor];
			descriptor.colorAttachments[0].texture = texture;
			descriptor.colorAttachments[0].loadAction = MTLLoadActionLoad;
			engine_device->width = width;
			engine_device->height = height;
			engine_device->base_descriptor = descriptor;
			engine_device->command_buffer = command;
			engine_device->encoder = [command renderCommandEncoderWithDescriptor: descriptor];
			[engine_device->encoder setViewport: (MTLViewport) { 0.0, 0.0, double(width), double(height), 0.0, 1.0 }];
			engine_device->InitDraw();
		}
		void PureMetalRenderingDeviceEndDraw(UI::IRenderingDevice * device)
		{
			auto engine_device = static_cast<MetalDevice *>(device);
			[engine_device->encoder endEncoding];
			engine_device->encoder = 0;
			engine_device->command_buffer = 0;
		}
	}
}