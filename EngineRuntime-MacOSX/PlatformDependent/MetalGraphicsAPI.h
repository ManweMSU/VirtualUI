#pragma once

#include "../Graphics/Graphics.h"

namespace Engine
{
	namespace MetalGraphics
	{
		void UpdateMetalTexture(Graphics::ITexture * dest, const void * data, uint stride, Graphics::SubresourceIndex subres);
		void QueryMetalTexture(Graphics::ITexture * from, void * buffer, uint stride, Graphics::SubresourceIndex subres);
	}
}