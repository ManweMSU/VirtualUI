#pragma once

#include "../EngineBase.h"
#include "../Miscellaneous/Time.h"
#include "../Miscellaneous/FileServices.h"

namespace Engine
{
	namespace IO
	{
		handle CreateFile(const string & path, Streaming::FileAccess access, Streaming::FileCreationMode mode);
		void CreatePipe(handle * pipe_in, handle * pipe_out);
		handle CloneHandle(handle file);
		void CloseHandle(handle file);

		void ReadFile(handle file, void * to, uint32 amount);
		void WriteFile(handle file, const void * data, uint32 amount);
		int64 Seek(handle file, int64 position, Streaming::SeekOrigin origin);
		uint64 GetFileSize(handle file);
		void SetFileSize(handle file, uint64 size);
		void Flush(handle file);

		bool FileExists(const string & path);
		void MoveFile(const string & from, const string & to);
		void RemoveFile(const string & path);
		void CreateDirectory(const string & path);
		void RemoveDirectory(const string & path);

		void CreateSymbolicLink(const string & at, const string & to);
		void CreateHardLink(const string & at, const string & to);
		FileType GetFileType(const string & file);
		string GetSymbolicLinkDestination(const string & file);
		void GetVolumeSpace(const string & volume, uint64 * total_bytes, uint64 * free_bytes, uint64 * user_available_bytes);

		string ExpandPath(const string & path);
		string GetCurrentDirectory(void);
		void SetCurrentDirectory(const string & path);
		string GetExecutablePath(void);

		handle GetStandardOutput(void);
		handle GetStandardInput(void);
		handle GetStandardError(void);
		void SetStandardOutput(handle file);
		void SetStandardInput(handle file);
		void SetStandardError(handle file);

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