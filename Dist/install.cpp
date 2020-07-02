#include <EngineRuntime.h>

#ifdef ENGINE_WINDOWS
#include <PlatformSpecific/WindowsRegistry.h>
#endif

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

void try_create_directory(const string & name)
{
	try { IO::CreateDirectory(name); } catch (...) {}
}
void try_create_file_directory(const string & name)
{
	for (int i = 0; i < name.Length(); i++) if (name[i] == L'/' || name[i] == L'\\') try_create_directory(name.Fragment(0, i));
}

int Main(void)
{
	SafePointer< Array<string> > args = GetCommandLine();
	if (args->Length() > 2 && string::CompareIgnoreCase(args->ElementAt(1), L":path") == 0) {
#ifdef ENGINE_WINDOWS
		IO::Console Console;
		try {
			Console.SetTextColor(15);
			Console.WriteLine(L"Accessing the environment registry key...");
			SafePointer<WindowsSpecific::RegistryKey> user = WindowsSpecific::OpenRootRegistryKey(WindowsSpecific::RegistryRootKey::CurrentUser);
			SafePointer<WindowsSpecific::RegistryKey> environment = user ? user->OpenKey(L"Environment", WindowsSpecific::RegistryKeyAccess::Full) : 0;
			string path = environment->GetValueString(L"Path");
			auto parts = path.Split(L';');
			Console.WriteLine(L"Reformatting...");
			for (int i = 0; i < parts.Length(); i++) {
				bool ok = true;
				try { IO::SetCurrentDirectory(WindowsSpecific::ExpandEnvironmentalString(parts[i])); } catch (...) { ok = false; }
				if (!ok) { parts.Remove(i); i--; }
			}
			parts << args->ElementAt(2);
			path = parts.FirstElement();
			for (int i = 1; i < parts.Length(); i++) path += L";" + parts[i];
			Console.WriteLine(L"Rewriting...");
			environment->SetValueAsExpandable(L"Path", path);
			Console.SetTextColor(10);
			Console.WriteLine(L"Succeed!");
		} catch (...) {
			Console.SetTextColor(12);
			Console.WriteLine(L"Failed.");
		}
		Engine::Sleep(5000);
		return 0;
#endif
#ifdef ENGINE_MACOSX
		string path = args->ElementAt(2);
		IO::SetCurrentDirectory(L"/etc/paths.d");
		SafePointer<Stream> file = new FileStream(L"engine_runtime", AccessReadWrite, CreateAlways);
		TextWriter writer(file, Encoding::UTF8);
		writer.WriteLine(path);
		return 0;
#endif
	}
	IO::Console Console;
	try {
		Console.SetTitle(ENGINE_VI_APPNAME);
		Console.SetTextColor(15);
		Console.SetBackgroundColor(0);
		Console.ClearLine();
		Console.WriteLine(ENGINE_VI_APPNAME);
		Console.SetTextColor(-1);
		Console.SetBackgroundColor(-1);
		Console.WriteLine(L"");
		Console.WriteLine(L"This tools installs, tests and configures the Engine Runtime.");
		Console.WriteLine(L"");
		Console.Write(L"Installation Path: ");
		string path = Console.ReadLine();
		if (!path.Length()) path = IO::Path::GetDirectory(IO::GetExecutablePath());
		try_create_directory(path);
		path = IO::ExpandPath(path);
		string arc_path = IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/install.ecsa";
		SafePointer<Stream> arc_stream = new FileStream(arc_path, AccessRead, OpenExisting);
		SafePointer<Archive> arc = OpenArchive(arc_stream);
		IO::SetCurrentDirectory(path);
		for (int i = 1; i <= arc->GetFileCount(); i++) {
			string local_name = arc->GetFileName(i);
			if (local_name.Length()) {
				Console << L"Extracting \"" + local_name + L"\"...";
				uint32 usr = arc->GetFileCustomData(i);
				if (usr == 1) local_name = L"Runtime/" + local_name;
				else if (usr == 2) local_name = L"Tools/" + local_name;
				else if (usr == 3) local_name = L"Test/" + local_name;
				else if (usr == 4) local_name = L"UIML/" + local_name;
				else local_name = L"Misc/" + local_name;
				try_create_file_directory(local_name);
				SafePointer<Stream> source = arc->QueryFileStream(i);
				SafePointer<Stream> dest = new FileStream(local_name, AccessReadWrite, CreateAlways);
				source->CopyTo(dest);
				Console << L"Succeed!" << IO::NewLineChar;
			}
		}
		SafePointer<Process> conf_tool = CreateProcess(path + L"/Tools/ertaconf", 0);
		if (!conf_tool) {
			Console.SetTextColor(4);
			Console << L"Failed to launch the runtime auto configuration tool." << IO::NewLineChar;
			Console.SetTextColor(-1);
			return 1;
		}
		conf_tool->Wait();
		if (conf_tool->GetExitCode()) return 1;
		Console << L"The Runtime has been successfully installed and tested." << IO::NewLineChar;
		Console << L"Do you want to install the Runtime tools into the PATH environment variable? (yes/no)" << IO::NewLineChar;
		Console << L"In case of 'YES' please restart your user session after the installation's end." << IO::NewLineChar;
		string ans = Console.ReadLine();
		if (string::CompareIgnoreCase(ans, L"yes") == 0) {
			Console << L"We need the root elevation to do it...";
			Array<string> args(2);
			args << L":path";
			args << IO::ExpandPath(path + L"/Tools");
			if (!CreateProcessElevated(IO::GetExecutablePath(), &args)) {
				Console.SetTextColor(4);
				Console << L"Failed to elevate." << IO::NewLineChar;
				Console.SetTextColor(-1);
				return 1;
			}
			Console << IO::NewLineChar;
		}
		Console << L"Finished!" << IO::NewLineChar;
	}
	catch (...) {
		Console.SetTextColor(4);
		Console << L"Installation failed." << IO::NewLineChar;
		Console.SetTextColor(-1);
		return 1;
	}
	return 0;
}