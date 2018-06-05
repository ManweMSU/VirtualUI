#include "Time.h"

namespace Engine
{
	Time::Time(void) {}
	Time::Time(uint64 ticks) : Ticks(ticks) {}
	Time::Time(uint32 year, uint32 month, uint32 day, uint32 hour, uint32 minute, uint32 second, uint32 millisecond)
	{
		if (year < 1) year = 1;
		if (month < 1) month = 1;
		if (day < 1) day = 1;
		uint64 base = millisecond + (second + (minute + hour * 60) * 60) * 1000;
		uint64 days = day - 1;
		if (IsYearOdd(year)) {
			for (uint i = 1; i < month; i++) days += OddMonthLength[i - 1];
		} else {
			for (uint i = 1; i < month; i++) days += RegularMonthLength[i - 1];
		}
		uint32 raw_year = year - 1;
		uint32 cycles = raw_year / 400;
		days += cycles * (365 * 303 + 366 * 97);
		for (uint y = cycles * 400; y < raw_year; y++) days += IsYearOdd(y + 1) ? 366 : 365;
		Ticks = base + days * 1000 * 60 * 60 * 24;
	}
	Time::Time(uint32 hour, uint32 minute, uint32 second, uint32 millisecond) { Ticks = millisecond + (second + (minute + hour * 60) * 60) * 1000; }
	bool operator==(Time a, Time b) { return a.Ticks == b.Ticks; }
	bool operator!=(Time a, Time b) { return a.Ticks != b.Ticks; }
	bool operator<(Time a, Time b) { return a.Ticks < b.Ticks; }
	bool operator>(Time a, Time b) { return a.Ticks > b.Ticks; }
	bool operator<=(Time a, Time b) { return a.Ticks <= b.Ticks; }
	bool operator>=(Time a, Time b) { return a.Ticks >= b.Ticks; }
	Time operator+(Time a, Time b) { return Time(a.Ticks + b.Ticks); }
	Time operator-(Time a, Time b) { return Time(a.Ticks - b.Ticks); }
	Time::operator uint64(void) const { return Ticks; }
	Time & Time::operator+=(Time a) { Ticks += a.Ticks; return *this; }
	Time & Time::operator-=(Time a) { Ticks -= a.Ticks; return *this; }
	void Time::GetDate(uint32 & year, uint32 & month, uint32 & day) const
	{
		year = 1;
		month = 1;
		day = 1;
		uint64 days = Ticks / (1000 * 60 * 60 * 24);
		uint64 cycles = days / (365 * 303 + 366 * 97);
		days -= cycles * (365 * 303 + 366 * 97);
		year += uint32(cycles * 400);
		while (true) {
			uint32 len = IsYearOdd(year) ? 366 : 365;
			if (days <= len) break;
			year++;
			days -= len;
		}
		if (IsYearOdd(year)) {
			while (days > OddMonthLength[month - 1]) {
				days -= OddMonthLength[month - 1];
				month++;
			}
		} else {
			while (days > RegularMonthLength[month - 1]) {
				days -= RegularMonthLength[month - 1];
				month++;
			}
		}
		day += uint32(days);
	}
	uint32 Time::GetYear(void) const { uint32 d, m, y; GetDate(y, m, d); return y; }
	uint32 Time::GetMonth(void) const { uint32 d, m, y; GetDate(y, m, d); return m; }
	uint32 Time::GetDay(void) const { uint32 d, m, y; GetDate(y, m, d); return d; }
	uint32 Time::GetHour(void) const { return (((Ticks / 1000) / 60) / 60) % 24; }
	uint32 Time::GetMinute(void) const { return ((Ticks / 1000) / 60) % 60; }
	uint32 Time::GetSecond(void) const { return (Ticks / 1000) % 60; }
	uint32 Time::GetMillisecond(void) const { return Ticks % 1000; }
	string Time::ToString(void) const
	{
		return string(GetDay(), L"0123456789", 2) + L"." + string(GetMonth(), L"0123456789", 2) + L"." + string(GetYear(), L"0123456789", 4) +
			L" " + string(GetHour(), L"0123456789", 2) + L":" + string(GetMinute(), L"0123456789", 2) + L":" + string(GetSecond(), L"0123456789", 2);
	}
	string Time::ToShortString(void) const
	{
		return string(GetHour(), L"0123456789", 2) + L":" + string(GetMinute(), L"0123456789", 2) + L":" + string(GetSecond(), L"0123456789", 2);
	}
	uint64 Time::ToWindowsTime(void) const { return (Ticks - Time(1601, 1, 1, 0, 0, 0, 0).Ticks) * 10000; }
	uint64 Time::ToUnixTime(void) const { return (Ticks - Time(1970, 1, 1, 0, 0, 0, 0).Ticks) / 1000; }
	Time Time::FromWindowsTime(uint64 time) { return time / 10000 + Time(1601, 1, 1, 0, 0, 0, 0).Ticks; }
	Time Time::FromUnixTime(uint64 time) { return time * 1000 + Time(1970, 1, 1, 0, 0, 0, 0).Ticks; }
	bool Time::IsYearOdd(uint32 year)
	{
		if (year % 400 == 0) return true;
		else if (year % 100 == 0) return false;
		else if (year % 4 == 0) return true;
		else return false;
	}
	Time Time::GetCurrentTime(void)
	{
#ifdef ENGINE_WINDOWS
		return FromWindowsTime(GetNativeTime());
#else
#ifdef ENGINE_UNIX
		return FromUnixTime(GetNativeTime());
#else
		return 0;
#endif
#endif
	}
	Time Time::ToLocal(void) const
	{
#ifdef ENGINE_WINDOWS
		return FromWindowsTime(TimeUniversalToLocal(ToWindowsTime()));
#else
#ifdef ENGINE_UNIX
		return FromUnixTime(TimeUniversalToLocal(ToUnixTime()));
#else
		return 0;
#endif
#endif
	}
	Time Time::ToUniversal(void) const
	{
#ifdef ENGINE_WINDOWS
		return FromWindowsTime(TimeLocalToUniversal(ToWindowsTime()));
#else
#ifdef ENGINE_UNIX
		return FromUnixTime(TimeLocalToUniversal(ToUnixTime()));
#else
		return 0;
#endif
#endif
	}
	int Time::DayOfWeek(void) const
	{
		uint64 days = Ticks / (1000 * 60 * 60 * 24);
		return int(days % 7);
	}
}