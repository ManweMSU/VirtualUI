#include "Complex.h"

namespace Engine
{
	namespace Math
	{
		const Complex Complex::i = Complex(0.0, 1.0);

		Complex::Complex(void) noexcept {}
		Complex::Complex(Real Re) noexcept : re(Re), im(0.0) {}
		Complex::Complex(Real Re, Real Im) noexcept : re(Re), im(Im) {}
		Complex Complex::FromPolar(Real absolute, Real argument) noexcept { return Complex(absolute * cos(argument), absolute * sin(argument)); }
		Complex & Complex::operator+=(const Complex & b) noexcept { re += b.re; im += b.im; return *this; }
		Complex & Complex::operator-=(const Complex & b) noexcept { re -= b.re; im -= b.im; return *this; }
		Complex & Complex::operator*=(const Complex & b) noexcept { *this = *this * b; return *this; }
		Complex & Complex::operator/=(const Complex & b) noexcept { *this = *this / b; return *this; }
		Complex Complex::operator-(void) const noexcept { return Complex(-re, -im); }
		Complex::operator string(void) const noexcept { return L"(" + string(re) + L"; " + string(im) + L")"; }
		Complex operator+(const Complex & a, const Complex & b) noexcept { return Complex(a.re + b.re, a.im + b.im); }
		Complex operator-(const Complex & a, const Complex & b) noexcept { return Complex(a.re - b.re, a.im - b.im); }
		Complex operator*(const Complex & a, const Complex & b) noexcept { return Complex(a.re * b.re - a.im * b.im, a.re * b.im + a.im * b.re); }
		Complex operator/(const Complex & a, const Complex & b) noexcept { return a * inverse(b); }
		bool operator==(const Complex & a, const Complex & b) noexcept { return a.re == b.re && a.im == b.im; }
		bool operator!=(const Complex & a, const Complex & b) noexcept { return a.re != b.re || a.im != b.im; }
		Real abs(Complex x) noexcept { return sqrt(x.re * x.re + x.im * x.im); }
		Real arg(Complex x) noexcept { if (x.re == 0.0 && x.im == 0.0) return 0.0; else if (x.im >= 0.0) return arccos(x.re / abs(x)); else return ENGINE_PI + arccos(-x.re / abs(x)); }
		Complex exp(Complex x) noexcept { Real m = exp(x.re), a = x.im; return Complex(m * cos(a), m * sin(a)); }
		Complex ln(Complex x) noexcept { return Complex(ln(abs(x)), arg(x)); }
		Complex sin(Complex x) noexcept { return Complex(sin(x.re) * ch(x.im), cos(x.re) * sh(x.im)); }
		Complex cos(Complex x) noexcept { return Complex(cos(x.re) * ch(x.im), -sin(x.re) * sh(x.im)); }
		Complex tg(Complex x) noexcept { return sin(x) / cos(x); }
		Complex ctg(Complex x) noexcept { return cos(x) / sin(x); }
		Complex sh(Complex x) noexcept { return -ENGINE_I * sin(ENGINE_I * x); }
		Complex ch(Complex x) noexcept { return cos(ENGINE_I * x); }
		Complex th(Complex x) noexcept { return -ENGINE_I * tg(ENGINE_I * x); }
		Complex cth(Complex x) noexcept { return ENGINE_I * ctg(ENGINE_I * x); }
		Complex sqrt(Complex x) noexcept { Real a = arg(x); return x.im >= 0.0 ? Complex::FromPolar(sqrt(abs(x)), a / 2.0) : Complex::FromPolar(sqrt(abs(x)), ENGINE_PI + a / 2.0); }
		Complex conjugate(const Complex & x) noexcept { return Complex(x.re, -x.im); }
		Complex inverse(const Complex & x) noexcept { Complex c = conjugate(x); Real d = x.re * x.re + x.im * x.im; return Complex(c.re / d, c.im / d); }
	}
}