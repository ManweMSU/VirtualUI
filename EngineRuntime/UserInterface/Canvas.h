#pragma once

#include "ShapeBase.h"
#include "../Math/Color.h"

namespace Engine
{
	namespace Drawing
	{
		class ITextureRenderingDevice;
		class ICanvasRenderingDevice : public UI::IRenderingDevice
		{
		public:
			virtual void DrawPolygon(const Math::Vector2 * points, int count, const Math::Color & color, double width) noexcept = 0;
			virtual void FillPolygon(const Math::Vector2 * points, int count, const Math::Color & color) noexcept = 0;
			virtual ITextureRenderingDevice * CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) noexcept = 0;
		};
		class ITextureRenderingDevice : public ICanvasRenderingDevice
		{
		public:
			virtual void BeginDraw(void) noexcept = 0;
			virtual void EndDraw(void) noexcept = 0;
			virtual UI::ITexture * GetRenderTargetAsTexture(void) noexcept = 0;
			virtual Codec::Frame * GetRenderTargetAsFrame(void) noexcept = 0;
		};
	}
}