#pragma once

#include "../EngineBase.h"
#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Clipboard
	{
		enum class Format { Text, Image };
		bool IsFormatAvailable(Format format);
		bool GetData(string & value);
		bool GetData(Codec::Frame ** value);
		bool SetData(const string & value);
		bool SetData(Codec::Frame * value);
	}
}