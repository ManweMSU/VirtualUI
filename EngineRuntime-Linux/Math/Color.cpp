#include "Color.h"

namespace Engine
{
	namespace Math
	{
		Color::Color(void) noexcept {}
		Color::Color(const double * source) noexcept : Vector4(source) {}
		Color::Color(double r, double g, double b, double a) noexcept : Vector4(r, g, b, a) {}
		Color::Color(UI::Color c) noexcept : Vector4(double(c.r) / 255.0, double(c.g) / 255.0, double(c.b) / 255.0, double(c.a) / 255.0) {}
		Color::Color(StandardColor c) noexcept
		{
			if (c == StandardColor::Black) *this = Color(0.0, 0.0, 0.0);
			else if (c == StandardColor::DarkBlue) *this = Color(0.0, 0.0, 0.5);
			else if (c == StandardColor::DarkGreen) *this = Color(0.0, 0.5, 0.0);
			else if (c == StandardColor::DarkCyan) *this = Color(0.0, 0.5, 0.5);
			else if (c == StandardColor::DarkRed) *this = Color(0.5, 0.0, 0.0);
			else if (c == StandardColor::DarkMagenta) *this = Color(0.5, 0.0, 0.5);
			else if (c == StandardColor::DarkYellow) *this = Color(0.5, 0.5, 0.0);
			else if (c == StandardColor::Gray) *this = Color(0.75, 0.75, 0.75);
			else if (c == StandardColor::DarkGray) *this = Color(0.5, 0.5, 0.5);
			else if (c == StandardColor::Blue) *this = Color(0.0, 0.0, 1.0);
			else if (c == StandardColor::Green) *this = Color(0.0, 1.0, 0.0);
			else if (c == StandardColor::Cyan) *this = Color(0.0, 1.0, 1.0);
			else if (c == StandardColor::Red) *this = Color(1.0, 0.0, 0.0);
			else if (c == StandardColor::Magenta) *this = Color(1.0, 0.0, 1.0);
			else if (c == StandardColor::Yellow) *this = Color(1.0, 1.0, 0.0);
			else if (c == StandardColor::White) *this = Color(1.0, 1.0, 1.0);
			else *this = Color(0.0, 0.0, 0.0, 0.0);
		}
		Color::Color(CGAColor c) noexcept
		{
			if (c == CGAColor::Black) *this = UI::Color(0x00, 0x00, 0x00);
			else if (c == CGAColor::Blue) *this = UI::Color(0x00, 0x00, 0xAA);
			else if (c == CGAColor::Green) *this = UI::Color(0x00, 0xAA, 0x00);
			else if (c == CGAColor::Cyan) *this = UI::Color(0x00, 0xAA, 0xAA);
			else if (c == CGAColor::Red) *this = UI::Color(0xAA, 0x00, 0x00);
			else if (c == CGAColor::Magenta) *this = UI::Color(0xAA, 0x00, 0xAA);
			else if (c == CGAColor::Brown) *this = UI::Color(0xAA, 0x55, 0x00);
			else if (c == CGAColor::LightGray) *this = UI::Color(0xAA, 0xAA, 0xAA);
			else if (c == CGAColor::DarkGray) *this = UI::Color(0x55, 0x55, 0x55);
			else if (c == CGAColor::BrightBlue) *this = UI::Color(0x55, 0x55, 0xFF);
			else if (c == CGAColor::BrightGreen) *this = UI::Color(0x55, 0xFF, 0x55);
			else if (c == CGAColor::BrightCyan) *this = UI::Color(0x55, 0xFF, 0xFF);
			else if (c == CGAColor::BrightRed) *this = UI::Color(0xFF, 0x55, 0x55);
			else if (c == CGAColor::BrightMagenta) *this = UI::Color(0xFF, 0x55, 0xFF);
			else if (c == CGAColor::Yellow) *this = UI::Color(0xFF, 0xFF, 0x55);
			else if (c == CGAColor::White) *this = UI::Color(0xFF, 0xFF, 0xFF);
			else *this = Color(0.0, 0.0, 0.0, 0.0);
		}
		Color & Color::operator += (const Color & a) noexcept { x += a.x; y += a.y; z += a.z; w += a.w; return *this; }
		Color & Color::operator -= (const Color & a) noexcept { x -= a.x; y -= a.y; z -= a.z; w -= a.w; return *this; }
		Color & Color::operator *= (double a) noexcept { x *= a; y *= a; z *= a; w *= a; return *this; }
		Color & Color::operator /= (double a) noexcept { x /= a; y /= a; z /= a; w /= a; return *this; }
		Color Color::operator - (void) const noexcept { return Color(-x, -y, -z, -w); }
		double & Color::operator [] (int i) noexcept { return c[i]; }
		const double & Color::operator [] (int i) const noexcept { return c[i]; }
		Color::operator UI::Color(void) const noexcept { return UI::Color(saturate(x), saturate(y), saturate(z), saturate(w)); }

