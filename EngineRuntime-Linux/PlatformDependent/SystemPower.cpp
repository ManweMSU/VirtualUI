#include "../Interfaces/SystemPower.h"

namespace Engine
{
	namespace Power
	{
		BatteryStatus GetBatteryStatus(void)
		{
			// TODO: IMPLEMENT
			return BatteryStatus::Unknown;
		}
		double GetBatteryChargeLevel(void)
		{
			// TODO: IMPLEMENT
			return 0.0;
		}
		void PreventIdleSleep(Prevent prevent)
		{
			// TODO: IMPLEMENT
		}
		bool ExitSystem(Exit exit, bool forced)
		{
			// TODO: IMPLEMENT
			return false;
		}
		bool SuspendSystem(bool hibernate, bool allow_wakeup)
		{
			// TODO: IMPLEMENT
			return false;
		}
	}
}