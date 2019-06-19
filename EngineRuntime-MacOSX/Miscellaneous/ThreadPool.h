#pragma once

#include "../Processes/Threading.h"

namespace Engine
{
	namespace Tasks
	{
		class ThreadPool;
		class ThreadJob : public Object
		{
		public:
			virtual void DoJob(ThreadPool * pool) = 0;
		};
		class ThreadPool : public Object
		{
			ObjectArray<Thread> Workers;
			SafePointer<Semaphore> Access;
			SafePointer<Semaphore> JobCount;
			SafePointer<Semaphore> Idle;
			ObjectArray<ThreadJob> Jobs;
			int ActiveThreads;

			static int ThreadProc(ThreadPool * owner);
		public:
			ThreadPool(void);
			ThreadPool(int num_threads);
			ThreadPool(int num_threads, uint32 stack_size);
			~ThreadPool(void) override;

			int GetThreadCount(void) const;
			int GetActiveThreads(void) const;
			int GetJobQueueLength(void) const;

			void SubmitJob(ThreadJob * job);
			void BeginSubmit(void);
			void AppendJob(ThreadJob * job);
			void EndSubmit(void);

			void Wait(void) const;
			bool Active(void) const;

			string ToString(void) const override;
		};
	}
	class Task : public Tasks::ThreadJob
	{
	public:
		virtual void DoJob(Tasks::ThreadPool * pool) override final;
		virtual void DoTask(void) = 0;
	};
}