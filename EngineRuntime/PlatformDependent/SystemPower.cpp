#include "../Interfaces/SystemPower.h"

#include <Windows.h>
#include <powrprof.h>

#pragma comment(lib, "PowrProf.lib")

namespace Engine
{
	namespace Power
	{
		BatteryStatus GetBatteryStatus(void)
		{
			SYSTEM_POWER_STATUS status;
			if (GetSystemPowerStatus(&status)) {
				if (status.BatteryFlag == 0xFF) return BatteryStatus::Unknown;
				else if (status.BatteryFlag & 0x80) return BatteryStatus::NoBattery;
				else if (status.BatteryFlag & 0x08) return BatteryStatus::Charging;
				else return BatteryStatus::InUse;
			} else return BatteryStatus::Unknown;
		}
		double GetBatteryChargeLevel(void)
		{
			SYSTEM_POWER_STATUS status;
			if (GetSystemPowerStatus(&status)) {
				if (status.BatteryLifePercent == 0xFF) return -1.0;
				else return double(status.BatteryLifePercent) / 100.0;
			} else return -1.0;
		}
		void PreventIdleSleep(Prevent prevent)
		{
			if (prevent == Prevent::IdleSystemSleep) SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED);
			else if (prevent == Prevent::IdleDisplaySleep) SetThreadExecutionState(ES_CONTINUOUS | ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED);
			else if (prevent == Prevent::None) SetThreadExecutionState(ES_CONTINUOUS);
		}
		bool ExitSystem(Exit exit, bool forced)
		{
			HANDLE token;
			TOKEN_PRIVILEGES privileges;
			privileges.PrivilegeCount = 1;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return false;
			if (!LookupPrivilegeValueW(0, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid)) {
				CloseHandle(token);
				return false;
			}
			if (!AdjustTokenPrivileges(token, FALSE, &privileges, 1, 0, 0)) {
				CloseHandle(token);
				return false;
			}
			if (GetLastError() != ERROR_SUCCESS) {
				CloseHandle(token);
				return false;
			}
			CloseHandle(token);
			DWORD flags = 0;
			if (exit == Exit::Shutdown) flags = EWX_POWEROFF;
			else if (exit == Exit::Reboot) flags = EWX_REBOOT;
			else if (exit == Exit::Logout) flags = EWX_LOGOFF;
			if (forced) flags |= EWX_FORCE;
			if (!ExitWindowsEx(flags, SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE)) return false;
			return true;
		}
		bool SuspendSystem(bool hibernate, bool allow_wakeup)
		{
			HANDLE token;
			TOKEN_PRIVILEGES privileges;
			privileges.PrivilegeCount = 1;
			if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) return false;
			if (!LookupPrivilegeValueW(0, SE_SHUTDOWN_NAME, &privileges.Privileges[0].Luid)) {
				CloseHandle(token);
				return false;
			}
			if (!AdjustTokenPrivileges(token, FALSE, &privileges, 1, 0, 0)) {
				CloseHandle(token);
				return false;
			}
			if (GetLastError() != ERROR_SUCCESS) {
				CloseHandle(token);
				return false;
			}
			CloseHandle(token);
			if (!SetSuspendState(hibernate, 0, !allow_wakeup)) return false;
			return true;
		}
	}
}