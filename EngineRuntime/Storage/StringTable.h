#pragma once

#include "../Streaming.h"

namespace Engine
{
	namespace Storage
	{
		class StringTable : public Object
		{
			struct StringEntity
			{
				int ID;
				string String;
			};
			Array<StringEntity> strings;
		public:
			StringTable(void);
			StringTable(Streaming::Stream * stream);
			~StringTable(void) override;

			const string & GetString(int ID) const;
			Array<int> * GetIndex(void) const;
			void AddString(const string & text, int ID);
			void RemoveString(int ID);
			void Save(Streaming::Stream * stream) const;
		};
	}
}