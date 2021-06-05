#include "ThreadPool.h"

namespace Engine
{
	ThreadPool::ThreadPool(void) : ThreadPool(-1) {}
	ThreadPool::ThreadPool(int num_threads) : Workers(8), Tasks(0x100)
	{
		if (num_threads <= 0) num_threads = GetProcessorsNumber();
		ActiveThreads = num_threads;
		Access = CreateSemaphore(1);
		TaskCount = CreateSemaphore(0);
		Idle = CreateSemaphore(0);
		for (int i = 0; i < num_threads; i++) {
			SafePointer<Thread> worker = CreateThread(reinterpret_cast<ThreadRoutine>(ThreadProc), this);
			Workers.Append(worker);
		}
	}
	ThreadPool::ThreadPool(int num_threads, uint32 stack_size) : Workers(8), Tasks(0x100)
	{
		if (num_threads <= 0) num_threads = GetProcessorsNumber();
		ActiveThreads = num_threads;
		Access = CreateSemaphore(1);
		TaskCount = CreateSemaphore(0);
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
			TaskCount->Open();
			Tasks.Append(0);
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
	int ThreadPool::GetTaskQueueLength(void) const
	{
		Access->Wait();
		int count = Tasks.Length();
		Access->Open();
		return count;
	}
	void Engine::ThreadPool::SubmitTask(IDispatchTask * task)
	{
		if (!task) throw InvalidArgumentException();
		BeginSubmit();
		try { AppendTask(task); } catch (...) { EndSubmit(); throw; }
		EndSubmit();
	}
	void ThreadPool::BeginSubmit(void) { Access->Wait(); }
	void Engine::ThreadPool::AppendTask(IDispatchTask * task)
	{
		if (!task) throw InvalidArgumentException();
		TaskCount->Open();
		try { Tasks.Append(task); } catch (...) { TaskCount->Wait(); throw; }
	}
	void ThreadPool::EndSubmit(void) { Access->Open(); }
	void ThreadPool::Wait(void) const
	{
		bool idle = false;
		do {
			Idle->Wait();
			Idle->Open();
			Access->Wait();
			if (!Tasks.Length() && !ActiveThreads) idle = true;
			Access->Open();
		} while (!idle);
	}
	bool ThreadPool::Active(void) const
	{
		if (Idle->TryWait()) {
			Idle->Open();
			Access->Wait();
			bool idle = (!Tasks.Length() && !ActiveThreads);
			Access->Open();
			return !idle;
		} else return true;
	}
	string ThreadPool::ToString(void) const
	{
		Access->Wait();
		string info = L"ThreadPool: " + string(Workers.Length()) + L" threads, " + string(Tasks.Length()) + L" tasks pending, " + string(ActiveThreads) + L" threads active";
		Access->Open();
		return info;
	}
	int ThreadPool::ThreadProc(ThreadPool * owner)
	{
		do {
			SafePointer<IDispatchTask> task;
			if (owner->TaskCount->TryWait()) {
				owner->Access->Wait();
				task.SetRetain(owner->Tasks.LastElement());
				owner->Tasks.RemoveLast();
				owner->Access->Open();
			} else {
				owner->Access->Wait();
				owner->ActiveThreads--;
				if (owner->ActiveThreads == 0) owner->Idle->Open();
				owner->Access->Open();
				owner->TaskCount->Wait();
				owner->Access->Wait();
				task.SetRetain(owner->Tasks.LastElement());
				owner->Tasks.RemoveLast();
				if (owner->ActiveThreads == 0) owner->Idle->Wait();
				owner->ActiveThreads++;
				owner->Access->Open();
			}
			if (task) {
				try { task->DoTask(owner); }
				catch (...) {}
				task.SetReference(0);
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
	void Task::DoTask(IDispatchQueue * queue) { DoTask(); }
}