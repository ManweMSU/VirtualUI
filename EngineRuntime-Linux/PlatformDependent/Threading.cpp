#include "ThreadingAPI.h"

#include <semaphore.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>

namespace Engine
{
	class SystemThread : public Thread
	{
		int _retval;
		SafePointer<Semaphore> _wait_sync;
		ThreadRoutine _routine;
		void * _argument;

		static void * _thread_proc(void * _self)
		{
			auto self = reinterpret_cast<SystemThread *>(_self);
			int result;
			try { result = self->_routine(self->_argument); } catch (...) { result = -1; }
			self->_retval = result;
			self->_wait_sync->Open();
			self->Release();
			return 0;
		}
	public:
		SystemThread(void) : _retval(-1) {}
		virtual ~SystemThread(void) override {}
		virtual bool Exited(void) override { bool result; if (result = _wait_sync->TryWait()) _wait_sync->Open(); return result; }
		virtual int GetExitCode(void) override { if (Exited()) return _retval; else return -1; }
		virtual void Wait(void) override { _wait_sync->Wait(); _wait_sync->Open(); }
		static Thread * InitThread(ThreadRoutine routine, void * argument, uint32 stack_size)
		{
			SafePointer<SystemThread> thread = new (std::nothrow) SystemThread;
			thread->_routine = routine;
			thread->_argument = argument;
			thread->_wait_sync = CreateSemaphore(0);
			if (!thread->_wait_sync) return 0;
			pthread_t thread_handle;
			pthread_attr_t thread_attr;
			if (pthread_attr_init(&thread_attr)) return 0;
			if (pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED)) { pthread_attr_destroy(&thread_attr); return 0; }
			if (pthread_attr_setstacksize(&thread_attr, stack_size)) { pthread_attr_destroy(&thread_attr); return 0; }
			thread->Retain();
			auto status = pthread_create(&thread_handle, &thread_attr, _thread_proc, thread.Inner());
			pthread_attr_destroy(&thread_attr);
			if (status) { thread->Release(); return 0; }
			thread->Retain();
			return thread;
		}
	};
	class SystemSemaphore : public Semaphore
	{
		sem_t _sem;
	public:
		SystemSemaphore(uint32 initial) { if (sem_init(&_sem, 0, initial) == -1) throw Exception(); }
		virtual ~SystemSemaphore(void) override { sem_destroy(&_sem); }
		virtual void Wait(void) override
		{
			while (true) {
				if (sem_wait(&_sem) == 0) return;
				if (errno != EINTR) throw InvalidStateException();
			}
		}
		virtual bool TryWait(void) override
		{
			while (true) {
				auto status = sem_trywait(&_sem);
				if (status == 0) return true;
				else if (errno == EAGAIN) return false;
				else if (errno == EINTR) continue;
				else throw InvalidStateException();
			}
		}
		virtual bool WaitFor(uint time) override
		{
			struct timespec date;
			if (clock_gettime(CLOCK_REALTIME, &date) == -1) throw InvalidStateException();
			uint64 new_ns = date.tv_nsec + uint64(time) * 1000000UL;
			uint64 sec_carry = new_ns / 1000000000UL;
			date.tv_nsec = new_ns % 1000000000UL;
			date.tv_sec += sec_carry;
			while (true) {
				auto status = sem_timedwait(&_sem, &date);
				if (status == 0) return true;
				else if (errno == EAGAIN || errno == ETIMEDOUT) return false;
				else if (errno == EINTR) continue;
				else throw InvalidStateException();
			}
		}
		virtual void Open(void) override { if (sem_post(&_sem) == -1) throw InvalidStateException(); }
	};

	uint64 ThisProcessID;

	Thread * CreateThread(ThreadRoutine routine, void * argument, uint32 stack_size) { return SystemThread::InitThread(routine, argument, stack_size); }
	Semaphore * CreateSemaphore(uint32 initial) { try { return new SystemSemaphore(initial); } catch (...) { return 0; } }
}