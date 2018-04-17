#pragma once

#include "Direct2D.h"

#include <d3d11_1.h>

namespace Engine
{
	namespace Direct3D
	{
		extern SafePointer<ID3D11Device> D3DDevice;
		extern SafePointer<ID2D1Device> D2DDevice;
		extern SafePointer<IDXGIDevice1> DXGIDevice;
		extern SafePointer<ID3D11DeviceContext> D3DDeviceContext;

		void CreateDevices(void);
		void CreateD2DDeviceContextForWindow(HWND Window, ID2D1DeviceContext ** Context, IDXGISwapChain1 ** SwapChain);
		void ResizeRenderBufferForD2DDevice(ID2D1DeviceContext * Context, IDXGISwapChain1 * SwapChain);
	}
}