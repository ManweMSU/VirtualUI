#include "../Interfaces/Shell.h"
#include "../Interfaces/SystemIO.h"
#include "ProcessAPI.h"

namespace Engine
{
	namespace Shell
	{
		bool OpenFile(const string & file)
		{
			auto expand = IO::ExpandPath(file);
			SafePointer<Process> opener = CreateProcess(L"xdg-open", &expand, 1, CreateProcessSearchPath);
			if (!opener) return false;
			opener->Wait();
			return opener->GetExitCode() == 0;
		}
		void ShowInBrowser(const string & path, bool directory) { if (directory) OpenFile(path); else OpenFile(IO::Path::GetDirectory(path)); }
		void OpenCommandPrompt(const string & working_directory)
		{
			try {
				string old_wd = IO::GetCurrentDirectory();
				IO::SetCurrentDirectory(working_directory);
				SafePointer<Process> process = CreateProcess(L"x-terminal-emulator", 0, 0, CreateProcessSearchPath);
				IO::SetCurrentDirectory(old_wd);
			} catch (...) {}
		}
	}
}