#include "FileServices.h"

namespace Engine
{
	namespace IO
	{
		FileAccessException::FileAccessException(uint ec) : code(ec) {}
		FileAccessException::FileAccessException(void) : code(Error::Unknown) {}
		string FileAccessException::ToString(void) const { return L"FileAccessException (" + string(code, HexadecimalBase, 8) + L")"; }
		FileReadEndOfFileException::FileReadEndOfFileException(uint32 data_read) : DataRead(data_read) {}
		string FileReadEndOfFileException::ToString(void) const { return L"FileReadEndOfFileException: Data read amount = " + string(DataRead); }
		string DirectoryAlreadyExistsException::ToString(void) const { return L"DirectoryAlreadyExistsException"; }
		string FileFormatException::ToString(void) const { return L"FileFormatException"; }

		namespace Path
		{
			string NormalizePath(const string & path)
			{
				if (PathDirectorySeparator == L'\\') return path.Replace(L'/', L'\\');
				else if (PathDirectorySeparator == L'/') return path.Replace(L'\\', L'/');
				return L"";
			}
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