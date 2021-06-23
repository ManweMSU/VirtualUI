#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Power
	{
		enum class BatteryStatus { NoBattery, Charging, InUse, Unknown };
		enum class Exit { Shutdown, Reboot, Logout };
		enum class Prevent { IdleSystemSleep, IdleDisplaySleep, None };

		BatteryStatus GetBatteryStatus(void);
		double GetBatteryChargeLevel(void);

		void PreventIdleSleep(Prevent prevent);

		bool ExitSystem(Exit exit = Exit::Shutdown, bool forced = false);
		bool SuspendSystem(bool hibernate = false, bool allow_wakeup = true);
	}
}