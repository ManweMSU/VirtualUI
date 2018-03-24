#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Clipboard
	{
		enum class Format { Text };
		bool IsFormatAvailable(Format format);
		bool GetData(string & value);
		bool SetData(const string & value);
	}
}