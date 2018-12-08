#include "WindowsShortcut.h"

#ifdef ENGINE_WINDOWS

#include "../Streaming.h"
#include "../Miscellaneous/DynamicString.h"
#include <ShObjIdl.h>
#include <ShlObj.h>

namespace Engine
{
	namespace WindowsSpecific
	{
		void CreateShellLink(const string & link, const string & path, const string & description)
		{
			SafePointer<IShellLinkW> Link;
			SafePointer<IPersistFile> File;
			if (CoCreateInstance(CLSID_ShellLink, 0, CLSCTX_INPROC_SERVER, IID_IShellLinkW, reinterpret_cast<void **>(Link.InnerRef())) != S_OK) return;
			Link->SetPath(IO::ExpandPath(path));
			Link->SetDescription(description);
			if (Link->QueryInterface(IID_IPersistFile, reinterpret_cast<void **>(File.InnerRef())) != S_OK) return;
			File->Save(IO::ExpandPath(link), false);
		}
		string GetShellFolderPath(ShellFolder folder)
		{
			int id;
			if (folder == ShellFolder::ApplicationData) id = CSIDL_APPDATA;
			else if (folder == ShellFolder::Desktop) id = CSIDL_DESKTOPDIRECTORY;
			else if (folder == ShellFolder::Documents) id = CSIDL_MYDOCUMENTS;
			else if (folder == ShellFolder::Music) id = CSIDL_MYMUSIC;
			else if (folder == ShellFolder::Pictures) id = CSIDL_MYPICTURES;
			else if (folder == ShellFolder::Videos) id = CSIDL_MYVIDEO;
			else if (folder == ShellFolder::UserRoot) id = CSIDL_PROFILE;
			else if (folder == ShellFolder::ProgramFiles) id = CSIDL_PROGRAM_FILES;
			else if (folder == ShellFolder::ProgramsMenu) id = CSIDL_PROGRAMS;
			else if (folder == ShellFolder::StartupPrograms) id = CSIDL_STARTUP;
			else if (folder == ShellFolder::System) id = CSIDL_SYSTEM;
			else if (folder == ShellFolder::WindowsRoot) id = CSIDL_WINDOWS;
			else return L"";
			DynamicString Path;
			Path.ReserveLength(MAX_PATH + 1);
			if (SHGetFolderPathW(0, id, 0, SHGFP_TYPE_CURRENT, Path) != S_OK) return L"";
			return Path;
		}
	}
}
#else
namespace Engine
{
	namespace WindowsSpecific
	{
		void CreateShellLink(const string & link, const string & path, const string & description) {}
		string GetShellFolderPath(ShellFolder folder) { return L""; }
	}
}
#endif