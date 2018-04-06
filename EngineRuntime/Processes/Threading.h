#pragma once

#include "../EngineBase.h"

namespace Engine
{
	typedef int (* ThreadRoutine) (void * argument);

	class Thread : public Object
	{
	public:
		virtual bool Exited(void) = 0;
		virtual int GetExitCode(void) = 0;
		virtual void Wait(void) = 0;
		virtual void Terminate(void) = 0;
		virtual void Suspend(void) = 0;
		virtual void Resume(void) = 0;
	};
	class Semaphore : public Object
	{
	public:
		virtual void Wait(void) = 0;
		virtual bool TryWait(void) = 0;
		virtual void Open(void) = 0;
	};

	Thread * CreateThread(ThreadRoutine routine, void * argument = 0, uint32 stack_size = 0x200000);
	Thread * GetCurrentThread(void);
	Semaphore * CreateSemaphore(uint32 initial);
}