#include "Process.h"

#include "../Miscellaneous/DynamicString.h"
#include "../PlatformDependent/FileApi.h"
#include "../PlatformDependent/CocoaInterop.h"

#include <time.h>
#include <crt_externs.h>

#include "../Streaming.h"

@import Foundation;

namespace Engine
{
	namespace CocoaProcess
	{
		class Process : public Engine::Process
		{
			NSTask * process;
		public:
			Process(NSTask * handle) { process = handle; }
			~Process(void) override { [process release]; }
			virtual bool Exited(void) override { return ![process isRunning]; }
			virtual int GetExitCode(void) override
			{
				if ([process isRunning]) return -1;
				if ([process terminationReason] == NSTaskTerminationReasonExit) {
					return [process terminationStatus];
				} else return -1;
			}
			virtual void Wait(void) override { [process waitUntilExit]; }
			virtual void Terminate(void) override { [process terminate]; }
		};
	}
	Process * CreateProcess(const string & image, const Array<string>* command_line)
	{
		NSMutableArray * argv = [[NSMutableArray alloc] init];
		if (command_line) {
			for (int i = 0; i < command_line->Length(); i++) {
				NSString * str = Cocoa::CocoaString(command_line->ElementAt(i));
				[argv addObject: str];
				[str release];
			}
		}
		@try {
			NSString * app = Cocoa::CocoaString(IO::ExpandPath(IO::NormalizePath(image)));
			NSTask * task = [NSTask launchedTaskWithLaunchPath: app arguments: argv];
			[app release];
			[argv release];
			argv = 0;
			return new CocoaProcess::Process(task);
		}
		@catch (NSException * e) {}
		[argv release];
		return 0;
	}
	Process * CreateCommandProcess(const string & command_image, const Array<string> * command_line)
	{
		NSMutableArray * argv = [[NSMutableArray alloc] init];
		if (command_line) {
			for (int i = 0; i < command_line->Length(); i++) {
				NSString * str = Cocoa::CocoaString(command_line->ElementAt(i));
				[argv addObject: str];
				[str release];
			}
		}
		@try {
			NSString * app = Cocoa::CocoaString(IO::NormalizePath(L"/bin/" + command_image));
			NSTask * task = [[NSTask alloc] init];
			[task setLaunchPath: app];
			[task setArguments: argv];
			[task launch];
			[app release];
			[argv release];
			argv = 0;
			return new CocoaProcess::Process(task);
		}
		@catch (NSException * e) {}
		[argv release];
		return 0;
	}
	Array<string>* GetCommandLine(void)
	{
		int argc = *_NSGetArgc();
		char ** argv = *_NSGetArgv();
		SafePointer< Array<string> > result = new Array<string>(0x10);
		for (int i = 0; i < argc; i++) result->Append(string(argv[i], -1, Encoding::UTF8));
		result->Retain();
		return result;
	}
	void Sleep(uint32 time)
	{
		struct timespec req, elasped;
		req.tv_nsec = (time % 1000) * 1000000;
		req.tv_sec = time / 1000;
		do {
			int result = nanosleep(&req, &elasped);
			if (result == -1) {
				if (errno == EINTR) req = elasped;
				else return;
			} else return;
		} while (true);
	}
	void ExitProcess(int exit_code)
	{
		_exit(exit_code);
	}
}