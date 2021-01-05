#pragma once

#include "Graphics.h"

#include "../Math/Matrix.h"

namespace Engine
{
	namespace Graphics
	{
		SamplerDesc DefaultSamplerDesc(void);
		PipelineStateDesc DefaultPipelineStateDesc(IShader * vs, IShader * ps, PixelFormat rt, bool use_depth = false, PixelFormat ds = PixelFormat::Invalid);
		BufferDesc CreateBufferDesc(uint32 length, uint32 usage, uint32 stride = 0, ResourceMemoryPool pool = ResourceMemoryPool::Default);
		TextureDesc CreateTextureDesc1D(PixelFormat format, uint32 width, uint32 mips, uint32 usage, ResourceMemoryPool pool = ResourceMemoryPool::Default);
		TextureDesc CreateTextureDesc2D(PixelFormat format, uint32 width, uint32 height, uint32 mips, uint32 usage, ResourceMemoryPool pool = ResourceMemoryPool::Default);
		TextureDesc CreateTextureDesc3D(PixelFormat format, uint32 width, uint32 height, uint32 depth, uint32 mips, uint32 usage, ResourceMemoryPool pool = ResourceMemoryPool::Default);
		ResourceInitDesc CreateInitDesc(const void * data, uint32 pitch = 0, uint32 slice_pitch = 0);
		ResourceDataDesc CreateDataDesc(void * data, uint32 pitch = 0, uint32 slice_pitch = 0);
		RenderTargetViewDesc CreateRenderTargetView(ITexture * rt, TextureLoadAction action = TextureLoadAction::DontCare);
		RenderTargetViewDesc CreateRenderTargetView(ITexture * rt, const Math::Vector4f & clear_color);
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, TextureLoadAction action = TextureLoadAction::DontCare);
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, float clear_depth);
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, float clear_depth, uint8 clear_stencil);
	}
}