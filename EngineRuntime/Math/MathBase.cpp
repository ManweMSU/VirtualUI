#include "MathBase.h"

#include <math.h>

namespace Engine
{
	namespace Math
	{
		Real inverse(Real x) noexcept { return 1.0 / x; }
		Real saturate(Real x) noexcept { return x < 0.0 ? 0.0 : (x > 1.0 ? 1.0 : x); }
		Real abs(Real x) noexcept { return x < 0.0 ? -x : x; }
		Real exp(Real x) noexcept { return ::exp(x); }
		Real ln(Real x) noexcept { return ::log(x); }
		Real sin(Real x) noexcept { return ::sin(x); }
		Real cos(Real x) noexcept { return ::cos(x); }
		Real tg(Real x) noexcept { return ::tan(x); }
		Real ctg(Real x) noexcept { return 1.0 / ::tan(x); }
		Real arcsin(Real x) noexcept { return ::asin(x); }
		Real arccos(Real x) noexcept { return ::acos(x); }
		Real arctg(Real x) noexcept { return ::atan(x); }
		Real arcctg(Real x) noexcept { return ::atan(1.0 / x); }
		Real sh(Real x) noexcept { return (exp(x) - exp(-x)) / 2.0; }
		Real ch(Real x) noexcept { return (exp(x) + exp(-x)) / 2.0; }
		Real th(Real x) noexcept { Real ep = exp(x), en = exp(-x); return (ep + en) / (ep - en); }
		Real cth(Real x) noexcept { Real ep = exp(x), en = exp(-x); return (ep - en) / (ep + en); }
		Real sqrt(Real x) noexcept { return ::sqrt(x); }
		bool IsInfinity(Real x) noexcept
		{
			auto & y = reinterpret_cast<uint64 &>(x);
			int e = (y & 0x7FF0000000000000) >> 52;
			y &= 0x000FFFFFFFFFFFFF;
			return (e == 0x7FF && y == 0);
		}
		bool IsNaN(Real x) noexcept
		{
			auto & y = reinterpret_cast<uint64 &>(x);
			int e = (y & 0x7FF0000000000000) >> 52;
			y &= 0x000FFFFFFFFFFFFF;
			return (e == 0x7FF && y != 0);
		}
		namespace Random
		{
			void Init(void) noexcept { srand(GetTimerValue()); }
			bool RandomBoolean(void) noexcept { return (rand() & 1) != 0; }
			uint8 RandomByte(void) noexcept { return uint8(rand() & 0xFF); }
			uint16 RandomWord(void) noexcept { return uint16(rand()) | (RandomBoolean() ? 0x8000 : 0); }
			uint32 RandomInteger(void) noexcept { return uint32(rand()) | (uint32(rand()) << 15) | (uint32(rand()) << 30); }
			uint64 RandomLong(void) noexcept { return uint64(rand()) | (uint64(rand()) << 15) | (uint64(rand()) << 30) | (uint64(rand()) << 45) | (uint64(rand()) << 60); }
			float RandomFloat(void) noexcept
			{
				uint32 y = RandomInteger();
				uint32 e = 127;
				y &= 0x007FFFFF;
				y |= e << 23;
				return reinterpret_cast<float &>(y) - 1.0f;
			}
			double RandomDouble(void) noexcept
			{
				uint64 y = RandomLong();
				uint64 e = 1023;
				y &= 0x000FFFFFFFFFFFFF;
				y |= e << 52;
				return reinterpret_cast<double &>(y) - 1.0;
			}
		}
	}
}
