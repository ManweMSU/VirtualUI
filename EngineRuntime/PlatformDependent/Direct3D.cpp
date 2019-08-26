#include "Direct3D.h"

#include <VersionHelpers.h>

#pragma comment(lib, "d3d11.lib")

#undef ZeroMemory

namespace Engine
{
	namespace Direct3D
	{
		SafePointer<ID3D11Device> D3DDevice;
		SafePointer<ID2D1Device> D2DDevice;
		SafePointer<IDXGIDevice1> DXGIDevice;
		SafePointer<ID3D11DeviceContext> D3DDeviceContext;

		void CreateDevices(void)
		{
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
				if (D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, FeatureLevel, 7, D3D11_SDK_VERSION,
					D3DDevice.InnerRef(), &LevelSelected, D3DDeviceContext.InnerRef()) != S_OK) {
					D3DDeviceContext.SetReference(0);
					D3DDevice.SetReference(0);
					DXGIDevice.SetReference(0);
					D2DDevice.SetReference(0);
					return;
				}
			}
			if (!DXGIDevice) {
				if (D3DDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**) DXGIDevice.InnerRef()) != S_OK) {
					D3DDeviceContext.SetReference(0);
					D3DDevice.SetReference(0);
					DXGIDevice.SetReference(0);
					D2DDevice.SetReference(0);
					return;
				}
			}
			if (!D2DDevice && Direct2D::D2DFactory1) {
				if (Direct2D::D2DFactory1->CreateDevice(DXGIDevice, D2DDevice.InnerRef()) != S_OK) {
					D3DDeviceContext.SetReference(0);
					D3DDevice.SetReference(0);
					DXGIDevice.SetReference(0);
					D2DDevice.SetReference(0);
					return;
				}
			}
		}
		void CreateD2DDeviceContextForWindow(HWND Window, ID2D1DeviceContext ** Context, IDXGISwapChain1 ** SwapChain)
		{
			if (!D3DDevice || !D2DDevice || !DXGIDevice || !D3DDeviceContext) throw Exception();
			SafePointer<ID2D1DeviceContext> Result;
			SafePointer<IDXGISwapChain1> SwapChainResult;
			if (D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, Result.InnerRef()) != S_OK) {
				throw Exception();
			}

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
			SwapChainDescription.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
			SwapChainDescription.Scaling = DXGI_SCALING_STRETCH;

			SafePointer<IDXGIAdapter> Adapter;
			DXGIDevice->GetAdapter(Adapter.InnerRef());
			SafePointer<IDXGIFactory2> Factory;
			Adapter->GetParent(IID_PPV_ARGS(Factory.InnerRef()));
			if (Factory->CreateSwapChainForHwnd(D3DDevice, Window, &SwapChainDescription, 0, 0, SwapChainResult.InnerRef()) != S_OK) {
				throw Exception();
			}
			DXGIDevice->SetMaximumFrameLatency(1);

			D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0.0f, 0.0f);
			SafePointer<IDXGISurface> Surface;
			if (SwapChainResult->GetBuffer(0, IID_PPV_ARGS(Surface.InnerRef())) != S_OK) {
				throw Exception();
			}
			SafePointer<ID2D1Bitmap1> Bitmap;
			if (Result->CreateBitmapFromDxgiSurface(Surface, props, Bitmap.InnerRef()) != S_OK) {
				throw Exception();
			}
			Result->SetTarget(Bitmap);

			*Context = Result;
			*SwapChain = SwapChainResult;
			Result->AddRef();
			SwapChainResult->AddRef();
		}
		void ResizeRenderBufferForD2DDevice(ID2D1DeviceContext * Context, IDXGISwapChain1 * SwapChain)
		{
			if (Context && SwapChain) {
				Context->SetTarget(0);
				if (SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0) != S_OK) {
					throw Exception();
				}
				D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
					D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0.0f, 0.0f);
				SafePointer<IDXGISurface> Surface;
				if (SwapChain->GetBuffer(0, IID_PPV_ARGS(Surface.InnerRef())) != S_OK) {
					throw Exception();
				}
				SafePointer<ID2D1Bitmap1> Bitmap;
				if (Context->CreateBitmapFromDxgiSurface(Surface, props, Bitmap.InnerRef()) != S_OK) {
					throw Exception();
				}
				Context->SetTarget(Bitmap);
			}
		}
	}
}