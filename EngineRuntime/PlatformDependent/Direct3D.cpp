#include "Direct3D.h"

#include <VersionHelpers.h>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

#undef ZeroMemory

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

		void CreateDevices(void)
		{
			if (!DXGIFactory) {
				CreateDXGIFactory(IID_PPV_ARGS(DXGIFactory.InnerRef()));
			}
			if (!D3DDevice) {
				D3D_FEATURE_LEVEL FeatureLevel[] = {
					D3D_FEATURE_LEVEL_11_1,
					D3D_FEATURE_LEVEL_11_0,
					D3D_FEATURE_LEVEL_10_1,
					D3D_FEATURE_LEVEL_10_0,
					D3D_FEATURE_LEVEL_9_3,
					D3D_FEATURE_LEVEL_9_2,
					D3D_FEATURE_LEVEL_9_1
				};
				D3D_FEATURE_LEVEL LevelSelected;
				if (D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, FeatureLevel, 7, D3D11_SDK_VERSION, D3DDevice.InnerRef(), &LevelSelected, D3DDeviceContext.InnerRef()) != S_OK) {
					if (D3D11CreateDevice(0, D3D_DRIVER_TYPE_WARP, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, FeatureLevel, 7, D3D11_SDK_VERSION, D3DDevice.InnerRef(), &LevelSelected, D3DDeviceContext.InnerRef()) != S_OK) {
						return;
					} else D3DDeviceClass = DeviceDriverClass::Warp;
				} else D3DDeviceClass = DeviceDriverClass::Hardware;
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
		D3DTexture::D3DTexture(void) { texture = 0; }
		D3DTexture::~D3DTexture(void) { if (texture) texture->Release(); }
	}
}