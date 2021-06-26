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
			bool throw_on_syntax_error;
		public:
			JsonSerializer(Streaming::Stream * stream);
			JsonSerializer(Streaming::Stream * stream, bool throw_on_syntax);
			virtual ~JsonSerializer(void) override;

			virtual void SerializeObject(Reflected & obj) override;
			virtual void DeserializeObject(Reflected & obj) override;
		};
	}
	string ConvertToBase64(const void * data, int length);
	Array<uint8> * ConvertFromBase64(const string & data);
}