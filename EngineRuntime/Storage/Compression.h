#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Storage
	{
		enum class CompressionMethod {
			Huffman = 1,
			LempelZivWelch = 3,
			RunLengthEncoding8bit = 9,
			RunLengthEncoding16bit = 10,
			RunLengthEncoding32bit = 11,
			RunLengthEncoding64bit = 12,
			RunLengthEncoding128bit = 13
		};
		Array<uint8> * Compress(const void * data, int length, CompressionMethod method);
		Array<uint8> * Compress(const Array<uint8> & data, CompressionMethod method);
		Array<uint8> * Decompress(const void * data, int length, CompressionMethod method);
		Array<uint8> * Decompress(const Array<uint8> & data, CompressionMethod method);
	}
}