#pragma once

#include "../Interfaces/SystemIO.h"

namespace Engine
{
	namespace IO
	{
		namespace Unix
		{
			enum AccessRight {
				AccessRightNone = 0x00,
				AccessRightExecute = 0x01,
				AccessRightWrite = 0x02,
				AccessRightRead = 0x04,

				AccessRightAll = AccessRightRead | AccessRightWrite | AccessRightExecute,
				AccessRightRegular = AccessRightRead | AccessRightWrite,
				AccessRightReadOnly = AccessRightRead | AccessRightExecute
			};
			void SetFileAccessRights(handle file, uint user, uint group, uint other);
			uint GetFileUserAccessRights(handle file);
			uint GetFileGroupAccessRights(handle file);
			uint GetFileOtherAccessRights(handle file);
		}
	}
}