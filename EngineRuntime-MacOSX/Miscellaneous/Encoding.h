#pragma once

namespace Engine
{
	enum class Encoding { Unknown = 0, ANSI = 1, UTF8 = 2, UTF16 = 3, UTF32 = 4 };

	int MeasureSequenceLength(const void * Source, int SourceLength, Encoding From, Encoding To);
	void ConvertEncoding(void * Dest, const void * Source, int SourceLength, Encoding From, Encoding To);
	int GetBytesPerChar(Encoding encoding);
}