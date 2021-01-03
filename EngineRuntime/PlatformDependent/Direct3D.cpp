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
				wrapper->Retain();
				device->GetImmediateContext(&context);
				// TODO: IMPLEMENT
			}
			virtual ~D3D11_DeviceContext(void) override
			{
				wrapper->Release();
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
			virtual void SetVertexShaderSamplerState(uint32 at, ISamplerState * sampler) noexcept override
			{
				// TODO: IMPLEMENT
			}
			virtual void SetPixelShaderResource(uint32 at, IDeviceResource * resource) noexcept override
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
			virtual Object * Query2DRenderingDevice(void) noexcept override
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
		public:
			D3D11_Device(ID3D11Device * _device)
			{
				context = new D3D11_DeviceContext(_device, this);
				device = _device;
				device->AddRef();
				// TODO: IMPLEMENT
			}
			virtual ~D3D11_Device(void) override
			{
				device->Release();
				context->Release();
				// TODO: IMPLEMENT
			}
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
			virtual ITexture * CreateTexture(const TextureDesc & desc, const ResourceInitDesc & init) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual string ToString(void) const override { return L"D3D11_Device"; }
		};
		class D3D11_DeviceFactory : public Graphics::IDeviceFactory
		{
			IDXGIFactory * dxgi_factory;
		public:
			D3D11_DeviceFactory(void) { dxgi_factory = DXGIFactory; dxgi_factory->AddRef(); }
			virtual ~D3D11_DeviceFactory(void) override { if (dxgi_factory) dxgi_factory->Release(); }
			virtual uint32 GetAvailableDeviceCount(void) noexcept override
			{
				uint32 count = 0;
				while (true) {
					IDXGIAdapter * adapter;
					if (dxgi_factory->EnumAdapters(count, &adapter) != S_OK) return count;
					adapter->Release();
					count++;
				}
			}
			virtual uint64 GetDeviceIdentifier(uint32 index) noexcept override
			{
				IDXGIAdapter * adapter;
				if (dxgi_factory->EnumAdapters(index, &adapter) != S_OK) return 0;
				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);
				adapter->Release();
				return reinterpret_cast<uint64 &>(desc.AdapterLuid);
			}
			virtual string GetDeviceName(uint32 index) noexcept override
			{
				IDXGIAdapter * adapter;
				if (dxgi_factory->EnumAdapters(index, &adapter) != S_OK) return L"";
				DXGI_ADAPTER_DESC desc;
				adapter->GetDesc(&desc);
				adapter->Release();
				return string(desc.Description);
			}
			virtual IDevice * CreateDevice(uint32 index) noexcept override
			{
				IDXGIAdapter * adapter;
				if (dxgi_factory->EnumAdapters(index, &adapter) != S_OK) return 0;
				auto device = CreateDeviceD3D11(adapter, D3D_DRIVER_TYPE_UNKNOWN);
				adapter->Release();
				if (!device) return 0;
				auto wrapper = CreateWrappedDeviceD3D11(device);
				device->Release();
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
			// TODO: IMPLEMENT
			return nullptr;
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
		
		// TODO: REMOVE
		/*class D3DTexture : public Graphics::ITexture
		{
		public:
			ID3D11Texture2D * texture;
			int width, height;

			D3DTexture(void);
			virtual ~D3DTexture(void) override;
		};

		D3DTexture::D3DTexture(void) { texture = 0; }
		D3DTexture::~D3DTexture(void) { if (texture) texture->Release(); }

		Graphics::ITexture * D2DRenderDevice::CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height)
		{
			if (!Direct3D::D3DDevice) return 0;
			if (width <= 0 || height <= 0) throw InvalidArgumentException();
			if (format != Graphics::PixelFormat::B8G8R8A8_unorm) throw InvalidArgumentException();
			D3D11_TEXTURE2D_DESC desc;
			desc.Width = width;
			desc.Height = height;
			desc.MipLevels = 1;
			desc.ArraySize = 1;
			desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.MiscFlags = 0;
			ID3D11Texture2D * texture = 0;
			if (Direct3D::D3DDevice->CreateTexture2D(&desc, 0, &texture) != S_OK) return 0;
			Direct3D::D3DTexture * wrapper = new (std::nothrow) Direct3D::D3DTexture;
			if (!wrapper) {
				texture->Release();
				return 0;
			}
			wrapper->texture = texture;
			wrapper->width = width;
			wrapper->height = height;
			return wrapper;
		}*/
		// TODO: END REMOVE
	}
}