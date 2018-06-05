#include "FileApi.h"
#include "../Miscellaneous/Dictionary.h"
#include "../Syntax/Regular.h"

#define _DARWIN_FEATURE_ONLY_64_BIT_INODE

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <mach-o/dyld.h>
#include <dirent.h>
#include <sys/time.h>

namespace Engine
{
	namespace IO
	{
        namespace PosixHelper {
            Dictionary::Dictionary<int, Array<uint8> > delete_on_close(0x10);
        }

        string FileAccessException::ToString(void) const { return L"FileAccessException"; }
		FileReadEndOfFileException::FileReadEndOfFileException(uint32 data_read) : DataRead(data_read) {}
		string FileReadEndOfFileException::ToString(void) const { return L"FileReadEndOfFileException: Data read amount = " + string(DataRead); }
        string DirectoryAlreadyExistsException::ToString(void) const { return L"DirectoryAlreadyExistsException"; }
		string FileFormatException::ToString(void) const { return L"FileFormatException"; }
		string NormalizePath(const string & path)
		{
			if (PathChar == L'\\') return path.Replace(L'/', L'\\');
			else if (PathChar == L'/') return path.Replace(L'\\', L'/');
			return L"";
		}
        string ExpandPath(const string & path)
        {
            SafePointer<Array<uint8> > FullPath = new Array<uint8>(PATH_MAX);
			FullPath->SetLength(PATH_MAX);
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
			realpath(reinterpret_cast<char *>(Path->GetBuffer()), reinterpret_cast<char *>(FullPath->GetBuffer()));
            return string(FullPath->GetBuffer(), -1, Encoding::UTF8);
        }
		handle CreateFile(const string & path, FileAccess access, FileCreationMode mode)
		{
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
            int flags = 0;
            if (access == AccessRead) flags = O_RDONLY;
            else if (access == AccessWrite) flags = O_WRONLY;
            else if (access == AccessReadWrite) flags = O_RDWR;
            int result = -1;
            if (mode == CreateNew) flags |= O_CREAT | O_EXCL;
            else if (mode == CreateAlways) flags |= O_CREAT | O_TRUNC;
            else if (mode == OpenAlways) flags |= O_CREAT;
            else if (mode == TruncateExisting) flags |= O_TRUNC;
            result = open(reinterpret_cast<char *>(Path->GetBuffer()), flags, 0777);
            if (result == -1) throw FileAccessException();
            return handle(result);
		}
		handle CreateFileTemporary(const string & path, FileAccess access, FileCreationMode mode, bool delete_on_close)
		{
            if (delete_on_close && mode != CreateNew) throw InvalidArgumentException();
			SafePointer<Array<uint8> > FullPath = new Array<uint8>(PATH_MAX);
			FullPath->SetLength(PATH_MAX);
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
			realpath(reinterpret_cast<char *>(Path->GetBuffer()), reinterpret_cast<char *>(FullPath->GetBuffer()));
			FullPath->SetLength(MeasureSequenceLength(FullPath->GetBuffer(), -1, Encoding::UTF8, Encoding::UTF8) + 1);
            int flags = 0;
            if (access == AccessRead) flags = O_RDONLY;
            else if (access == AccessWrite) flags = O_WRONLY;
            else if (access == AccessReadWrite) flags = O_RDWR;
            int result = -1;
            if (mode == CreateNew) flags |= O_CREAT | O_EXCL;
            else if (mode == CreateAlways) flags |= O_CREAT | O_TRUNC;
            else if (mode == OpenAlways) flags |= O_CREAT;
            else if (mode == TruncateExisting) flags |= O_TRUNC;
            result = open(reinterpret_cast<char *>(FullPath->GetBuffer()), flags, 0777);
            if (result == -1) throw FileAccessException();
			if (delete_on_close) PosixHelper::delete_on_close.Append(result, FullPath);
            return handle(result);
		}
		void CreatePipe(handle * pipe_in, handle * pipe_out)
        {
            int result[2];
            if (pipe(result) == -1) throw Exception();
            *pipe_in = reinterpret_cast<handle>(result[1]);
            *pipe_out = reinterpret_cast<handle>(result[0]);
        }
		handle GetStandartOutput(void) { return handle(1); }
		handle GetStandartInput(void) { return handle(0); }
		handle GetStandartError(void) { return handle(2); }
		void SetStandartOutput(handle file) { close(1); dup2(reinterpret_cast<intptr>(file), 1); }
		void SetStandartInput(handle file) { close(0); dup2(reinterpret_cast<intptr>(file), 0); }
		void SetStandartError(handle file) { close(2); dup2(reinterpret_cast<intptr>(file), 2); }
		handle CloneHandle(handle file)
		{
			int new_file = dup(reinterpret_cast<intptr>(file));
			if (new_file == -1) throw FileAccessException();
			return handle(new_file);
		}
		void CloseFile(handle file) {
			close(reinterpret_cast<intptr>(file));
			auto Path = PosixHelper::delete_on_close.ElementByKey(reinterpret_cast<intptr>(file));
			if (Path) {
				unlink(reinterpret_cast<char *>(Path->GetBuffer()));
				PosixHelper::delete_on_close.RemoveByKey(reinterpret_cast<intptr>(file));
			}
		}
		void Flush(handle file) { if (fsync(reinterpret_cast<intptr>(file)) == -1) throw FileAccessException(); }
		uint64 GetFileSize(handle file)
		{
            struct stat64 info;
            if (fstat64(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException();
            return info.st_size;
		}
		void MoveFile(const string & from, const string & to)
        {
            SafePointer<Array<uint8>> From = NormalizePath(from).EncodeSequence(Encoding::UTF8, true);
            SafePointer<Array<uint8>> To = NormalizePath(to).EncodeSequence(Encoding::UTF8, true);
            if (rename(reinterpret_cast<char *>(From->GetBuffer()), reinterpret_cast<char *>(To->GetBuffer())) == -1) throw FileAccessException();
        }
		bool FileExists(const string & path)
		{
            SafePointer<Array<uint8>> Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
            int file = open(reinterpret_cast<char *>(Path->GetBuffer()), O_RDONLY);
            if (file >= 0) {
                close(file);
                return true;
            } else return false;
		}
		void ReadFile(handle file, void * to, uint32 amount)
		{
            auto Read = read(reinterpret_cast<intptr>(file), to, amount);
            if (Read == -1) throw FileAccessException();
            if (Read < amount) throw FileReadEndOfFileException(Read);
		}
		void WriteFile(handle file, const void * data, uint32 amount)
		{
            auto Written = write(reinterpret_cast<intptr>(file), data, amount);
            if (Written == -1 || Written != amount) throw FileAccessException();
		}
		int64 Seek(handle file, int64 position, SeekOrigin origin)
		{
            int org = SEEK_SET;
            if (origin == Current) org = SEEK_CUR;
            else if (origin == End) org = SEEK_END;
            auto result = lseek(reinterpret_cast<intptr>(file), position, org);
            if (result == -1) throw FileAccessException();
			return result;
		}
		void SetFileSize(handle file, uint64 size)
		{
            if (ftruncate(reinterpret_cast<intptr>(file), size) == -1) throw FileAccessException();
		}
        void RemoveFile(const string & path)
        {
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
            if (unlink(reinterpret_cast<char *>(Path->GetBuffer())) == -1) throw FileAccessException();
        }
        void SetCurrentDirectory(const string & path)
        {
            SafePointer<Array<uint8> > FullPath = new Array<uint8>(PATH_MAX);
			FullPath->SetLength(PATH_MAX);
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
			realpath(reinterpret_cast<char *>(Path->GetBuffer()), reinterpret_cast<char *>(FullPath->GetBuffer()));
            if (chdir(reinterpret_cast<char *>(FullPath->GetBuffer())) != 0) throw Exception();
        }
		string GetCurrentDirectory(void)
        {
            SafePointer< Array<uint8> > Path = new Array<uint8>(PATH_MAX);
            Path->SetLength(PATH_MAX);
            do {
                if (getcwd(reinterpret_cast<char *>(Path->GetBuffer()), Path->Length())) break;
                if (errno == ENOENT) throw Exception();
                else if (errno == ENOMEM) throw OutOfMemoryException();
                else if (errno != ERANGE) throw Exception();
                Path->SetLength(Path->Length() * 2);
            } while(true);
            return string(Path->GetBuffer(), -1, Encoding::UTF8);
        }
        void CreateDirectory(const string & path)
		{
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
            if (mkdir(reinterpret_cast<char *>(Path->GetBuffer()), 0777) == -1) {
                if (errno == EEXIST) {
                    throw DirectoryAlreadyExistsException();
                }
                throw FileAccessException();
            }
		}
		void RemoveDirectory(const string & path)
		{
            SafePointer<Array<uint8> > Path = NormalizePath(path).EncodeSequence(Encoding::UTF8, true);
            if (rmdir(reinterpret_cast<char *>(Path->GetBuffer())) == -1) throw FileAccessException();
		}
		string GetExecutablePath(void)
		{
            Array<uint8> Path(0x800);
            Path.SetLength(0x800);
            uint32 length = Path.Length();
            if (_NSGetExecutablePath(reinterpret_cast<char *>(Path.GetBuffer()), &length) == -1) {
                Path.SetLength(length);
                _NSGetExecutablePath(reinterpret_cast<char *>(Path.GetBuffer()), &length);
            }
            return ExpandPath(string(Path.GetBuffer(), -1, Encoding::UTF8));
		}
		namespace Path
		{
			string GetExtension(const string & path)
			{
				int s = path.Length() - 1;
				while (s >= 0 && (path[s] == L'\\' || path[s] == L'/')) s--;
				for (int i = s; i >= 0; i--) {
					if (path[i] == L'.') {
						if (i != 0 && path[i - 1] != L'\\' && path[i - 1] != L'/') return path.Fragment(i + 1, s - i);
					}
					if (path[i] == L'/' || path[i] == L'\\') return L"";
				}
				return L"";
			}
			string GetFileName(const string & path)
			{
				int s = path.Length() - 1;
				while (s >= 0 && (path[s] == L'\\' || path[s] == L'/')) s--;
				for (int i = s; i >= 0; i--) {
					if (path[i] == L'\\' || path[i] == L'/') return path.Fragment(i + 1, s - i);
				}
				return path.Fragment(0, s + 1);
			}
			string GetDirectory(const string & path)
			{
				int s = path.Length() - 1;
				while (s >= 0 && (path[s] == L'\\' || path[s] == L'/')) s--;
				for (int i = s; i >= 0; i--) {
					if (path[i] == L'\\' || path[i] == L'/') return path.Fragment(0, i);
				}
				return L"";
			}
			string GetFileNameWithoutExtension(const string & path)
			{
				string name = GetFileName(path);
				for (int i = name.Length() - 1; i > 0; i--) {
					if (name[i] == L'.') return name.Fragment(0, i);
				}
				return name;
			}
		}
		namespace Search
		{
			namespace SearchHelper
			{
				void FillFiles(Array<string> * to, const string & path, const string & filter, const string & prefix, bool recursive)
				{
					if (recursive) {
						SafePointer< Array<string> > subs = GetDirectories(path + L"/*");
						for (int i = 0; i < subs->Length(); i++) {
							FillFiles(to, path + L"/" + subs->ElementAt(i), filter, prefix + subs->ElementAt(i) + string(PathChar), recursive);
						}
					}
					SafePointer< Array<uint8> > path_utf8 = path.EncodeSequence(Encoding::UTF8, true);
					struct dirent ** elements;
					int count = scandir(reinterpret_cast<char *>(path_utf8->GetBuffer()), &elements, 0, alphasort);
					if (count >= 0) {
						for (int i = 0; i < count; i++) {
							if (elements[i]->d_type == DT_REG) {
								string name = string(elements[i]->d_name, elements[i]->d_namlen, Encoding::UTF8);
								if (Syntax::MatchFilePattern(name, filter)) to->Append(prefix + name);
							}
							free(elements[i]);
						}
						free(elements);
					}
				}
			}
			Array<string>* GetFiles(const string & path, bool recursive)
			{
				SafePointer< Array<string> > result = new Array<string>(0x10);
				auto epath = ExpandPath(path);
				SearchHelper::FillFiles(result, Path::GetDirectory(epath), Path::GetFileName(epath), L"", recursive);
				result->Retain();
				return result;
			}
			Array<string>* GetDirectories(const string & path)
			{
				SafePointer< Array<string> > result = new Array<string>(0x10);
				auto epath = ExpandPath(path);
				auto filter = Path::GetFileName(epath);
				SafePointer< Array<uint8> > path_utf8 = Path::GetDirectory(epath).EncodeSequence(Encoding::UTF8, true);
				struct dirent ** elements;
				int count = scandir(reinterpret_cast<char *>(path_utf8->GetBuffer()), &elements, 0, alphasort);
				if (count >= 0) {
					for (int i = 0; i < count; i++) {
						{
							string name = string(elements[i]->d_name, elements[i]->d_namlen, Encoding::UTF8);
							if (name != L"." && name != L".." && Syntax::MatchFilePattern(name, filter)) result->Append(name);
						}
						free(elements[i]);
					}
					free(elements);
				}
				result->Retain();
				return result;
			}
		}
		namespace DateTime
		{
			Time GetFileCreationTime(handle file)
			{
				struct stat64 info;
            	if (fstat64(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException();
            	return Time::FromUnixTime(info.st_birthtimespec.tv_sec);
			}
			Time GetFileAccessTime(handle file)
			{
				struct stat64 info;
            	if (fstat64(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException();
            	return Time::FromUnixTime(info.st_atimespec.tv_sec);
			}
			Time GetFileAlterTime(handle file)
			{
				struct stat64 info;
            	if (fstat64(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException();
            	return Time::FromUnixTime(info.st_mtimespec.tv_sec);
			}
			void SetFileCreationTime(handle file, Time time) {}
			void SetFileAccessTime(handle file, Time time)
			{
				struct stat64 info;
            	if (fstat64(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException();
				struct timeval times[2];
				times[0].tv_sec = time.ToUnixTime();
				times[0].tv_usec = 0;
				times[1].tv_sec = info.st_mtimespec.tv_sec;
				times[1].tv_usec = info.st_mtimespec.tv_nsec / 1000;
				if (futimes(reinterpret_cast<intptr>(file), times) == -1) throw FileAccessException();
			}
			void SetFileAlterTime(handle file, Time time)
			{
				struct stat64 info;
            	if (fstat64(reinterpret_cast<intptr>(file), &info) == -1) throw FileAccessException();
				struct timeval times[2];
				times[0].tv_sec = info.st_atimespec.tv_sec;
				times[0].tv_usec = info.st_atimespec.tv_nsec / 1000;
				times[1].tv_sec = time.ToUnixTime();
				times[1].tv_usec = 0;
				if (futimes(reinterpret_cast<intptr>(file), times) == -1) throw FileAccessException();
			}
		}
	}
}

    