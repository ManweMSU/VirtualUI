#include "Process.h"

#include "../Miscellaneous/DynamicString.h"
#include "../PlatformDependent/FileApi.h"

#include <Windows.h>

#undef CreateProcess
#undef GetCommandLine

namespace Engine
{
	namespace WinapiProcess
	{
		class Process : public Engine::Process
		{
			HANDLE process;
		public:
			Process(HANDLE handle) : process(handle) {}
			~Process(void) override { CloseHandle(process); }
			virtual bool Exited(void) override
			{
				DWORD code;
				auto result = GetExitCodeProcess(process, &code);
				return result && code != STILL_ACTIVE;
			}
			virtual int GetExitCode(void) override
			{
				DWORD code;
				auto result = GetExitCodeProcess(process, &code);
				return (result && code != STILL_ACTIVE) ? int(code) : -1;
			}
			virtual void Wait(void) override { WaitForSingleObject(process, INFINITE); }
			virtual void Terminate(void) override { TerminateProcess(process, -1); }
		};
		void AppendCommandLine(DynamicString & cmd, const string & arg)
		{
			if (cmd.Length() != 0) cmd += L' ';
			cmd += L'\"';
			for (int i = 0; i < arg.Length(); i++) {
				if (arg[i] == L'\"') {
					cmd += L'\\';
					cmd += arg[i];
				} else cmd += arg[i];
			}
			cmd += L'\"';
		}
	}
	Process * CreateProcess(const string & image, const Array<string>* command_line)
	{
		PROCESS_INFORMATION pi;
		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		DynamicString cmd;
		WinapiProcess::AppendCommandLine(cmd, IO::ExpandPath(image));
		if (command_line) {
			for (int i = 0; i < command_line->Length(); i++) WinapiProcess::AppendCommandLine(cmd, command_line->ElementAt(i));
		}
		DWORD Flags = 0;
		if (!CreateProcessW(0, cmd, 0, 0, TRUE, Flags, 0, 0, &si, &pi)) return 0;
		CloseHandle(pi.hThread);
		return new WinapiProcess::Process(pi.hProcess);
	}
	Process * CreateCommandProcess(const string & command_image, const Array<string>* command_line)
	{
		PROCESS_INFORMATION pi;
		STARTUPINFOW si;
		ZeroMemory(&si, sizeof(si));
		si.cb = sizeof(si);
		si.dwFlags = STARTF_USESTDHANDLES;
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		DynamicString cmd;
		WinapiProcess::AppendCommandLine(cmd, command_image);
		if (command_line) {
			for (int i = 0; i < command_line->Length(); i++) WinapiProcess::AppendCommandLine(cmd, command_line->ElementAt(i));
		}
		DWORD Flags = 0;	
		if (!CreateProcessW(0, cmd, 0, 0, TRUE, Flags, 0, 0, &si, &pi)) return 0;
		CloseHandle(pi.hThread);
		return new WinapiProcess::Process(pi.hProcess);
	}
	bool CreateProcessElevated(const string & image, const Array<string>* command_line)
	{
		SHELLEXECUTEINFOW info;
		DynamicString cmd;
		if (command_line) {
			for (int i = 0; i < command_line->Length(); i++) WinapiProcess::AppendCommandLine(cmd, command_line->ElementAt(i));
		}
		ZeroMemory(&info, sizeof(info));
		info.cbSize = sizeof(info);
		info.fMask = SEE_MASK_UNICODE | SEE_MASK_NO_CONSOLE | SEE_MASK_FLAG_NO_UI | SEE_MASK_NOASYNC;
		info.lpVerb = L"runas";
		info.lpFile = IO::ExpandPath(image);
		info.lpParameters = cmd;
		info.nShow = SW_SHOW;
		if (!ShellExecuteExW(&info)) return false;
		return true;
	}
	Array<string>* GetCommandLine(void)
	{
		int count;
		LPWSTR * cmd = CommandLineToArgvW(GetCommandLineW(), &count);
		SafePointer< Array<string> > result = new Array<string>(0x10);
		for (int i = 0; i < count; i++) result->Append(cmd[i]);
		LocalFree(reinterpret_cast<HLOCAL>(cmd));
		result->Retain();
		return result;
	}
	void Sleep(uint32 time) { ::Sleep(time); }
	void ExitProcess(int exit_code) { ::ExitProcess(exit_code); }
}