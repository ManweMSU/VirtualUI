#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace Shell
	{
		bool OpenFile(const string & file);
		void ShowInBrowser(const string & path, bool directory);
		void OpenCommandPrompt(const string & working_directory);
	}
}