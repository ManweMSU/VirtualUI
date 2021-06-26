#include "../Interfaces/Threading.h"

#include <new>

@import Foundation;

@class ThreadStartupInfo;
@interface ThreadStartupInfo : NSObject
{
@public
	Engine::ThreadRoutine routine;
	Engine::Semaphore * wait_sem;
	void * argument;
	int exit_code;
}
- (void) doJob: (id) arg;
- (void) dealloc;
@end
@implementation ThreadStartupInfo
- (void) doJob: (id) arg
{
	exit_code = routine(argument);
	wait_sem->Open();
}
- (void) dealloc
{
	wait_sem->Release();
	[super dealloc];
}
@end

namespace Engine
{
	namespace CocoaThreading
	{
		class Semaphore : public Engine::Semaphore
		{
			NSConditionLock * lock;
			uint32 semaphore;
		public:
			Semaphore(uint32 value) : semaphore(value)
			{
				lock = [[NSConditionLock alloc] initWithCondition: ((semaphore == 0) ? 0 : 1) ];
				if (!lock) throw Exception();
			}
			~Semaphore(void) override
			{
				[lock release];
			}
			virtual void Wait(void) override
			{
				[lock lockWhenCondition: 1];
				if (semaphore == 0) {
					[lock unlockWithCondition: 1];
					throw InvalidArgumentException();
				}
				semaphore--;
				[lock unlockWithCondition: ((semaphore == 0) ? 0 : 1) ];
			}
			virtual bool TryWait(void) override
			{
				while (true) {
					if ([lock tryLockWhenCondition: 1]) {
						if (semaphore == 0) {
							[lock unlockWithCondition: 1];
							throw InvalidArgumentException();
						}
						semaphore--;
						[lock unlockWithCondition: ((semaphore == 0) ? 0 : 1) ];
						return true;
					}
					if ([lock tryLockWhenCondition: 0]) {
						[lock unlockWithCondition: 0];
						return false;
					}
				}
			}
			virtual bool WaitFor(uint time) override
			{
				@autoreleasepool {
					auto date = [NSDate dateWithTimeIntervalSinceNow: double(time) / 1000.0];
					auto result = [lock lockWhenCondition: 1 beforeDate: date];
					if (!result) return false;
				}
				if (semaphore == 0) {
					[lock unlockWithCondition: 1];
					throw InvalidArgumentException();
				}
				semaphore--;
				[lock unlockWithCondition: ((semaphore == 0) ? 0 : 1) ];
				return true;
			}
			virtual void Open(void) override
			{
				while (![lock tryLock]);
				semaphore++;
				[lock unlockWithCondition: ((semaphore == 0) ? 0 : 1) ];
			}
		};
		class Thread : public Engine::Thread
		{
			NSThread * thread;
			ThreadStartupInfo * thread_info;
		public:
			Thread(NSThread * handle, ThreadStartupInfo * info) : thread(handle), thread_info(info) {}
			~Thread(void) override { [thread release]; [thread_info release]; }
			virtual bool Exited(void) override
			{
				return [thread isFinished];
			}
			virtual int GetExitCode(void) override
			{
				return thread_info ? thread_info->exit_code : -1;
			}
			virtual void Wait(void) override
			{
				if (!thread_info) throw InvalidArgumentException();
				thread_info->wait_sem->Wait();
				thread_info->wait_sem->Open();
			}
		};
	}
	Thread * CreateThread(ThreadRoutine routine, void * argument, uint32 stack_size)
	{
		if (!routine) throw InvalidArgumentException();
		ThreadStartupInfo * info = [[ThreadStartupInfo alloc] init];
		if (!info) throw Exception();
		info->routine = routine;
		info->argument = argument;
		info->wait_sem = CreateSemaphore(0);
		info->exit_code = -1;
		NSThread * thread = [[NSThread alloc] initWithTarget: info selector: @selector(doJob:) object: NULL];
		[[thread threadDictionary] setObject: info forKey: @"einst"];
		[thread setStackSize: uint32((uint64(stack_size) * 0x1000 + 0x0FFF) / 0x1000)];
		[thread start];
		return new CocoaThreading::Thread(thread, info);
	}
	Semaphore * CreateSemaphore(uint32 initial)
	{
		return new (std::nothrow) CocoaThreading::Semaphore(initial);
	}
}