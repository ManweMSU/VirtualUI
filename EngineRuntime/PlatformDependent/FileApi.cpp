#include "FileApi.h"
#include "../Miscellaneous/DynamicString.h"

#include <Windows.h>

#undef CreateFile
#undef MoveFile
#undef SetCurrentDirectory
#undef GetCurrentDirectory
#undef CreateDirectory
#undef RemoveDirectory

namespace Engine
{
	namespace IO
	{
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
			string Path = NormalizePath(path);
			DynamicString exp(0x100);
			exp.ReserveLength(0x1000);
			do {
				auto result = GetFullPathNameW(Path, exp.ReservedLength(), exp, 0);
				if (!result) throw Exception();
				if (result > uint(exp.ReservedLength())) exp.ReserveLength(result);
				else break;
			} while (true);
			return exp;
		}
		handle CreateFile(const string & path, FileAccess access, FileCreationMode mode)
		{
			DWORD Access = 0;
			DWORD Share = 0;
			if (access == FileAccess::AccessRead) {
				Access = GENERIC_READ;
				Share = FILE_SHARE_READ;
			} else if (access == FileAccess::AccessWrite) {
				Access = GENERIC_WRITE;
			} else if (access == FileAccess::AccessReadWrite) {
				Access = GENERIC_READ | GENERIC_WRITE;
			}
			DWORD Creation = 0;
			if (mode == FileCreationMode::CreateAlways) Creation = CREATE_ALWAYS;
			else if (mode == FileCreationMode::CreateNew) Creation = CREATE_NEW;
			else if (mode == FileCreationMode::OpenAlways) Creation = OPEN_ALWAYS;
			else if (mode == FileCreationMode::OpenExisting) Creation = OPEN_EXISTING;
			else if (mode == FileCreationMode::TruncateExisting) Creation = TRUNCATE_EXISTING;
			DWORD Flags = FILE_ATTRIBUTE_NORMAL;
			handle file = CreateFileW(NormalizePath(path), Access, Share, 0, Creation, Flags, 0);
			if (file == INVALID_HANDLE_VALUE) throw FileAccessException();
			return file;
		}
		handle CreateFileTemporary(const string & path, FileAccess access, FileCreationMode mode, bool delete_on_close)
		{
			if (delete_on_close && mode != CreateNew) throw InvalidArgumentException();
			DWORD Access = 0;
			DWORD Share = 0;
			if (access == FileAccess::AccessRead) {
				Access = GENERIC_READ;
				Share = FILE_SHARE_READ;
			} else if (access == FileAccess::AccessWrite) {
				Access = GENERIC_WRITE;
			} else if (access == FileAccess::AccessReadWrite) {
				Access = GENERIC_READ | GENERIC_WRITE;
			}
			DWORD Creation = 0;
			if (mode == FileCreationMode::CreateAlways) Creation = CREATE_ALWAYS;
			else if (mode == FileCreationMode::CreateNew) Creation = CREATE_NEW;
			else if (mode == FileCreationMode::OpenAlways) Creation = OPEN_ALWAYS;
			else if (mode == FileCreationMode::OpenExisting) Creation = OPEN_EXISTING;
			else if (mode == FileCreationMode::TruncateExisting) Creation = TRUNCATE_EXISTING;
			DWORD Flags = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY;
			if (delete_on_close) Flags |= FILE_FLAG_DELETE_ON_CLOSE;
			handle file = CreateFileW(NormalizePath(path), Access, Share, 0, Creation, Flags, 0);
			if (file == INVALID_HANDLE_VALUE) throw FileAccessException();
			return file;
		}
		void CreatePipe(handle * pipe_in, handle * pipe_out) { if (!::CreatePipe(pipe_out, pipe_in, 0, 0)) throw Exception(); }
		handle GetStandartOutput(void) { return GetStdHandle(STD_OUTPUT_HANDLE); }
		handle GetStandartInput(void) { return GetStdHandle(STD_INPUT_HANDLE); }
		handle GetStandartError(void) { return GetStdHandle(STD_ERROR_HANDLE); }
		void SetStandartOutput(handle file)
		{
			handle dup;
			if (!DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup, 0, TRUE, DUPLICATE_SAME_ACCESS)) throw FileAccessException();
			CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
			SetStdHandle(STD_OUTPUT_HANDLE, dup);
		}
		void SetStandartInput(handle file)
		{
			handle dup;
			if (!DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup, 0, TRUE, DUPLICATE_SAME_ACCESS)) throw FileAccessException();
			CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
			SetStdHandle(STD_INPUT_HANDLE, dup);
		}
		void SetStandartError(handle file)
		{
			handle dup;
			if (!DuplicateHandle(GetCurrentProcess(), file, GetCurrentProcess(), &dup, 0, TRUE, DUPLICATE_SAME_ACCESS)) throw FileAccessException();
			CloseHandle(GetStdHandle(STD_ERROR_HANDLE));
			SetStdHandle(STD_ERROR_HANDLE, dup);
		}
		void CloseFile(handle file) { CloseHandle(file); }
		void Flush(handle file) { if (!FlushFileBuffers(file)) throw FileAccessException(); }
		uint64 GetFileSize(handle file)
		{
			LARGE_INTEGER v;
			if (!GetFileSizeEx(file, &v)) throw FileAccessException();
			return uint64(v.QuadPart);
		}
		void MoveFile(const string & from, const string & to) { if (!MoveFileW(NormalizePath(from), NormalizePath(to))) throw FileAccessException(); }
		bool FileExists(const string & path)
		{
			handle file = CreateFileW(NormalizePath(path), 0, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
			if (file == INVALID_HANDLE_VALUE) return false;
			CloseHandle(file);
			return true;
		}
		void ReadFile(handle file, void * to, uint32 amount)
		{
			DWORD Read;
			if (!::ReadFile(file, to, amount, &Read, 0)) throw FileAccessException();
			if (Read != amount) throw FileReadEndOfFileException(Read);
		}
		void WriteFile(handle file, const void * data, uint32 amount)
		{
			DWORD Written;
			if (!::WriteFile(file, data, amount, &Written, 0)) throw FileAccessException();
			if (Written != amount) throw FileAccessException();
		}
		int64 Seek(handle file, int64 position, SeekOrigin origin)
		{
			LARGE_INTEGER set, get;
			set.QuadPart = position;
			DWORD org = FILE_BEGIN;
			if (origin == SeekOrigin::Current) org = FILE_CURRENT;
			else if (origin == SeekOrigin::End) org = FILE_END;
			if (!SetFilePointerEx(file, set, &get, org)) throw FileAccessException();
			return get.QuadPart;
		}
		void SetFileSize(handle file, uint64 size)
		{
			LARGE_INTEGER pos, len;
			len.QuadPart = int64(size);
			if (!SetFilePointerEx(file, { 0, 0 }, &pos, FILE_CURRENT)) throw FileAccessException();
			if (!SetFilePointerEx(file, len, 0, FILE_BEGIN)) throw FileAccessException();
			if (!SetEndOfFile(file)) { SetFilePointerEx(file, pos, 0, FILE_BEGIN); throw FileAccessException(); }
			SetFilePointerEx(file, pos, 0, FILE_BEGIN);
		}
		void RemoveFile(const string & path)
		{
			if (!DeleteFileW(NormalizePath(path))) throw FileAccessException();
		}
		void SetCurrentDirectory(const string & path)
		{
			if (!SetCurrentDirectoryW(ExpandPath(path))) throw FileAccessException();
		}
		string GetCurrentDirectory(void)
		{
			DynamicString exp(0x100);
			exp.ReserveLength(0x1000);
			do {
				auto result = GetCurrentDirectoryW(exp.ReservedLength(), exp);
				if (!result) throw Exception();
				if (result > uint(exp.ReservedLength())) exp.ReserveLength(result);
				else break;
			} while (true);
			return exp;
		}
		void CreateDirectory(const string & path)
		{
			if (!CreateDirectoryW(NormalizePath(path), 0)) {
				if (GetLastError() == ERROR_ALREADY_EXISTS) throw DirectoryAlreadyExistsException();
				throw FileAccessException();
			}
		}
		void RemoveDirectory(const string & path)
		{
			if (!RemoveDirectoryW(NormalizePath(path))) throw FileAccessException();
		}
		string GetExecutablePath(void)
		{
			DynamicString path;
			path.ReserveLength(0x1000);
			do {
				SetLastError(ERROR_SUCCESS);
				if (!GetModuleFileNameW(0, path, path.ReservedLength())) throw Exception();
				if (GetLastError() == ERROR_SUCCESS) return path;
				else if (GetLastError() != ERROR_INSUFFICIENT_BUFFER) throw Exception();
				path.ReserveLength(path.ReservedLength() * 2);
			} while (true);
			return path;
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
	}
}
