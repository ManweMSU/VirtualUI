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

		void CreateDevices(void);
		void ReleaseDevices(void);
		void RestartDevicesIfNecessary(void);
		DeviceDriverClass GetDeviceDriverClass(void);

		bool CreateD2DDeviceContextForWindow(HWND Window, ID2D1DeviceContext ** Context, IDXGISwapChain1 ** SwapChain);
		bool CreateSwapChainForWindow(HWND Window, IDXGISwapChain ** SwapChain);
		bool CreateSwapChainDevice(IDXGISwapChain * SwapChain, ID2D1RenderTarget ** Target);
		bool ResizeRenderBufferForD2DDevice(ID2D1DeviceContext * Context, IDXGISwapChain1 * SwapChain);
		bool ResizeRenderBufferForSwapChainDevice(IDXGISwapChain * SwapChain);

		class D3DTexture : public Graphics::ITexture
		{
		public:
			ID3D11Texture2D * texture;
			int width, height;

			D3DTexture(void);
			virtual ~D3DTexture(void) override;
		};
	}
}