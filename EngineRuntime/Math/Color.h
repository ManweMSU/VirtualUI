#pragma once

#include "Vector.h"
#include "../Graphics/GraphicsBase.h"

namespace Engine
{
	namespace Math
	{
		enum class StandardColor {
			Black = 0, DarkBlue = 1, DarkGreen = 2, DarkCyan = 3, DarkRed = 4, DarkMagenta = 5, DarkYellow = 6,
			Gray = 7, DarkGray = 8, Blue = 9, Green = 10, Cyan = 11, Red = 12, Magenta = 13, Yellow = 14, White = 15,
			Null = -1
		};
		enum class CGAColor {
			Black = 0, Blue = 1, Green = 2, Cyan = 3, Red = 4, Magenta = 5, Brown = 6,
			LightGray = 7, DarkGray = 8, BrightBlue = 9, BrightGreen = 10, BrightCyan = 11, BrightRed = 12, BrightMagenta = 13, Yellow = 14, White = 15
		};

		class ColorF : public Vector4
		{
		public:
			ColorF(void) noexcept;
			ColorF(const double * source) noexcept;
			ColorF(double r, double g, double b, double a = 1.0) noexcept;
			ColorF(Color c) noexcept;
			ColorF(StandardColor c) noexcept;
			ColorF(CGAColor c) noexcept;
			ColorF & operator += (const ColorF & a) noexcept;
			ColorF & operator -= (const ColorF & a) noexcept;
			ColorF & operator *= (double a) noexcept;
			ColorF & operator /= (double a) noexcept;
			ColorF operator - (void) const noexcept;
			double & operator [] (int i) noexcept;
			const double & operator [] (int i) const noexcept;
			operator Color (void) const noexcept;

			friend bool operator == (const ColorF & a, const ColorF & b) noexcept;
			friend bool operator != (const ColorF & a, const ColorF & b) noexcept;
			friend ColorF operator + (const ColorF & a, const ColorF & b) noexcept;
			friend ColorF operator - (const ColorF & a, const ColorF & b) noexcept;
			friend ColorF operator * (const ColorF & a, double b) noexcept;
			friend ColorF operator * (double b, const ColorF & a) noexcept;
			friend ColorF operator * (const ColorF & a, const ColorF & b) noexcept;
			friend ColorF operator / (const ColorF & a, double b) noexcept;
		};
		typedef ColorF ColorRGB;
		class ColorHSV
		{
		public:
			double h, s, v, a;
			ColorHSV(void) noexcept;
			ColorHSV(double _h, double _s, double _v, double _a = 1.0) noexcept;
			ColorHSV(const ColorF & c) noexcept;
			ColorHSV(Color c) noexcept;
			operator Color(void) const noexcept;
			operator ColorF(void) const noexcept;
			friend bool operator == (const ColorHSV & a, const ColorHSV & b) noexcept;
			friend bool operator != (const ColorHSV & a, const ColorHSV & b) noexcept;
			ColorHSV Rotate(double shift) const noexcept;
			void ClampChannels(void) noexcept;
		};
		typedef ColorHSV ColorHSB;
		class ColorHSL
		{
		public:
			double h, s, l, a;
			ColorHSL(void) noexcept;
			ColorHSL(double _h, double _s, double _l, double _a = 1.0) noexcept;
			ColorHSL(const ColorF & c) noexcept;
			ColorHSL(const ColorHSV & c) noexcept;
			ColorHSL(Color c) noexcept;
			operator Color(void) const noexcept;
			operator ColorF(void) const noexcept;
			operator ColorHSV(void) const noexcept;
			friend bool operator == (const ColorHSL & a, const ColorHSL & b) noexcept;
			friend bool operator != (const ColorHSL & a, const ColorHSL & b) noexcept;
			ColorHSL Rotate(double shift) const noexcept;
			void ClampChannels(void) noexcept;
		};
		typedef ColorHSL ColorHSI;
		class ColorCMY
		{
		public:
			double c, m, y, a;
			ColorCMY(void) noexcept;
			ColorCMY(double _c, double _m, double _y, double _a = 1.0) noexcept;
			ColorCMY(const ColorF & c) noexcept;
			ColorCMY(const ColorHSV & c) noexcept;
			ColorCMY(const ColorHSL & c) noexcept;
			ColorCMY(Color c) noexcept;
			operator Color(void) const noexcept;
			operator ColorF(void) const noexcept;
			operator ColorHSV(void) const noexcept;
			operator ColorHSL(void) const noexcept;
			friend bool operator == (const ColorCMY & a, const ColorCMY & b) noexcept;
			friend bool operator != (const ColorCMY & a, const ColorCMY & b) noexcept;
		};
		class ColorCMYK
		{
		public:
			double c, m, y, k, a;
			ColorCMYK(void) noexcept;
			ColorCMYK(double _c, double _m, double _y, double _k, double _a = 1.0) noexcept;
			ColorCMYK(const ColorF & c) noexcept;
			ColorCMYK(const ColorHSV & c) noexcept;
			ColorCMYK(const ColorHSL & c) noexcept;
			ColorCMYK(const ColorCMY & c) noexcept;
			ColorCMYK(Color c) noexcept;
			operator Color(void) const noexcept;
			operator ColorF(void) const noexcept;
			operator ColorHSV(void) const noexcept;
			operator ColorHSL(void) const noexcept;
			operator ColorCMY(void) const noexcept;
			friend bool operator == (const ColorCMYK & a, const ColorCMYK & b) noexcept;
			friend bool operator != (const ColorCMYK & a, const ColorCMYK & b) noexcept;
		};
	}
}