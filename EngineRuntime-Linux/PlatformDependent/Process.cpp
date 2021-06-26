#include "ProcessAPI.h"

#include "../Interfaces/SystemIO.h"
#include "../Interfaces/Threading.h"

#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <wait.h>

namespace Engine
{
	class SystemProcess : public Process
	{
		static SafePointer<Thread> _wait_thread;
		static SafePointer<Semaphore> _table_sync;
		static ObjectArray<SystemProcess> _process_table;
		static uint _num_processes;

		pid_t _pid;
		int _retval;
		SafePointer<Semaphore> _wait_sync;

		static int _watcher_thread(void *)
		{
			SafePointer<Semaphore> sync;
			sync.SetRetain(_table_sync);
			while (true) {
				int status;
				pid_t pid = wait(&status);
				if (pid == -1) {
					if (errno == ECHILD) break;
					else if (errno == EINTR) continue;
					else break;
				}
				sync->Wait();
				for (int i = 0; i < _process_table.Length(); i++) {
					if (_process_table.ElementAt(i) && _process_table[i]._pid == pid) {
						_process_table[i]._pid = -1;
						if (WIFEXITED(status)) _process_table[i]._retval = WEXITSTATUS(status);
						else _process_table[i]._retval = -1;
						_process_table[i]._wait_sync->Open();
						_process_table.SetElement(0, i);
						_num_processes--;
						break;
					}
				}
				for (int i = _process_table.Length() - 1; i >= 0; i--) {
					if (_process_table.ElementAt(i)) {
						while (_process_table.Length() > i + 1) _process_table.RemoveLast();
						break;
					}
					if (i == 0) _process_table.Clear();
				}
				if (!_num_processes) {
					_wait_thread.SetReference(0);
					sync->Open();
					return 0;
				}
				sync->Open();
			}
			sync->Wait();
			_wait_thread.SetReference(0);
			sync->Open();
			return 0;
		}
	public:
		SystemProcess(void) : _pid(-1), _retval(-1) { _wait_sync = CreateSemaphore(0); if (!_wait_sync) throw Exception(); }
		virtual ~SystemProcess(void) override {}
		virtual bool Exited(void) override
		{
			bool result;
			if (result = _wait_sync->TryWait()) _wait_sync->Open();
			return result;
		}
		virtual int GetExitCode(void) override { return Exited() ? _retval : -1; }
		virtual void Wait(void) override { _wait_sync->Wait(); _wait_sync->Open(); }
		virtual void Terminate(void) override { if (_pid < 0) return; kill(_pid, SIGKILL); }
		static void WaiterThreadLock(void) noexcept { if (_table_sync) _table_sync->Wait(); }
		static void UpdatePID(SystemProcess * process, pid_t pid) noexcept { process->_pid = pid; }
		static bool Initialize(void) noexcept
		{
			if (_table_sync) return true;
			_table_sync = CreateSemaphore(1);
			if (_table_sync) return true; else return false;
		}
		static void LockProcessTable(void) noexcept { _table_sync->Wait(); }
		static void UnlockProcessTable(void) noexcept { _table_sync->Open(); }
		static void RegisterProcess(SystemProcess * process) noexcept
		{
			try {
				if (!_wait_thread) {
					_wait_thread = CreateThread(_watcher_thread, 0, 0x10000);
					if (!_wait_thread) throw Exception();
				}
				int insert = -1;
				for (int i = 0; i < _process_table.Length(); i++) if (!_process_table.ElementAt(i)) { insert = i; break; }
				if (insert >= 0) _process_table.SetElement(process, insert);
				else _process_table.Append(process);
				_num_processes++;
			} catch (...) {
				process->_retval = -1;
				process->_pid = -1;
				process->_wait_sync->Open();
			}
		}
		static char * StringToUtf8(const string & str) noexcept
		{
			char * result = reinterpret_cast<char *>(malloc(str.GetEncodedLength(Encoding::UTF8) + 1));
			if (!result) return 0;
			str.Encode(result, Encoding::UTF8, true);
			return result;
		}
		static void FreeArgV(char ** argv, int argc) noexcept
		{
			for (int i = 0; i < argc; i++) free(argv[i]);
			free(argv);
		}
	};

	char ** ThisProcessArgV = 0;
	int ThisProcessArgC = 0;
	SafePointer<Thread> SystemProcess::_wait_thread;
	SafePointer<Semaphore> SystemProcess::_table_sync;
	ObjectArray<SystemProcess> SystemProcess::_process_table(0x20);
	uint SystemProcess::_num_processes = 0;

