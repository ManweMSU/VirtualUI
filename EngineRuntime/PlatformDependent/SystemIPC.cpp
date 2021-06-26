#include "../Interfaces/SystemIPC.h"

#include <Windows.h>

#undef CreateNamedPipe

namespace Engine
{
	namespace IPC
	{
		class SystemPipe : public IPipe
		{
			handle _pipe;
			string _name, _path;
			uint _mode;
		public:
			SystemPipe(const string & pipe_name, uint pipe_flags, uint * error)
			{
				try {
					_name = pipe_name;
					_path = L"\\\\.\\pipe\\" + _name;
				} catch (...) { if (error) *error = ErrorAllocation; throw; }
				if (pipe_flags & PipeCreateNew) {
					if (pipe_flags & PipeAccessModeRead) {
						if (pipe_flags & PipeAccessModeWrite) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
						_pipe = CreateNamedPipeW(_path, PIPE_ACCESS_INBOUND, 0, 1, 512, 512, 0, 0);
						_mode = PipeAccessModeRead;
					} else if (pipe_flags & PipeAccessModeWrite) {
						if (pipe_flags & PipeAccessModeRead) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
						_pipe = CreateNamedPipeW(_path, PIPE_ACCESS_OUTBOUND, 0, 1, 512, 512, 0, 0);
						_mode = PipeAccessModeWrite;
						if (_pipe != INVALID_HANDLE_VALUE) ConnectNamedPipe(_pipe, 0);
					} else {
						if (error) *error = ErrorInvalidArgument;
						throw Exception();
					}
					if (_pipe == INVALID_HANDLE_VALUE) {
						if (error) {
							auto last_error = GetLastError();
							if (last_error == ERROR_PIPE_BUSY) *error = ErrorAlreadyExists;
							else *error = ErrorAllocation;
						}
						throw Exception();
					} else if (error) *error = ErrorSuccess;
				} else {
					if (pipe_flags & PipeAccessModeRead) {
						if (pipe_flags & PipeAccessModeWrite) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
						_pipe = CreateFileW(_path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
						_mode = PipeAccessModeRead;
					} else if (pipe_flags & PipeAccessModeWrite) {
						if (pipe_flags & PipeAccessModeRead) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
						_pipe = CreateFileW(_path, GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, 0, 0);
						_mode = PipeAccessModeWrite;
					} else {
						if (error) *error = ErrorInvalidArgument;
						throw Exception();
					}
					if (_pipe == INVALID_HANDLE_VALUE) {
						if (error) {
							auto last_error = GetLastError();
							if (last_error == ERROR_FILE_NOT_FOUND) *error = ErrorNotExists;
							else if (last_error == ERROR_PIPE_BUSY) *error = ErrorBusyNow;
							else if (last_error == ERROR_ACCESS_DENIED) *error = ErrorWrongMode;
							else *error = ErrorAllocation;
						}
						throw Exception();
					} else if (error) *error = ErrorSuccess;
				}
			}
			virtual ~SystemPipe(void) override { CloseHandle(_pipe); }
			virtual handle GetIOHandle(void) noexcept override { return _pipe; }
			virtual string GetOSPath(void) override { return _path; }
			virtual string GetPipeName(void) override { return _name; }
			virtual uint GetAccessMode(void) noexcept override { return _mode; }
		};
		class SystemSharedMemory : public ISharedMemory
		{
			handle _file;
			string _name;
			void * _lock_addr;
			uint _length;
		public:
			SystemSharedMemory(const string & segment_name, uint length, uint flags, uint * error) : _lock_addr(0), _length(length)
			{
				try { _name = segment_name; } catch (...) { if (error) *error = ErrorAllocation; throw; }
				if (!length) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
				if (flags & SharedMemoryCreateNew) {
					_file = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, length, _name);
					if (_file && GetLastError() == ERROR_ALREADY_EXISTS) {
						CloseHandle(_file);
						if (error) *error = ErrorAlreadyExists;
						throw Exception();
					} else if (!_file) {
						if (error) *error = ErrorAllocation;
						throw Exception();
					}
				} else {
					_file = CreateFileMappingW(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, length, _name);
					if (_file && GetLastError() != ERROR_ALREADY_EXISTS) {
						CloseHandle(_file);
						if (error) *error = ErrorNotExists;
						throw Exception();
					} else if (!_file) {
						if (error) *error = ErrorAllocation;
						throw Exception();
					}
				}
				if (error) *error = ErrorSuccess;
			}
			virtual ~SystemSharedMemory(void) override { if (_lock_addr) Unmap(); CloseHandle(_file); }
			virtual string GetSegmentName(void) override { return _name; }
			virtual uint GetLength(void) noexcept override { return _length; }
			virtual bool Map(void ** pdata, uint map_flags) noexcept override
			{
				if (_lock_addr) return false;
				DWORD flags = 0;
				if (map_flags & SharedMemoryMapRead) flags |= FILE_MAP_READ;
				if (map_flags & SharedMemoryMapWrite) flags |= FILE_MAP_WRITE;
				if (!flags) return false;
				_lock_addr = MapViewOfFile(_file, flags, 0, 0, _length);
				if (!_lock_addr) return false;
				if (pdata) *pdata = _lock_addr;
				return true;
			}
			virtual void Unmap(void) noexcept override { if (_lock_addr) UnmapViewOfFile(_lock_addr); _lock_addr = 0; }
		};
		class SystemSharedSemaphore : public ISharedSemaphore
		{
			handle _semaphore;
			string _name;
			bool _locked;
		public:
			SystemSharedSemaphore(const string & semaphore_name, uint * error) : _locked(false)
			{
				try { _name = semaphore_name; } catch (...) {
					if (error) *error = ErrorAllocation;
					throw;
				}
				_semaphore = CreateSemaphoreW(0, 1, 1, _name);
				if (!_semaphore) {
					if (error) *error = ErrorAlreadyExists;
					throw Exception();
				}
				if (error) *error = ErrorSuccess;
			}
			virtual ~SystemSharedSemaphore(void) override { Open(); CloseHandle(_semaphore); }
			virtual string GetSemaphoreName(void) override { return _name; }
			virtual bool TryWait(void) noexcept override
			{
				if (_locked) return false;
				auto result = WaitForSingleObject(_semaphore, 0) == WAIT_OBJECT_0;
				if (result) _locked = true;
				return result;
			}
			virtual void Open(void) noexcept override
			{
				if (_locked) {
					ReleaseSemaphore(_semaphore, 1, 0);
					_locked = false;
				}
			}
		};

		IPipe * CreateNamedPipe(const string & pipe_name, uint pipe_flags, uint * error) { try { return new SystemPipe(pipe_name, pipe_flags, error); } catch (...) { return 0; } }
		ISharedMemory * CreateSharedMemory(const string & segment_name, uint length, uint flags, uint * error) { try { return new SystemSharedMemory(segment_name, length, flags, error); } catch (...) { return 0; } }
		ISharedSemaphore * CreateSharedSemaphore(const string & semaphore_name, uint * error) { try { return new SystemSharedSemaphore(semaphore_name, error); } catch (...) { return 0; } }

		void DestroyNamedPipe(const string & name) {}
		void DestroySharedMemory(const string & name) {}
		void DestroySharedSemaphore(const string & name) {}
	}
}