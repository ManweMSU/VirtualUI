#include "Process.h"

#include "../Miscellaneous/DynamicString.h"
#include "../PlatformDependent/FileApi.h"
#include "../PlatformDependent/CocoaInterop.h"

#include <time.h>
#include <crt_externs.h>

@import Foundation;

extern char **environ;

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
		NSString * app = Cocoa::CocoaString(IO::ExpandPath(IO::NormalizePath(image)));
		@try {
			NSTask * task = [NSTask launchedTaskWithLaunchPath: app arguments: argv];
			[app release];
			[argv release];
			return new CocoaProcess::Process(task);
		}
		@catch (NSException * e) {}
		[app release];
		[argv release];
		return 0;
	}
	Process * CreateCommandProcess(const string & command_image, const Array<string> * command_line)
	{
		if (command_image[0] == L'/') return CreateProcess(command_image, command_line);
		Array<string> search_list(0x10);
		search_list << IO::GetCurrentDirectory() + L"/" + command_image;
		search_list << IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + command_image;
		int envind = 0;
		while (environ[envind]) {
			if (environ[envind][0] == 'P' && environ[envind][1] == 'A' && environ[envind][2] == 'T' && environ[envind][3] == 'H' && environ[envind][4] == '=') {
				string path_var = string(environ[envind] + 5, -1, Encoding::UTF8);
				Array<string> paths = path_var.Split(L':');
				for (int i = 0; i < paths.Length(); i++) search_list << paths[i] + L"/" + command_image;
				break;
			}
			envind++;
		}
		NSMutableArray * argv = [[NSMutableArray alloc] init];
		if (command_line) {
			for (int i = 0; i < command_line->Length(); i++) {
				NSString * str = Cocoa::CocoaString(command_line->ElementAt(i));
				[argv addObject: str];
				[str release];
			}
		}
		for (int i = 0; i < search_list.Length(); i++) {
			NSString * app = Cocoa::CocoaString(IO::NormalizePath(search_list[i]));
			@try {
				NSTask * task = [NSTask launchedTaskWithLaunchPath: app arguments: argv];
				[app release];
				[argv release];
				return new CocoaProcess::Process(task);
			}
			@catch (NSException * e) {}
			[app release];
		}
		[argv release];
		return 0;
	}
	bool CreateProcessElevated(const string & image, const Array<string> * command_line)
	{
		int argc = 1 + (command_line ? command_line->Length() : 0);
		char ** argv = new (std::nothrow) char * [argc + 1];
		if (!argv) return false;
		ZeroMemory(argv, (argc + 1) * sizeof(char *));
		string image_full = IO::ExpandPath(image);
		argv[0] = new (std::nothrow) char[image_full.GetEncodedLength(Encoding::UTF8) + 1];
		if (!argv[0]) { delete[] argv; return false; }
		image_full.Encode(argv[0], Encoding::UTF8, true);
		if (command_line) for (int i = 0; i < command_line->Length(); i++) {
			argv[i + 1] = new (std::nothrow) char[command_line->ElementAt(i).GetEncodedLength(Encoding::UTF8) + 1];
			if (!argv[i + 1]) {
				for (int j = 0; j < i; j++) delete[] argv[j];
				delete[] argv;
				return false;
			}
			command_line->ElementAt(i).Encode(argv[i + 1], Encoding::UTF8, true);
		}
		AuthorizationRef auth;
		if (AuthorizationCreate(0, kAuthorizationEmptyEnvironment, kAuthorizationFlagDefaults, &auth) != errAuthorizationSuccess) {
			for (int j = 0; j < argc; j++) delete[] argv[j];
			delete[] argv;
			return false;
		}
		if (AuthorizationExecuteWithPrivileges(auth, argv[0], kAuthorizationFlagDefaults, argv + 1, 0) != errAuthorizationSuccess) {
			AuthorizationFree(auth, kAuthorizationFlagDefaults);
			for (int j = 0; j < argc; j++) delete[] argv[j];
			delete[] argv;
			return false;
		}
		AuthorizationFree(auth, kAuthorizationFlagDefaults);
		for (int j = 0; j < argc; j++) delete[] argv[j];
		delete[] argv;
		return true;
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