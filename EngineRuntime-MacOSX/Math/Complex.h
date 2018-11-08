#pragma once

#include "MathBase.h"

#define ENGINE_I	(::Engine::Math::Complex::i)

namespace Engine
{
	namespace Math
	{
		class Complex
		{
		public:
			Real re, im;

			static const Complex i;

			Complex(void) noexcept;
			Complex(Real Re) noexcept;
			Complex(Real Re, Real Im) noexcept;
			static Complex FromPolar(Real absolute, Real argument) noexcept;

			Complex friend operator + (const Complex & a, const Complex & b) noexcept;
			Complex friend operator - (const Complex & a, const Complex & b) noexcept;
			Complex friend operator * (const Complex & a, const Complex & b) noexcept;
			Complex friend operator / (const Complex & a, const Complex & b) noexcept;
			Complex & operator += (const Complex & b) noexcept;
			Complex & operator -= (const Complex & b) noexcept;
			Complex & operator *= (const Complex & b) noexcept;
			Complex & operator /= (const Complex & b) noexcept;
			Complex operator - (void) const noexcept;

			bool friend operator == (const Complex & a, const Complex & b) noexcept;
			bool friend operator != (const Complex & a, const Complex & b) noexcept;

			operator string (void) const noexcept;
		};

		typedef Complex complex;

		Real abs(Complex x) noexcept;
		Real arg(Complex x) noexcept;

		Complex exp(Complex x) noexcept;
		Complex ln(Complex x) noexcept;
		Complex sin(Complex x) noexcept;
		Complex cos(Complex x) noexcept;
		Complex tg(Complex x) noexcept;
		Complex ctg(Complex x) noexcept;

		Complex sh(Complex x) noexcept;
		Complex ch(Complex x) noexcept;
		Complex th(Complex x) noexcept;
		Complex cth(Complex x) noexcept;

		Complex sqrt(Complex x) noexcept;

		Complex conjugate(const Complex & x) noexcept;
		Complex inverse(const Complex & x) noexcept;
	}
}