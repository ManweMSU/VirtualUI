#pragma once

#include "../Streaming.h"

namespace Engine
{
	namespace Graphics
	{
		class IDevice;
		class IDeviceChild;
		class IShader;
		class IShaderLibrary;
		class IPipelineState;
		class ISamplerState;
		class IDeviceResource;
		class IBuffer;
		class ITexture;
		class ICommandQueue;

		enum class PixelFormat {
			Invalid,
			// Color formats
			// 8 bpp
			A8_unorm,
			R8_unorm, R8_snorm, R8_uint, R8_sint,

			// 16 bpp
			R16_unorm, R16_snorm, R16_uint, R16_sint, R16_float,
			R8G8_unorm, R8G8_snorm, R8G8_uint, R8G8_sint,
			B5G6R5_unorm,
			B5G5R5A1_unorm,
			B4G4R4A4_unorm,

			// 32 bpp
			R32_uint, R32_sint, R32_float,
			R16G16_unorm, R16G16_snorm, R16G16_uint, R16G16_sint, R16G16_float,
			B8G8R8A8_unorm,
			R8G8B8A8_unorm, R8G8B8A8_snorm, R8G8B8A8_uint, R8G8B8A8_sint,
			R10G10B10A2_unorm, R10G10B10A2_uint,
			R11G11B10_float,
			R9G9B9E5_float,

			// 64 bpp
			R32G32_uint, R32G32_sint, R32G32_float,
			R16G16B16A16_unorm, R16G16B16A16_snorm, R16G16B16A16_uint, R16G16B16A16_sint, R16G16B16A16_float,

			// 128 bpp
			R32G32B32A32_uint, R32G32B32A32_sint, R32G32B32A32_float,

			// Depth/Stencil formats
			D16_unorm, D32_float, D24S8_unorm, D32S8_float
		};
		enum class SamplerFilter { Point, Linear, Anisotropic };
		enum class SamplerAddressMode { Wrap, Mirror, Clamp, Border };
		enum class ShaderType { Vertex, Pixel };
		enum class BlendingFactor {
			Zero, One,
			OverColor, InvertedOverColor, OverAlpha, InvertedOverAlpha,
			BaseColor, InvertedBaseColor, BaseAlpha, InvertedBaseAlpha,
			SecondaryColor, InvertedSecondaryColor, SecondaryAlpha, InvertedSecondaryAlpha,
			OverAlphaSaturated
		};
		enum class BlendingFunction { Add, SubtractOverFromBase, SubtractBaseFromOver, Min, Max };
		enum class CompareFunction { Always, Lesser, Greater, Equal, LesserEqual, GreaterEqual, NotEqual, Never };
		enum class StencilFunction { Keep, SetZero, Replace, IncrementWrap, DecrementWrap, IncrementClamp, DecrementClamp, Invert };
		enum class FillMode { Solid, Wireframe };
		enum class CullMode { None, Front, Back };
		enum class ResourceType { Buffer, Texture };
		enum class ResourceMemoryPool { Default, Immutable };
		enum class TextureType { Type1D, TypeArray1D, Type2D, TypeArray2D, TypeCube, TypeArrayCube, Type3D };
		
		enum RenderTargetFlags {
			RenderTargetFlagBlendingEnabled = 0x00000001,
			RenderTargetFlagRestrictWriteRed = 0x00000002,
			RenderTargetFlagRestrictWriteGreen = 0x00000004,
			RenderTargetFlagRestrictWriteBlue = 0x00000008,
			RenderTargetFlagRestrictWriteAlpha = 0x00000010
		};
		enum DepthStencilFlags {
			DepthStencilFlagDepthTestEnabled = 0x00000001,
			DepthStencilFlagStencilTestEnabled = 0x00000004,
			DepthStencilFlagDepthWriteEnabled = 0x00000002
		};
		enum BufferResourceUsage {
			BufferResourceUsageShaderResource = 0x00000001,
			BufferResourceUsageIndexBuffer = 0x00000002
		};
		enum TextureResourceUsage {
			TextureResourceUsageShaderRead = 0x00000001,
			TextureResourceUsageShaderWrite = 0x00000002,
			TextureResourceUsageRenderTarget = 0x00000004,
			TextureResourceUsageDepthStencil = 0x00000008
		};

