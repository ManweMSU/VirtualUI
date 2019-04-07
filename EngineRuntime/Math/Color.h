#pragma once

#include "Vector.h"
#include "../UserInterface/ShapeBase.h"

namespace Engine
{
	namespace Math
	{
		class Color : public Vector4
		{
		public:
			Color(void) noexcept;
			Color(const double * source) noexcept;
			Color(double r, double g, double b, double a = 1.0) noexcept;
			Color(UI::Color c) noexcept;
			Color & operator += (const Color & a) noexcept;
			Color & operator -= (const Color & a) noexcept;
			Color & operator *= (double a) noexcept;
			Color & operator /= (double a) noexcept;
			Color operator - (void) const noexcept;
			double & operator [] (int i) noexcept;
			const double & operator [] (int i) const noexcept;
			operator UI::Color (void) const noexcept;

			friend bool operator == (const Color & a, const Color & b) noexcept;
			friend bool operator != (const Color & a, const Color & b) noexcept;
			friend Color operator + (const Color & a, const Color & b) noexcept;
			friend Color operator - (const Color & a, const Color & b) noexcept;
			friend Color operator * (const Color & a, double b) noexcept;
			friend Color operator * (double b, const Color & a) noexcept;
			friend Color operator * (const Color & a, const Color & b) noexcept;
			friend Color operator / (const Color & a, double b) noexcept;
		};
	}
}