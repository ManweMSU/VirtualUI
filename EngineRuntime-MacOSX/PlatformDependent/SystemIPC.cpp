#include "../Interfaces/SystemIPC.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

namespace Engine
{
	namespace IPC
	{
		class SystemPipe : public IPipe
		{
			bool _unlink;
			int _fid;
			uint _access;
			string _name, _path;
			Array<char> _path_chars;
		public:
			SystemPipe(const string & pipe_name, uint pipe_flags, uint * error) : _path_chars(0x10)
			{
				try {
					_name = pipe_name;
					_path = L"/tmp/pipe_" + _name;
					_path_chars.SetLength(_path.GetEncodedLength(Encoding::UTF8) + 1);
					_path.Encode(_path_chars.GetBuffer(), Encoding::UTF8, true);
				} catch (...) {
					if (error) *error = ErrorAllocation;
					throw;
				}
				if (pipe_flags & PipeCreateNew) {
					if (mkfifo(_path_chars, 0666) == -1) {
						if (error) {
							if (errno == EEXIST) *error = ErrorAlreadyExists;
							else *error = ErrorAllocation;
						}
						throw Exception();
					}
					_unlink = true;
				} else _unlink = false;
				try {
					if (pipe_flags & PipeAccessModeRead) {
						_access = PipeAccessModeRead;
						if (pipe_flags & PipeAccessModeWrite) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
						_fid = open(_path_chars, O_RDONLY | O_NONBLOCK);
					} else if (pipe_flags & PipeAccessModeWrite) {
						_access = PipeAccessModeWrite;
						if (pipe_flags & PipeAccessModeRead) { if (error) *error = ErrorInvalidArgument; throw Exception(); }
						if (pipe_flags & PipeCreateNew) {
							_fid = open(_path_chars, O_WRONLY);
						} else {
							_fid = open(_path_chars, O_WRONLY | O_NONBLOCK);
						}
					} else { if (error) *error = ErrorInvalidArgument; throw Exception(); }
					if (_fid == -1) {
						if (error) {
							if (errno == ENOENT) *error = ErrorNotExists;
							else if (errno == ENXIO) *error = ErrorWrongMode;
							else *error = ErrorAllocation;
						}
						throw Exception();
					}
					fcntl(_fid, F_SETFL, fcntl(_fid, F_GETFL) & ~O_NONBLOCK);
					struct stat fs;
					if (fstat(_fid, &fs) == -1) {
						close(_fid);
						if (error) *error = ErrorAllocation;
						throw Exception();
					}
					if ((fs.st_mode & S_IFMT) != S_IFIFO) {
						close(_fid);
						if (error) *error = ErrorAllocation;
						throw Exception();
					}
				} catch (...) {
					if (_unlink) unlink(_path_chars);
					throw;
				}
				if (error) *error = ErrorSuccess;
			}
			virtual ~SystemPipe(void) override { close(_fid); if (_unlink) unlink(_path_chars); }
			virtual handle GetIOHandle(void) noexcept override { return handle(sintptr(_fid)); }
			virtual string GetOSPath(void) override { return _path; }
			virtual string GetPipeName(void) override { return _name; }
			virtual uint GetAccessMode(void) noexcept override { return _access; }
		};
		class SystemSharedMemory : public ISharedMemory
		{
			string _name;
			Array<char> _name_chars;
			int _file;
			uint _length;
			bool _unlink;
			void * _pdata;
		public:
			SystemSharedMemory(const string & segment_name, uint length, uint flags, uint * error) : _pdata(0)
			{
				try {
					_name = segment_name;
					_name_chars.SetLength(_name.GetEncodedLength(Encoding::UTF8) + 1);
					_name.Encode(_name_chars.GetBuffer(), Encoding::UTF8, true);
				} catch (...) {
					if (error) *error = ErrorAllocation;
					throw;
				}
				if (!length) {
					if (error) *error = ErrorInvalidArgument;
					throw Exception();
				}
				if (flags & SharedMemoryCreateNew) {
					_length = length;
					_unlink = true;
					_file = shm_open(_name_chars, O_RDWR | O_CREAT | O_EXCL, 0666);
					if (_file == -1) {
						if (error) {
							if (errno == EEXIST) *error = ErrorAlreadyExists;
							else *error = ErrorAllocation;
						}
						throw Exception();
					}
					if (ftruncate(_file, length) == -1) {
						close(_file);
						shm_unlink(_name_chars);
						if (error) *error = ErrorAllocation;
						throw Exception();
					}
				} else {
					_unlink = false;
					_file = shm_open(_name_chars, O_RDWR);
					if (_file == -1) {
						if (error) {
							if (errno == ENOENT) *error = ErrorNotExists;
							else *error = ErrorAllocation;
						}
						throw Exception();
					}
					struct stat fs;
					if (fstat(_file, &fs) == -1) {
						close(_file);
						if (error) *error = ErrorAllocation;
						throw Exception();
					}
					if (fs.st_size > 0xFFFFFFFF) _length = 0xFFFFFFFF;
					else _length = fs.st_size;
				}
				if (error) *error = ErrorSuccess;
			}
			virtual ~SystemSharedMemory(void) override { Unmap(); close(_file); if (_unlink) shm_unlink(_name_chars); }
			virtual string GetSegmentName(void) override { return _name; }
			virtual uint GetLength(void) noexcept override { return _length; }
			virtual bool Map(void ** pdata, uint map_flags) noexcept override
			{
				if (_pdata) return false;
				int prot = PROT_NONE;
				if (map_flags & SharedMemoryMapRead) prot |= PROT_READ;
				if (map_flags & SharedMemoryMapWrite) prot |= PROT_WRITE;
				_pdata = mmap(0, _length, prot, MAP_SHARED, _file, 0);
				if (_pdata == MAP_FAILED) {
					_pdata = 0;
					return false;
				}
				if (pdata) *pdata = _pdata;
				return true;
			}
			virtual void Unmap(void) noexcept override
			{
				if (_pdata) {
					munmap(_pdata, _length);
					_pdata = 0;
				}
			}
		};
		class SystemSharedSemaphore : public ISharedSemaphore
		{
			string _name;
			Array<char> _name_chars;
			int _file;
			bool _locked;
		public:
			SystemSharedSemaphore(const string & semaphore_name, uint * error) : _locked(false)
			{
				try {
					string name_ex = L"/tmp/lock_" + semaphore_name;
					_name = semaphore_name;
					_name_chars.SetLength(name_ex.GetEncodedLength(Encoding::UTF8) + 1);
					name_ex.Encode(_name_chars.GetBuffer(), Encoding::UTF8, true);
				} catch (...) {
					if (error) *error = ErrorAllocation;
					throw;
				}
				_file = open(_name_chars, O_RDONLY | O_CREAT, 0666);
				if (_file == -1) {
					if (error) *error = ErrorAllocation;
					throw Exception();
				}
				if (error) *error = ErrorSuccess;
			}
			virtual ~SystemSharedSemaphore(void) override { if (_locked) flock(_file, LOCK_UN); close(_file); }
			virtual string GetSemaphoreName(void) override { return _name; }
			virtual bool TryWait(void) noexcept override
			{
				if (_locked) return false;
				int result = flock(_file, LOCK_EX | LOCK_NB);
				if (result == -1) return false;
				_locked = true;
				return true;
			}
			virtual void Open(void) noexcept override { if (_locked) { flock(_file, LOCK_UN); _locked = false; } }
		};

