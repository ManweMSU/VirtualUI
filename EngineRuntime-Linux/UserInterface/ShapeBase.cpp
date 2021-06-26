#include "ShapeBase.h"
#include "Templates.h"
#include "../Math/Color.h"

namespace Engine
{
	namespace UI
	{
		FrameShape::FrameShape(const Rectangle & position) : Children(0x10), RenderMode(FrameRenderMode::Normal), Opacity(1.0) { Position = position; }
		FrameShape::FrameShape(const Rectangle & position, FrameRenderMode mode, double opacity) : Children(0x10), RenderMode(mode), Opacity(opacity) { Position = position; }
		FrameShape::~FrameShape(void) {}
		void FrameShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			if (RenderMode == FrameRenderMode::Normal) {
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
			} else if (RenderMode == FrameRenderMode::Clipping) {
				Device->PushClip(my);
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
				Device->PopClip();
			} else if (RenderMode == FrameRenderMode::Layering) {
				Device->BeginLayer(my, Opacity);
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(Device, my);
				Device->EndLayer();
			}
		}
		void FrameShape::ClearCache(void) noexcept { for (int i = Children.Length() - 1; i >= 0; i--) Children[i].ClearCache(); }
		Shape * FrameShape::Clone(void) const
		{
			SafePointer<FrameShape> clone = new FrameShape(Position, RenderMode, Opacity);
			for (int i = 0; i < Children.Length(); i++) {
				SafePointer<Shape> child = Children[i].Clone();
				clone->Children.Append(child);
			}
			clone->Retain();
			return clone;
		}
		string FrameShape::ToString(void) const { return L"FrameShape"; }
		GradientPoint::GradientPoint(void) noexcept {}
		GradientPoint::GradientPoint(const UI::Color & color) noexcept : Color(color), Position(0.0) {}
		GradientPoint::GradientPoint(const UI::Color & color, double position) noexcept : Color(color), Position(position) {}
		BarShape::BarShape(const Rectangle & position, const Color & color) : GradientAngle(0.0) { Position = position; Gradient << GradientPoint(color); }
		BarShape::BarShape(const Rectangle & position, const Array<GradientPoint>& gradient, double angle) : Gradient(gradient), GradientAngle(angle) { Position = position; }
		BarShape::~BarShape(void) {}
		void BarShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateBarRenderingInfo(Gradient, GradientAngle));
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->RenderBar(Info, my);
		}
		void BarShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * BarShape::Clone(void) const { return new BarShape(Position, Gradient, GradientAngle); }
		string BarShape::ToString(void) const { return L"BarShape"; }
		IRenderingDevice::~IRenderingDevice(void) {}
		IBarRenderingInfo::~IBarRenderingInfo(void) {}
		IBlurEffectRenderingInfo::~IBlurEffectRenderingInfo(void) {}
		BlurEffectShape::BlurEffectShape(const Rectangle & position, double power) : BlurPower(power) { Position = position; }
		BlurEffectShape::~BlurEffectShape(void) {}
		void BlurEffectShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateBlurEffectRenderingInfo(BlurPower));
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->ApplyBlur(Info, my);
		}
		void BlurEffectShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * BlurEffectShape::Clone(void) const { return new BlurEffectShape(Position, BlurPower); }
		string BlurEffectShape::ToString(void) const { return L"BlurEffectShape"; }
		ITextureRenderingInfo::~ITextureRenderingInfo(void) {}
		TextureShape::TextureShape(const Rectangle & position, ITexture * texture, const Rectangle & take_from, TextureRenderMode mode) : Texture(texture), From(take_from), Mode(mode) { Position = position; if (Texture) Texture->Retain(); }
		TextureShape::~TextureShape(void) { if (Texture) Texture->Release(); }
		void TextureShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (Texture) {
				if (!Info) {
					FromBox = Box(From, Box(0, 0, Texture->GetWidth(), Texture->GetHeight()));
					Info.SetReference(Device->CreateTextureRenderingInfo(Texture, FromBox, Mode == TextureRenderMode::FillPattern));
				}
				Box to(Position, Outer);
				if (to.Right < to.Left || to.Bottom < to.Top) return;
				if (Mode == TextureRenderMode::Fit) {
					double ta = double(to.Right - to.Left) / double(to.Bottom - to.Top);
					double fa = double(FromBox.Right - FromBox.Left) / double(FromBox.Bottom - FromBox.Top);
					if (ta > fa) {
						int adjx = int(double(to.Bottom - to.Top) * fa);
						int xc = (to.Right + to.Left) >> 1;
						to.Left = xc - (adjx >> 1);
						to.Right = xc + (adjx >> 1);
					} else if (fa > ta) {
						int adjy = int(double(to.Right - to.Left) / fa);
						int yc = (to.Bottom + to.Top) >> 1;
						to.Top = yc - (adjy >> 1);
						to.Bottom = yc + (adjy >> 1);
					}
				} else if (Mode == TextureRenderMode::AsIs) {
					int sx = FromBox.Right - FromBox.Left;
					int sy = FromBox.Bottom - FromBox.Top;
					if (to.Right - to.Left < sx || to.Bottom - to.Top < sy) return;
					to.Right = to.Left = (to.Right + to.Left) / 2;
					to.Bottom = to.Top = (to.Bottom + to.Top) / 2;
					int rx = sx / 2, ry = sy / 2;
					to.Left -= rx; to.Top -= ry;
					to.Right += sx - rx; to.Bottom += sy - ry;
				}
				Device->RenderTexture(Info, to);
			}
		}
		void TextureShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * TextureShape::Clone(void) const { return new TextureShape(Position, Texture, From, Mode); }
		string TextureShape::ToString(void) const { return L"TextureShape"; }
		ITextRenderingInfo::~ITextRenderingInfo(void) {}
		TextShape::TextShape(const Rectangle & position, const string & text, IFont * font, const Color & color, TextHorizontalAlign horizontal_align, TextVerticalAlign vertical_align) :
			Text(text), Font(font), TextColor(color), halign(horizontal_align), valign(vertical_align) { Position = position; if (Font) Font->Retain(); }
		TextShape::~TextShape(void) { if (Font) Font->Release(); }
		void TextShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateTextRenderingInfo(Font, Text, int(halign), int(valign), TextColor));
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->RenderText(Info, my, true);
		}
		void TextShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * TextShape::Clone(void) const { return new TextShape(Position, Text, Font, TextColor, halign, valign); }
		string TextShape::ToString(void) const { return L"TextShape"; }
		IInversionEffectRenderingInfo::~IInversionEffectRenderingInfo(void) {}
		InversionEffectShape::InversionEffectShape(const Rectangle & position) { Position = position; }
		InversionEffectShape::~InversionEffectShape(void) {}
		void InversionEffectShape::Render(IRenderingDevice * Device, const Box & Outer) const noexcept
		{
			if (!Info) Info.SetReference(Device->CreateInversionEffectRenderingInfo());
			Box my(Position, Outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			Device->ApplyInversion(Info, my, false);
		}
		void InversionEffectShape::ClearCache(void) noexcept { Info.SetReference(0); }
		Shape * InversionEffectShape::Clone(void) const { return new InversionEffectShape(Position); }
		string InversionEffectShape::ToString(void) const { return L"InversionEffectShape"; }
	}
}