		struct SamplerDesc
		{
			SamplerFilter MinificationFilter;
			SamplerFilter MagnificationFilter;
			SamplerFilter MipFilter;
			SamplerAddressMode AddressU;
			SamplerAddressMode AddressV;
			SamplerAddressMode AddressW;
			uint32 MaximalAnisotropy;
			float MinimalLOD;
			float MaximalLOD;
			float BorderColor[4];
		};
		struct RenderTargetDesc
		{
			PixelFormat Format;
			RenderTargetFlags Flags;
			BlendingFunction BlendRGB;
			BlendingFunction BlendAlpha;
			BlendingFactor BaseFactorRGB;
			BlendingFactor BaseFactorAlpha;
			BlendingFactor OverFactorRGB;
			BlendingFactor OverFactorAlpha;
		};
		struct StencilDesc
		{
			CompareFunction TestFunction;
			StencilFunction OnStencilTestFailed;
			StencilFunction OnDepthTestFailed;
			StencilFunction OnTestsPassed;
		};
		struct DepthStencilDesc
		{
			PixelFormat Format;
			DepthStencilFlags Flags;
			CompareFunction DepthTestFunction;
			uint8 StencilWriteMask;
			uint8 StencilReadMask;
			StencilDesc FrontStencil;
			StencilDesc BackStencil;
		};
		struct RasterizationDesc
		{
			FillMode Fill;
			CullMode Cull;
			bool FrontIsCounterClockwise;
			int DepthBias;
			float DepthBiasClamp;
			float SlopeScaledDepthBias;
			bool DepthClipEnable;
		};
		struct PipelineStateDesc
		{
			IShader * VertexShader;
			IShader * PixelShader;
			uint32 RenderTargetCount;
			RenderTargetDesc RenderTarget[8];
			DepthStencilDesc DepthStencil;
			RasterizationDesc Rasterization;
		};
		struct BufferDesc
		{
			uint32 Length;
			uint32 Stride;
			BufferResourceUsage Usage;
			ResourceMemoryPool MemoryPool;
		};
		struct TextureDesc
		{
			TextureType Type;
			PixelFormat Format;
			uint32 Width;
			uint32 Height;
			uint32 DepthOrArraySize;
			uint32 MipmapCount;
			TextureResourceUsage Usage;
			ResourceMemoryPool MemoryPool;
		};
		struct ResourceInitDesc
		{
			const void * Data;
			intptr DataPitch;
			intptr DataSlicePitch;
		};

		class IDevice : public Object
		{
		public:
			virtual string GetDeviceName(void) noexcept = 0;
			virtual uint64 GetDeviceIdentifier(void) noexcept = 0;
			virtual void GetImplementationInfo(string & tech, uint32 & version) noexcept = 0;
			virtual IShaderLibrary * LoadShaderLibrary(const void * data, int length) noexcept = 0;
			virtual IShaderLibrary * LoadShaderLibrary(const DataBlock * data) noexcept = 0;
			virtual IShaderLibrary * LoadShaderLibrary(Streaming::Stream * stream) noexcept = 0;
			virtual ICommandQueue * CreateCommandQueue(void) noexcept = 0;
			virtual IPipelineState * CreateRenderingPipelineState(const PipelineStateDesc & desc) noexcept = 0;
			virtual ISamplerState * CreateSamplerState(const SamplerDesc & desc) noexcept = 0;
			virtual IBuffer * CreateBuffer(const BufferDesc & desc) noexcept = 0;
			virtual IBuffer * CreateBuffer(const BufferDesc & desc, const ResourceInitDesc & init) noexcept = 0;
			virtual ITexture * CreateTexture(const TextureDesc & desc) noexcept = 0;
			virtual ITexture * CreateTexture(const TextureDesc & desc, const ResourceInitDesc & init) noexcept = 0;
		};
		class IDeviceChild : public Object
		{
		public:
			virtual IDevice * GetParentDevice(void) noexcept = 0;
		};
		class IShader : public IDeviceChild
		{
		public:
			virtual string GetName(void) noexcept = 0;
			virtual ShaderType GetType(void) noexcept = 0;
		};
		class IShaderLibrary : public IDeviceChild
		{
		public:
			virtual Array<string> * GetShaderNames(void) noexcept = 0;
			virtual IShader * CreateShader(const string & name) noexcept = 0;
		};
		class IPipelineState : public IDeviceChild {};
		class ISamplerState : public IDeviceChild {};
		class IDeviceResource : public IDeviceChild
		{
		public:
			virtual ResourceType GetResourceType(void) noexcept = 0;
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept = 0;
		};
		class IBuffer : public IDeviceResource
		{
		public:
			virtual uint32 GetLength(void) noexcept = 0;
			virtual BufferResourceUsage GetBufferUsage(void) noexcept = 0;
		};
		class ITexture : public IDeviceResource
		{
		public:
			virtual TextureType GetTextureType(void) noexcept = 0;
			virtual PixelFormat GetPixelFormat(void) noexcept = 0;
			virtual uint32 GetWidth(void) noexcept = 0;
			virtual uint32 GetHeight(void) noexcept = 0;
			virtual uint32 GetDepth(void) noexcept = 0;
			virtual uint32 GetMipmapCount(void) noexcept = 0;
			virtual uint32 GetArraySize(void) noexcept = 0;
			virtual TextureResourceUsage GetTextureUsage(void) noexcept = 0;
		};
		class ICommandQueue : public IDeviceChild
		{
		public:
			// TODO: Add parallel command encoding and fences
			// TODO: FILL
		};

		// TODO: CREATE DEFAULT DEVICE
		// TODO: ENUMERATE DEVICES
		// TODO: CREATE SELECTED DEVICE
	}
}