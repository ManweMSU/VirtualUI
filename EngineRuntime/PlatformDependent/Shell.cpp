#include "../Interfaces/Shell.h"

#include "../Interfaces/SystemIO.h"
#include "../Miscellaneous/DynamicString.h"

#include <Windows.h>

#undef CreateProcess
#undef ZeroMemory

namespace Engine
{
	namespace Shell
	{
		bool OpenFile(const string & file)
		{
			return ShellExecuteW(0, L"open", IO::ExpandPath(file), 0, 0, SW_SHOWNORMAL) != 0;
		}
		void ShowInBrowser(const string & path, bool directory)
		{
			if (directory) {
				ShellExecuteW(0, L"explore", IO::ExpandPath(path), 0, 0, SW_SHOWNORMAL);
			} else {
				STARTUPINFOW si;
				ZeroMemory(&si, sizeof(si));
				si.cb = sizeof(si);
				PROCESS_INFORMATION pi;
				DynamicString cmd;
				cmd += L"explorer.exe /select,\"" + IO::ExpandPath(path) + L"\"";
				if (CreateProcessW(0, cmd, 0, 0, FALSE, 0, 0, 0, &si, &pi)) {
					CloseHandle(pi.hProcess);
					CloseHandle(pi.hThread);
				}
			}
		}
		void OpenCommandPrompt(const string & working_directory)
		{
			string dir = IO::ExpandPath(working_directory);
			STARTUPINFOW si;
			ZeroMemory(&si, sizeof(si));
			si.cb = sizeof(si);
			PROCESS_INFORMATION pi;
			DynamicString cmd;
			cmd += string(L"cmd.exe");
			if (CreateProcessW(0, cmd, 0, 0, FALSE, CREATE_NEW_CONSOLE, 0, dir, &si, &pi)) {
				CloseHandle(pi.hProcess);
				CloseHandle(pi.hThread);
			}
		}
	}
}