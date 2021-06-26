#include "../Interfaces/Assembly.h"

#include <locale.h>

namespace Engine
{
	namespace Assembly
	{
		string CurrentLocale;
		SafePointer<Storage::StringTable> CommonStrings;

		string GetCurrentUserLocale(void) { return string(setlocale(LC_ALL, 0), -1, Encoding::UTF8).Fragment(0, 2); }
		Streaming::Stream * QueryResource(const widechar * identifier, bool local_first)
		{
			auto base = IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + string(identifier).LowerCase();
			try {
				auto name = local_first ? (base + L"." + CurrentLocale + L".ersrc") : (base + L".ersrc");
				return new Streaming::FileStream(name, Streaming::AccessRead, Streaming::OpenExisting);
			} catch (...) {}
			auto name = (!local_first) ? (base + L"." + CurrentLocale + L".ersrc") : (base + L".ersrc");
			return new Streaming::FileStream(name, Streaming::AccessRead, Streaming::OpenExisting);
		}
		Streaming::Stream * QueryResource(const widechar * identifier) { try { return QueryResource(identifier, false); } catch (...) { return 0; } }
		Streaming::Stream * QueryLocalizedResource(const widechar * identifier) { try { return QueryResource(identifier, true); } catch (...) { return 0; } }
		void SetLocalizedCommonStrings(Storage::StringTable * table) { CommonStrings.SetRetain(table); }
		Storage::StringTable * GetLocalizedCommonStrings(void) { return CommonStrings; }
		const widechar * GetLocalizedCommonString(int ID, const widechar * alternate) { if (CommonStrings) return CommonStrings->GetString(ID); else return alternate; }
	}
}