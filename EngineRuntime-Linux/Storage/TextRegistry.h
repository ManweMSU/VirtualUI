#pragma once

#include "Registry.h"

namespace Engine
{
	namespace Storage
	{
		string RegistryToText(Registry * registry);
		void RegistryToText(Registry * registry, Streaming::ITextWriter * output);
		void RegistryToText(Registry * registry, Streaming::Stream * output, Encoding encoding = Encoding::UTF16);
		
		Registry * CompileTextRegistry(const string & code);
		Registry * CompileTextRegistry(Streaming::ITextReader * input);
		Registry * CompileTextRegistry(Streaming::Stream * input);
		Registry * CompileTextRegistry(Streaming::Stream * input, Encoding encoding);
	}
}