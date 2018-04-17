#pragma once

#include "../EngineBase.h"

namespace Engine
{
	class Process : public Object
	{
	public:
		virtual bool Exited(void) = 0;
		virtual int GetExitCode(void) = 0;
		virtual void Wait(void) = 0;
		virtual void Terminate(void) = 0;
	};

	Process * CreateProcess(const string & image, const Array<string> * command_line = 0);
	Array<string> * GetCommandLine(void);
	void Sleep(uint32 time);
	void ExitProcess(int exit_code);
}