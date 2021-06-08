#include "../Interfaces/Assembly.h"

#include "../Miscellaneous/DynamicString.h"

#include <Windows.h>

namespace Engine
{
	namespace Assembly
	{
		string CurrentLocale;
		SafePointer<Storage::StringTable> LocalizedStrings;

		string GetCurrentUserLocale(void)
		{
			DynamicString locale;
			locale.ReserveLength(LOCALE_NAME_MAX_LENGTH);
			GetLocaleInfoEx(LOCALE_NAME_USER_DEFAULT, LOCALE_SNAME, locale, locale.ReservedLength());
			string loc = locale.ToString();
			return loc.Fragment(0, loc.FindFirst(L'-'));
		}
		Streaming::Stream * QueryResource(const widechar * identifier)
		{
			HMODULE me = GetModuleHandleW(0);
			HRSRC search = FindResourceW(me, identifier, RT_RCDATA);
			if (!search) {
				if (CurrentLocale.Length()) {
					search = FindResourceW(me, string(identifier) + L"-" + CurrentLocale, RT_RCDATA);
				}
				if (!search) return 0;
			}
			HGLOBAL resource = LoadResource(me, search);
			if (!resource) return 0;
			void * data = LockResource(resource);
			if (!data) return 0;
			try {
				SafePointer<Streaming::MemoryStream> stream = new Streaming::MemoryStream(data, SizeofResource(me, search));
				stream->Retain();
				return stream;
			}
			catch (...) { return 0; }
		}
		Streaming::Stream * QueryLocalizedResource(const widechar * identifier)
		{
			HMODULE me = GetModuleHandleW(0);
			HRSRC search = FindResourceW(me, string(identifier) + L"-" + CurrentLocale, RT_RCDATA);
			if (!search) {
				if (CurrentLocale.Length()) {
					search = FindResourceW(me, identifier, RT_RCDATA);
				}
				if (!search) return 0;
			}
			HGLOBAL resource = LoadResource(me, search);
			if (!resource) return 0;
			void * data = LockResource(resource);
			if (!data) return 0;
			try {
				SafePointer<Streaming::MemoryStream> stream = new Streaming::MemoryStream(data, SizeofResource(me, search));
				stream->Retain();
				return stream;
			}
			catch (...) { return 0; }
		}
		void SetLocalizedCommonStrings(Storage::StringTable * table) { LocalizedStrings.SetRetain(table); }
		Storage::StringTable * GetLocalizedCommonStrings(void) { return LocalizedStrings; }
		const widechar * GetLocalizedCommonString(int ID, const widechar * alternate) { if (LocalizedStrings) return LocalizedStrings->GetString(ID); else return alternate; }
	}
}