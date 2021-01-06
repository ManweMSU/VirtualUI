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
		WindowLayerDesc CreateWindowLayerDesc(uint32 width, uint32 height, PixelFormat format = PixelFormat::B8G8R8A8_unorm, uint32 usage = ResourceUsageRenderTarget);
		ResourceInitDesc CreateInitDesc(const void * data, uint32 pitch = 0, uint32 slice_pitch = 0);
		ResourceDataDesc CreateDataDesc(void * data, uint32 pitch = 0, uint32 slice_pitch = 0);
		RenderTargetViewDesc CreateRenderTargetView(ITexture * rt, TextureLoadAction action = TextureLoadAction::DontCare);
		RenderTargetViewDesc CreateRenderTargetView(ITexture * rt, const Math::Vector4f & clear_color);
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, TextureLoadAction action = TextureLoadAction::DontCare);
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, float clear_depth);
		DepthStencilViewDesc CreateDepthStencilView(ITexture * ds, float clear_depth, uint8 clear_stencil);

		Math::Matrix4x4f MakeTranslateTransform(const Math::Vector3f & shift);
		Math::Matrix4x4f MakeScaleTransform(const Math::Vector3f & scale);
		Math::Matrix4x4f MakeRotateTransformX(float angle);
		Math::Matrix4x4f MakeRotateTransformY(float angle);
		Math::Matrix4x4f MakeRotateTransformZ(float angle);
		Math::Matrix4x4f MakeRotateTransform(float angle, const Math::Vector3f & axis);
		Math::Matrix4x4f MakeReflectTransform(const Math::Vector4f & plane);
		Math::Matrix4x4f MakeLookAtTransform(const Math::Vector3f & cam, const Math::Vector3f & dir, const Math::Vector3f & up);
		Math::Matrix4x4f MakeOrthogonalViewTransform(float width, float height, float near_plane, float far_plane);
		Math::Matrix4x4f MakePerspectiveViewTransform(float near_width, float near_height, float near_plane, float far_plane);
		Math::Matrix4x4f MakePerspectiveViewTransformFoV(float fov_angle, float aspect, float near_plane, float far_plane);

		ITexture * LoadTexture(IDevice * device, Codec::Frame * frame, uint32 mip_levels = 0, uint32 usage = ResourceUsageShaderRead,
			PixelFormat format = PixelFormat::Invalid, ResourceMemoryPool pool = ResourceMemoryPool::Immutable);
		ITexture * LoadTexture(IDevice * device, Streaming::Stream * stream, uint32 mip_levels = 0, uint32 usage = ResourceUsageShaderRead,
			PixelFormat format = PixelFormat::Invalid, ResourceMemoryPool pool = ResourceMemoryPool::Immutable);
		ITexture * LoadCubeTexture(IDevice * device, Codec::Image * image, uint32 mip_levels = 0, uint32 usage = ResourceUsageShaderRead,
			PixelFormat format = PixelFormat::Invalid, ResourceMemoryPool pool = ResourceMemoryPool::Immutable);
		Codec::Frame * CreateMipLevel(Codec::Frame * source);
	}
}