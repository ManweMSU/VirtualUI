#include "../Interfaces/SystemPower.h"

#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/pwr_mgt/IOPMLib.h>

namespace Engine
{
	namespace Power
	{
		CFTypeRef _GetDictionaryValue(CFDictionaryRef dict, const char * key)
		{
			CFStringRef key_wrapped = CFStringCreateWithCString(0, key, kCFStringEncodingASCII);
			CFTypeRef result;
			bool present = CFDictionaryGetValueIfPresent(dict, key_wrapped, &result);
			CFRelease(key_wrapped);
			if (present) return result; else return 0;
		}
		bool _StringsAreEqual(CFTypeRef obj, const char * ref)
		{
			CFStringRef ref_wrapped = CFStringCreateWithCString(0, ref, kCFStringEncodingASCII);
			auto result = CFStringCompare(reinterpret_cast<CFStringRef>(obj), ref_wrapped, 0) == kCFCompareEqualTo;
			CFRelease(ref_wrapped);
			return result;
		}
		BatteryStatus GetBatteryStatus(void)
		{
			BatteryStatus result = BatteryStatus::NoBattery;
			CFTypeRef info = IOPSCopyPowerSourcesInfo();
			CFArrayRef array = IOPSCopyPowerSourcesList(info);
			auto count = CFArrayGetCount(array);
			for (int i = 0; i < count; i++) {
				CFDictionaryRef props = IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(array, i));
				auto value = _GetDictionaryValue(props, kIOPSTypeKey);
				if (_StringsAreEqual(value, kIOPSInternalBatteryType)) {
					auto charging = _GetDictionaryValue(props, kIOPSIsChargingKey);
					if (CFBooleanGetValue(reinterpret_cast<CFBooleanRef>(charging))) result = BatteryStatus::Charging;
					else result = BatteryStatus::InUse;
					break;
				}
			}
			CFRelease(array);
			CFRelease(info);
			if (!array || !info) return BatteryStatus::Unknown;
			return result;
		}
		double GetBatteryChargeLevel(void)
		{
			double result = -1.0;
			CFTypeRef info = IOPSCopyPowerSourcesInfo();
			CFArrayRef array = IOPSCopyPowerSourcesList(info);
			auto count = CFArrayGetCount(array);
			for (int i = 0; i < count; i++) {
				CFDictionaryRef props = IOPSGetPowerSourceDescription(info, CFArrayGetValueAtIndex(array, i));
				auto value = _GetDictionaryValue(props, kIOPSTypeKey);
				if (_StringsAreEqual(value, kIOPSInternalBatteryType)) {
					auto current = _GetDictionaryValue(props, kIOPSCurrentCapacityKey);
					auto maximal = _GetDictionaryValue(props, kIOPSMaxCapacityKey);
					int num_current, num_maximal;
					CFNumberGetValue(reinterpret_cast<CFNumberRef>(current), kCFNumberIntType, &num_current);
					CFNumberGetValue(reinterpret_cast<CFNumberRef>(maximal), kCFNumberIntType, &num_maximal);
					result = double(num_current) / double(num_maximal);
					break;
				}
			}
			CFRelease(array);
			CFRelease(info);
			return result;
		}
		IOPMAssertionID assertion = 0;
		bool assertion_set = false;
		void PreventIdleSleep(Prevent prevent)
		{
			if (assertion_set) {
				IOPMAssertionRelease(assertion);
				assertion = 0;
				assertion_set = false;
			}
			CFStringRef desc = CFStringCreateWithCString(0, "Engine Runtime Routine", kCFStringEncodingASCII);
			if (prevent == Prevent::IdleSystemSleep) {
				if (IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleSystemSleep, kIOPMAssertionLevelOn, desc, &assertion) == kIOReturnSuccess)
					assertion_set = true;
			} else if (prevent == Prevent::IdleDisplaySleep) {
				if (IOPMAssertionCreateWithName(kIOPMAssertionTypePreventUserIdleDisplaySleep, kIOPMAssertionLevelOn, desc, &assertion) == kIOReturnSuccess)
					assertion_set = true;
			}
			CFRelease(desc);
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