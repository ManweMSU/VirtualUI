#include "Color.h"

namespace Engine
{
	namespace Math
	{
		Color::Color(void) noexcept {}
		Color::Color(const double * source) noexcept : Vector4(source) {}
		Color::Color(double r, double g, double b, double a) noexcept : Vector4(r, g, b, a) {}
		Color::Color(UI::Color c) noexcept : Vector4(double(c.r) / 255.0, double(c.g) / 255.0, double(c.b) / 255.0, double(c.a) / 255.0) {}
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
	}
}