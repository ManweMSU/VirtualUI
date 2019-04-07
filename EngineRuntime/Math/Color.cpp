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
	}
}