		bool operator == (const Color & a, const Color & b) noexcept { for (int i = 0; i < 4; i++) if (a.c[i] != b.c[i]) return false; return true; }
		bool operator != (const Color & a, const Color & b) noexcept { for (int i = 0; i < 4; i++) if (a.c[i] != b.c[i]) return true; return false; }
		Color operator + (const Color & a, const Color & b) noexcept { Color result; for (int i = 0; i < 4; i++) result.c[i] = a.c[i] + b.c[i]; return result; }
		Color operator - (const Color & a, const Color & b) noexcept { Color result; for (int i = 0; i < 4; i++) result.c[i] = a.c[i] - b.c[i]; return result; }
		Color operator * (const Color & a, double b) noexcept { Color result; for (int i = 0; i < 4; i++) result.c[i] = a.c[i] * b; return result; }
		Color operator * (double b, const Color & a) noexcept { Color result; for (int i = 0; i < 4; i++) result.c[i] = a.c[i] * b; return result; }
		Color operator * (const Color & a, const Color & b) noexcept { Color result; for (int i = 0; i < 4; i++) result.c[i] = a.c[i] * b.c[i]; return result; }
		Color operator / (const Color & a, double b) noexcept { Color result; for (int i = 0; i < 4; i++) result.c[i] = a.c[i] / b; return result; }