		IPipe * CreateNamedPipe(const string & pipe_name, uint pipe_flags, uint * error) { try { return new SystemPipe(pipe_name, pipe_flags, error); } catch (...) { return 0; } }
		ISharedMemory * CreateSharedMemory(const string & segment_name, uint length, uint flags, uint * error) { try { return new SystemSharedMemory(segment_name, length, flags, error); } catch (...) { return 0; } }
		ISharedSemaphore * CreateSharedSemaphore(const string & semaphore_name, uint * error) { try { return new SystemSharedSemaphore(semaphore_name, error); } catch (...) { return 0; } }
		
		void DestroyNamedPipe(const string & name)
		{
			Array<char> chars(1);
			string path = L"/tmp/pipe_" + name;
			chars.SetLength(path.GetEncodedLength(Encoding::UTF8) + 1);
			path.Encode(chars.GetBuffer(), Encoding::UTF8, true);
			unlink(chars);
		}
		void DestroySharedMemory(const string & name)
		{
			Array<char> chars(1);
			chars.SetLength(name.GetEncodedLength(Encoding::UTF8) + 1);
			name.Encode(chars.GetBuffer(), Encoding::UTF8, true);
			shm_unlink(chars);
		}
		void DestroySharedSemaphore(const string & name)
		{
			Array<char> chars(1);
			string path = L"/tmp/lock_" + name;
			chars.SetLength(path.GetEncodedLength(Encoding::UTF8) + 1);
			path.Encode(chars.GetBuffer(), Encoding::UTF8, true);
			unlink(chars);
		}
	}
}