#pragma once

#include "../EngineBase.h"

namespace Engine
{
	namespace IO
	{
		class FileAccessException : public Exception { public: string ToString(void) const override; };

		enum FileAccess { AccessRead, AccessWrite, AccessReadWrite };
		enum FileCreationMode { CreateAlways, CreateNew, OpenAlways, OpenExisting, TruncateExisting };
		enum SeekOrigin { Begin, Current, End };

#ifdef WINDOWS
		constexpr widechar PathChar = L'\\';
		constexpr wchar_t * NewLineChar = L"\r\n";
		constexpr Encoding TextFileEncoding = Encoding::ANSI;
#endif

		string NormalizePath(const string & path);
		handle CreateFile(const string & path, FileAccess access, FileCreationMode mode);
		handle CreateFileTemporary(const string & path, FileAccess access, FileCreationMode mode, bool delete_on_close);
		void CreatePipe(handle * pipe_in, handle * pipe_out);
		handle GetStandartOutput(void);
		handle GetStandartInput(void);
		handle GetStandartError(void);
		void SetStandartOutput(handle file);
		void SetStandartInput(handle file);
		void SetStandartError(handle file);
		void CloseFile(handle file);
		void Flush(handle file);
		uint64 GetFileSize(handle file);
		void MoveFile(const string & from, const string & to);
		bool FileExists(const string & path);
		void ReadFile(handle file, void * to, uint32 amount);
		void WriteFile(handle file, const void * data, uint32 amount);
		int64 Seek(handle file, int64 position, SeekOrigin origin);
		void SetFileSize(handle file, uint64 size);
	}
}