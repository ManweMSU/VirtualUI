#pragma once

#include "../EngineBase.h"
#include "../ImageCodec/CodecBase.h"

namespace Engine
{
	namespace Clipboard
	{
		enum class Format { Text, Image, RichText, Custom };
		bool IsFormatAvailable(Format format);
		bool GetData(string & value);
		bool GetData(Codec::Frame ** value);
		bool GetData(string & value, bool attributed);
		bool GetData(const string & subclass, Array<uint8> ** value);
		bool SetData(const string & value);
		bool SetData(Codec::Frame * value);
		bool SetData(const string & plain, const string & attributed);
		bool SetData(const string & subclass, const void * data, int size);
		string GetCustomSubclass(void);
	}
}