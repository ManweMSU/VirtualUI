#include "../Interfaces/SystemPower.h"

#include <IOKit/ps/IOPowerSources.h>
#include <IOKit/ps/IOPSKeys.h>
#include <IOKit/pwr_mgt/IOPMLib.h>
#include <CoreServices/CoreServices.h>
#include <Carbon/Carbon.h>

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
		bool _CoreSendSystemEvent(AEEventID event)
		{
			ProcessSerialNumber system_process = { 0, kSystemProcess };
			AEAddressDesc address_desc;
			if (AECreateDesc(typeProcessSerialNumber, &system_process, sizeof(system_process), &address_desc) != noErr) return false;
			AppleEvent event_send = { typeNull, 0 };
			if (AECreateAppleEvent(kCoreEventClass, event, &address_desc, kAutoGenerateReturnID, kAnyTransactionID, &event_send) != noErr) {
				AEDisposeDesc(&address_desc);
				return false;
			}
			AEDisposeDesc(&address_desc);
			AppleEvent event_reply = { typeNull, 0 };
			if (AESend(&event_send, &event_reply, kAENoReply, kAENormalPriority, kAEDefaultTimeout, 0, 0) != noErr) {
				AEDisposeDesc(&event_send);
				return false;
			}
			AEDisposeDesc(&event_send);
			AEDisposeDesc(&event_reply);
			return true;
		}
		bool ExitSystem(Exit exit, bool forced)
		{
			if (exit == Exit::Shutdown) return _CoreSendSystemEvent(kAEShutDown);
			else if (exit == Exit::Reboot) return _CoreSendSystemEvent(kAERestart);
			else if (exit == Exit::Logout) return _CoreSendSystemEvent(kAEReallyLogOut);
			else return false;
		}
		bool SuspendSystem(bool hibernate, bool allow_wakeup)
		{
			if (hibernate) return false;
			else return _CoreSendSystemEvent(kAESleep);
		}
	}
}