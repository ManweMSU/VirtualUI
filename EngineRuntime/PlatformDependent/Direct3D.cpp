#include "Direct3D.h"

#include <VersionHelpers.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#undef ZeroMemory

using namespace Engine::Graphics;

namespace Engine
{
	namespace Direct3D
	{
		DeviceDriverClass D3DDeviceClass = DeviceDriverClass::None;
		SafePointer<ID3D11Device> D3DDevice;
		SafePointer<ID2D1Device> D2DDevice;
		SafePointer<IDXGIDevice1> DXGIDevice;
		SafePointer<ID3D11DeviceContext> D3DDeviceContext;
		SafePointer<IDXGIFactory> DXGIFactory;
		SafePointer<Graphics::IDevice> WrappedDevice;

		void CreateDevices(void)
		{
			if (!DXGIFactory) {
				CreateDXGIFactory(IID_PPV_ARGS(DXGIFactory.InnerRef()));
			}
			if (!D3DDevice) {
				D3DDeviceContext.SetReference(0);
				auto device = CreateDeviceD3D11(0, D3D_DRIVER_TYPE_HARDWARE);
				if (!device) {
					device = CreateDeviceD3D11(0, D3D_DRIVER_TYPE_WARP);
					if (!device) return; else D3DDeviceClass = DeviceDriverClass::Warp;
				} else D3DDeviceClass = DeviceDriverClass::Hardware;
				D3DDevice.SetReference(device);
				D3DDevice->GetImmediateContext(D3DDeviceContext.InnerRef());
				WrappedDevice = CreateWrappedDeviceD3D11(D3DDevice);
			}
			if (!DXGIDevice) {
				if (D3DDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**) DXGIDevice.InnerRef()) != S_OK) return;
			}
			if (!D2DDevice && Direct2D::D2DFactory1) {
				if (Direct2D::D2DFactory1->CreateDevice(DXGIDevice, D2DDevice.InnerRef()) != S_OK) return;
			}
		}
		void ReleaseDevices(void)
		{
			D3DDevice.SetReference(0);
			D2DDevice.SetReference(0);
			DXGIDevice.SetReference(0);
			D3DDeviceContext.SetReference(0);
			WrappedDevice.SetReference(0);
			D3DDeviceClass = DeviceDriverClass::None;
		}
		void RestartDevicesIfNecessary(void)
		{
			if (!D3DDevice) {
				ReleaseDevices();
				CreateDevices();
			} else {
				auto reason = D3DDevice->GetDeviceRemovedReason();
				if (reason != S_OK) {
					ReleaseDevices();
					CreateDevices();
				}
			}
		}
		DeviceDriverClass GetDeviceDriverClass(void) { return D3DDeviceClass; }
		bool CreateD2DDeviceContextForWindow(HWND Window, ID2D1DeviceContext ** Context, IDXGISwapChain1 ** SwapChain)
		{
			if (!D3DDevice || !D2DDevice || !DXGIDevice || !D3DDeviceContext) return false;
			SafePointer<ID2D1DeviceContext> Result;
			SafePointer<IDXGISwapChain1> SwapChainResult;
			if (D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, Result.InnerRef()) != S_OK) return false;

			DXGI_SWAP_CHAIN_DESC1 SwapChainDescription;
			ZeroMemory(&SwapChainDescription, sizeof(SwapChainDescription));
			SwapChainDescription.Width = 0;
			SwapChainDescription.Height = 0;
			SwapChainDescription.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			SwapChainDescription.Stereo = false;
			SwapChainDescription.SampleDesc.Count = 1;
			SwapChainDescription.SampleDesc.Quality = 0;
			SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			SwapChainDescription.BufferCount = 2;
			SwapChainDescription.Scaling = DXGI_SCALING_NONE;
			SwapChainDescription.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
			SwapChainDescription.Flags = 0;
			SwapChainDescription.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
			SwapChainDescription.Scaling = DXGI_SCALING_STRETCH;

			SafePointer<IDXGIAdapter> Adapter;
			DXGIDevice->GetAdapter(Adapter.InnerRef());
			SafePointer<IDXGIFactory2> Factory;
			Adapter->GetParent(IID_PPV_ARGS(Factory.InnerRef()));
			if (Factory->CreateSwapChainForHwnd(D3DDevice, Window, &SwapChainDescription, 0, 0, SwapChainResult.InnerRef()) != S_OK) return false;
			DXGIDevice->SetMaximumFrameLatency(1);

			D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f);
			SafePointer<IDXGISurface> Surface;
			if (SwapChainResult->GetBuffer(0, IID_PPV_ARGS(Surface.InnerRef())) != S_OK) return false;
			SafePointer<ID2D1Bitmap1> Bitmap;
			if (Result->CreateBitmapFromDxgiSurface(Surface, props, Bitmap.InnerRef()) != S_OK) return false;
			Result->SetTarget(Bitmap);

			*Context = Result;
			*SwapChain = SwapChainResult;
			Result->AddRef();
			SwapChainResult->AddRef();
			return true;
		}
		bool CreateSwapChainForWindow(HWND Window, IDXGISwapChain ** SwapChain)
		{
			if (!D3DDevice || !DXGIFactory) return false;
			DXGI_SWAP_CHAIN_DESC SwapChainDescription;
			ZeroMemory(&SwapChainDescription, sizeof(SwapChainDescription));
			SwapChainDescription.BufferDesc.Width = 0;
			SwapChainDescription.BufferDesc.Height = 0;
			SwapChainDescription.BufferDesc.RefreshRate.Numerator = 60;
			SwapChainDescription.BufferDesc.RefreshRate.Denominator = 1;
			SwapChainDescription.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			SwapChainDescription.BufferDesc.Scaling = DXGI_MODE_SCALING_STRETCHED;
			SwapChainDescription.SampleDesc.Count = 1;
			SwapChainDescription.SampleDesc.Quality = 0;
			SwapChainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			SwapChainDescription.BufferCount = 1;
			SwapChainDescription.OutputWindow = Window;
			SwapChainDescription.Windowed = TRUE;
			if (DXGIFactory->CreateSwapChain(D3DDevice, &SwapChainDescription, SwapChain) != S_OK) return false;
			return true;
		}
		bool CreateSwapChainDevice(IDXGISwapChain * SwapChain, ID2D1RenderTarget ** Target)
		{
			if (!Direct2D::D2DFactory) return false;
			IDXGISurface * Surface;
			if (SwapChain->GetBuffer(0, IID_PPV_ARGS(&Surface)) != S_OK) return false;
			D2D1_RENDER_TARGET_PROPERTIES RenderTargetProps;
			RenderTargetProps.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
			RenderTargetProps.pixelFormat.format = DXGI_FORMAT_UNKNOWN;
			RenderTargetProps.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
			RenderTargetProps.dpiX = RenderTargetProps.dpiY = 0.0f;
			RenderTargetProps.usage = D2D1_RENDER_TARGET_USAGE_NONE;
			RenderTargetProps.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
			if (Direct2D::D2DFactory->CreateDxgiSurfaceRenderTarget(Surface, &RenderTargetProps, Target) != S_OK) {
				Surface->Release();
				return false;
			}
			Surface->Release();
			return true;
		}
		bool ResizeRenderBufferForD2DDevice(ID2D1DeviceContext * Context, IDXGISwapChain1 * SwapChain)
		{
			if (Context && SwapChain) {
				Context->SetTarget(0);
				if (SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0) != S_OK) return false;
				D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), 0.0f, 0.0f);
				SafePointer<IDXGISurface> Surface;
				if (SwapChain->GetBuffer(0, IID_PPV_ARGS(Surface.InnerRef())) != S_OK) return false;
				SafePointer<ID2D1Bitmap1> Bitmap;
				if (Context->CreateBitmapFromDxgiSurface(Surface, props, Bitmap.InnerRef()) != S_OK) return false;
				Context->SetTarget(Bitmap);
				return true;
			} else return false;
		}
		bool ResizeRenderBufferForSwapChainDevice(IDXGISwapChain * SwapChain)
		{
			if (SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0) != S_OK) return false;
			return true;
		}

		class D3D11_Buffer : public Graphics::IBuffer
		{
			IDevice * wrapper;
		public:
			ID3D11Buffer * buffer;
			ID3D11Buffer * buffer_staging;
			ID3D11ShaderResourceView * view;
			ResourceMemoryPool pool;
			uint32 usage_flags, length, stride;

			D3D11_Buffer(IDevice * _wrapper) : wrapper(_wrapper), buffer(0), buffer_staging(0), view(0),
				pool(ResourceMemoryPool::Default), usage_flags(0), length(0), stride(0) {}
			virtual ~D3D11_Buffer(void) override
			{
				if (buffer) buffer->Release();
				if (buffer_staging) buffer_staging->Release();
				if (view) view->Release();
			}
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual ResourceType GetResourceType(void) noexcept override { return ResourceType::Buffer; }
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept override { return pool; }
			virtual uint32 GetResourceUsage(void) noexcept override { return usage_flags; }
			virtual uint32 GetLength(void) noexcept override { return length; }
			virtual string ToString(void) const override { return L"D3D11_Buffer"; }
		};
		class D3D11_Texture : public Graphics::ITexture
		{
			TextureType type;
			IDevice * wrapper;
		public:
			ID3D11Texture1D * tex_1d;
			ID3D11Texture2D * tex_2d;
			ID3D11Texture3D * tex_3d;
			ID3D11Texture1D * tex_staging_1d;
			ID3D11Texture2D * tex_staging_2d;
			ID3D11Texture3D * tex_staging_3d;
			ID3D11ShaderResourceView * view;
			ResourceMemoryPool pool;
			PixelFormat format;
			uint32 usage_flags;
			uint32 width, height, depth, size;

			D3D11_Texture(TextureType _type, IDevice * _wrapper) : type(_type), wrapper(_wrapper), tex_1d(0), tex_2d(0), tex_3d(0),
				tex_staging_1d(0), tex_staging_2d(0), tex_staging_3d(0), view(0), pool(ResourceMemoryPool::Default),
				format(PixelFormat::Invalid), usage_flags(0), width(0), height(0), depth(0), size(0) {}
			virtual ~D3D11_Texture(void) override
			{
				if (tex_1d) tex_1d->Release();
				if (tex_2d) tex_2d->Release();
				if (tex_3d) tex_3d->Release();
				if (tex_staging_1d) tex_staging_1d->Release();
				if (tex_staging_2d) tex_staging_2d->Release();
				if (tex_staging_3d) tex_staging_3d->Release();
				if (view) view->Release();
			}
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual ResourceType GetResourceType(void) noexcept override { return ResourceType::Texture; }
			virtual ResourceMemoryPool GetMemoryPool(void) noexcept override { return pool; }
			virtual uint32 GetResourceUsage(void) noexcept override { return usage_flags; }
			virtual TextureType GetTextureType(void) noexcept override { return type; }
			virtual PixelFormat GetPixelFormat(void) noexcept override { return format; }
			virtual uint32 GetWidth(void) noexcept override { return width; }
			virtual uint32 GetHeight(void) noexcept override { return height; }
			virtual uint32 GetDepth(void) noexcept override { return depth; }
			virtual uint32 GetMipmapCount(void) noexcept override
			{
				if (tex_1d) {
					D3D11_TEXTURE1D_DESC desc;
					tex_1d->GetDesc(&desc);
					return desc.MipLevels;
				} else if (tex_2d) {
					D3D11_TEXTURE2D_DESC desc;
					tex_2d->GetDesc(&desc);
					return desc.MipLevels;
				} else if (tex_3d) {
					D3D11_TEXTURE3D_DESC desc;
					tex_3d->GetDesc(&desc);
					return desc.MipLevels;
				} else return 0;
			}
			virtual uint32 GetArraySize(void) noexcept override { return size; }
			virtual string ToString(void) const override { return L"D3D11_Texture"; }
		};
		class D3D11_DeviceContext : public Graphics::IDeviceContext
		{
			IDevice * wrapper;
			ID3D11Device * device;
			ID3D11DeviceContext * context;
		public:
			D3D11_DeviceContext(ID3D11Device * _device, IDevice * _wrapper)
			{
				context = 0;
				device = _device;
				wrapper = _wrapper;
				device->AddRef();
				device->GetImmediateContext(&context);
				// TODO: IMPLEMENT
			}
			virtual ~D3D11_DeviceContext(void) override
			{
				device->Release();
				context->Release();
				// TODO: IMPLEMENT
			}
			virtual IDevice * GetParentDevice(void) noexcept override { return wrapper; }
			virtual bool BeginRenderingPass(uint32 rtc, const RenderTargetViewDesc * rtv, const DepthStencilViewDesc * dsv) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool Begin2DRenderingPass(const RenderTargetViewDesc & rtv) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool BeginMemoryManagementPass(void) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool EndCurrentPass(void) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual void Wait(void) noexcept override
			{
				// TODO: IMPLEMENT
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
			virtual void SetVertexShaderResourceData(uint32 at, const void * data, int length) noexcept override
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
			virtual void SetPixelShaderResourceData(uint32 at, const void * data, int length) noexcept override
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
			virtual void DrawIndexedPrimitives(uint32 index_count, uint32 first_index, uint32 base_vertex) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual UI::IRenderingDevice * Get2DRenderingDevice(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual void GenerateMipmaps(ITexture * texture) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void CopyResourceData(IDeviceResource * dest, IDeviceResource * src) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void CopySubresourceData(IDeviceResource * dest, SubresourceIndex dest_subres, VolumeIndex dest_origin, IDeviceResource * src, SubresourceIndex src_subres, VolumeIndex src_origin, VolumeIndex size) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void UpdateResourceData(IDeviceResource * dest, SubresourceIndex subres, VolumeIndex origin, VolumeIndex size, const ResourceInitDesc & src) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void QueryResourceData(ResourceDataDesc & dest, IDeviceResource * src, SubresourceIndex subres, VolumeIndex origin, VolumeIndex size) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual string ToString(void) const override { return L"D3D11_DeviceContext"; }
		};
		class D3D11_Device : public Graphics::IDevice
		{
			ID3D11Device * device;
			IDeviceContext * context;
			D3D11_SUBRESOURCE_DATA * _make_subres_data(const ResourceInitDesc * init, int length)
			{
				D3D11_SUBRESOURCE_DATA * result = reinterpret_cast<D3D11_SUBRESOURCE_DATA *>(malloc(length * sizeof(D3D11_SUBRESOURCE_DATA)));
				if (!result) return 0;
				for (int i = 0; i < length; i++) {
					result[i].pSysMem = init[i].Data;
					result[i].SysMemPitch = init[i].DataPitch;
					result[i].SysMemSlicePitch = init[i].DataSlicePitch;
				}
				return result;
			}
			uint32 _calculate_mip_levels(uint32 w, uint32 h, uint32 d)
			{
				uint32 r = 1;
				auto s = max(max(w, h), d);
				while (s > 1) { s /= 2; r++; }
				return r;
			}
		public:
			D3D11_Device(ID3D11Device * _device) { context = new D3D11_DeviceContext(_device, this); device = _device; device->AddRef(); }
			virtual ~D3D11_Device(void) override { device->Release(); context->Release(); }
			virtual string GetDeviceName(void) noexcept override
			{
				IDXGIDevice * dxgi_device;
				IDXGIAdapter * adapter;
				if (device->QueryInterface(IID_PPV_ARGS(&dxgi_device)) != S_OK) return L"";
				if (dxgi_device->GetAdapter(&adapter) != S_OK) { dxgi_device->Release(); return L""; }
				dxgi_device->Release();
				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);
				adapter->Release();
				return string(desc.Description);
			}
			virtual uint64 GetDeviceIdentifier(void) noexcept override
			{
				IDXGIDevice * dxgi_device;
				IDXGIAdapter * adapter;
				if (device->QueryInterface(IID_PPV_ARGS(&dxgi_device)) != S_OK) return 0;
				if (dxgi_device->GetAdapter(&adapter) != S_OK) { dxgi_device->Release(); return 0; }
				dxgi_device->Release();
				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);
				adapter->Release();
				return reinterpret_cast<uint64 &>(desc.AdapterLuid);
			}
			virtual bool DeviceIsValid(void) noexcept override { if (device->GetDeviceRemovedReason() == S_OK) return true; return false; }
			virtual void GetImplementationInfo(string & tech, uint32 & version) noexcept override { tech = L"Direct3D"; version = 11; }
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
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) return 0;
				SafePointer<D3D11_Buffer> result = new (std::nothrow) D3D11_Buffer(this);
				if (!result) return 0;
				result->pool = desc.MemoryPool;
				D3D11_BUFFER_DESC bd;
				bd.ByteWidth = desc.Length;
				bd.Usage = D3D11_USAGE_DEFAULT;
				bd.BindFlags = 0;
				bd.CPUAccessFlags = 0;
				bd.MiscFlags = 0;
				bd.StructureByteStride = desc.Stride;
				if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
					bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
					result->usage_flags |= ResourceUsageShaderRead;
					result->usage_flags |= ResourceUsageShaderWrite;
				}
				if (desc.Usage & ResourceUsageIndexBuffer) {
					bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
					result->usage_flags |= ResourceUsageIndexBuffer;
				}
				if (desc.Stride) bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				if (device->CreateBuffer(&bd, 0, &result->buffer) != S_OK) return 0;
				bool create_staging = false;
				if (desc.Usage & ResourceUsageCPURead) {
					create_staging = true;
					result->usage_flags |= ResourceUsageCPURead;
				}
				if (desc.Usage & ResourceUsageCPUWrite) result->usage_flags |= ResourceUsageCPUWrite;
				if (bd.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
					if (device->CreateShaderResourceView(result->buffer, 0, &result->view) != S_OK) return 0;
				}
				result->length = desc.Length;
				result->stride = desc.Stride;
				if (create_staging) {
					bd.Usage = D3D11_USAGE_STAGING;
					bd.BindFlags = 0;
					bd.MiscFlags = 0;
					if (desc.Usage & ResourceUsageCPURead) bd.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
					if (device->CreateBuffer(&bd, 0, &result->buffer_staging) != S_OK) return 0;
				}
				result->Retain();
				return result;
			}
			virtual IBuffer * CreateBuffer(const BufferDesc & desc, const ResourceInitDesc & init) noexcept override
			{
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) return 0;
				SafePointer<D3D11_Buffer> result = new (std::nothrow) D3D11_Buffer(this);
				if (!result) return 0;
				result->pool = desc.MemoryPool;
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) {
					if (desc.Usage & ResourceUsageShaderWrite) return 0;
					if (desc.Usage & ResourceUsageCPURead) return 0;
					if (desc.Usage & ResourceUsageCPUWrite) return 0;
				}
				D3D11_BUFFER_DESC bd;
				bd.ByteWidth = desc.Length;
				if (desc.MemoryPool == ResourceMemoryPool::Default) bd.Usage = D3D11_USAGE_DEFAULT;
				else if (desc.MemoryPool == ResourceMemoryPool::Immutable) bd.Usage = D3D11_USAGE_IMMUTABLE;
				bd.BindFlags = 0;
				bd.CPUAccessFlags = 0;
				bd.MiscFlags = 0;
				bd.StructureByteStride = desc.Stride;
				if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
					bd.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
					result->usage_flags |= ResourceUsageShaderRead;
					if (desc.MemoryPool == ResourceMemoryPool::Default) result->usage_flags |= ResourceUsageShaderWrite;
				}
				if (desc.Usage & ResourceUsageIndexBuffer) {
					bd.BindFlags |= D3D11_BIND_INDEX_BUFFER;
					result->usage_flags |= ResourceUsageIndexBuffer;
				}
				if (desc.Stride) bd.MiscFlags |= D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
				D3D11_SUBRESOURCE_DATA sr;
				sr.pSysMem = init.Data;
				sr.SysMemPitch = sr.SysMemSlicePitch = 0;
				if (device->CreateBuffer(&bd, &sr, &result->buffer) != S_OK) return 0;
				bool create_staging = false;
				if (desc.Usage & ResourceUsageCPURead) {
					create_staging = true;
					result->usage_flags |= ResourceUsageCPURead;
				}
				if (desc.Usage & ResourceUsageCPUWrite) result->usage_flags |= ResourceUsageCPUWrite;
				if (bd.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
					if (device->CreateShaderResourceView(result->buffer, 0, &result->view) != S_OK) return 0;
				}
				result->length = desc.Length;
				result->stride = desc.Stride;
				if (create_staging) {
					bd.Usage = D3D11_USAGE_STAGING;
					bd.BindFlags = 0;
					bd.MiscFlags = 0;
					if (desc.Usage & ResourceUsageCPURead) bd.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
					if (device->CreateBuffer(&bd, 0, &result->buffer_staging) != S_OK) return 0;
				}
				result->Retain();
				return result;
			}
			virtual ITexture * CreateTexture(const TextureDesc & desc) noexcept override
			{
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) return 0;
				SafePointer<D3D11_Texture> result = new (std::nothrow) D3D11_Texture(desc.Type, this);
				if (!result) return 0;
				DXGI_FORMAT dxgi_format = MakeDxgiFormat(desc.Format);
				if (dxgi_format == DXGI_FORMAT_UNKNOWN) return 0;
				result->format = desc.Format;
				result->pool = desc.MemoryPool;
				if ((desc.Usage & ResourceUsageRenderTarget) && !IsColorFormat(desc.Format)) return 0;
				if ((desc.Usage & ResourceUsageDepthStencil) && !IsDepthStencilFormat(desc.Format)) return 0;
				if (desc.Type == TextureType::Type1D || desc.Type == TextureType::TypeArray1D) {
					D3D11_TEXTURE1D_DESC td;
					td.Width = desc.Width;
					td.MipLevels = desc.MipmapCount;
					td.ArraySize = (desc.Type == TextureType::TypeArray1D) ? desc.DepthOrArraySize : 1;
					td.Format = dxgi_format;
					td.Usage = D3D11_USAGE_DEFAULT;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					if (device->CreateTexture1D(&td, 0, &result->tex_1d) != S_OK) return 0;
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_1d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = result->depth = 1;
					result->size = td.ArraySize;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = 0;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture1D(&td, 0, &result->tex_staging_1d) != S_OK) return 0;
					}
				} else if (desc.Type == TextureType::Type2D || desc.Type == TextureType::TypeArray2D) {
					D3D11_TEXTURE2D_DESC td;
					td.Width = desc.Width;
					td.Height = desc.Height;
					td.MipLevels = desc.MipmapCount;
					td.ArraySize = (desc.Type == TextureType::TypeArray2D) ? desc.DepthOrArraySize : 1;
					td.Format = dxgi_format;
					td.SampleDesc.Count = 1;
					td.SampleDesc.Quality = 0;
					td.Usage = D3D11_USAGE_DEFAULT;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					if (device->CreateTexture2D(&td, 0, &result->tex_2d) != S_OK) return 0;
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_2d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = desc.Height;
					result->depth = 1;
					result->size = td.ArraySize;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = 0;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture2D(&td, 0, &result->tex_staging_2d) != S_OK) return 0;
					}
				} else if (desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) {
					if (desc.Width != desc.Height) return 0;
					D3D11_TEXTURE2D_DESC td;
					td.Width = desc.Width;
					td.Height = desc.Height;
					td.MipLevels = desc.MipmapCount;
					td.ArraySize = (desc.Type == TextureType::TypeArrayCube) ? (desc.DepthOrArraySize * 6) : 6;
					td.Format = dxgi_format;
					td.SampleDesc.Count = 1;
					td.SampleDesc.Quality = 0;
					td.Usage = D3D11_USAGE_DEFAULT;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					if (device->CreateTexture2D(&td, 0, &result->tex_2d) != S_OK) return 0;
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_2d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = desc.Height;
					result->depth = 1;
					result->size = td.ArraySize / 6;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture2D(&td, 0, &result->tex_staging_2d) != S_OK) return 0;
					}
				} else if (desc.Type == TextureType::Type3D) {
					D3D11_TEXTURE3D_DESC td;
					td.Width = desc.Width;
					td.Height = desc.Height;
					td.Depth = desc.DepthOrArraySize;
					td.MipLevels = desc.MipmapCount;
					td.Format = dxgi_format;
					td.Usage = D3D11_USAGE_DEFAULT;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					if (device->CreateTexture3D(&td, 0, &result->tex_3d) != S_OK) return 0;
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_3d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = desc.Height;
					result->depth = desc.DepthOrArraySize;
					result->size = 1;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = 0;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture3D(&td, 0, &result->tex_staging_3d) != S_OK) return 0;
					}
				} else return 0;
				result->Retain();
				return result;
			}
			virtual ITexture * CreateTexture(const TextureDesc & desc, const ResourceInitDesc * init) noexcept override
			{
				if (!init) return 0;
				SafePointer<D3D11_Texture> result = new (std::nothrow) D3D11_Texture(desc.Type, this);
				if (!result) return 0;
				DXGI_FORMAT dxgi_format = MakeDxgiFormat(desc.Format);
				if (dxgi_format == DXGI_FORMAT_UNKNOWN) return 0;
				result->format = desc.Format;
				result->pool = desc.MemoryPool;
				if ((desc.Usage & ResourceUsageRenderTarget) && !IsColorFormat(desc.Format)) return 0;
				if ((desc.Usage & ResourceUsageDepthStencil) && !IsDepthStencilFormat(desc.Format)) return 0;
				if (desc.MemoryPool == ResourceMemoryPool::Immutable) {
					if (desc.Usage & ResourceUsageShaderWrite) return 0;
					if (desc.Usage & ResourceUsageRenderTarget) return 0;
					if (desc.Usage & ResourceUsageDepthStencil) return 0;
					if (desc.Usage & ResourceUsageCPURead) return 0;
					if (desc.Usage & ResourceUsageCPUWrite) return 0;
				}
				if (desc.Type == TextureType::Type1D || desc.Type == TextureType::TypeArray1D) {
					D3D11_TEXTURE1D_DESC td;
					td.Width = desc.Width;
					td.MipLevels = desc.MipmapCount;
					if (!td.MipLevels) td.MipLevels = _calculate_mip_levels(desc.Width, 1, 1);
					td.ArraySize = (desc.Type == TextureType::TypeArray1D) ? desc.DepthOrArraySize : 1;
					td.Format = dxgi_format;
					if (desc.MemoryPool == ResourceMemoryPool::Default) td.Usage = D3D11_USAGE_DEFAULT;
					else if (desc.MemoryPool == ResourceMemoryPool::Immutable) td.Usage = D3D11_USAGE_IMMUTABLE;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						if (desc.MemoryPool == ResourceMemoryPool::Default) result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					auto sr = _make_subres_data(init, td.MipLevels * td.ArraySize);
					if (!sr) return 0;
					if (device->CreateTexture1D(&td, sr, &result->tex_1d) != S_OK) { free(sr); return 0; }
					free(sr);
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_1d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = result->depth = 1;
					result->size = td.ArraySize;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = 0;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture1D(&td, 0, &result->tex_staging_1d) != S_OK) return 0;
					}
				} else if (desc.Type == TextureType::Type2D || desc.Type == TextureType::TypeArray2D) {
					D3D11_TEXTURE2D_DESC td;
					td.Width = desc.Width;
					td.Height = desc.Height;
					td.MipLevels = desc.MipmapCount;
					if (!td.MipLevels) td.MipLevels = _calculate_mip_levels(desc.Width, desc.Height, 1);
					td.ArraySize = (desc.Type == TextureType::TypeArray2D) ? desc.DepthOrArraySize : 1;
					td.Format = dxgi_format;
					td.SampleDesc.Count = 1;
					td.SampleDesc.Quality = 0;
					if (desc.MemoryPool == ResourceMemoryPool::Default) td.Usage = D3D11_USAGE_DEFAULT;
					else if (desc.MemoryPool == ResourceMemoryPool::Immutable) td.Usage = D3D11_USAGE_IMMUTABLE;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						if (desc.MemoryPool == ResourceMemoryPool::Default) result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					auto sr = _make_subres_data(init, td.MipLevels * td.ArraySize);
					if (!sr) return 0;
					if (device->CreateTexture2D(&td, sr, &result->tex_2d) != S_OK) { free(sr); return 0; }
					free(sr);
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_2d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = desc.Height;
					result->depth = 1;
					result->size = td.ArraySize;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = 0;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture2D(&td, 0, &result->tex_staging_2d) != S_OK) return 0;
					}
				} else if (desc.Type == TextureType::TypeCube || desc.Type == TextureType::TypeArrayCube) {
					if (desc.Width != desc.Height) return 0;
					D3D11_TEXTURE2D_DESC td;
					td.Width = desc.Width;
					td.Height = desc.Height;
					td.MipLevels = desc.MipmapCount;
					if (!td.MipLevels) td.MipLevels = _calculate_mip_levels(desc.Width, desc.Height, 1);
					td.ArraySize = (desc.Type == TextureType::TypeArrayCube) ? (desc.DepthOrArraySize * 6) : 6;
					td.Format = dxgi_format;
					td.SampleDesc.Count = 1;
					td.SampleDesc.Quality = 0;
					if (desc.MemoryPool == ResourceMemoryPool::Default) td.Usage = D3D11_USAGE_DEFAULT;
					else if (desc.MemoryPool == ResourceMemoryPool::Immutable) td.Usage = D3D11_USAGE_IMMUTABLE;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						if (desc.MemoryPool == ResourceMemoryPool::Default) result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					auto sr = _make_subres_data(init, td.MipLevels * td.ArraySize);
					if (!sr) return 0;
					if (device->CreateTexture2D(&td, sr, &result->tex_2d) != S_OK) { free(sr); return 0; }
					free(sr);
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_2d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = desc.Height;
					result->depth = 1;
					result->size = td.ArraySize / 6;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture2D(&td, 0, &result->tex_staging_2d) != S_OK) return 0;
					}
				} else if (desc.Type == TextureType::Type3D) {
					D3D11_TEXTURE3D_DESC td;
					td.Width = desc.Width;
					td.Height = desc.Height;
					td.Depth = desc.DepthOrArraySize;
					td.MipLevels = desc.MipmapCount;
					if (!td.MipLevels) td.MipLevels = _calculate_mip_levels(desc.Width, desc.Height, desc.DepthOrArraySize);
					td.Format = dxgi_format;
					if (desc.MemoryPool == ResourceMemoryPool::Default) td.Usage = D3D11_USAGE_DEFAULT;
					else if (desc.MemoryPool == ResourceMemoryPool::Immutable) td.Usage = D3D11_USAGE_IMMUTABLE;
					td.BindFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) || (desc.Usage & ResourceUsageShaderWrite)) {
						td.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
						result->usage_flags |= ResourceUsageShaderRead;
						if (desc.MemoryPool == ResourceMemoryPool::Default) result->usage_flags |= ResourceUsageShaderWrite;
					}
					if (desc.Usage & ResourceUsageRenderTarget) {
						td.BindFlags |= D3D11_BIND_RENDER_TARGET;
						result->usage_flags |= ResourceUsageRenderTarget;
					}
					if (desc.Usage & ResourceUsageDepthStencil) {
						td.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
						result->usage_flags |= ResourceUsageDepthStencil;
					}
					td.CPUAccessFlags = 0;
					td.MiscFlags = 0;
					if ((desc.Usage & ResourceUsageShaderRead) && (desc.Usage & ResourceUsageRenderTarget)) {
						td.MiscFlags |= D3D11_RESOURCE_MISC_GENERATE_MIPS;
					}
					auto sr = _make_subres_data(init, td.MipLevels);
					if (!sr) return 0;
					if (device->CreateTexture3D(&td, sr, &result->tex_3d) != S_OK) { free(sr); return 0; }
					free(sr);
					bool create_staging = false;
					if (desc.Usage & ResourceUsageCPURead) {
						create_staging = true;
						result->usage_flags |= ResourceUsageCPURead;
					}
					if (desc.Usage & ResourceUsageCPUWrite) {
						if (desc.Usage & ResourceUsageDepthStencil) create_staging = true;
						result->usage_flags |= ResourceUsageCPUWrite;
					}
					if (td.BindFlags & D3D11_BIND_SHADER_RESOURCE) {
						if (device->CreateShaderResourceView(result->tex_3d, 0, &result->view) != S_OK) return 0;
					}
					result->width = desc.Width;
					result->height = desc.Height;
					result->depth = desc.DepthOrArraySize;
					result->size = 1;
					if (create_staging) {
						td.Usage = D3D11_USAGE_STAGING;
						td.BindFlags = 0;
						td.MiscFlags = 0;
						if (desc.Usage & ResourceUsageCPURead) td.CPUAccessFlags |= D3D11_CPU_ACCESS_READ;
						if (desc.Usage & ResourceUsageCPUWrite) td.CPUAccessFlags |= D3D11_CPU_ACCESS_WRITE;
						if (device->CreateTexture3D(&td, 0, &result->tex_staging_3d) != S_OK) return 0;
					}
				} else return 0;
				result->Retain();
				return result;
			}
			virtual string ToString(void) const override { return L"D3D11_Device"; }
		};
		class D3D11_DeviceFactory : public Graphics::IDeviceFactory
		{
			IDXGIFactory * dxgi_factory;
		public:
			D3D11_DeviceFactory(void) { dxgi_factory = DXGIFactory; dxgi_factory->AddRef(); }
			virtual ~D3D11_DeviceFactory(void) override { if (dxgi_factory) dxgi_factory->Release(); }
			virtual Dictionary::PlainDictionary<uint64, string> * GetAvailableDevices(void) noexcept override
			{
				try {
					SafePointer< Dictionary::PlainDictionary<uint64, string> > result = new Dictionary::PlainDictionary<uint64, string>(0x10);
					uint32 index = 0;
					DXGI_ADAPTER_DESC desc;
					while (true) {
						IDXGIAdapter * adapter;
						if (dxgi_factory->EnumAdapters(index, &adapter) != S_OK) break;
						adapter->GetDesc(&desc);
						adapter->Release();
						result->Append(reinterpret_cast<uint64 &>(desc.AdapterLuid), string(desc.Description));
						index++;
					}
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual IDevice * CreateDevice(uint64 identifier) noexcept override
			{
				uint32 index = 0;
				DXGI_ADAPTER_DESC desc;
				ID3D11Device * new_device;
				while (true) {
					IDXGIAdapter * adapter;
					if (dxgi_factory->EnumAdapters(index, &adapter) != S_OK) return 0;
					adapter->GetDesc(&desc);
					if (reinterpret_cast<uint64 &>(desc.AdapterLuid) == identifier) {
						new_device = CreateDeviceD3D11(adapter, D3D_DRIVER_TYPE_UNKNOWN);
						adapter->Release();
						break;
					}
					adapter->Release();
					index++;
				}
				if (!new_device) return 0;
				auto wrapper = CreateWrappedDeviceD3D11(new_device);
				new_device->Release();
				return wrapper;
			}
			virtual IDevice * CreateDefaultDevice(void) noexcept override
			{
				auto device = CreateDeviceD3D11(0, D3D_DRIVER_TYPE_HARDWARE);
				if (!device) device = CreateDeviceD3D11(0, D3D_DRIVER_TYPE_WARP);
				if (!device) return 0;
				auto wrapper = CreateWrappedDeviceD3D11(device);
				device->Release();
				return wrapper;
			}
			virtual string ToString(void) const override { return L"D3D11_DeviceFactory"; }
		};

		Graphics::IDeviceFactory * CreateDeviceFactoryD3D11(void)
		{
			if (!DXGIFactory) { if (CreateDXGIFactory(IID_PPV_ARGS(DXGIFactory.InnerRef())) != S_OK) return 0; }
			return new D3D11_DeviceFactory();
		}
		ID3D11Resource * QueryInnerObject(Graphics::IDeviceResource * resource)
		{
			if (resource->GetResourceType() == ResourceType::Texture) {
				auto object = static_cast<D3D11_Texture *>(resource);
				if (object->tex_1d) return object->tex_1d;
				else if (object->tex_2d) return object->tex_2d;
				else if (object->tex_3d) return object->tex_3d;
				else return 0;
			} else if (resource->GetResourceType() == ResourceType::Buffer) {
				auto object = static_cast<D3D11_Buffer *>(resource);
				return object->buffer;
			} else return 0;
		}
		ID3D11Device * CreateDeviceD3D11(IDXGIAdapter * adapter, D3D_DRIVER_TYPE driver)
		{
			ID3D11Device * result = 0;
			D3D_FEATURE_LEVEL feature_level[] = {
				D3D_FEATURE_LEVEL_11_1,
				D3D_FEATURE_LEVEL_11_0,
				D3D_FEATURE_LEVEL_10_1,
				D3D_FEATURE_LEVEL_10_0,
				D3D_FEATURE_LEVEL_9_3,
				D3D_FEATURE_LEVEL_9_2,
				D3D_FEATURE_LEVEL_9_1
			};
			D3D_FEATURE_LEVEL level_selected;
			if (D3D11CreateDevice(adapter, driver, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, feature_level, 7, D3D11_SDK_VERSION, &result, &level_selected, 0) != S_OK) return 0;
			return result;
		}
		Graphics::IDevice * CreateWrappedDeviceD3D11(ID3D11Device * device) { try { auto wrapper = new D3D11_Device(device); return wrapper; } catch (...) { return 0; } }
		DXGI_FORMAT MakeDxgiFormat(Graphics::PixelFormat format)
		{
			if (IsColorFormat(format)) {
				auto bpp = GetFormatBitsPerPixel(format);
				if (bpp == 8) {
					if (format == PixelFormat::A8_unorm) return DXGI_FORMAT_A8_UNORM;
					else if (format == PixelFormat::R8_unorm) return DXGI_FORMAT_R8_UNORM;
					else if (format == PixelFormat::R8_snorm) return DXGI_FORMAT_R8_SNORM;
					else if (format == PixelFormat::R8_uint) return DXGI_FORMAT_R8_UINT;
					else if (format == PixelFormat::R8_sint) return DXGI_FORMAT_R8_SINT;
					else return DXGI_FORMAT_UNKNOWN;
				} else if (bpp == 16) {
					if (format == PixelFormat::R16_unorm) return DXGI_FORMAT_R16_UNORM;
					else if (format == PixelFormat::R16_snorm) return DXGI_FORMAT_R16_SNORM;
					else if (format == PixelFormat::R16_uint) return DXGI_FORMAT_R16_UINT;
					else if (format == PixelFormat::R16_sint) return DXGI_FORMAT_R16_SINT;
					else if (format == PixelFormat::R16_float) return DXGI_FORMAT_R16_FLOAT;
					else if (format == PixelFormat::R8G8_unorm) return DXGI_FORMAT_R8G8_UNORM;
					else if (format == PixelFormat::R8G8_snorm) return DXGI_FORMAT_R8G8_SNORM;
					else if (format == PixelFormat::R8G8_uint) return DXGI_FORMAT_R8G8_UINT;
					else if (format == PixelFormat::R8G8_sint) return DXGI_FORMAT_R8G8_SINT;
					else if (format == PixelFormat::B5G6R5_unorm) return DXGI_FORMAT_B5G6R5_UNORM;
					else if (format == PixelFormat::B5G5R5A1_unorm) return DXGI_FORMAT_B5G5R5A1_UNORM;
					else if (format == PixelFormat::B4G4R4A4_unorm) return DXGI_FORMAT_B4G4R4A4_UNORM;
					else return DXGI_FORMAT_UNKNOWN;
				} else if (bpp == 32) {
					if (format == PixelFormat::R32_uint) return DXGI_FORMAT_R32_UINT;
					else if (format == PixelFormat::R32_sint) return DXGI_FORMAT_R32_SINT;
					else if (format == PixelFormat::R32_float) return DXGI_FORMAT_R32_FLOAT;
					else if (format == PixelFormat::R16G16_unorm) return DXGI_FORMAT_R16G16_UNORM;
					else if (format == PixelFormat::R16G16_snorm) return DXGI_FORMAT_R16G16_SNORM;
					else if (format == PixelFormat::R16G16_uint) return DXGI_FORMAT_R16G16_UINT;
					else if (format == PixelFormat::R16G16_sint) return DXGI_FORMAT_R16G16_SINT;
					else if (format == PixelFormat::R16G16_float) return DXGI_FORMAT_R16G16_FLOAT;
					else if (format == PixelFormat::B8G8R8A8_unorm) return DXGI_FORMAT_B8G8R8A8_UNORM;
					else if (format == PixelFormat::R8G8B8A8_unorm) return DXGI_FORMAT_R8G8B8A8_UNORM;
					else if (format == PixelFormat::R8G8B8A8_snorm) return DXGI_FORMAT_R8G8B8A8_SNORM;
					else if (format == PixelFormat::R8G8B8A8_uint) return DXGI_FORMAT_R8G8B8A8_UINT;
					else if (format == PixelFormat::R8G8B8A8_sint) return DXGI_FORMAT_R8G8B8A8_SINT;
					else if (format == PixelFormat::R10G10B10A2_unorm) return DXGI_FORMAT_R10G10B10A2_UNORM;
					else if (format == PixelFormat::R10G10B10A2_uint) return DXGI_FORMAT_R10G10B10A2_UINT;
					else if (format == PixelFormat::R11G11B10_float) return DXGI_FORMAT_R11G11B10_FLOAT;
					else if (format == PixelFormat::R9G9B9E5_float) return DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
					else return DXGI_FORMAT_UNKNOWN;
				} else if (bpp == 64) {
					if (format == PixelFormat::R32G32_uint) return DXGI_FORMAT_R32G32_UINT;
					else if (format == PixelFormat::R32G32_sint) return DXGI_FORMAT_R32G32_SINT;
					else if (format == PixelFormat::R32G32_float) return DXGI_FORMAT_R32G32_FLOAT;
					else if (format == PixelFormat::R16G16B16A16_unorm) return DXGI_FORMAT_R16G16B16A16_UNORM;
					else if (format == PixelFormat::R16G16B16A16_snorm) return DXGI_FORMAT_R16G16B16A16_SNORM;
					else if (format == PixelFormat::R16G16B16A16_uint) return DXGI_FORMAT_R16G16B16A16_UINT;
					else if (format == PixelFormat::R16G16B16A16_sint) return DXGI_FORMAT_R16G16B16A16_SINT;
					else if (format == PixelFormat::R16G16B16A16_float) return DXGI_FORMAT_R16G16B16A16_FLOAT;
					else return DXGI_FORMAT_UNKNOWN;
				} else if (bpp == 128) {
					if (format == PixelFormat::R32G32B32A32_uint) return DXGI_FORMAT_R32G32B32A32_UINT;
					else if (format == PixelFormat::R32G32B32A32_sint) return DXGI_FORMAT_R32G32B32A32_SINT;
					else if (format == PixelFormat::R32G32B32A32_float) return DXGI_FORMAT_R32G32B32A32_FLOAT;
					else return DXGI_FORMAT_UNKNOWN;
				} else return DXGI_FORMAT_UNKNOWN;
			} else if (IsDepthStencilFormat(format)) {
				if (format == PixelFormat::D16_unorm) return DXGI_FORMAT_D16_UNORM;
				else if (format == PixelFormat::D32_float) return DXGI_FORMAT_D32_FLOAT;
				else if (format == PixelFormat::D24S8_unorm) return DXGI_FORMAT_D24_UNORM_S8_UINT;
				else if (format == PixelFormat::D32S8_float) return DXGI_FORMAT_D32_FLOAT_S8X24_UINT;
				else return DXGI_FORMAT_UNKNOWN;
			} else return DXGI_FORMAT_UNKNOWN;
		}
	}
}