		ColorHSV::ColorHSV(void) noexcept {}
		ColorHSV::ColorHSV(double _h, double _s, double _v, double _a) noexcept : h(_h), s(_s), v(_v), a(_a) {}
		ColorHSV::ColorHSV(const Color & c) noexcept
		{
			a = c.w;
			double x = max(c.x, max(c.y, c.z));
			double m = min(c.x, min(c.y, c.z));
			if (x == m) h = 0.0;
			else if ((x == c.x) && (c.y >= c.z)) h = (c.y - c.z) / (x - m) * ENGINE_PI / 3.0;
			else if ((x == c.x) && (c.y < c.z)) h = (c.y - c.z) / (x - m) * ENGINE_PI / 3.0 + 2.0 * ENGINE_PI;
			else if (x == c.y) h = (c.z - c.x) / (x - m) * ENGINE_PI / 3.0 + 2.0 * ENGINE_PI / 3.0;
			else h = (c.x - c.y) / (x - m) * ENGINE_PI / 3.0 + 4.0 * ENGINE_PI / 3.0;
			if (x == 0.0) s = 0.0;
			else s = 1.0 - m / x;
			v = x;
		}
		ColorHSV::ColorHSV(UI::Color c) noexcept : ColorHSV(Color(c)) {}
		ColorHSV::operator UI::Color(void) const noexcept
		{
			Color c;
			int Hi = int(3.0 * h / ENGINE_PI);
			double Hr = (h - Hi * ENGINE_PI / 3.0) * 3.0 / ENGINE_PI;
			double Vmin = (1.0 - s) * v;
			double shift = (v - Vmin) * Hr;
			double Vinc = Vmin + shift;
			double Vdec = v - shift;
			if (Hi == 0) c = Color(v, Vinc, Vmin);
			else if (Hi == 1) c = Color(Vdec, v, Vmin);
			else if (Hi == 2) c = Color(Vmin, v, Vinc);
			else if (Hi == 3) c = Color(Vmin, Vdec, v);
			else if (Hi == 4) c = Color(Vinc, Vmin, v);
			else if (Hi == 5) c = Color(v, Vmin, Vdec);
			c.w = a;
			return c;
		}
		ColorHSV::operator Color(void) const noexcept
		{
			Color c;
			int Hi = int(3.0 * h / ENGINE_PI);
			double Hr = (h - Hi * ENGINE_PI / 3.0) * 3.0 / ENGINE_PI;
			double Vmin = (1.0 - s) * v;
			double shift = (v - Vmin) * Hr;
			double Vinc = Vmin + shift;
			double Vdec = v - shift;
			if (Hi == 0) c = Color(v, Vinc, Vmin);
			else if (Hi == 1) c = Color(Vdec, v, Vmin);
			else if (Hi == 2) c = Color(Vmin, v, Vinc);
			else if (Hi == 3) c = Color(Vmin, Vdec, v);
			else if (Hi == 4) c = Color(Vinc, Vmin, v);
			else if (Hi == 5) c = Color(v, Vmin, Vdec);
			c.w = a;
			return c;
		}
		bool operator==(const ColorHSV & a, const ColorHSV & b) noexcept { return (a.h == b.h) && (a.s == b.s) && (a.v == b.v) && (a.a == b.a); }
		bool operator!=(const ColorHSV & a, const ColorHSV & b) noexcept { return (a.h != b.h) || (a.s != b.s) || (a.v != b.v) || (a.a != b.a); }
		ColorHSV ColorHSV::Rotate(double shift) const noexcept
		{
			auto clr  = ColorHSV(h + shift, s, v, a);
			clr.ClampChannels();
			return clr;
		}
		void ColorHSV::ClampChannels(void) noexcept
		{
			while (h >= 2.0 * ENGINE_PI) h -= 2.0 * ENGINE_PI;
			while (h < 0.0) h += 2.0 * ENGINE_PI;
			s = saturate(s);
			v = saturate(v);
			a = saturate(a);
		}

		ColorHSL::ColorHSL(void) noexcept {}
		ColorHSL::ColorHSL(double _h, double _s, double _l, double _a) noexcept : h(_h), s(_s), l(_l), a(_a) {}
		ColorHSL::ColorHSL(const Color & c) noexcept
		{
			auto hsv = Math::ColorHSV(c);
			h = hsv.h; a = hsv.a;
			auto x = max(max(c.r, c.g), c.b);
			auto m = min(min(c.r, c.g), c.b);
			l = (x + m) / 2.0;
			s = Math::saturate((l == 0 || l == x || l >= 1.0) ? 0.0 : (x - m) / (1.0 - Math::abs(1.0 - x - m)));
		}
		ColorHSL::ColorHSL(const ColorHSV & c) noexcept { Color cc = c; *this = ColorHSL(cc); }
		ColorHSL::ColorHSL(UI::Color c) noexcept { Color cc = c; *this = ColorHSL(cc); }
		ColorHSL::operator UI::Color(void) const noexcept { ColorHSV hsv = *this; return hsv; }
		ColorHSL::operator Color(void) const noexcept { ColorHSV hsv = *this; return hsv; }
		ColorHSL::operator ColorHSV(void) const noexcept
		{
			auto x = (l < 0.5) ? (1.0 + s) * l : (1.0 - s) * l + s;
			auto m = 2.0 * l - x;
			Math::ColorHSV hsv;
			hsv.h = h; hsv.a = a;
			hsv.v = x;
			hsv.s = x ? 1.0 - m / x : 0.0;
			return hsv;
		}
		bool operator==(const ColorHSL & a, const ColorHSL & b) noexcept { return a.h == b.h && a.s == b.s && a.l == b.l && a.a == b.a; }
		bool operator!=(const ColorHSL & a, const ColorHSL & b) noexcept { return a.h != b.h || a.s != b.s || a.l != b.l || a.a != b.a; }
		ColorHSL ColorHSL::Rotate(double shift) const noexcept
		{
			auto clr = ColorHSV(h + shift, s, l, a);
			clr.ClampChannels();
			return clr;
		}
		void ColorHSL::ClampChannels(void) noexcept
		{
			while (h >= 2.0 * ENGINE_PI) h -= 2.0 * ENGINE_PI;
			while (h < 0.0) h += 2.0 * ENGINE_PI;
			s = saturate(s);
			l = saturate(l);
			a = saturate(a);
		}

