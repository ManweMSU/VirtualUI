#include "Threading.h"

#include <Windows.h>

#undef CreateSemaphore

namespace Engine
{
	namespace WinapiThreading
	{
		class Thread : public Engine::Thread
		{
			HANDLE thread;
		public:
			Thread(HANDLE handle) : thread(handle) {}
			~Thread(void) override { CloseHandle(thread); }
			virtual bool Exited(void) override
			{
				DWORD code;
				auto result = GetExitCodeThread(thread, &code);
				return result && code != STILL_ACTIVE;
			}
			virtual int GetExitCode(void) override
			{
				DWORD code;
				auto result = GetExitCodeThread(thread, &code);
				return (result && code != STILL_ACTIVE) ? int(code) : -1;
			}
			virtual void Wait(void) override { WaitForSingleObject(thread, INFINITE); }
		};
		class Semaphore : public Engine::Semaphore
		{
			HANDLE semaphore;
		public:
			Semaphore(HANDLE handle) : semaphore(handle) {}
			~Semaphore(void) override { CloseHandle(semaphore); }
			virtual void Wait(void) override { WaitForSingleObject(semaphore, INFINITE); }
			virtual bool TryWait(void) override { return WaitForSingleObject(semaphore, 0) == WAIT_OBJECT_0; }
			virtual void Open(void) override { ReleaseSemaphore(semaphore, 1, 0); }
		};
		struct NewThreadInfo
		{
			ThreadRoutine routine;
			void * argument;
		};
		DWORD WINAPI EngineThreadProc(LPVOID Argument)
		{
			NewThreadInfo * info = reinterpret_cast<NewThreadInfo *>(Argument);
			ThreadRoutine routine = info->routine;
			void * argument = info->argument;
			delete info;
			return DWORD(routine(argument));
		}
	}
	Thread * CreateThread(ThreadRoutine routine, void * argument, uint32 stack_size)
	{
		if (!routine) throw InvalidArgumentException();
		auto arg = new WinapiThreading::NewThreadInfo;
		arg->argument = argument;
		arg->routine = routine;
		HANDLE handle = ::CreateThread(0, stack_size, WinapiThreading::EngineThreadProc, arg, 0, 0);
		if (handle) return new WinapiThreading::Thread(handle); else { delete arg; return 0; }
	}
	Thread * GetCurrentThread(void) { return new WinapiThreading::Thread(::GetCurrentThread()); }
	Semaphore * CreateSemaphore(uint32 initial)
	{
		HANDLE handle = CreateSemaphoreW(0, initial, 0x40000000, 0);
		if (handle) return new WinapiThreading::Semaphore(handle); else return 0;
	}
}