#include "Canvas.h"

namespace Engine
{
	namespace Drawing
	{
		void ICanvasRenderingDevice::DrawImage(UI::ITexture * image, int left, int top, int right, int bottom)
		{
			SafePointer<UI::ITextureRenderingInfo> info = CreateTextureRenderingInfo(image,
				UI::Box(0, 0, image->GetWidth(), image->GetHeight()), false);
			RenderTexture(info, UI::Box(left, top, right, bottom));
		}
		void ICanvasRenderingDevice::DrawText(const string & text, UI::IFont * font, const Math::Color & color, int left, int top)
		{
			SafePointer<UI::ITextRenderingInfo> info = CreateTextRenderingInfo(font, text, 0, 0, color);
			RenderText(info, UI::Box(left, top, left, top), false);
		}
	}
}