#include "UnixFileAccess.h"

#ifdef ENGINE_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#endif

namespace Engine
{
	namespace IO
	{
		namespace Unix
		{
			void SetFileAccessRights(handle file, uint user, uint group, uint other)
			{
				#ifdef ENGINE_UNIX
				uint mode = (other & 0x07) | ((group & 0x07) << 3) | ((user & 0x07) << 6);
				if (fchmod(reinterpret_cast<intptr>(file), mode) == -1) throw FileAccessException(errno);
				#endif
			}
			uint GetFileUserAccessRights(handle file)
			{
				#ifdef ENGINE_UNIX
				struct stat info;
				if (fstat(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException(errno);
				return (info.st_mode & 0700) >> 6;
				#else
				return 0;
				#endif
			}
			uint GetFileGroupAccessRights(handle file)
			{
				#ifdef ENGINE_UNIX
				struct stat info;
				if (fstat(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException(errno);
				return (info.st_mode & 0070) >> 3;
				#else
				return 0;
				#endif
			}
			uint GetFileOtherAccessRights(handle file)
			{
				#ifdef ENGINE_UNIX
				struct stat info;
				if (fstat(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException(errno);
				return info.st_mode & 0007;
				#else
				return 0;
				#endif
			}
		}
	}
}