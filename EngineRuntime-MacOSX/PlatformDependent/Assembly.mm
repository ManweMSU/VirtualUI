#include "../Interfaces/Assembly.h"

#include "CocoaInterop.h"

@import Foundation;

using namespace Engine::Streaming;

namespace Engine
{
	namespace Assembly
	{
		string CurrentLocale;
		SafePointer<Storage::StringTable> LocalizedStrings;

		string GetCurrentUserLocale(void)
		{
			return Cocoa::EngineString([[NSLocale preferredLanguages] objectAtIndex: 0]).Fragment(0, 2);
		}
		Streaming::Stream * QueryResource(const widechar * identifier)
		{
			NSBundle * bundle = [NSBundle mainBundle];
			if (!bundle) return 0;
			NSString * name = Cocoa::CocoaString(identifier);
			string path;
			NSURL * url;
			@autoreleasepool {
				url = [bundle URLForResource: name withExtension: nil];
				if (!url) {
					[name release];
					if (CurrentLocale.Length()) {
						name = Cocoa::CocoaString(string(identifier) + L"-" + CurrentLocale);
						url = [bundle URLForResource: name withExtension: nil];
						[name release];
						if (!url) return 0;
					} else {
						return 0;
					}
				} else {
					[name release];
				}
				path = Cocoa::EngineString([url path]);
			}
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
			string path;
			NSURL * url;
			@autoreleasepool {
				url = [bundle URLForResource: name withExtension: nil];
				if (!url) {
					[name release];
					if (CurrentLocale.Length()) {
						name = Cocoa::CocoaString(identifier);
						url = [bundle URLForResource: name withExtension: nil];
						[name release];
						if (!url) return 0;
					} else {
						return 0;
					}
				} else {
					[name release];
				}
				path = Cocoa::EngineString([url path]);
			}
			try {
				SafePointer<Streaming::FileStream> stream = new Streaming::FileStream(path, Streaming::AccessRead, Streaming::OpenExisting);
				stream->Retain();
				return stream;
			} catch (...) { return 0; }
		}
		void SetLocalizedCommonStrings(Storage::StringTable * table) { LocalizedStrings.SetRetain(table); }
		Storage::StringTable * GetLocalizedCommonStrings(void) { return LocalizedStrings; }
		const widechar * GetLocalizedCommonString(int ID, const widechar * alternate) { if (LocalizedStrings) return LocalizedStrings->GetString(ID); else return alternate; }
	}
}