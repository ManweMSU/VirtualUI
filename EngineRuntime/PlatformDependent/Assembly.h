#pragma once

#include "../Streaming.h"
#include "../Storage/StringTable.h"

namespace Engine
{
	namespace Assembly
	{
		extern string CurrentLocale;

		string GetCurrentUserLocale(void);
		Streaming::Stream * QueryResource(const widechar * identifier);
		Streaming::Stream * QueryLocalizedResource(const widechar * identifier);

		void SetLocalizedCommonStrings(Storage::StringTable * table);
		Storage::StringTable * GetLocalizedCommonStrings(void);
		const widechar * GetLocalizedCommonString(int ID, const widechar * alternate = L"");
	}
}