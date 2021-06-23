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
		virtual void DoTask(IDispatchQueue * queue) noexcept = 0;
	};

	class Task : public IDispatchTask
	{
	public:
		virtual void DoTask(IDispatchQueue * queue) noexcept override final;
		virtual void DoTask(void) noexcept = 0;
	};
	template<class F> class FunctionalTask final : public IDispatchTask
	{
		F f;
	public:
		FunctionalTask(const F & func) : f(func) {}
		virtual ~FunctionalTask(void) override {}
		virtual void DoTask(IDispatchQueue * queue) noexcept override { f(); }
	};
	template<class T1, class F> class StructuredFunctionalTask final : public IDispatchTask
	{
		F f;
	public:
		T1 Value1;
		StructuredFunctionalTask(const F & func) : f(func) {}
		virtual ~StructuredFunctionalTask(void) override {}
		virtual void DoTask(IDispatchQueue * queue) noexcept override { f(Value1); }
	};
	template<class T1, class T2, class F> class StructuredFunctionalTask2 final : public IDispatchTask
	{
		F f;
	public:
		T1 Value1;
		T2 Value2;
		StructuredFunctionalTask2(const F & func) : f(func) {}
		virtual ~StructuredFunctionalTask2(void) override {}
		virtual void DoTask(IDispatchQueue * queue) noexcept override { f(Value1, Value2); }
	};
	template<class F> SafePointer< FunctionalTask<F> > CreateFunctionalTask(const F & func) { return new FunctionalTask<F>(func); }
	template<class T1, class F> SafePointer< StructuredFunctionalTask<T1, F> > CreateStructuredTask(const F & func) { return new StructuredFunctionalTask<T1, F>(func); }
	template<class T1, class T2, class F> SafePointer< StructuredFunctionalTask2<T1, T2, F> > CreateStructuredTask(const F & func) { return new StructuredFunctionalTask2<T1, T2, F>(func); }

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
		virtual ~ThreadPool(void) override;

		int GetThreadCount(void) const;
		int GetActiveThreads(void) const;
		int GetTaskQueueLength(void) const;

		virtual void SubmitTask(IDispatchTask * task) override;
		virtual void BeginSubmit(void) override;
		virtual void AppendTask(IDispatchTask * task) override;
		virtual void EndSubmit(void) override;

		void Wait(void) const;
		bool Active(void) const;

		virtual string ToString(void) const override;
	};
	class TaskQueue : public IDispatchQueue
	{
		struct ListTask { SafePointer<IDispatchTask> task; ListTask * next; };
		SafePointer<Semaphore> Access;
		SafePointer<Semaphore> TaskCount;
		ListTask * First, * Last;
		int IntTaskCount;
	public:
		TaskQueue(void);
		virtual ~TaskQueue(void) override;

		virtual void SubmitTask(IDispatchTask * task) override;
		virtual void BeginSubmit(void) override;
		virtual void AppendTask(IDispatchTask * task) override;
		virtual void EndSubmit(void) override;

		int GetTaskQueueLength(void) const;
		void Process(void);
		bool ProcessOnce(void);
		void Quit(void);
		void Break(void);

		virtual string ToString(void) const override;
	};
}