#pragma once

#include "../Miscellaneous/Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		class JsonSerializer : public ISerializer
		{
			SafePointer<Streaming::TextReader> reader;
			SafePointer<Streaming::TextWriter> writer;
		public:
			JsonSerializer(Streaming::Stream * stream);
			virtual ~JsonSerializer(void) override;

			virtual void SerializeObject(Reflected & obj) override;
			virtual void DeserializeObject(Reflected & obj) override;
		};
	}
	string ConvertToBase64(const void * data, int length);
	Array<uint8> * ConvertFromBase64(const string & data);
}