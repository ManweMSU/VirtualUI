#include "GraphicsHelper.h"

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
			result.Height = result.DepthOrArraySize = 0;
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
			result.DepthOrArraySize = 0;
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
			result.DepthOrArraySize = depth;
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
			auto c = Math::cos(angle);
			auto s = Math::sin(angle);
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(1.0f, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, c, -s, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, s, c, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransformY(float angle)
		{
			auto c = Math::cos(angle);
			auto s = Math::sin(angle);
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(c, 0.0f, s, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, 1.0f, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(-s, 0.0f, c, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransformZ(float angle)
		{
			auto c = Math::cos(angle);
			auto s = Math::sin(angle);
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(c, -s, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(s, c, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, 1.0f, 0.0f);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, 0.0f, 1.0f);
			return result;
		}
		Math::Matrix4x4f MakeRotateTransform(float angle, const Math::Vector3f & axis)
		{
			auto c = Math::cos(angle);
			auto s = Math::sin(angle);
			auto z = Math::normalize(axis);
			Math::Vector3f orth;
			if (z.x > 0.5f) orth = Math::Vector3f(0.0f, 1.0f, 0.0f);
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
			auto scale_y = Math::ctg(fov_angle / 2.0f);
			auto scale_x = scale_y / aspect;
			Math::Matrix4x4f result;
			result.row[0] = Math::Vector4f(scale_x, 0.0f, 0.0f, 0.0f);
			result.row[1] = Math::Vector4f(0.0f, scale_y, 0.0f, 0.0f);
			result.row[2] = Math::Vector4f(0.0f, 0.0f, inv_depth * far_plane, inv_depth * far_plane * near_plane);
			result.row[3] = Math::Vector4f(0.0f, 0.0f, -1.0f, 0.0f);
			return result;
		}

		ITexture * LoadTexture(IDevice * device, Codec::Frame * frame, uint32 mip_levels, uint32 usage, PixelFormat format, ResourceMemoryPool pool)
		{
			// TODO: IMPLEMENT
			return nullptr;
		}
		ITexture * LoadTexture(IDevice * device, Streaming::Stream * stream, uint32 mip_levels, uint32 usage, PixelFormat format, ResourceMemoryPool pool)
		{
			// TODO: IMPLEMENT
			return nullptr;
		}
		ITexture * LoadCubeTexture(IDevice * device, Codec::Image * image, uint32 mip_levels, uint32 usage, PixelFormat format, ResourceMemoryPool pool)
		{
			// TODO: IMPLEMENT
			return nullptr;
		}
		Codec::Frame * CreateMipLevel(Codec::Frame * source)
		{
			// TODO: IMPLEMENT
			return nullptr;
		}
	}
}