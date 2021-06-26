#include "ThreadPool.h"

namespace Engine
{
	void Task::DoTask(IDispatchQueue * queue) noexcept { DoTask(); }
	ThreadPool::ThreadPool(void) : ThreadPool(0) {}
	ThreadPool::ThreadPool(int num_threads) : Workers(8), Tasks(0x100)
	{
		if (num_threads <= 0) num_threads = GetProcessorsNumber() + num_threads;
		if (num_threads <= 0) num_threads = 1;
		ActiveThreads = num_threads;
		Access = CreateSemaphore(1);
		TaskCount = CreateSemaphore(0);
		Idle = CreateSemaphore(0);
		if (!Access || !TaskCount || !Idle) throw Exception();
		for (int i = 0; i < num_threads; i++) {
			SafePointer<Thread> worker = CreateThread(reinterpret_cast<ThreadRoutine>(ThreadProc), this);
			if (!worker) {
				BeginSubmit();
				for (int j = 0; j < i; j++) { TaskCount->Open(); Tasks.Append(0); }
				EndSubmit();
				Wait();
				throw Exception();
			}
			Workers.Append(worker);
		}
	}
	ThreadPool::ThreadPool(int num_threads, uint32 stack_size) : Workers(8), Tasks(0x100)
	{
		if (num_threads <= 0) num_threads = GetProcessorsNumber() + num_threads;
		if (num_threads <= 0) num_threads = 1;
		ActiveThreads = num_threads;
		Access = CreateSemaphore(1);
		TaskCount = CreateSemaphore(0);
		Idle = CreateSemaphore(0);
		if (!Access || !TaskCount || !Idle) throw Exception();
		for (int i = 0; i < num_threads; i++) {
			SafePointer<Thread> worker = CreateThread(reinterpret_cast<ThreadRoutine>(ThreadProc), this, stack_size);
			if (!worker) {
				BeginSubmit();
				for (int j = 0; j < i; j++) { TaskCount->Open(); Tasks.Append(0); }
				EndSubmit();
				Wait();
				throw Exception();
			}
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
				task->DoTask(owner);
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
	int TaskQueue::ThreadProc(void * arg)
	{
		auto self = reinterpret_cast<TaskQueue *>(arg);
		self->Process();
		self->Release();
		return 0;
	}
	TaskQueue::TaskQueue(void) : First(0), Last(0), IntTaskCount(0)
	{
		Access = CreateSemaphore(1);
		TaskCount = CreateSemaphore(0);
		if (!Access || !TaskCount) throw Exception();
	}
	TaskQueue::~TaskQueue(void)
	{
		auto current = First;
		while (current) { auto copy = current; current = current->next; delete copy; }
	}
	void TaskQueue::SubmitTask(IDispatchTask * task)
	{
		if (!task) throw InvalidArgumentException();
		BeginSubmit();
		try { AppendTask(task); } catch (...) { EndSubmit(); throw; }
		EndSubmit();
	}
	void TaskQueue::BeginSubmit(void) { Access->Wait(); }
	void TaskQueue::AppendTask(IDispatchTask * task)
	{
		if (!task) throw InvalidArgumentException();
		auto list = new (std::nothrow) ListTask;
		if (!list) throw OutOfMemoryException();
		list->task.SetRetain(task);
		list->next = 0;
		TaskCount->Open();
		IntTaskCount++;
		if (Last) {
			Last->next = list;
			Last = list;
		} else First = Last = list;
	}
	void TaskQueue::EndSubmit(void) { Access->Open(); }
	int TaskQueue::GetTaskQueueLength(void) const
	{
		Access->Wait();
		int count = IntTaskCount;
		Access->Open();
		return count;
	}
	void TaskQueue::Process(void)
	{
		bool loop = true;
		while (loop) {
			TaskCount->Wait();
			Access->Wait();
			IntTaskCount--;
			auto task = First;
			if (First) First = First->next;
			if (!First) Last = 0;
			Access->Open();
			if (task) {
				if (task->task) task->task->DoTask(this);
				else loop = false;
				delete task;
			}
		}
	}
	bool TaskQueue::ProcessOnce(void)
	{
		if (TaskCount->TryWait()) {
			Access->Wait();
			IntTaskCount--;
			auto task = First;
			if (First) First = First->next;
			if (!First) Last = 0;
			Access->Open();
			if (task) {
				if (task->task) task->task->DoTask(this);
				delete task;
				return true;
			} else return false;
		} else return false;
	}
	void TaskQueue::Quit(void)
	{
		auto list = new (std::nothrow) ListTask;
		if (!list) throw OutOfMemoryException();
		Access->Wait();
		TaskCount->Open();
		IntTaskCount++;
		list->next = 0;
		if (Last) {
			Last->next = list;
			Last = list;
		} else First = Last = list;
		Access->Open();
	}
	void TaskQueue::Break(void)
	{
		auto list = new (std::nothrow) ListTask;
		if (!list) throw OutOfMemoryException();
		Access->Wait();
		TaskCount->Open();
		IntTaskCount++;
		list->next = First;
		First = list;
		if (!Last) Last = list;
		Access->Open();
	}
	bool TaskQueue::ProcessAsSeparateThread(Thread ** thread)
	{
		SafePointer<Thread> off_thread;
		Retain();
		try {
			off_thread = CreateThread(ThreadProc, this);
			if (!off_thread) throw Exception();
		} catch (...) { Release(); return false; }
		if (thread) {
			*thread = off_thread.Inner();
			off_thread->Retain();
		}
		return true;
	}
	string TaskQueue::ToString(void) const
	{
		Access->Wait();
		string info = L"TaskQueue: " + string(IntTaskCount) + L" tasks pending";
		Access->Open();
		return info;
	}
	
	class VoidSchedulerTask : public ISchedulerTask
	{
	public:
		VoidSchedulerTask(void) {}
		virtual void DoTask(Scheduler * scheduler) noexcept override {}
		virtual void Cancelled(Scheduler * scheduler) noexcept override {}
	};
	int Scheduler::ThreadProc(void * arg)
	{
		auto self = reinterpret_cast<Scheduler *>(arg);
		self->Process();
		self->Release();
		return 0;
	}
	Scheduler::Scheduler(void) : _tasks(0x10)
	{
		_void_task = new VoidSchedulerTask;
		_access = CreateSemaphore(1);
		_interrupt = CreateSemaphore(0);
		if (!_access || !_interrupt) throw Exception();
	}
	Scheduler::~Scheduler(void) { CancelAll(); }
	bool Scheduler::Schedule(ISchedulerTask * task, uint time_on)
	{
		bool result = true;
		int slot = -1;
		int current_time = GetTimerValue();
		int nearest = 0x7FFFFFFF;
		int wait_for_this = time_on - current_time;
		_access->Wait();
		try {
			for (int i = 0; i < _tasks.Length(); i++) {
				if (_tasks[i].flags == 1) {
					int wt = _tasks[i].time - current_time;
					if (wt < nearest) nearest = wt;
				}
				if (!_tasks[i].flags && slot == -1) slot = i;
			}
			if (slot == -1) {
				slot = _tasks.Length();
				task_rec new_task;
				new_task.flags = new_task.time = 0;
				_tasks << new_task;
			}
			_tasks[slot].time = time_on;
			_tasks[slot].flags = 1;
			_tasks[slot].object.SetRetain(task);
		} catch (...) { result = false; }
		_access->Open();
		if (wait_for_this < nearest) if (!Interrupt(_void_task)) result = false;
		return result;
	}
	bool Scheduler::Interrupt(ISchedulerTask * with)
	{
		bool result = true;
		int slot = -1;
		_access->Wait();
		try {
			for (int i = 0; i < _tasks.Length(); i++) if (!_tasks[i].flags) { slot = i; break; }
			if (slot == -1) {
				slot = _tasks.Length();
				task_rec new_task;
				new_task.flags = new_task.time = 0;
				_tasks << new_task;
			}
			_tasks[slot].time = 0;
			_tasks[slot].flags = 2;
			_tasks[slot].object.SetRetain(with);
			_interrupt->Open();
		} catch (...) { result = false; }
		_access->Open();
		return result;
	}
	void Scheduler::CancelAll(void)
	{
		_access->Wait();
		for (auto & t : _tasks) {
			if (t.object) t.object->Cancelled(this);
			t.object.SetReference(0);
			t.time = t.flags = 0;
		}
		while (_interrupt->TryWait());
		_access->Open();
	}
	void Scheduler::Break(void)
	{
		int slot = -1;
		_access->Wait();
		try {
			for (int i = 0; i < _tasks.Length(); i++) { if (!_tasks[i].flags && slot == -1) { slot = i; break; } }
			if (slot == -1) {
				slot = _tasks.Length();
				task_rec new_task;
				new_task.flags = new_task.time = 0;
				_tasks << new_task;
			}
			_tasks[slot].time = 0;
			_tasks[slot].flags = 3;
			_tasks[slot].object.SetReference(0);
			_interrupt->Open();
		} catch (...) {}
		_access->Open();
	}
	void Scheduler::Process(void)
	{
		while (true) {
			_access->Wait();
			int current = GetTimerValue();
			int nearest = 0x7FFFFFFF;
			for (int i = 0; i < _tasks.Length(); i++) {
				if (_tasks[i].flags == 1) {
					int wt = _tasks[i].time - current;
					if (wt < nearest) nearest = wt;
				}
			}
			_access->Open();
			bool interrupted;
			if (nearest == 0x7FFFFFFF) {
				_interrupt->Wait();
				interrupted = true;
			} else interrupted = _interrupt->WaitFor(max(nearest, 0));
			_access->Wait();
			if (interrupted) {
				SafePointer<ISchedulerTask> task;
				for (auto & t : _tasks) if (t.flags > 1) {
					if (t.flags == 2) {
						task = t.object;
					} else if (t.flags == 3) {
						t.time = 0;
						t.flags = 0;
						t.object.SetReference(0);
						_access->Open();
						return;
					}
					t.time = 0;
					t.flags = 0;
					t.object.SetReference(0);
					break;
				}
				_access->Open();
				if (task) task->DoTask(this);
			} else {
				int current = GetTimerValue();
				int nearest = 0x7FFFFFFF;
				int nearest_at = -1;
				for (int i = 0; i < _tasks.Length(); i++) {
					if (_tasks[i].flags == 1) {
						int wt = _tasks[i].time - current;
						if (wt < nearest) { nearest = wt; nearest_at = i; }
					}
				}
				SafePointer<ISchedulerTask> task;
				if (nearest_at >= 0 && _tasks[nearest_at].flags == 1) {
					task = _tasks[nearest_at].object;
					_tasks[nearest_at].object.SetReference(0);
					_tasks[nearest_at].time = 0;
					_tasks[nearest_at].flags = 0;
				}
				_access->Open();
				if (task) task->DoTask(this);
			}
		}
	}
	bool Scheduler::ProcessAsSeparateThread(Thread ** thread)
	{
		SafePointer<Thread> off_thread;
		Retain();
		try {
			off_thread = CreateThread(ThreadProc, this);
			if (!off_thread) throw Exception();
		} catch (...) { Release(); return false; }
		if (thread) {
			*thread = off_thread.Inner();
			off_thread->Retain();
		}
		return true;
	}
}