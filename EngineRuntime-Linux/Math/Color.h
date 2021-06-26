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

		class Color : public Vector4
		{
		public:
			Color(void) noexcept;
			Color(const double * source) noexcept;
			Color(double r, double g, double b, double a = 1.0) noexcept;
			Color(UI::Color c) noexcept;
			Color(StandardColor c) noexcept;
			Color(CGAColor c) noexcept;
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
		typedef Color ColorRGB;
		class ColorHSV
		{
		public:
			double h, s, v, a;
			ColorHSV(void) noexcept;
			ColorHSV(double _h, double _s, double _v, double _a = 1.0) noexcept;
			ColorHSV(const Color & c) noexcept;
			ColorHSV(UI::Color c) noexcept;
			operator UI::Color(void) const noexcept;
			operator Color(void) const noexcept;
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
			ColorHSL(const Color & c) noexcept;
			ColorHSL(const ColorHSV & c) noexcept;
			ColorHSL(UI::Color c) noexcept;
			operator UI::Color(void) const noexcept;
			operator Color(void) const noexcept;
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
			ColorCMY(const Color & c) noexcept;
			ColorCMY(const ColorHSV & c) noexcept;
			ColorCMY(const ColorHSL & c) noexcept;
			ColorCMY(UI::Color c) noexcept;
			operator UI::Color(void) const noexcept;
			operator Color(void) const noexcept;
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
			ColorCMYK(const Color & c) noexcept;
			ColorCMYK(const ColorHSV & c) noexcept;
			ColorCMYK(const ColorHSL & c) noexcept;
			ColorCMYK(const ColorCMY & c) noexcept;
			ColorCMYK(UI::Color c) noexcept;
			operator UI::Color(void) const noexcept;
			operator Color(void) const noexcept;
			operator ColorHSV(void) const noexcept;
			operator ColorHSL(void) const noexcept;
			operator ColorCMY(void) const noexcept;
			friend bool operator == (const ColorCMYK & a, const ColorCMYK & b) noexcept;
			friend bool operator != (const ColorCMYK & a, const ColorCMYK & b) noexcept;
		};
	}
}