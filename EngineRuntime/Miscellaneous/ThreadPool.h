#pragma once

#include "../Interfaces/Threading.h"

namespace Engine
{
	class IDispatchQueue;
	class IDispatchTask;

	class IDispatchQueue : public Object
	{
	public:
		virtual void SubmitTask(IDispatchTask * task) = 0;
		virtual void BeginSubmit(void) = 0;
		virtual void AppendTask(IDispatchTask * task) = 0;
		virtual void EndSubmit(void) = 0;
	};
	class IDispatchTask : public Object
	{
	public:
		virtual void DoTask(IDispatchQueue * queue) = 0;
	};

	class ThreadPool : public IDispatchQueue
	{
		ObjectArray<Thread> Workers;
		SafePointer<Semaphore> Access;
		SafePointer<Semaphore> TaskCount;
		SafePointer<Semaphore> Idle;
		ObjectArray<IDispatchTask> Tasks;
		int ActiveThreads;

		static int ThreadProc(ThreadPool * owner);
	public:
		ThreadPool(void);
		ThreadPool(int num_threads);
		ThreadPool(int num_threads, uint32 stack_size);
		~ThreadPool(void) override;

		int GetThreadCount(void) const;
		int GetActiveThreads(void) const;
		int GetTaskQueueLength(void) const;

		virtual void SubmitTask(IDispatchTask * task) override;
		virtual void BeginSubmit(void) override;
		virtual void AppendTask(IDispatchTask * task) override;
		virtual void EndSubmit(void) override;

		void Wait(void) const;
		bool Active(void) const;

		string ToString(void) const override;
	};
	class Task : public IDispatchTask
	{
	public:
		virtual void DoTask(IDispatchQueue * queue) override final;
		virtual void DoTask(void) = 0;
	};
}