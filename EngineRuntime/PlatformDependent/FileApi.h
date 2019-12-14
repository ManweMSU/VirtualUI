#pragma once

#include "../EngineBase.h"
#include "../Miscellaneous/Time.h"

namespace Engine
{
	namespace IO
	{
		namespace Error
		{
			enum IOError {
				Success				= 0x0000,
				Unknown				= 0x0001,
				FileNotFound		= 0x0002,
				PathNotFound		= 0x0003,
				TooManyOpenFiles	= 0x0004,
				AccessDenied		= 0x0005,
				InvalidHandle		= 0x0006,
				NotEnoughMemory		= 0x0007,
				InvalidDevice		= 0x0008,
				IsReadOnly			= 0x0009,
				NoDiskSpace			= 0x000A,
				FileExists			= 0x000B,
				NotImplemented		= 0x000C,
				DirectoryNotEmpty	= 0x000D,
				DirectoryIsCurrent	= 0x000E,
				NotSameDevice		= 0x000F,
				BadPathName			= 0x0010,
				FileNameTooLong		= 0x0011,
				FileTooLarge		= 0x0012,
				ReadFailure			= 0x0013,
				WriteFailure		= 0x0014,
				CreateFailure		= 0x0015,
				OpenFailure			= 0x0016,
			};
		}
		class FileAccessException : public Exception { public: uint code; FileAccessException(uint ec); FileAccessException(void); string ToString(void) const override; };
		class FileReadEndOfFileException : public Exception { public: uint32 DataRead; FileReadEndOfFileException(uint32 data_read); string ToString(void) const override; };
		class DirectoryAlreadyExistsException : public Exception { public: string ToString(void) const override; };
		class FileFormatException : public Exception { public: string ToString(void) const override; };

		enum FileAccess { AccessRead, AccessWrite, AccessReadWrite };
		enum FileCreationMode { CreateAlways, CreateNew, OpenAlways, OpenExisting, TruncateExisting };
		enum SeekOrigin { Begin, Current, End };

#ifdef ENGINE_WINDOWS
		constexpr widechar PathChar = L'\\';
		constexpr const widechar * NewLineChar = L"\r\n";
		constexpr Encoding TextFileEncoding = Encoding::ANSI;
#endif
#ifdef ENGINE_UNIX
		constexpr widechar PathChar = L'/';
		constexpr const widechar * NewLineChar = L"\n";
		constexpr Encoding TextFileEncoding = Encoding::UTF8;
#endif

		string NormalizePath(const string & path);
		string ExpandPath(const string & path);
		handle CreateFile(const string & path, FileAccess access, FileCreationMode mode);
		handle CreateFileTemporary(const string & path, FileAccess access, FileCreationMode mode, bool delete_on_close);
		void CreatePipe(handle * pipe_in, handle * pipe_out);
		handle GetStandardOutput(void);
		handle GetStandardInput(void);
		handle GetStandardError(void);
		void SetStandardOutput(handle file);
		void SetStandardInput(handle file);
		void SetStandardError(handle file);
		handle CloneHandle(handle file);
		void CloseFile(handle file);
		void Flush(handle file);
		uint64 GetFileSize(handle file);
		void MoveFile(const string & from, const string & to);
		bool FileExists(const string & path);
		void ReadFile(handle file, void * to, uint32 amount);
		void WriteFile(handle file, const void * data, uint32 amount);
		int64 Seek(handle file, int64 position, SeekOrigin origin);
		void SetFileSize(handle file, uint64 size);
		void RemoveFile(const string & path);
		void SetCurrentDirectory(const string & path);
		string GetCurrentDirectory(void);
		void CreateDirectory(const string & path);
		void RemoveDirectory(const string & path);
		void CreateSymbolicLink(const string & at, const string & to);
		string GetExecutablePath(void);
		namespace Path
		{
			string GetExtension(const string & path);
			string GetFileName(const string & path);
			string GetDirectory(const string & path);
			string GetFileNameWithoutExtension(const string & path);
		}
		namespace Search
		{
			struct Volume
			{
				string Path;
				string Label;
			};
			Array<string> * GetFiles(const string & path, bool recursive = false);
			Array<string> * GetDirectories(const string & path);
			Array<Volume> * GetVolumes(void);
		}
		namespace DateTime
		{
			Time GetFileCreationTime(handle file);
			Time GetFileAccessTime(handle file);
			Time GetFileAlterTime(handle file);
			void SetFileCreationTime(handle file, Time time);
			void SetFileAccessTime(handle file, Time time);
			void SetFileAlterTime(handle file, Time time);
		}
	}
}