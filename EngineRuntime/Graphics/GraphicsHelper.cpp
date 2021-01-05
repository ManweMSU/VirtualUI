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
	}
}