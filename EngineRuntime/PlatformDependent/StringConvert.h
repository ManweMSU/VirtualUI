#pragma once

namespace Engine
{
	enum class Encoding { ANSI = 0, UTF8 = 1, UTF16 = 2, UTF32 = 3 };

	int MeasureSequenceLength(const void * Source, int SourceLength, Encoding From, Encoding To);
	void ConvertEncoding(void * Dest, const void * Source, int SourceLength, Encoding From, Encoding To);
	int GetBytesPerChar(Encoding encoding);
}