#pragma once

#include "Direct2D.h"

#include <d3d11_1.h>

namespace Engine
{
	namespace Direct3D
	{
		enum class DeviceDriverClass { Hardware, Warp, None };

		extern SafePointer<ID3D11Device> D3DDevice;
		extern SafePointer<ID2D1Device> D2DDevice;
		extern SafePointer<IDXGIDevice1> DXGIDevice;
		extern SafePointer<ID3D11DeviceContext> D3DDeviceContext;
		extern SafePointer<IDXGIFactory> DXGIFactory;
		extern SafePointer<Graphics::IDevice> WrappedDevice;

		void CreateDevices(void);
		void ReleaseDevices(void);
		void RestartDevicesIfNecessary(void);
		DeviceDriverClass GetDeviceDriverClass(void);

		bool CreateD2DDeviceContextForWindow(HWND Window, ID2D1DeviceContext ** Context, IDXGISwapChain1 ** SwapChain);
		bool CreateSwapChainForWindow(HWND Window, IDXGISwapChain ** SwapChain);
		bool CreateSwapChainDevice(IDXGISwapChain * SwapChain, ID2D1RenderTarget ** Target);
		bool ResizeRenderBufferForD2DDevice(ID2D1DeviceContext * Context, IDXGISwapChain1 * SwapChain);
		bool ResizeRenderBufferForSwapChainDevice(IDXGISwapChain * SwapChain);

		Graphics::IDeviceFactory * CreateDeviceFactoryD3D11(void);
		ID3D11Resource * QueryInnerObject(Graphics::IDeviceResource * resource);
		ID3D11Device * CreateDeviceD3D11(IDXGIAdapter * adapter, D3D_DRIVER_TYPE driver);
		Graphics::IDevice * CreateWrappedDeviceD3D11(ID3D11Device * device);
		DXGI_FORMAT MakeDxgiFormat(Graphics::PixelFormat format);

		ID3D11Device * GetD3D11Device(Graphics::IDevice * device);
		ID3D11Texture2D * GetD3D11Texture2D(Graphics::ITexture * texture);
		IDXGIDevice * QueryDXGIDevice(Graphics::IDevice * device);
		IUnknown * GetVideoAccelerationDevice(Graphics::IDevice * device);
		void SetVideoAccelerationDevice(Graphics::IDevice * device_for, IUnknown * device_set);
	}
}