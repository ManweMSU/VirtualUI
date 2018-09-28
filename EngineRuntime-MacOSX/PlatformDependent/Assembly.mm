#include "Assembly.h"

#include "CocoaInterop.h"

@import Foundation;

namespace Engine
{
	namespace Assembly
	{
		string CurrentLocale;

		string GetCurrentUserLocale(void)
		{
			return Cocoa::EngineString([[NSLocale currentLocale] languageCode]);
		}
		Streaming::Stream * QueryResource(const widechar * identifier)
		{
			NSBundle * bundle = [NSBundle mainBundle];
			if (!bundle) return 0;
			NSString * name = Cocoa::CocoaString(identifier);
			NSURL * url = [bundle URLForResource: name withExtension: nil];
			if (!url) {
				[name release];
				if (CurrentLocale.Length()) {
					name = Cocoa::CocoaString(string(identifier) + L"-" + CurrentLocale);
					url = [bundle URLForResource: name withExtension: nil];
					[name release];
					[bundle release];
					if (!url) return 0;
				} else {
					[bundle release];
					return 0;
				}
			} else {
				[name release];
				[bundle release];
			}
			string path = Cocoa::EngineString([url path]);
			[url release];
			try {
				SafePointer<Streaming::FileStream> stream = new Streaming::FileStream(path, Streaming::AccessRead, Streaming::OpenExisting);
				stream->Retain();
				return stream;
			} catch (...) { return 0; }
		}
		Streaming::Stream * QueryLocalizedResource(const widechar * identifier)
		{
			NSBundle * bundle = [NSBundle mainBundle];
			if (!bundle) return 0;
			NSString * name = Cocoa::CocoaString(string(identifier) + L"-" + CurrentLocale);
			NSURL * url = [bundle URLForResource: name withExtension: nil];
			if (!url) {
				[name release];
				if (CurrentLocale.Length()) {
					name = Cocoa::CocoaString(identifier);
					url = [bundle URLForResource: name withExtension: nil];
					[name release];
					[bundle release];
					if (!url) return 0;
				} else {
					[bundle release];
					return 0;
				}
			} else {
				[name release];
				[bundle release];
			}
			string path = Cocoa::EngineString([url path]);
			[url release];
			try {
				SafePointer<Streaming::FileStream> stream = new Streaming::FileStream(path, Streaming::AccessRead, Streaming::OpenExisting);
				stream->Retain();
				return stream;
			} catch (...) { return 0; }
		}
	}
}