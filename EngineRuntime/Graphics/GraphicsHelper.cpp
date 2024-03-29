#include "GraphicsHelper.h"

#include "../Math/Color.h"

namespace Engine
{
	namespace Graphics
	{
		SamplerDesc DefaultSamplerDesc(void)
		{
			SamplerDesc result;
			result.MinificationFilter = result.MagnificationFilter = result.MipFilter = SamplerFilter::Point;
			result.AddressU = result.AddressV = result.AddressW = SamplerAddressMode::Wrap;
			result.MaximalAnisotropy = 1;
			result.MinimalLOD = -1.0E+35f;
			result.MaximalLOD = +1.0E+35f;
			ZeroMemory(&result.BorderColor, sizeof(result.BorderColor));
			return result;
		}
		PipelineStateDesc DefaultPipelineStateDesc(IShader * vs, IShader * ps, PixelFormat rt, bool use_depth, PixelFormat ds)
		{
			PipelineStateDesc result;
			ZeroMemory(&result, sizeof(result));
			result.VertexShader = vs;
			result.PixelShader = ps;
			result.RenderTargetCount = 1;
			for (int i = 0; i < 8; i++) {
				result.RenderTarget[i].Format = rt;
				result.RenderTarget[i].BlendRGB = result.RenderTarget[i].BlendAlpha = BlendingFunction::Add;
				result.RenderTarget[i].BaseFactorRGB = result.RenderTarget[i].BaseFactorAlpha = BlendingFactor::Zero;
				result.RenderTarget[i].OverFactorRGB = result.RenderTarget[i].OverFactorAlpha = BlendingFactor::One;
			}
			if (use_depth) {
				result.DepthStencil.Format = ds;
				result.DepthStencil.Flags = DepthStencilFlagDepthTestEnabled | DepthStencilFlagDepthWriteEnabled;
			}
			result.DepthStencil.DepthTestFunction = CompareFunction::Lesser;
			result.DepthStencil.StencilReadMask = result.DepthStencil.StencilWriteMask = 0xFF;
			result.DepthStencil.FrontStencil.TestFunction = result.DepthStencil.BackStencil.TestFunction = CompareFunction::Always;
			result.DepthStencil.FrontStencil.OnStencilTestFailed = result.DepthStencil.BackStencil.OnStencilTestFailed = StencilFunction::Keep;
			result.DepthStencil.FrontStencil.OnDepthTestFailed = result.DepthStencil.BackStencil.OnDepthTestFailed = StencilFunction::Keep;
			result.DepthStencil.FrontStencil.OnTestsPassed = result.DepthStencil.BackStencil.OnTestsPassed = StencilFunction::Keep;
			result.Rasterization.Fill = FillMode::Solid;
			result.Rasterization.Cull = CullMode::None;
			result.Rasterization.DepthClipEnable = true;
			result.Topology = PrimitiveTopology::TriangleList;
			return result;
		}
		BufferDesc CreateBufferDesc(uint32 length, uint32 usage, uint32 stride, ResourceMemoryPool pool)
		{
			BufferDesc result;
			result.Length = length;
			result.Usage = usage;
			result.Stride = stride;
			result.MemoryPool = pool;
			return result;
		}
		TextureDesc CreateTextureDesc1D(PixelFormat format, uint32 width, uint32 mips, uint32 usage, ResourceMemoryPool pool)
		{
			TextureDesc result;
			result.Type = TextureType::Type1D;
			result.Format = format;
			result.Width = width;
			result.Height = result.Depth = 0;
			result.MipmapCount = mips;
			result.Usage = usage;
			result.MemoryPool = pool;
			return result;
		}
		TextureDesc CreateTextureDesc2D(PixelFormat format, uint32 width, uint32 height, uint32 mips, uint32 usage, ResourceMemoryPool pool)
		{
			TextureDesc result;
			result.Type = TextureType::Type2D;
			result.Format = format;
			result.Width = width;
			result.Height = height;
			result.Depth = 0;
			result.MipmapCount = mips;
			result.Usage = usage;
			result.MemoryPool = pool;
			return result;
		}
		TextureDesc CreateTextureDesc3D(PixelFormat format, uint32 width, uint32 height, uint32 depth, uint32 mips, uint32 usage, ResourceMemoryPool pool)
		{
			TextureDesc result;
			result.Type = TextureType::Type3D;
			result.Format = format;
			result.Width = width;
			result.Height = height;
			result.Depth = depth;
			result.MipmapCount = mips;
			result.Usage = usage;
			result.MemoryPool = pool;
			return result;
		}
		WindowLayerDesc CreateWindowLayerDesc(uint32 width, uint32 height, PixelFormat format, uint32 usage)
		{
			WindowLayerDesc result;
			result.Format = format;
			result.Width = width;
			result.Height = height;
			result.Usage = usage;
			return result;
		}
		ResourceInitDesc CreateInitDesc(const void * data, uint32 pitch, uint32 slice_pitch)
		{
			ResourceInitDesc result;
			result.Data = data;
			result.DataPitch = pitch;
			result.DataSlicePitch = slice_pitch;
			return result;
		}
		ResourceDataDesc CreateDataDesc(void * data, uint32 pitch, uint32 slice_pitch)
		{
			ResourceDataDesc result;
			result.Data = data;
			result.DataPitch = pitch;
			result.DataSlicePitch = slice_pitch;
			return result;
		}
		RenderTargetViewDesc CreateRenderTargetView(ITexture * rt, TextureLoadAction action)
		{
			RenderTargetViewDesc result;
			result.Texture = rt;
			result.LoadAction = action;
			ZeroMemory(&result.ClearValue, sizeof(result.ClearValue));
			return result;
		}
		RenderTargetViewDesc CreateRenderTargetView(ITexture * rt, const Math::Vector4f & clear_color)
		{
			RenderTargetViewDesc result;
			result.Texture = rt;
			result.LoadAction = TextureLoadAction::Clear;
			MemoryCopy(&result.ClearValue, &clear_color, sizeof(result.ClearValue));
			return result;
		}
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, TextureLoadAction action)
		{
			DepthStencilViewDesc result;
			result.Texture = ds;
			result.DepthLoadAction = result.StencilLoadAction = action;
			result.DepthClearValue = 0.0f;
			result.StencilClearValue = 0;
			return result;
		}
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, float clear_depth)
		{
			DepthStencilViewDesc result;
			result.Texture = ds;
			result.DepthLoadAction = TextureLoadAction::Clear;
			result.StencilLoadAction = TextureLoadAction::DontCare;
			result.DepthClearValue = clear_depth;
			result.StencilClearValue = 0;
			return result;
		}
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, float clear_depth, uint8 clear_stencil)
		{
			DepthStencilViewDesc result;
			result.Texture = ds;
			result.DepthLoadAction = result.StencilLoadAction = TextureLoadAction::Clear;
			result.DepthClearValue = clear_depth;
			result.StencilClearValue = clear_stencil;
			return result;
		}
		
		Math::Matrix4x4f MakeTranslateTransform(const Math::Vector3f & shift)
		{
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(1.0f, 0.0f, 0.0f, shift.x);
			result.row[1] = Math::Vector4f(0.0f, 1.0f, 0.0f, shift.y);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, 1.0f, shift.z);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeScaleTransform(const Math::Vector3f & scale)
		{
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(scale.x, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, scale.y, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, scale.z, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransformX(float angle)
		{
			auto c = float(Math::cos(angle));
			auto s = float(Math::sin(angle));
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(1.0f, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, c, -s, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, s, c, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransformY(float angle)
		{
			auto c = float(Math::cos(angle));
			auto s = float(Math::sin(angle));
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(c, 0.0f, s, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, 1.0f, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(-s, 0.0f, c, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransformZ(float angle)
		{
			auto c = float(Math::cos(angle));
			auto s = float(Math::sin(angle));
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(c, -s, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(s, c, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, 1.0f, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransform(float angle, const Math::Vector3f & axis)
		{
			auto c = float(Math::cos(angle));
			auto s = float(Math::sin(angle));
			auto z = Math::normalize(axis);
			Math::Vector3f orth;
			if (abs(z.x) > 0.5f) orth = Math::Vector3f(0.0f, 1.0f, 0.0f);
			else orth = Math::Vector3f(1.0f, 0.0f, 0.0f);
			auto x = Math::normalize(Math::cross(z, orth));
			auto y = Math::cross(z, x);
			Math::Matrix3x3f trans, rot;
			trans.row[0] = x;
			trans.row[1] = y;
			trans.row[2] = z;
			Math::Matrix3x3f rev = Math::transpone(trans);
			rot.row[0] = Math::Vector3f(c, -s, 0.0f);
			rot.row[1] = Math::Vector3f(s, c, 0.0f);
			rot.row[2] = Math::Vector3f(0.0f, 0.0f, 1.0f);
			Math::Matrix3x3f full = rev * rot * trans;
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(full.row[0].x, full.row[0].y, full.row[0].z, 0.0f);
			result.row[1] = Math::Vector4f(full.row[1].x, full.row[1].y, full.row[1].z, 0.0f);
			result.row[2] = Math::Vector4f(full.row[2].x, full.row[2].y, full.row[2].z, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeReflectTransform(const Math::Vector4f & plane)
		{
			auto norm = float(Math::sqrt(plane.x * plane.x + plane.y * plane.y + plane.z * plane.z));
			auto p = plane / norm;
			Math::Matrix4x4f result;
			result.row[0] = -2.0f * p.x * p;
			result.row[1] = -2.0f * p.y * p;
			result.row[2] = -2.0f * p.z * p;
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 0.0f);
			return result + Math::identity<float, 4>();
		}
		Math::Matrix4x4f MakeLookAtTransform(const Math::Vector3f & cam, const Math::Vector3f & dir, const Math::Vector3f & up)
		{
			auto z = -Math::normalize(dir);
			auto x = Math::normalize(Math::cross(up, z));
			auto y = Math::cross(z, x);
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(x.x, x.y, x.z, -Math::dot(x, cam));
			result.row[1] = Math::Vector4f(y.x, y.y, y.z, -Math::dot(y, cam));
			result.row[2] = Math::Vector4f(z.x, z.y, z.z, -Math::dot(z, cam));
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeOrthogonalViewTransform(float width, float height, float near_plane, float far_plane)
		{
			auto inv_depth = 1.0f / (near_plane - far_plane);
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(2.0f / width, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, 2.0f / height, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, inv_depth, inv_depth * near_plane);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakePerspectiveViewTransform(float near_width, float near_height, float near_plane, float far_plane)
		{
			auto inv_depth = 1.0f / (near_plane - far_plane);
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(2.0f * near_plane / near_width, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, 2.0f * near_plane / near_height, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, inv_depth * far_plane, inv_depth * far_plane * near_plane);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, -1.0f, 0.0f);
			return result;
		}
		Math::Matrix4x4f MakePerspectiveViewTransformFoV(float fov_angle, float aspect, float near_plane, float far_plane)
		{
			auto inv_depth = 1.0f / (near_plane - far_plane);
			auto scale_y = float(Math::ctg(fov_angle / 2.0f));
			auto scale_x = scale_y / aspect;
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(scale_x, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, scale_y, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, inv_depth * far_plane, inv_depth * far_plane * near_plane);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, -1.0f, 0.0f);
			return result;
		}

		ITexture * LoadTexture(IDevice * device, Codec::Frame * frame, uint32 mip_levels, uint32 usage, PixelFormat format, ResourceMemoryPool pool, Codec::AlphaMode alpha, Codec::ScanOrigin origin)
		{
			Codec::PixelFormat codec_format;
			if (format == PixelFormat::Invalid) {
				if (frame->GetPixelFormat() == Codec::PixelFormat::B8G8R8A8) format = PixelFormat::B8G8R8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B8G8R8X8) format = PixelFormat::B8G8R8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B8G8R8) format = PixelFormat::B8G8R8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) format = PixelFormat::R8G8B8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R8G8B8X8) format = PixelFormat::R8G8B8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R8G8B8) format = PixelFormat::R8G8B8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B5G5R5A1) format = PixelFormat::B5G5R5A1_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B5G5R5X1) format = PixelFormat::B5G5R5A1_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R5G5B5A1) format = PixelFormat::B5G5R5A1_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R5G5B5X1) format = PixelFormat::B5G5R5A1_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B5G6R5) format = PixelFormat::B5G6R5_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R5G6B5) format = PixelFormat::B5G6R5_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B4G4R4A4) format = PixelFormat::B8G8R8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::B4G4R4X4) format = PixelFormat::B8G8R8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R4G4B4A4) format = PixelFormat::R8G8B8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R4G4B4X4) format = PixelFormat::R8G8B8A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::A8) format = PixelFormat::A8_unorm;
				else if (frame->GetPixelFormat() == Codec::PixelFormat::R8) format = PixelFormat::R8_unorm;
				else format = PixelFormat::B8G8R8A8_unorm;
			}
			if (format == PixelFormat::B8G8R8A8_unorm) codec_format = Codec::PixelFormat::B8G8R8A8;
			else if (format == PixelFormat::R8G8B8A8_unorm) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::R8G8B8A8_snorm) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::R8G8B8A8_uint) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::R8G8B8A8_sint) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::B5G5R5A1_unorm) codec_format = Codec::PixelFormat::B5G5R5A1;
			else if (format == PixelFormat::B5G6R5_unorm) codec_format = Codec::PixelFormat::B5G6R5;
			else if (format == PixelFormat::A8_unorm) codec_format = Codec::PixelFormat::A8;
			else if (format == PixelFormat::R8_unorm) codec_format = Codec::PixelFormat::R8;
			else if (format == PixelFormat::R8_snorm) codec_format = Codec::PixelFormat::R8;
			else if (format == PixelFormat::R8_uint) codec_format = Codec::PixelFormat::R8;
			else if (format == PixelFormat::R8_sint) codec_format = Codec::PixelFormat::R8;
			else throw InvalidArgumentException();
			SafePointer<Codec::Frame> converted;
			bool convert = false;
			if (frame->GetPixelFormat() != codec_format) convert = true;
			if (frame->GetAlphaMode() != alpha) convert = true;
			if (frame->GetScanOrigin() != origin) convert = true;
			if (convert) converted = frame->ConvertFormat(codec_format, alpha, origin);
			else converted.SetRetain(frame);
			Array<ResourceInitDesc> init(0x10);
			ObjectArray<Codec::Frame> mips(0x10);
			TextureDesc desc;
			desc.Type = TextureType::Type2D;
			desc.Format = format;
			desc.Width = converted->GetWidth();
			desc.Height = converted->GetHeight();
			desc.Depth = 0;
			desc.MipmapCount = mip_levels;
			desc.Usage = usage;
			desc.MemoryPool = pool;
			uint current_mip = 0;
			do {
				if (!converted) throw InvalidArgumentException();
				mips.Append(converted);
				init.Append(CreateInitDesc(converted->GetData(), converted->GetScanLineLength()));
				converted = CreateMipLevel(converted);
				current_mip++;
				if (!mip_levels) desc.MipmapCount++;
			} while ((mip_levels && current_mip < mip_levels) || (!mip_levels && converted));
			return device->CreateTexture(desc, init.GetBuffer());
		}
		ITexture * LoadTexture(IDevice * device, Streaming::Stream * stream, uint32 mip_levels, uint32 usage, PixelFormat format, ResourceMemoryPool pool, Codec::AlphaMode alpha, Codec::ScanOrigin origin)
		{
			SafePointer<Codec::Frame> frame = Codec::DecodeFrame(stream);
			if (!frame) throw InvalidFormatException();
			return LoadTexture(device, frame, mip_levels, usage, format, pool, alpha, origin);
		}
		ITexture * LoadCubeTexture(IDevice * device, Codec::Image * image, uint32 mip_levels, uint32 usage, PixelFormat format, ResourceMemoryPool pool, Codec::AlphaMode alpha, Codec::ScanOrigin origin)
		{
			if (image->Frames.Length() != 6) throw InvalidArgumentException();
			for (int i = 0; i < image->Frames.Length(); i++) {
				if (image->Frames[i].GetWidth() != image->Frames[i].GetHeight()) throw InvalidArgumentException();
				if (image->Frames[i].GetWidth() != image->Frames[0].GetWidth()) throw InvalidArgumentException();
			}
			Codec::PixelFormat codec_format;
			if (format == PixelFormat::Invalid) {
				if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B8G8R8A8) format = PixelFormat::B8G8R8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B8G8R8X8) format = PixelFormat::B8G8R8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B8G8R8) format = PixelFormat::B8G8R8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R8G8B8A8) format = PixelFormat::R8G8B8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R8G8B8X8) format = PixelFormat::R8G8B8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R8G8B8) format = PixelFormat::R8G8B8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B5G5R5A1) format = PixelFormat::B5G5R5A1_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B5G5R5X1) format = PixelFormat::B5G5R5A1_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R5G5B5A1) format = PixelFormat::B5G5R5A1_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R5G5B5X1) format = PixelFormat::B5G5R5A1_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B5G6R5) format = PixelFormat::B5G6R5_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R5G6B5) format = PixelFormat::B5G6R5_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B4G4R4A4) format = PixelFormat::B8G8R8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::B4G4R4X4) format = PixelFormat::B8G8R8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R4G4B4A4) format = PixelFormat::R8G8B8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R4G4B4X4) format = PixelFormat::R8G8B8A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::A8) format = PixelFormat::A8_unorm;
				else if (image->Frames[0].GetPixelFormat() == Codec::PixelFormat::R8) format = PixelFormat::R8_unorm;
				else format = PixelFormat::B8G8R8A8_unorm;
			}
			if (format == PixelFormat::B8G8R8A8_unorm) codec_format = Codec::PixelFormat::B8G8R8A8;
			else if (format == PixelFormat::R8G8B8A8_unorm) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::R8G8B8A8_snorm) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::R8G8B8A8_uint) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::R8G8B8A8_sint) codec_format = Codec::PixelFormat::R8G8B8A8;
			else if (format == PixelFormat::B5G5R5A1_unorm) codec_format = Codec::PixelFormat::B5G5R5A1;
			else if (format == PixelFormat::B5G6R5_unorm) codec_format = Codec::PixelFormat::B5G6R5;
			else if (format == PixelFormat::A8_unorm) codec_format = Codec::PixelFormat::A8;
			else if (format == PixelFormat::R8_unorm) codec_format = Codec::PixelFormat::R8;
			else if (format == PixelFormat::R8_snorm) codec_format = Codec::PixelFormat::R8;
			else if (format == PixelFormat::R8_uint) codec_format = Codec::PixelFormat::R8;
			else if (format == PixelFormat::R8_sint) codec_format = Codec::PixelFormat::R8;
			else throw InvalidArgumentException();
			Array<ResourceInitDesc> init(0x10);
			ObjectArray<Codec::Frame> mips(0x10);
			TextureDesc desc;
			desc.Type = TextureType::TypeCube;
			desc.Format = format;
			desc.Width = image->Frames[0].GetWidth();
			desc.Height = image->Frames[0].GetHeight();
			desc.Depth = 0;
			desc.MipmapCount = mip_levels;
			desc.Usage = usage;
			desc.MemoryPool = pool;
			for (int k = 0; k < image->Frames.Length(); k++) {
				auto frame = image->Frames.ElementAt(k);
				SafePointer<Codec::Frame> converted;
				bool convert = false;
				if (frame->GetPixelFormat() != codec_format) convert = true;
				if (frame->GetAlphaMode() != alpha) convert = true;
				if (frame->GetScanOrigin() != origin) convert = true;
				if (convert) converted = frame->ConvertFormat(codec_format, alpha, origin);
				else converted.SetRetain(frame);
				uint current_mip = 0;
				do {
					if (!converted) throw InvalidArgumentException();
					mips.Append(converted);
					init.Append(CreateInitDesc(converted->GetData(), converted->GetScanLineLength()));
					converted = CreateMipLevel(converted);
					current_mip++;
					if (!mip_levels) desc.MipmapCount++;
				} while ((mip_levels && current_mip < mip_levels) || (!mip_levels && converted));
				if (!mip_levels) mip_levels = desc.MipmapCount;
			}
			return device->CreateTexture(desc, init.GetBuffer());
		}
		Codec::Frame * CreateMipLevel(Codec::Frame * source)
		{
			SafePointer<Codec::Frame> source_corrected;
			if (GetBitsPerPixel(source->GetPixelFormat()) <= 16) source_corrected = source->ConvertFormat(Codec::PixelFormat::B8G8R8A8);
			else source_corrected.SetRetain(source);
			int width = source->GetWidth();
			int height = source->GetHeight();
			int mw = max(width / 2, 1);
			int mh = max(height / 2, 1);
			if (width == 1 && height == 1) return 0;
			SafePointer<Codec::Frame> mip = new Codec::Frame(mw, mh, -1, source_corrected->GetPixelFormat(), source_corrected->GetAlphaMode(), source_corrected->GetScanOrigin());
			for (int y = 0; y < mh; y++) for (int x = 0; x < mw; x++) {
				Math::ColorF sum = Math::ColorF(Color(source_corrected->GetPixel(x << 1, y << 1)));
				int c = 1;
				if (width > 1) {
					sum += Math::ColorF(Color(source_corrected->GetPixel((x << 1) + 1, y << 1)));
					c++;
					if (height > 1) {
						sum += Math::ColorF(Color(source_corrected->GetPixel(x << 1, (y << 1) + 1)));
						sum += Math::ColorF(Color(source_corrected->GetPixel((x << 1) + 1, (y << 1) + 1)));
						c += 2;
					}
				} else {
					sum += Math::ColorF(Color(source_corrected->GetPixel(x << 1, (y << 1) + 1)));
					c++;
				}
				auto pixel = Color(sum / double(c));
				mip->SetPixel(x, y, pixel);
			}
			if (mip->GetPixelFormat() != source->GetPixelFormat()) mip = mip->ConvertFormat(source->GetPixelFormat());
			mip->Retain();
			return mip;
		}
	}
}