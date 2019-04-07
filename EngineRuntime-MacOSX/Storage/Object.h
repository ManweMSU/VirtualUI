#pragma once

#include "../Miscellaneous/Reflection.h"

namespace Engine
{
	namespace Reflection
	{
		class BinarySerializer : public ISerializer
		{
			SafePointer< Array<uint8> > _data;
		public:
			BinarySerializer(Streaming::Stream * stream, int length);
			BinarySerializer(const void * data, int length);
			BinarySerializer(void);
			virtual ~BinarySerializer(void) override;

			virtual void SerializeObject(Reflected & obj) override;
			virtual void DeserializeObject(Reflected & obj) override;

			Array<uint8> * GetData(void);
			void SetData(Streaming::Stream * stream, int length);
			void SetData(const void * data, int length);
		};
		void SerializeToBinaryObject(Reflected & object, Streaming::Stream * dest);
		void RestoreFromBinaryObject(Reflected & object, Streaming::Stream * from);
	}
}