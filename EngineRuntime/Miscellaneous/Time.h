#pragma once

#include "../EngineBase.h"

namespace Engine
{
	constexpr uint32 RegularMonthLength[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
	constexpr uint32 OddMonthLength[] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	class Time
	{
	public:
		uint64 Ticks;

		Time(void);
		Time(uint64 ticks);
		Time(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 minute, uint32 second, uint32 millisecond);
		Time(uint32 hour, uint32 minute, uint32 second, uint32 millisecond);

		bool friend operator == (Time a, Time b);
		bool friend operator != (Time a, Time b);
		bool friend operator < (Time a, Time b);
		bool friend operator > (Time a, Time b);
		bool friend operator <= (Time a, Time b);
		bool friend operator >= (Time a, Time b);

		Time friend operator + (Time a, Time b);
		Time friend operator - (Time a, Time b);

		operator uint64(void) const;
		Time & operator += (Time a);
		Time & operator -= (Time a);

		void GetDate(uint32 & year, uint32 & month, uint32 & day) const;
		uint32 GetYear(void) const;
		uint32 GetMonth(void) const;
		uint32 GetDay(void) const;
		uint32 GetHour(void) const;
		uint32 GetMinute(void) const;
		uint32 GetSecond(void) const;
		uint32 GetMillisecond(void) const;

		string ToString(void) const;
		string ToShortString(void) const;

		uint64 ToWindowsTime(void) const;
		uint64 ToUnixTime(void) const;
		static Time FromWindowsTime(uint64 time);
		static Time FromUnixTime(uint64 time);

		static bool IsYearOdd(uint32 year);
		static Time GetCurrentTime(void);

		Time ToLocal(void) const;
		Time ToUniversal(void) const;

		int DayOfWeek(void) const;
	};
}