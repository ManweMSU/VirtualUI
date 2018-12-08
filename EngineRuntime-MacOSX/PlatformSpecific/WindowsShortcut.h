#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace WindowsSpecific
	{
		enum class ShellFolder { ApplicationData, Desktop, Documents, Music, Pictures, Videos, UserRoot, ProgramFiles, ProgramsMenu, StartupPrograms, System, WindowsRoot };
		void CreateShellLink(const string & link, const string & path, const string & description);
		string GetShellFolderPath(ShellFolder folder);
	}
}