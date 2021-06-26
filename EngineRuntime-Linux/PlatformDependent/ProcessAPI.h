#pragma once

#include "../Interfaces/Process.h"

namespace Engine
{
	enum CreateProcessSearchFlags {
		CreateProcessSearchPath		= 0x01,
		CreateProcessSearchCurrent	= 0x02,
		CreateProcessSearchSelf		= 0x04,
		CreateProcessSearchAll		= CreateProcessSearchPath | CreateProcessSearchCurrent | CreateProcessSearchSelf
	};

	extern char ** ThisProcessArgV;
	extern int ThisProcessArgC;

	void ProcessWaiterThreadLock(void);
	Process * CreateProcess(const string & image, const string * cmdv, int cmdc);
	Process * CreateProcess(const string & image, const string * cmdv, int cmdc, int flags);
	bool CreateDetachedProcess(char ** argv);
}