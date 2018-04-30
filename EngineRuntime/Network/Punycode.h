#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Network
	{
		string UnicodeToPunycode(const string & text);
		string PunycodeToUnicode(const string & text);
		string DomainNameToPunycode(const string & text);
		string DomainNameToUnicode(const string & text);
	}
}