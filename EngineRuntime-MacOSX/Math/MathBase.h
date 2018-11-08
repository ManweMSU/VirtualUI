#pragma once

#include "../EngineBase.h"

#define ENGINE_E	2.71828182845904523536

namespace Engine
{
	namespace Math
	{
		typedef double Real;
		typedef Real real;

		Real inverse(Real x) noexcept;

		Real saturate(Real x) noexcept;
		Real abs(Real x) noexcept;
		
		Real exp(Real x) noexcept;
		Real ln(Real x) noexcept;
		Real sin(Real x) noexcept;
		Real cos(Real x) noexcept;
		Real tg(Real x) noexcept;
		Real ctg(Real x) noexcept;
		Real arcsin(Real x) noexcept;
		Real arccos(Real x) noexcept;
		Real arctg(Real x) noexcept;
		Real arcctg(Real x) noexcept;

		Real sh(Real x) noexcept;
		Real ch(Real x) noexcept;
		Real th(Real x) noexcept;
		Real cth(Real x) noexcept;

		Real sqrt(Real x) noexcept;

		bool IsInfinity(Real x) noexcept;
		bool IsNaN(Real x) noexcept;

		template <class V> V lerp(const V & x, const V & y, Real a) noexcept { return x * (1.0 - a) + y * a; };
		template <class V> V lerp(const V & x, const V & y, const V & z, Real a, Real b) noexcept { return x * (1.0 - a - b) + y * a + z * b; };

		namespace Random
		{
			void Init(void) noexcept;
			bool RandomBoolean(void) noexcept;
			uint8 RandomByte(void) noexcept;
			uint16 RandomWord(void) noexcept;
			uint32 RandomInteger(void) noexcept;
			uint64 RandomLong(void) noexcept;
			float RandomFloat(void) noexcept;
			double RandomDouble(void) noexcept;
		}
	}
}