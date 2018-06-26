#include "ThreadPool.h"

namespace Engine
{
	namespace Tasks
	{
		ThreadPool::ThreadPool(void) : ThreadPool(-1) {}
		ThreadPool::ThreadPool(int num_threads) : Workers(8), Jobs(0x100)
		{
			if (num_threads <= 0) num_threads = GetProcessorsNumber();
			ActiveThreads = num_threads;
			Access = CreateSemaphore(1);
			JobCount = CreateSemaphore(0);
			Idle = CreateSemaphore(0);
			for (int i = 0; i < num_threads; i++) {
				SafePointer<Thread> worker = CreateThread(reinterpret_cast<ThreadRoutine>(ThreadProc), this);
				Workers.Append(worker);
			}
		}
		ThreadPool::ThreadPool(int num_threads, uint32 stack_size) : Workers(8), Jobs(0x100)
		{
			if (num_threads <= 0) num_threads = GetProcessorsNumber();
			ActiveThreads = num_threads;
			Access = CreateSemaphore(1);
			JobCount = CreateSemaphore(0);
			Idle = CreateSemaphore(0);
			for (int i = 0; i < num_threads; i++) {
				SafePointer<Thread> worker = CreateThread(reinterpret_cast<ThreadRoutine>(ThreadProc), this, stack_size);
				Workers.Append(worker);
			}
		}
		ThreadPool::~ThreadPool(void)
		{
			Wait();
			BeginSubmit();
			for (int i = 0; i < Workers.Length(); i++) {
				JobCount->Open();
				Jobs.Append(0);
			}
			EndSubmit();
			Wait();
		}
		int ThreadPool::GetThreadCount(void) const
		{
			Access->Wait();
			int count = Workers.Length();
			Access->Open();
			return count;
		}
		int ThreadPool::GetActiveThreads(void) const
		{
			Access->Wait();
			int count = ActiveThreads;
			Access->Open();
			return count;
		}
		int ThreadPool::GetJobQueueLength(void) const
		{
			Access->Wait();
			int count = Jobs.Length();
			Access->Open();
			return count;
		}
		void ThreadPool::SubmitJob(ThreadJob * job)
		{
			if (!job) throw InvalidArgumentException();
			BeginSubmit();
			try { AppendJob(job); } catch (...) { EndSubmit(); throw; }
			EndSubmit();
		}
		void ThreadPool::BeginSubmit(void)
		{
			Access->Wait();
		}
		void ThreadPool::AppendJob(ThreadJob * job)
		{
			if (!job) throw InvalidArgumentException();
			JobCount->Open();
			try { Jobs.Append(job); } catch (...) { JobCount->Wait(); throw; }
		}
		void ThreadPool::EndSubmit(void)
		{
			Access->Open();
		}
		void ThreadPool::Wait(void) const
		{
			bool idle = false;
			do {
				Idle->Wait();
				Idle->Open();
				Access->Wait();
				if (!Jobs.Length() && !ActiveThreads) idle = true;
				Access->Open();
			} while (!idle);
		}
		bool ThreadPool::Active(void) const
		{
			if (Idle->TryWait()) {
				Idle->Open();
				Access->Wait();
				bool idle = (!Jobs.Length() && !ActiveThreads);
				Access->Open();
				return !idle;
			} else return true;
		}
		string ThreadPool::ToString(void) const
		{
			Access->Wait();
			string info = L"ThreadPool: " + string(Workers.Length()) + L" threads, " + string(Jobs.Length()) + L" jobs pending, " + string(ActiveThreads) + L" threads active";
			Access->Open();
			return info;
		}
		int ThreadPool::ThreadProc(ThreadPool * owner)
		{
			do {
				SafePointer<ThreadJob> job;
				if (owner->JobCount->TryWait()) {
					owner->Access->Wait();
					job.SetRetain(owner->Jobs.LastElement());
					owner->Jobs.RemoveLast();
					owner->Access->Open();
				} else {
					owner->Access->Wait();
					owner->ActiveThreads--;
					if (owner->ActiveThreads == 0) owner->Idle->Open();
					owner->Access->Open();
					owner->JobCount->Wait();
					owner->Access->Wait();
					job.SetRetain(owner->Jobs.LastElement());
					owner->Jobs.RemoveLast();
					if (owner->ActiveThreads == 0) owner->Idle->Wait();
					owner->ActiveThreads++;
					owner->Access->Open();
				}
				if (job) {
					try { job->DoJob(owner); }
					catch (...) {}
					job.SetReference(0);
				} else {
					owner->Access->Wait();
					owner->ActiveThreads--;
					if (owner->ActiveThreads == 0) owner->Idle->Open();
					owner->Access->Open();
					return 0;
				}
			} while (true);
			return 0;
		}
	}
}