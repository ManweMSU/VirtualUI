#include "ShapeBase.h"
#include "Templates.h"
#include "../Math/Color.h"
#include "../Graphics/GraphicsBase.h"

namespace Engine
{
	namespace UI
	{
		FrameShape::FrameShape(const Rectangle & position) : Children(0x10), RenderMode(FrameRenderMode::Normal), Opacity(1.0) { Position = position; }
		FrameShape::FrameShape(const Rectangle & position, FrameRenderMode mode, double opacity) : Children(0x10), RenderMode(mode), Opacity(opacity) { Position = position; }
		FrameShape::~FrameShape(void) {}
		void FrameShape::Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept
		{
			Box my(Position, outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			if (RenderMode == FrameRenderMode::Normal) {
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(device, my);
			} else if (RenderMode == FrameRenderMode::Clipping) {
				device->PushClip(my);
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(device, my);
				device->PopClip();
			} else if (RenderMode == FrameRenderMode::Layering) {
				device->BeginLayer(my, Opacity);
				for (int i = Children.Length() - 1; i >= 0; i--) Children[i].Render(device, my);
				device->EndLayer();
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
		BarShape::BarShape(const Rectangle & position, const Color & color) : _x1(0), _y1(0), _x2(0), _y2(0), _w(-1), _h(-1), _p1(-1, -1), _p2(-1, -1) { Position = position; _gradient << GradientPoint(color); }
		BarShape::BarShape(const Rectangle & position, const Array<GradientPoint> & gradient, const Coordinate & x1, const Coordinate & y1, const Coordinate & x2, const Coordinate & y2) :
			_gradient(gradient), _x1(x1), _y1(y1), _x2(x2), _y2(y2), _w(-1), _h(-1), _p1(-1, -1), _p2(-1, -1) { Position = position; }
		BarShape::~BarShape(void) {}
		void BarShape::Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept
		{
			Box my(Position, outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			if (_gradient.Length() > 1) {
				int lw = my.Right - my.Left;
				int lh = my.Bottom - my.Top;
				if (lw <= 0 || lh <= 0) return;
				if (lw != _w || lh != _h) {
					_w = lw; _h = lh;
					auto p1 = Point(_x1.ToInteger(_w), _y1.ToInteger(_h));
					auto p2 = Point(_x2.ToInteger(_w), _y2.ToInteger(_h));
					if (p1 != _p1 || p2 != _p2) {
						_p1 = p1; _p2 = p2;
						_info.SetReference(0);
					}
				}
				if (!_info) _info = device->CreateGradientBrush(_p1, _p2, _gradient.GetBuffer(), _gradient.Length());
			} else {
				if (!_info && _gradient.Length()) _info = device->CreateSolidColorBrush(_gradient[0].Value);
			}
			if (_info) {
				device->Render(_info, my);
			}
		}
		void BarShape::ClearCache(void) noexcept { _info.SetReference(0); }
		Shape * BarShape::Clone(void) const { return new BarShape(Position, _gradient, _x1, _y1, _x2, _y2); }
		string BarShape::ToString(void) const { return L"BarShape"; }
		BlurEffectShape::BlurEffectShape(const Rectangle & position, double power) : _power(power) { Position = position; }
		BlurEffectShape::~BlurEffectShape(void) {}
		void BlurEffectShape::Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept
		{
			if (!_info) _info = device->CreateBlurEffectBrush(_power);
			Box my(Position, outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			device->Render(_info, my);
		}
		void BlurEffectShape::ClearCache(void) noexcept { _info.SetReference(0); }
		Shape * BlurEffectShape::Clone(void) const { return new BlurEffectShape(Position, _power); }
		string BlurEffectShape::ToString(void) const { return L"BlurEffectShape"; }
		TextureShape::TextureShape(const Rectangle & position, Graphics::IBitmap * bitmap, const Rectangle & take_from, TextureRenderMode mode) : _from(take_from), _mode(mode), _area(0, 0, 0, 0) { Position = position; _bitmap.SetRetain(bitmap); }
		TextureShape::~TextureShape(void) {}
		void TextureShape::Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept
		{
			if (_bitmap) {
				if (!_info && _bitmap) {
					_area = Box(_from, Box(0, 0, _bitmap->GetWidth(), _bitmap->GetHeight()));
					_info = device->CreateBitmapBrush(_bitmap, _area, _mode == TextureRenderMode::FillPattern);
				}
				Box to(Position, outer);
				if (to.Right < to.Left || to.Bottom < to.Top) return;
				if (_info) {
					if (_mode == TextureRenderMode::Fit) {
						double ta = double(to.Right - to.Left) / double(to.Bottom - to.Top);
						double fa = double(_area.Right - _area.Left) / double(_area.Bottom - _area.Top);
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
					} else if (_mode == TextureRenderMode::AsIs) {
						int sx = _area.Right - _area.Left;
						int sy = _area.Bottom - _area.Top;
						if (to.Right - to.Left < sx || to.Bottom - to.Top < sy) return;
						to.Right = to.Left = (to.Right + to.Left) / 2;
						to.Bottom = to.Top = (to.Bottom + to.Top) / 2;
						int rx = sx / 2, ry = sy / 2;
						to.Left -= rx; to.Top -= ry;
						to.Right += sx - rx; to.Bottom += sy - ry;
					}
					device->Render(_info, to);
				}
			}
		}
		void TextureShape::ClearCache(void) noexcept { _info.SetReference(0); }
		Shape * TextureShape::Clone(void) const { return new TextureShape(Position, _bitmap, _from, _mode); }
		string TextureShape::ToString(void) const { return L"TextureShape"; }
		TextShape::TextShape(const Rectangle & position, const string & text, Graphics::IFont * font, const Color & color, uint32 flags) : _atoms(0x10), _text(text), _color(color), _flags(flags)
		{
			_w = _h = -1;
			Position = position;
			_font.SetRetain(font);
			if ((_flags & TextRenderMultiline) && !(_flags & TextRenderAllowWordWrap)) {
				if (_flags & TextRenderAllowEllipsis) throw InvalidArgumentException();
				DynamicString current;
				int length = _text.Length();
				int pos = 0;
				while (pos < length) {
					_text_atom a;
					a.end_line = false;
					a.x = a.y = 0;
					while (pos < length) {
						if (_text[pos] >= 32) current << _text[pos];
						else if (_text[pos] == L'\n') { pos++; a.end_line = true; break; }
						pos++;
					}
					a.text = current.ToString();
					_atoms << a;
					current.Clear();
				}
			} else if (_flags & TextRenderAllowWordWrap) {
				if (_flags & TextRenderAllowEllipsis) throw InvalidArgumentException();
				DynamicString current;
				int length = _text.Length();
				int pos = 0;
				while (pos < length) {
					_text_atom a;
					a.end_line = false;
					a.x = a.y = 0;
					while (pos < length) {
						if (_text[pos] > 32) current << _text[pos];
						else if (_text[pos] == L' ') break;
						else if (_text[pos] == L'\n') { pos++; a.end_line = true; break; }
						pos++;
					}
					while (pos < length && _text[pos] == L' ') { current << _text[pos]; pos++; }
					a.text = current.ToString();
					_atoms << a;
					current.Clear();
				}
			} else if (_flags & TextRenderAllowEllipsis) {
				_text_atom a;
				a.end_line = false;
				a.text = _text;
				a.x = a.y = 0;
				_atoms << a;
			}
		}
		TextShape::~TextShape(void) {}
		void TextShape::Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept
		{
			Box my(Position, outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			if (_atoms.Length()) {
				if (_flags & TextRenderAllowEllipsis) {
					auto & a = _atoms[0];
					int vpw = my.Right - my.Left;
					if (a.info && a.x < vpw) a.info.SetReference(0);
					if (!a.info) {
						a.x = vpw;
						a.text = _text;
						a.info = device->CreateTextBrush(_font, a.text, (_flags >> 4) & 0xF, (_flags >> 8) & 0xF, _color);
					}
					while (a.info) {
						int w, h;
						a.info->GetExtents(w, h);
						if (!a.text.Length() || w <= vpw) break;
						a.x = vpw;
						a.text = a.text.Fragment(0, a.text.Length() - 1);
						a.info = device->CreateTextBrush(_font, a.text + L"\x2026", (_flags >> 4) & 0xF, (_flags >> 8) & 0xF, _color);
					}
					if (a.info) device->Render(a.info, my, true);
				} else {
					int vpw = my.Right - my.Left;
					int vph = my.Bottom - my.Top;
					for (auto & a : _atoms) if (!a.info) a.info = device->CreateTextBrush(_font, a.text, 0, 0, _color);
					if (_w != vpw || _h != vph) {
						_w = vpw; _h = vph;
						int pos = 0;
						int extents_w = 0, extents_h = 0;
						while (pos < _atoms.Length()) {
							int sp = pos;
							while (pos < _atoms.Length() && !_atoms[pos].end_line) pos++;
							if (pos < _atoms.Length()) pos++;
							int x = 0;
							_atoms[sp].x = 0;
							_atoms[sp].y = extents_h;
							int lw = 0, lh;
							if (_atoms[sp].info) _atoms[sp].info->GetExtents(lw, lh);
							if (lw > extents_w) extents_w = lw;
							for (int i = sp + 1; i < pos; i++) {
								lw = 0;
								if (_atoms[i - 1].info) _atoms[i - 1].info->GetExtents(lw, lh);
								_atoms[i].x = _atoms[i - 1].x + lw;
								_atoms[i].y = extents_h;
								if (_atoms[i].info) _atoms[i].info->GetExtents(lw, lh);
								if (_atoms[i].x + lw > vpw) { pos = i; break; }
								if (_atoms[i].x + lw > extents_w) extents_w = _atoms[i].x + lw;
							}
							extents_h += _font ? _font->GetLineSpacing() : 0;
						}
						if (_flags & TextRenderAlignVCenter) {
							int dy = (vph - extents_h) / 2;
							for (auto & a : _atoms) a.y += dy;
						} else if (_flags & TextRenderAlignBottom) {
							int dy = vph - extents_h;
							for (auto & a : _atoms) a.y += dy;
						}
						if ((_flags & TextRenderAlignRight) || (_flags & TextRenderAlignCenter)) {
							int pos = 0;
							while (pos < _atoms.Length()) {
								int sp = pos;
								pos++;
								while (pos < _atoms.Length() && _atoms[pos].y == _atoms[sp].y) pos++;
								int strip_width = 0, lh;
								if (_atoms[pos - 1].info) _atoms[pos - 1].info->GetExtents(strip_width, lh);
								strip_width += _atoms[pos - 1].x;
								int dx = vpw - strip_width;
								if (_flags & TextRenderAlignCenter) dx /= 2;
								for (int i = sp; i < pos; i++) _atoms[i].x += dx;
							}
						}
					}
					device->PushClip(my);
					for (auto & a : _atoms) if (a.info) device->Render(a.info, Box(my.Left + a.x, my.Top + a.y, my.Right, my.Bottom), false);
					device->PopClip();
				}
			} else {
				if (!_info) _info = device->CreateTextBrush(_font, _text, (_flags >> 4) & 0xF, (_flags >> 8) & 0xF, _color);
				if (_info) device->Render(_info, my, true);
			}
		}
		void TextShape::ClearCache(void) noexcept { _info.SetReference(0); for (auto & a : _atoms) a.info.SetReference(0); }
		Shape * TextShape::Clone(void) const { return new TextShape(Position, _text, _font, _color, _flags); }
		string TextShape::ToString(void) const { return L"TextShape"; }
		InversionEffectShape::InversionEffectShape(const Rectangle & position) { Position = position; }
		InversionEffectShape::~InversionEffectShape(void) {}
		void InversionEffectShape::Render(Graphics::I2DDeviceContext * device, const Box & outer) noexcept
		{
			if (!_info) _info = device->CreateInversionEffectBrush();
			Box my(Position, outer);
			if (my.Right < my.Left || my.Bottom < my.Top) return;
			device->Render(_info, my, false);
		}
		void InversionEffectShape::ClearCache(void) noexcept { _info.SetReference(0); }
		Shape * InversionEffectShape::Clone(void) const { return new InversionEffectShape(Position); }
		string InversionEffectShape::ToString(void) const { return L"InversionEffectShape"; }
	}
}