	void ProcessWaiterThreadLock(void) { SystemProcess::WaiterThreadLock(); }
	Process * CreateProcess(const string & image, const string * cmdv, int cmdc)
	{
		if (!SystemProcess::Initialize()) return 0;
		SafePointer<SystemProcess> result;
		try { result = new SystemProcess; } catch (...) { return 0; }
		int argc = 1;
		if (cmdv) argc += cmdc;
		char ** argv = reinterpret_cast<char **>(malloc(sizeof(char *) * (argc + 1)));
		if (!argv) return 0;
		ZeroMemory(argv, sizeof(char *) * (argc + 1));
		try {
			auto image_full = IO::ExpandPath(image);
			argv[0] = SystemProcess::StringToUtf8(image_full);
			if (!argv[0]) throw OutOfMemoryException();
			if (cmdv) for (int i = 0; i < cmdc; i++) {
				argv[i + 1] = SystemProcess::StringToUtf8(cmdv[i]);
				if (!argv[i + 1]) throw OutOfMemoryException();
			}
		} catch (...) {
			SystemProcess::FreeArgV(argv, argc);
			return 0;
		}
		int stat_pipe[2];
		if (pipe2(stat_pipe, O_CLOEXEC)) {
			SystemProcess::FreeArgV(argv, argc);
			return 0;
		}
		SystemProcess::LockProcessTable();
		pid_t fork_stat = fork();
		if (fork_stat == -1) {
			close(stat_pipe[0]);
			close(stat_pipe[1]);
			SystemProcess::UnlockProcessTable();
			SystemProcess::FreeArgV(argv, argc);
			return 0;
		} else if (fork_stat == 0) {
			close(stat_pipe[0]);
			execv(argv[0], argv);
			uint32 err = errno;
			write(stat_pipe[1], &err, 4);
			close(stat_pipe[1]);
			_exit(-1);
		}
		close(stat_pipe[1]);
		SystemProcess::FreeArgV(argv, argc);
		while (true) {
			uint32 err_stat;
			auto err_length = read(stat_pipe[0], &err_stat, 4);
			if (err_length > 0) {
				close(stat_pipe[0]);
				SystemProcess::UnlockProcessTable();
				return 0;
			} else if (err_length == 0) {
				break;
			} else if (err_length == -1 && errno != EINTR) {
				close(stat_pipe[0]);
				SystemProcess::UnlockProcessTable();
				return 0;
			}
		}
		close(stat_pipe[0]);
		SystemProcess::UpdatePID(result, fork_stat);
		SystemProcess::RegisterProcess(result);
		SystemProcess::UnlockProcessTable();
		result->Retain();
		return result;
	}
	Process * CreateProcess(const string & image, const string * cmdv, int cmdc, int flags)
	{
		if (image[0] == L'/') return CreateProcess(image, cmdv, cmdc);
		if (flags & CreateProcessSearchCurrent) {
			auto path = IO::ExpandPath(IO::GetCurrentDirectory() + L"/" + image);
			auto process = CreateProcess(path, cmdv, cmdc);
			if (process) return process;
		}
		if (flags & CreateProcessSearchSelf) {
			auto path = IO::ExpandPath(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + image);
			auto process = CreateProcess(path, cmdv, cmdc);
			if (process) return process;
		}
		if (flags & CreateProcessSearchPath) {
			int envind = 0;
			while (environ[envind]) {
				if (MemoryCompare(environ[envind], "PATH=", 5) == 0) {
					auto paths = string(environ[envind] + 5, -1, Encoding::UTF8).Split(L':');
					for (auto & pref : paths) {
						auto path = IO::ExpandPath(pref + L"/" + image);
						auto process = CreateProcess(path, cmdv, cmdc);
						if (process) return process;
					}
					break;
				}
				envind++;
			}
		}
		return 0;
	}
	Process * CreateProcess(const string & image, const Array<string> * command_line)
	{
		if (command_line) return CreateProcess(image, command_line->GetBuffer(), command_line->Length());
		else return CreateProcess(image, 0, 0);
	}
	Process * CreateCommandProcess(const string & command_image, const Array<string> * command_line)
	{
		const string * argv = command_line ? command_line->GetBuffer() : 0;
		int argc = command_line ? command_line->Length() : 0;
		return CreateProcess(command_image, argv, argc, CreateProcessSearchAll);
	}
	bool CreateProcessElevated(const string & image, const Array<string> * command_line)
	{
		try {
			Array<string> cmds(0x10);
			cmds << IO::GetExecutablePath();
			cmds << L"--ert_elevate";
			cmds << IO::ExpandPath(image);
			if (command_line) cmds.Append(*command_line);
			cmds << L"--ert_end";
			SafePointer<Process> agent = CreateProcess(L"pkexec", cmds.GetBuffer(), cmds.Length(), CreateProcessSearchPath);
			if (!agent) return false;
			agent->Wait();
			return agent->GetExitCode() == 0;
		} catch (...) { return false; }
	}
	bool CreateDetachedProcess(char ** argv)
	{
		int stat_pipe[2];
		if (pipe2(stat_pipe, O_CLOEXEC)) return false;
		pid_t fork_stat = fork();
		if (fork_stat == -1) {
			close(stat_pipe[0]);
			close(stat_pipe[1]);
			return false;
		} else if (fork_stat == 0) {
			close(stat_pipe[0]);
			execv(argv[0], argv);
			uint32 err = errno;
			write(stat_pipe[1], &err, 4);
			close(stat_pipe[1]);
			_exit(-1);
		}
		close(stat_pipe[1]);
		while (true) {
			uint32 err_stat;
			auto err_length = read(stat_pipe[0], &err_stat, 4);
			if (err_length > 0) {
				close(stat_pipe[0]);
				return false;
			} else if (err_length == 0) {
				break;
			} else if (err_length == -1 && errno != EINTR) {
				close(stat_pipe[0]);
				return false;
			}
		}
		close(stat_pipe[0]);
		return true;
	}
	Array<string> * GetCommandLine(void)
	{
		SafePointer< Array<string> > result = new Array<string>(0x10);
		if (ThisProcessArgV) for (int i = 0; i < ThisProcessArgC; i++) result->Append(string(ThisProcessArgV[i], -1, Encoding::UTF8));
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
	void ExitProcess(int exit_code) { _exit(exit_code); }
}