		ColorCMY::ColorCMY(void) noexcept {}
		ColorCMY::ColorCMY(double _c, double _m, double _y, double _a) noexcept : c(_c), m(_m), y(_m), a(_a) {}
		ColorCMY::ColorCMY(const Color & clr) noexcept : c(1.0 - clr.r), m(1.0 - clr.g), y(1.0 - clr.b), a(clr.a) {}
		ColorCMY::ColorCMY(const ColorHSV & clr) noexcept { Color cc = clr; *this = ColorCMY(cc); }
		ColorCMY::ColorCMY(const ColorHSL & clr) noexcept { Color cc = clr; *this = ColorCMY(cc); }
		ColorCMY::ColorCMY(UI::Color clr) noexcept { Color cc = clr; *this = ColorCMY(cc); }
		ColorCMY::operator UI::Color(void) const noexcept { Color clr = *this; return clr; }
		ColorCMY::operator Color(void) const noexcept { return Color(1.0 - c, 1.0 - m, 1.0 - y, a); }
		ColorCMY::operator ColorHSV(void) const noexcept { Color clr = *this; return clr; }
		ColorCMY::operator ColorHSL(void) const noexcept { Color clr = *this; return clr; }
		bool operator==(const ColorCMY & a, const ColorCMY & b) noexcept { return a.c == b.c && a.m == b.m && a.y == b.y && a.a == b.a; }
		bool operator!=(const ColorCMY & a, const ColorCMY & b) noexcept { return a.c != b.c || a.m != b.m || a.y != b.y || a.a != b.a; }

		ColorCMYK::ColorCMYK(void) noexcept {}
		ColorCMYK::ColorCMYK(double _c, double _m, double _y, double _k, double _a) noexcept : c(_c), m(_m), y(_m), k(_k), a(_a) {}
		ColorCMYK::ColorCMYK(const Color & clr) noexcept { ColorCMY cc = clr; *this = ColorCMYK(cc); }
		ColorCMYK::ColorCMYK(const ColorHSV & clr) noexcept { ColorCMY cc = clr; *this = ColorCMYK(cc); }
		ColorCMYK::ColorCMYK(const ColorHSL & clr) noexcept { ColorCMY cc = clr; *this = ColorCMYK(cc); }
		ColorCMYK::ColorCMYK(const ColorCMY & clr) noexcept
		{
			k = min(min(clr.c, clr.m), clr.y);
			if (k == 1.0) {
				c = m = y = 0.0;
			} else {
				c = (clr.c - k) / (1.0 - k);
				m = (clr.m - k) / (1.0 - k);
				y = (clr.y - k) / (1.0 - k);
			}
			a = clr.a;
		}
		ColorCMYK::ColorCMYK(UI::Color clr) noexcept { ColorCMY cc = clr; *this = ColorCMYK(cc); }
		ColorCMYK::operator UI::Color(void) const noexcept { ColorCMY clr = *this; return clr; }
		ColorCMYK::operator Color(void) const noexcept { ColorCMY clr = *this; return clr; }
		ColorCMYK::operator ColorHSV(void) const noexcept { ColorCMY clr = *this; return clr; }
		ColorCMYK::operator ColorHSL(void) const noexcept { ColorCMY clr = *this; return clr; }
		ColorCMYK::operator ColorCMY(void) const noexcept { return ColorCMY(c * (1.0 - k) + k, m * (1.0 - k) + k, y * (1.0 - k) + k, a); }
		bool operator==(const ColorCMYK & a, const ColorCMYK & b) noexcept { return a.c == b.c && a.m == b.m && a.y == b.y && a.k == b.k && a.a == b.a; }
		bool operator!=(const ColorCMYK & a, const ColorCMYK & b) noexcept { return a.c != b.c || a.m != b.m || a.y != b.y || a.k != b.k || a.a != b.a; }
	}
}