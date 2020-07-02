#include <EngineRuntime.h>

#ifdef ENGINE_WINDOWS
#include <PlatformSpecific/WindowsRegistry.h>
#endif

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

Codec::Frame * prod_frame(int w)
{
	Codec::Frame * frame = new Codec::Frame(w, w, w * 4, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::TopDown);
	int b = w / 16;
	for (int y = 0; y < w; y++) for (int x = 0; x < w; x++) {
		uint32 p = 0x4444FF00;
		if (y < b || y > w - b - 1 || x < b || x > w - b - 1) p = 0xCCFF4400;
		frame->SetPixel(x, y, p);
	}
	return frame;
}
int InvokeBuildTool(IO::Console & console, const string & target, bool x64, bool debug)
{
	Array<string> args(3);
	args << target;
	args << L":clean";
	if (x64) args << L":x64";
	if (debug) args << L":debug";
	SafePointer<Process> builder = CreateProcess(L"Tools/ertbuild", &args);
	if (!builder) {
		console.SetTextColor(4);
		console << L"Failed to launch the builder tool." << IO::NewLineChar;
		console.SetTextColor(-1);
		return -1;
	}
	builder->Wait();
	return builder->GetExitCode();
}
bool RunTest(IO::Console & console, const string & module)
{
	SafePointer<Process> test = CreateProcess(module);
	if (!test) {
		console.SetTextColor(4);
		console << L"Failed to launch the Test." << IO::NewLineChar;
		console.SetTextColor(-1);
		return false;
	}
	test->Wait();
	return true;
}

#ifdef ENGINE_WINDOWS
struct compiler_toolset
{
	string cl_x86;
	string cl_x64;
	string link_x86;
	string link_x64;
	string vc_include;
	string vc_lib_x86;
	string vc_lib_x64;
	string win10_sdk_ver;
};
compiler_toolset toolset;
Array<string> search_for_msvc(void)
{
	Array<string> vs_cand(0x10), cl_cand;
	{
		string drive = WindowsSpecific::GetShellFolderPath(WindowsSpecific::ShellFolder::WindowsRoot).Fragment(0, 3);
		SafePointer< Array<string> > search_1 = IO::Search::GetDirectories(drive + L"Program Files\\*Visual Studio*");
		SafePointer< Array<string> > search_2 = IO::Search::GetDirectories(drive + L"Program Files (x86)\\*Visual Studio*");
		for (int i = 0; i < search_1->Length(); i++) vs_cand << drive + L"Program Files\\" + search_1->ElementAt(i);
		for (int i = 0; i < search_2->Length(); i++) vs_cand << drive + L"Program Files (x86)\\" + search_2->ElementAt(i);
	}
	for (int i = 0; i < vs_cand.Length(); i++) {
		SafePointer< Array<string> > search_cl = IO::Search::GetFiles(vs_cand[i] + L"\\cl.exe", true);
		for (int j = 0; j < search_cl->Length(); j++) cl_cand << vs_cand[i] + L"\\" + search_cl->ElementAt(j);
	}
	return cl_cand;
}
string get_stdout(const string & exe)
{
	handle pipe_in, pipe_out;
	handle clone_out;
	clone_out = IO::CloneHandle(IO::GetStandardOutput());
	IO::CreatePipe(&pipe_in, &pipe_out);
	IO::SetStandardOutput(pipe_in);
	IO::SetStandardError(pipe_in);
	SafePointer<Process> process = CreateProcess(exe);
	if (!process) {
		IO::SetStandardOutput(clone_out);
		IO::SetStandardError(clone_out);
		IO::CloseFile(pipe_in);
		IO::CloseFile(pipe_out);
		IO::CloseFile(clone_out);
		return L"";
	}
	IO::SetStandardOutput(clone_out);
	IO::SetStandardError(clone_out);
	IO::CloseFile(pipe_in);
	IO::CloseFile(clone_out);
	process->Wait();
	FileStream read_stream(pipe_out);
	TextReader reader(&read_stream, Encoding::ANSI);
	string text = reader.ReadAll();
	IO::CloseFile(pipe_out);
	return text;
}
bool contains(const Array<string> & arr, const string & s)
{
	for (int i = 0; i < arr.Length(); i++) if (string::CompareIgnoreCase(arr[i], s) == 0) return true;
	return false;
}
string get_lib_platform_path(const string & lib, bool x64)
{
	SafePointer< Array<string> > vars = IO::Search::GetDirectories(lib + L"\\*");
	if (contains(*vars, L"amd64")) {
		if (x64) return lib + L"\\amd64";
		else return lib;
	} else if (contains(*vars, L"x86") && contains(*vars, L"x64")) {
		if (x64) return lib + L"\\x64";
		else return lib + L"\\x86";
	} else return L"";
}
void set_toolset(void)
{
	auto cand = search_for_msvc();
	Array<string> x86_cand(0x10), x64_cand(0x10);
	for (int i = 0; i < cand.Length(); i++) {
		string output = get_stdout(cand[i]);
		if (output.Length() && IO::FileExists(IO::Path::GetDirectory(cand[i]) + L"\\link.exe")) {
			if (output.FindFirst(L"x86") != -1) x86_cand << cand[i];
			else if (output.FindFirst(L"x64") != -1) x64_cand << cand[i];
		}
	}
	if (x86_cand.Length()) {
		toolset.cl_x86 = x86_cand.FirstElement();
		toolset.link_x86 = IO::Path::GetDirectory(toolset.cl_x86) + L"\\link.exe";
	}
	if (x64_cand.Length()) {
		toolset.cl_x64 = x64_cand.FirstElement();
		toolset.link_x64 = IO::Path::GetDirectory(toolset.cl_x64) + L"\\link.exe";
	}
	int pos = toolset.cl_x86.LowerCase().FindFirst(L"\\bin\\");
	toolset.vc_include = toolset.cl_x86.Fragment(0, pos) + L"\\include";
	string vc_lib = toolset.cl_x86.Fragment(0, pos) + L"\\lib";
	toolset.vc_lib_x86 = get_lib_platform_path(vc_lib, false);
	toolset.vc_lib_x64 = get_lib_platform_path(vc_lib, true);
}
Array<string> get_include(void)
{
	Array<string> inc(8);
	try {
		SafePointer<WindowsSpecific::RegistryKey> sys_root = WindowsSpecific::OpenRootRegistryKey(WindowsSpecific::RegistryRootKey::LocalMachine);
		if (!sys_root) return inc;
		SafePointer<WindowsSpecific::RegistryKey> soft_root = sys_root->OpenKey(L"Software", WindowsSpecific::RegistryKeyAccess::ReadOnly);
		SafePointer<WindowsSpecific::RegistryKey> microsoft_root = soft_root ? soft_root->OpenKey(L"Microsoft", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		SafePointer<WindowsSpecific::RegistryKey> win_root = microsoft_root ? microsoft_root->OpenKey(L"Windows Kits", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		SafePointer<WindowsSpecific::RegistryKey> kits_root = win_root ? win_root->OpenKey(L"Installed Roots", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		if (!kits_root) return inc;
		string w10_root = kits_root->GetValueString(L"KitsRoot10") + L"Include\\";
		SafePointer< Array<string> > vers = IO::Search::GetDirectories(w10_root + L"*");
		SortArray(*vers);
		w10_root += vers->LastElement() + L"\\";
		toolset.win10_sdk_ver = vers->LastElement();
		SafePointer< Array<string> > parts = IO::Search::GetDirectories(w10_root + L"*");
		for (int i = 0; i < parts->Length(); i++) inc << w10_root + parts->ElementAt(i);
		inc << toolset.vc_include;
	} catch (...) { return inc; }
	return inc;
}
Array<string> get_lib(bool x64)
{
	Array<string> lib(8);
	try {
		SafePointer<WindowsSpecific::RegistryKey> sys_root = WindowsSpecific::OpenRootRegistryKey(WindowsSpecific::RegistryRootKey::LocalMachine);
		if (!sys_root) return lib;
		SafePointer<WindowsSpecific::RegistryKey> soft_root = sys_root->OpenKey(L"Software", WindowsSpecific::RegistryKeyAccess::ReadOnly);
		SafePointer<WindowsSpecific::RegistryKey> microsoft_root = soft_root ? soft_root->OpenKey(L"Microsoft", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		SafePointer<WindowsSpecific::RegistryKey> win_root = microsoft_root ? microsoft_root->OpenKey(L"Windows Kits", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		SafePointer<WindowsSpecific::RegistryKey> kits_root = win_root ? win_root->OpenKey(L"Installed Roots", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		if (!kits_root) return lib;
		string w10_root = kits_root->GetValueString(L"KitsRoot10") + L"Lib\\";
		SafePointer< Array<string> > vers = IO::Search::GetDirectories(w10_root + L"*");
		SortArray(*vers);
		w10_root += vers->LastElement() + L"\\";
		SafePointer< Array<string> > parts = IO::Search::GetDirectories(w10_root + L"*");
		for (int i = 0; i < parts->Length(); i++) lib << w10_root + parts->ElementAt(i);
		for (int i = 0; i < lib.Length(); i++) {
			string nl = get_lib_platform_path(lib[i], x64);
			if (nl.Length()) lib[i] = nl; else { lib.Remove(i); i--; }
		}
		lib << (x64 ? toolset.vc_lib_x64 : toolset.vc_lib_x86);
	} catch (...) { return lib; }
	return lib;
}
string get_cl_path(bool x64) { return x64 ? toolset.cl_x64 : toolset.cl_x86; }
string get_link_path(bool x64) { return x64 ? toolset.link_x64 : toolset.link_x86; }
string get_rc_path(void)
{
	try {
		SafePointer<WindowsSpecific::RegistryKey> sys_root = WindowsSpecific::OpenRootRegistryKey(WindowsSpecific::RegistryRootKey::LocalMachine);
		if (!sys_root) return L"";
		SafePointer<WindowsSpecific::RegistryKey> soft_root = sys_root->OpenKey(L"Software", WindowsSpecific::RegistryKeyAccess::ReadOnly);
		SafePointer<WindowsSpecific::RegistryKey> microsoft_root = soft_root ? soft_root->OpenKey(L"Microsoft", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		SafePointer<WindowsSpecific::RegistryKey> win_root = microsoft_root ? microsoft_root->OpenKey(L"Windows Kits", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		SafePointer<WindowsSpecific::RegistryKey> kits_root = win_root ? win_root->OpenKey(L"Installed Roots", WindowsSpecific::RegistryKeyAccess::ReadOnly) : 0;
		if (!kits_root) return L"";
		string w10_root = kits_root->GetValueString(L"KitsRoot10");
		SafePointer< Array<string> > rcs = IO::Search::GetFiles(w10_root + L"rc.exe", true);
		if (!rcs->Length()) return L"";
		string path;
		for (int i = 0; i < rcs->Length(); i++) if (rcs->ElementAt(i).FindFirst(L"x86") != -1) { path = rcs->ElementAt(i); break; }
		return w10_root + path;
	} catch (...) { return L""; }
}
void local_configure(RegistryNode * node, const string & rt_path, const string & tools, bool x64, bool debug)
{
	node->CreateValue(L"RuntimePath", RegistryValueType::String);
	node->SetValue(L"RuntimePath", rt_path);
	node->CreateValue(L"UseWhitelist", RegistryValueType::Boolean);
	node->SetValue(L"UseWhitelist", false);
	node->CreateValue(L"ResourceTool", RegistryValueType::String);
	node->SetValue(L"ResourceTool", string(L"ertres.exe"));
	node->CreateValue(L"Bootstrapper", RegistryValueType::String);
	node->SetValue(L"Bootstrapper", string(L"bootstrapper.cpp"));
	node->CreateValue(L"CompileFilter", RegistryValueType::String);
	node->SetValue(L"CompileFilter", string(L"*.c;*.cpp;*.cxx"));
	node->CreateValue(L"ObjectExtension", RegistryValueType::String);
	node->SetValue(L"ObjectExtension", string(L"obj"));
	node->CreateValue(L"ObjectPath", RegistryValueType::String);
	node->SetValue(L"ObjectPath", string(L"_build/win") + (x64 ? string(L"64") : string(L"32")) + (debug ? string(L"_debug") : string(L"")));
	node->CreateValue(L"ExecutableExtension", RegistryValueType::String);
	node->SetValue(L"ExecutableExtension", string(L"exe"));
	node->CreateValue(L"LibraryExtension", RegistryValueType::String);
	node->SetValue(L"LibraryExtension", string(L"dll"));
	node->CreateNode(L"Compiler");
	node->CreateNode(L"Linker");
	SafePointer<RegistryNode> compiler = node->OpenNode(L"Compiler");
	SafePointer<RegistryNode> linker = node->OpenNode(L"Linker");
	compiler->CreateValue(L"Path", RegistryValueType::String);
	compiler->SetValue(L"Path", get_cl_path(x64));
	compiler->CreateValue(L"IncludeArgument", RegistryValueType::String);
	compiler->SetValue(L"IncludeArgument", string(L"/I"));
	compiler->CreateValue(L"OutputArgument", RegistryValueType::String);
	compiler->SetValue(L"OutputArgument", string(L"/Fo$"));
	compiler->CreateValue(L"DefineArgument", RegistryValueType::String);
	compiler->SetValue(L"DefineArgument", string(L"/D"));
	compiler->CreateNode(L"Arguments");
	compiler->CreateValue(L"Arguments/01", RegistryValueType::String);
	compiler->SetValue(L"Arguments/01", string(L"/c"));
	compiler->CreateValue(L"Arguments/02", RegistryValueType::String);
	compiler->SetValue(L"Arguments/02", string(L"/GS"));
	if (debug) {
		compiler->CreateValue(L"Arguments/03", RegistryValueType::String);
		compiler->SetValue(L"Arguments/03", string(L"/Z7"));
	} else {
		compiler->CreateValue(L"Arguments/03", RegistryValueType::String);
		compiler->SetValue(L"Arguments/03", string(L"/GL"));
	}
	compiler->CreateValue(L"Arguments/04", RegistryValueType::String);
	compiler->SetValue(L"Arguments/04", string(L"/W3"));
	if (debug) {
		compiler->CreateValue(L"Arguments/05", RegistryValueType::String);
		compiler->SetValue(L"Arguments/05", string(L"/RTC1"));
	} else {
		compiler->CreateValue(L"Arguments/05", RegistryValueType::String);
		compiler->SetValue(L"Arguments/05", string(L"/Gy"));
	}
	compiler->CreateValue(L"Arguments/06", RegistryValueType::String);
	compiler->SetValue(L"Arguments/06", string(L"/Gm-"));
	if (debug) {
		compiler->CreateValue(L"Arguments/07", RegistryValueType::String);
		compiler->SetValue(L"Arguments/07", string(L"/Od"));
	} else {
		compiler->CreateValue(L"Arguments/07", RegistryValueType::String);
		compiler->SetValue(L"Arguments/07", string(L"/O2"));
	}
	compiler->CreateValue(L"Arguments/08", RegistryValueType::String);
	compiler->SetValue(L"Arguments/08", string(L"/WX-"));
	compiler->CreateValue(L"Arguments/09", RegistryValueType::String);
	compiler->SetValue(L"Arguments/09", string(L"/Gd"));
	compiler->CreateValue(L"Arguments/10", RegistryValueType::String);
	compiler->SetValue(L"Arguments/10", string(L"/Oy-"));
	if (debug) {
		compiler->CreateValue(L"Arguments/11", RegistryValueType::String);
		compiler->SetValue(L"Arguments/11", string(L"/FC"));
	} else {
		compiler->CreateValue(L"Arguments/11", RegistryValueType::String);
		compiler->SetValue(L"Arguments/11", string(L"/Oi"));
	}
	compiler->CreateValue(L"Arguments/12", RegistryValueType::String);
	compiler->SetValue(L"Arguments/12", string(L"/Zc:wchar_t"));
	compiler->CreateValue(L"Arguments/13", RegistryValueType::String);
	compiler->SetValue(L"Arguments/13", string(L"/Zc:forScope"));
	compiler->CreateValue(L"Arguments/14", RegistryValueType::String);
	compiler->SetValue(L"Arguments/14", string(L"/Zc:inline"));
	if (debug) {
		compiler->CreateValue(L"Arguments/15", RegistryValueType::String);
		compiler->SetValue(L"Arguments/15", string(L"/MDd"));
	} else {
		compiler->CreateValue(L"Arguments/15", RegistryValueType::String);
		compiler->SetValue(L"Arguments/15", string(L"/MT"));
	}
	if (debug) {
		compiler->CreateValue(L"Arguments/16", RegistryValueType::String);
		compiler->SetValue(L"Arguments/16", string(L"/errorReport:prompt"));
	} else {
		compiler->CreateValue(L"Arguments/16", RegistryValueType::String);
		compiler->SetValue(L"Arguments/16", string(L"/errorReport:none"));
	}
	compiler->CreateValue(L"Arguments/17", RegistryValueType::String);
	compiler->SetValue(L"Arguments/17", string(L"/fp:precise"));
	compiler->CreateValue(L"Arguments/18", RegistryValueType::String);
	compiler->SetValue(L"Arguments/18", string(L"/EHsc"));
	compiler->CreateValue(L"Arguments/19", RegistryValueType::String);
	compiler->SetValue(L"Arguments/19", string(L"/DWIN32"));
	if (x64) {
		compiler->CreateValue(L"Arguments/20", RegistryValueType::String);
		compiler->SetValue(L"Arguments/20", string(L"/D_WIN64"));
	} else if (debug) {
		compiler->CreateValue(L"Arguments/20", RegistryValueType::String);
		compiler->SetValue(L"Arguments/20", string(L"/analyze-"));
	}
	if (debug) {
		compiler->CreateValue(L"Arguments/21", RegistryValueType::String);
		compiler->SetValue(L"Arguments/21", string(L"/D_DEBUG"));
	} else {
		compiler->CreateValue(L"Arguments/21", RegistryValueType::String);
		compiler->SetValue(L"Arguments/21", string(L"/DNDEBUG"));
	}
	compiler->CreateValue(L"Arguments/22", RegistryValueType::String);
	compiler->SetValue(L"Arguments/22", string(L"/D_UNICODE"));
	compiler->CreateValue(L"Arguments/23", RegistryValueType::String);
	compiler->SetValue(L"Arguments/23", string(L"/DUNICODE"));
	if (debug) {
		compiler->CreateValue(L"Arguments/24", RegistryValueType::String);
		compiler->SetValue(L"Arguments/24", string(L"/diagnostics:column"));
	}
	{
		uint num = 25;
		auto inc = get_include();
		for (int i = 0; i < inc.Length(); i++) {
			string val = string(L"Arguments/") + string(num, DecimalBase, 2);
			compiler->CreateValue(val, RegistryValueType::String);
			compiler->SetValue(val, string(L"/I") + inc[i]);
			num++;
		}
	}
	linker->CreateValue(L"Path", RegistryValueType::String);
	linker->SetValue(L"Path", get_link_path(x64));
	linker->CreateValue(L"OutputArgument", RegistryValueType::String);
	linker->SetValue(L"OutputArgument", string(L"/OUT:$"));
	linker->CreateNode(L"Arguments");
	linker->CreateNode(L"ArgumentsConsole");
	linker->CreateNode(L"ArgumentsGUI");
	linker->CreateNode(L"ArgumentsLibrary");
	string os = x64 ? L"6.00" : L"5.01";
	linker->CreateValue(L"ArgumentsConsole/01", RegistryValueType::String);
	linker->SetValue(L"ArgumentsConsole/01", string(L"/SUBSYSTEM:CONSOLE,") + os);
	linker->CreateValue(L"ArgumentsGUI/01", RegistryValueType::String);
	linker->SetValue(L"ArgumentsGUI/01", string(L"/SUBSYSTEM:WINDOWS,") + os);
	linker->CreateValue(L"ArgumentsLibrary/01", RegistryValueType::String);
	linker->SetValue(L"ArgumentsLibrary/01", string(L"/SUBSYSTEM:WINDOWS,") + os);
	linker->CreateValue(L"ArgumentsLibrary/02", RegistryValueType::String);
	linker->SetValue(L"ArgumentsLibrary/02", string(L"/DLL"));
	linker->CreateValue(L"Arguments/01", RegistryValueType::String);
	linker->SetValue(L"Arguments/01", string(L"/LTCG:INCREMENTAL"));
	linker->CreateValue(L"Arguments/02", RegistryValueType::String);
	linker->SetValue(L"Arguments/02", string(L"/NXCOMPAT"));
	linker->CreateValue(L"Arguments/03", RegistryValueType::String);
	linker->SetValue(L"Arguments/03", string(L"/DYNAMICBASE"));
	if (x64) {
		linker->CreateValue(L"Arguments/04", RegistryValueType::String);
		linker->SetValue(L"Arguments/04", string(L"/MACHINE:X64"));
	} else {
		linker->CreateValue(L"Arguments/04", RegistryValueType::String);
		linker->SetValue(L"Arguments/04", string(L"/MACHINE:X86"));
	}
	if (!x64 && !debug) {
		linker->CreateValue(L"Arguments/05", RegistryValueType::String);
		linker->SetValue(L"Arguments/05", string(L"/SAFESEH"));
	}
	if (debug) {
		linker->CreateValue(L"Arguments/06", RegistryValueType::String);
		linker->SetValue(L"Arguments/06", string(L"/DEBUG"));
	} else {
		linker->CreateValue(L"Arguments/06", RegistryValueType::String);
		linker->SetValue(L"Arguments/06", string(L"/OPT:REF"));
		linker->CreateValue(L"Arguments/07", RegistryValueType::String);
		linker->SetValue(L"Arguments/07", string(L"/OPT:ICF"));
	}
	if (debug) {
		linker->CreateValue(L"Arguments/08", RegistryValueType::String);
		linker->SetValue(L"Arguments/08", string(L"/ERRORREPORT:PROMPT"));
	} else {
		linker->CreateValue(L"Arguments/08", RegistryValueType::String);
		linker->SetValue(L"Arguments/08", string(L"/ERRORREPORT:NONE"));
	}
	uint num = 9;
	{
		auto lib = get_lib(x64);
		for (int i = 0; i < lib.Length(); i++) {
			string val = string(L"Arguments/") + string(num, DecimalBase, 2);
			linker->CreateValue(val, RegistryValueType::String);
			linker->SetValue(val, string(L"/LIBPATH:") + lib[i]);
			num++;
		}
	}
	Array<string> std_lib(12);
	std_lib << L"kernel32.lib";
	std_lib << L"user32.lib";
	std_lib << L"gdi32.lib";
	std_lib << L"winspool.lib";
	std_lib << L"comdlg32.lib";
	std_lib << L"advapi32.lib";
	std_lib << L"shell32.lib";
	std_lib << L"ole32.lib";
	std_lib << L"oleaut32.lib";
	std_lib << L"uuid.lib";
	std_lib << L"odbc32.lib";
	std_lib << L"odbccp32.lib";
	for (int i = 0; i < std_lib.Length(); i++) {
		string val = string(L"Arguments/") + string(num, DecimalBase, 2);
		linker->CreateValue(val, RegistryValueType::String);
		linker->SetValue(val, std_lib[i]);
		num++;
	}
}
void configure(const string & rt_path, const string & tools)
{
	set_toolset();
	{
		SafePointer<Registry> ertbuild = CreateRegistry();
		ertbuild->CreateNode(L"Windows-X86");
		SafePointer<RegistryNode> x86_node = ertbuild->OpenNode(L"Windows-X86");
		local_configure(x86_node, rt_path, tools, false, false);
		ertbuild->CreateNode(L"Windows-X86-Debug");
		SafePointer<RegistryNode> x86_node_debug = ertbuild->OpenNode(L"Windows-X86-Debug");
		local_configure(x86_node_debug, rt_path, tools, false, true);
		if (IsPlatformAvailable(Platform::X64)) {
			ertbuild->CreateNode(L"Windows-X64");
			SafePointer<RegistryNode> x64_node = ertbuild->OpenNode(L"Windows-X64");
			local_configure(x64_node, rt_path, tools, true, false);
			ertbuild->CreateNode(L"Windows-X64-Debug");
			SafePointer<RegistryNode> x64_node_debug = ertbuild->OpenNode(L"Windows-X64-Debug");
			local_configure(x64_node_debug, rt_path, tools, true, true);
		}
		SafePointer<Stream> bld_stream = new FileStream(tools + L"/ertbuild.ini", AccessReadWrite, CreateAlways);
		RegistryToText(ertbuild, bld_stream, Encoding::UTF8);
	}
	{
		SafePointer<Registry> envconf = CreateRegistry();
		envconf->CreateValue(L"Compiler", RegistryValueType::String);
		envconf->SetValue(L"Compiler", get_cl_path(false));
		envconf->CreateValue(L"SDK", RegistryValueType::String);
		envconf->SetValue(L"SDK", toolset.win10_sdk_ver);
		SafePointer<Stream> conf_stream = new FileStream(tools + L"/envconf.ini", AccessReadWrite, CreateAlways);
		RegistryToText(envconf, conf_stream, Encoding::UTF8);
	}
	{
		SafePointer<Registry> ertres = CreateRegistry();
		ertres->CreateNode(L"Windows");
		SafePointer<RegistryNode> win_node = ertres->OpenNode(L"Windows");
		win_node->CreateValue(L"IconCodec", RegistryValueType::String);
		win_node->SetValue(L"IconCodec", string(L"ICO"));
		win_node->CreateValue(L"IconExtension", RegistryValueType::String);
		win_node->SetValue(L"IconExtension", string(L"ico"));
		win_node->CreateNode(L"IconFormats");
		win_node->CreateValue(L"IconFormats/01", RegistryValueType::Integer);
		win_node->SetValue(L"IconFormats/01", 16);
		win_node->CreateValue(L"IconFormats/02", RegistryValueType::Integer);
		win_node->SetValue(L"IconFormats/02", 24);
		win_node->CreateValue(L"IconFormats/03", RegistryValueType::Integer);
		win_node->SetValue(L"IconFormats/03", 32);
		win_node->CreateValue(L"IconFormats/04", RegistryValueType::Integer);
		win_node->SetValue(L"IconFormats/04", 48);
		win_node->CreateValue(L"IconFormats/05", RegistryValueType::Integer);
		win_node->SetValue(L"IconFormats/05", 64);
		win_node->CreateValue(L"IconFormats/06", RegistryValueType::Integer);
		win_node->SetValue(L"IconFormats/06", 256);
		win_node->CreateNode(L"Compiler");
		SafePointer<RegistryNode> compiler = win_node->OpenNode(L"Compiler");
		compiler->CreateValue(L"Path", RegistryValueType::String);
		compiler->SetValue(L"Path", get_rc_path());
		compiler->CreateValue(L"OutputArgument", RegistryValueType::String);
		compiler->SetValue(L"OutputArgument", string(L"/fo$"));
		compiler->CreateNode(L"Arguments");
		compiler->CreateValue(L"Arguments/01", RegistryValueType::String);
		compiler->SetValue(L"Arguments/01", string(L"/r"));
		{
			uint num = 2;
			auto inc = get_include();
			for (int i = 0; i < inc.Length(); i++) {
				string val = string(L"Arguments/") + string(num, DecimalBase, 2);
				compiler->CreateValue(val, RegistryValueType::String);
				compiler->SetValue(val, string(L"/I") + inc[i]);
				num++;
			}
		}
		SafePointer<Stream> res_stream = new FileStream(tools + L"/ertres.ini", AccessReadWrite, CreateAlways);
		RegistryToText(ertres, res_stream, Encoding::UTF8);
	}
}
#endif
#ifdef ENGINE_MACOSX
void configure(const string & rt_path, const string & tools)
{
	{
		SafePointer<Registry> ertbuild = CreateRegistry();
		ertbuild->CreateNode(L"MacOSX-X64");
		{
			SafePointer<RegistryNode> x64_node = ertbuild->OpenNode(L"MacOSX-X64");
			x64_node->CreateValue(L"RuntimePath", RegistryValueType::String);
			x64_node->SetValue(L"RuntimePath", rt_path);
			x64_node->CreateValue(L"UseWhitelist", RegistryValueType::Boolean);
			x64_node->SetValue(L"UseWhitelist", false);
			x64_node->CreateValue(L"ResourceTool", RegistryValueType::String);
			x64_node->SetValue(L"ResourceTool", string(L"ertres"));
			x64_node->CreateValue(L"Bootstrapper", RegistryValueType::String);
			x64_node->SetValue(L"Bootstrapper", string(L"bootstrapper.cpp"));
			x64_node->CreateValue(L"CompileFilter", RegistryValueType::String);
			x64_node->SetValue(L"CompileFilter", string(L"*.c;*.cpp;*.cxx;*.m;*.mm"));
			x64_node->CreateValue(L"ObjectExtension", RegistryValueType::String);
			x64_node->SetValue(L"ObjectExtension", string(L"o"));
			x64_node->CreateValue(L"ObjectPath", RegistryValueType::String);
			x64_node->SetValue(L"ObjectPath", string(L"_build/release"));
			x64_node->CreateValue(L"ExecutableExtension", RegistryValueType::String);
			x64_node->SetValue(L"ExecutableExtension", string(L""));
			x64_node->CreateValue(L"LibraryExtension", RegistryValueType::String);
			x64_node->SetValue(L"LibraryExtension", string(L"dylib"));
			x64_node->CreateNode(L"Compiler");
			x64_node->CreateNode(L"Linker");
			SafePointer<RegistryNode> compiler = x64_node->OpenNode(L"Compiler");
			SafePointer<RegistryNode> linker = x64_node->OpenNode(L"Linker");
			compiler->CreateValue(L"Path", RegistryValueType::String);
			compiler->SetValue(L"Path", string(L"clang++"));
			compiler->CreateValue(L"IncludeArgument", RegistryValueType::String);
			compiler->SetValue(L"IncludeArgument", string(L"-I"));
			compiler->CreateValue(L"OutputArgument", RegistryValueType::String);
			compiler->SetValue(L"OutputArgument", string(L"-o"));
			compiler->CreateValue(L"DefineArgument", RegistryValueType::String);
			compiler->SetValue(L"DefineArgument", string(L"-D"));
			compiler->CreateNode(L"Arguments");
			compiler->CreateValue(L"Arguments/01", RegistryValueType::String);
			compiler->SetValue(L"Arguments/01", string(L"-c"));
			compiler->CreateValue(L"Arguments/02", RegistryValueType::String);
			compiler->SetValue(L"Arguments/02", string(L"-O3"));
			compiler->CreateValue(L"Arguments/03", RegistryValueType::String);
			compiler->SetValue(L"Arguments/03", string(L"-std=c++14"));
			compiler->CreateValue(L"Arguments/04", RegistryValueType::String);
			compiler->SetValue(L"Arguments/04", string(L"-v"));
			compiler->CreateValue(L"Arguments/05", RegistryValueType::String);
			compiler->SetValue(L"Arguments/05", string(L"-fvisibility=hidden"));
			compiler->CreateValue(L"Arguments/06", RegistryValueType::String);
			compiler->SetValue(L"Arguments/06", string(L"-fmodules"));
			compiler->CreateValue(L"Arguments/07", RegistryValueType::String);
			compiler->SetValue(L"Arguments/07", string(L"-fcxx-modules"));
			linker->CreateValue(L"Path", RegistryValueType::String);
			linker->SetValue(L"Path", string(L"clang++"));
			linker->CreateValue(L"OutputArgument", RegistryValueType::String);
			linker->SetValue(L"OutputArgument", string(L"-o"));
			linker->CreateNode(L"Arguments");
			linker->CreateValue(L"Arguments/01", RegistryValueType::String);
			linker->SetValue(L"Arguments/01", string(L"-O3"));
			linker->CreateValue(L"Arguments/02", RegistryValueType::String);
			linker->SetValue(L"Arguments/02", string(L"-v"));
			linker->CreateValue(L"Arguments/03", RegistryValueType::String);
			linker->SetValue(L"Arguments/03", string(L"-s"));
			linker->CreateValue(L"Arguments/04", RegistryValueType::String);
			linker->SetValue(L"Arguments/04", string(L"-fvisibility=hidden"));
			linker->CreateNode(L"ArgumentsLibrary");
			linker->CreateValue(L"ArgumentsLibrary/01", RegistryValueType::String);
			linker->SetValue(L"ArgumentsLibrary/01", string(L"-dynamiclib"));
		}
		ertbuild->CreateNode(L"MacOSX-X64-Debug");
		{
			SafePointer<RegistryNode> x64_node = ertbuild->OpenNode(L"MacOSX-X64-Debug");
			x64_node->CreateValue(L"RuntimePath", RegistryValueType::String);
			x64_node->SetValue(L"RuntimePath", rt_path);
			x64_node->CreateValue(L"UseWhitelist", RegistryValueType::Boolean);
			x64_node->SetValue(L"UseWhitelist", false);
			x64_node->CreateValue(L"ResourceTool", RegistryValueType::String);
			x64_node->SetValue(L"ResourceTool", string(L"ertres"));
			x64_node->CreateValue(L"Bootstrapper", RegistryValueType::String);
			x64_node->SetValue(L"Bootstrapper", string(L"bootstrapper.cpp"));
			x64_node->CreateValue(L"CompileFilter", RegistryValueType::String);
			x64_node->SetValue(L"CompileFilter", string(L"*.c;*.cpp;*.cxx;*.m;*.mm"));
			x64_node->CreateValue(L"ObjectExtension", RegistryValueType::String);
			x64_node->SetValue(L"ObjectExtension", string(L"o"));
			x64_node->CreateValue(L"ObjectPath", RegistryValueType::String);
			x64_node->SetValue(L"ObjectPath", string(L"_build/debug"));
			x64_node->CreateValue(L"ExecutableExtension", RegistryValueType::String);
			x64_node->SetValue(L"ExecutableExtension", string(L""));
			x64_node->CreateValue(L"LibraryExtension", RegistryValueType::String);
			x64_node->SetValue(L"LibraryExtension", string(L"dylib"));
			x64_node->CreateNode(L"Compiler");
			x64_node->CreateNode(L"Linker");
			SafePointer<RegistryNode> compiler = x64_node->OpenNode(L"Compiler");
			SafePointer<RegistryNode> linker = x64_node->OpenNode(L"Linker");
			compiler->CreateValue(L"Path", RegistryValueType::String);
			compiler->SetValue(L"Path", string(L"clang++"));
			compiler->CreateValue(L"IncludeArgument", RegistryValueType::String);
			compiler->SetValue(L"IncludeArgument", string(L"-I"));
			compiler->CreateValue(L"OutputArgument", RegistryValueType::String);
			compiler->SetValue(L"OutputArgument", string(L"-o"));
			compiler->CreateValue(L"DefineArgument", RegistryValueType::String);
			compiler->SetValue(L"DefineArgument", string(L"-D"));
			compiler->CreateNode(L"Arguments");
			compiler->CreateValue(L"Arguments/01", RegistryValueType::String);
			compiler->SetValue(L"Arguments/01", string(L"-c"));
			compiler->CreateValue(L"Arguments/02", RegistryValueType::String);
			compiler->SetValue(L"Arguments/02", string(L"-O0"));
			compiler->CreateValue(L"Arguments/03", RegistryValueType::String);
			compiler->SetValue(L"Arguments/03", string(L"-std=c++14"));
			compiler->CreateValue(L"Arguments/04", RegistryValueType::String);
			compiler->SetValue(L"Arguments/04", string(L"-v"));
			compiler->CreateValue(L"Arguments/05", RegistryValueType::String);
			compiler->SetValue(L"Arguments/05", string(L"-g"));
			compiler->CreateValue(L"Arguments/06", RegistryValueType::String);
			compiler->SetValue(L"Arguments/06", string(L"-fmodules"));
			compiler->CreateValue(L"Arguments/07", RegistryValueType::String);
			compiler->SetValue(L"Arguments/07", string(L"-fcxx-modules"));
			linker->CreateValue(L"Path", RegistryValueType::String);
			linker->SetValue(L"Path", string(L"clang++"));
			linker->CreateValue(L"OutputArgument", RegistryValueType::String);
			linker->SetValue(L"OutputArgument", string(L"-o"));
			linker->CreateNode(L"Arguments");
			linker->CreateValue(L"Arguments/01", RegistryValueType::String);
			linker->SetValue(L"Arguments/01", string(L"-O0"));
			linker->CreateValue(L"Arguments/02", RegistryValueType::String);
			linker->SetValue(L"Arguments/02", string(L"-v"));
			linker->CreateValue(L"Arguments/03", RegistryValueType::String);
			linker->SetValue(L"Arguments/03", string(L"-g"));
			linker->CreateNode(L"ArgumentsLibrary");
			linker->CreateValue(L"ArgumentsLibrary/01", RegistryValueType::String);
			linker->SetValue(L"ArgumentsLibrary/01", string(L"-dynamiclib"));
		}
		SafePointer<Stream> bld_stream = new FileStream(tools + L"/ertbuild.ini", AccessReadWrite, CreateAlways);
		RegistryToText(ertbuild, bld_stream, Encoding::UTF8);
	}
	{
		SafePointer<Registry> ertres = CreateRegistry();
		ertres->CreateNode(L"MacOSX");
		SafePointer<RegistryNode> mac_node = ertres->OpenNode(L"MacOSX");
		mac_node->CreateValue(L"IconCodec", RegistryValueType::String);
		mac_node->SetValue(L"IconCodec", string(L"ICNS"));
		mac_node->CreateValue(L"IconExtension", RegistryValueType::String);
		mac_node->SetValue(L"IconExtension", string(L"icns"));
		mac_node->CreateNode(L"IconFormats");
		mac_node->CreateValue(L"IconFormats/01", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/01", 16);
		mac_node->CreateValue(L"IconFormats/02", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/02", 32);
		mac_node->CreateValue(L"IconFormats/03", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/03", 64);
		mac_node->CreateValue(L"IconFormats/04", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/04", 128);
		mac_node->CreateValue(L"IconFormats/05", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/05", 256);
		mac_node->CreateValue(L"IconFormats/06", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/06", 512);
		mac_node->CreateValue(L"IconFormats/07", RegistryValueType::Integer);
		mac_node->SetValue(L"IconFormats/07", 1024);
		SafePointer<Stream> res_stream = new FileStream(tools + L"/ertres.ini", AccessReadWrite, CreateAlways);
		RegistryToText(ertres, res_stream, Encoding::UTF8);
	}
}
#endif

int Main(void)
{
	IO::SetCurrentDirectory(IO::ExpandPath(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/.."));
#ifdef ENGINE_WINDOWS
	handle OutputClone = IO::CloneHandle(IO::GetStandardOutput());
	IO::Console Console(OutputClone);
#endif
#ifdef ENGINE_MACOSX
	IO::Console Console;
#endif
	try {
		UI::Windows::InitializeCodecCollection();
		Console << L"Configuring Engine Runtime tools...";
		configure(L"../Runtime", L"Tools");
		Console << L"Succeed!" << IO::NewLineChar;
#ifdef ENGINE_WINDOWS
		Console << L"Succeed!" << IO::NewLineChar;
		Console << L"The configuration was produces by a robot." << IO::NewLineChar;
		Console << L"Please, validate Tools\\ertbuild.ini and Tools\\ertres.ini." << IO::NewLineChar;
		Console << L"Press \"RETURN\" to continue..." << IO::NewLineChar;
		Console.ReadLine();
#endif
		{
#ifdef ENGINE_WINDOWS
			IO::SetStandardOutput(OutputClone);
			IO::SetStandardError(OutputClone);
#endif
			if (IsPlatformAvailable(Platform::X86)) {
				auto exit_code = InvokeBuildTool(Console, L":asmrt", false, false);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the Runtime on x86 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
				exit_code = InvokeBuildTool(Console, L":asmrt", false, true);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the debuggable Runtime on x86 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
			}
			if (IsPlatformAvailable(Platform::X64)) {
				auto exit_code = InvokeBuildTool(Console, L":asmrt", true, false);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the Runtime on x64 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
				exit_code = InvokeBuildTool(Console, L":asmrt", true, true);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the debuggable Runtime on x64 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
			}
			Console << L"The Runtime object cache has been built successfully." << IO::NewLineChar;
		}
		{
			SafePointer<Codec::Image> test_icon = new Codec::Image;
			SafePointer<Codec::Frame> frame;
			frame = prod_frame(16);
			test_icon->Frames.Append(frame);
			frame = prod_frame(24);
			test_icon->Frames.Append(frame);
			frame = prod_frame(32);
			test_icon->Frames.Append(frame);
			frame = prod_frame(48);
			test_icon->Frames.Append(frame);
			frame = prod_frame(64);
			test_icon->Frames.Append(frame);
			frame = prod_frame(128);
			test_icon->Frames.Append(frame);
			frame = prod_frame(256);
			test_icon->Frames.Append(frame);
			frame = prod_frame(512);
			test_icon->Frames.Append(frame);
			frame = prod_frame(1024);
			test_icon->Frames.Append(frame);
			SafePointer<Stream> icon = new FileStream(L"Test/test.eiwv", AccessReadWrite, CreateAlways);
			Codec::EncodeImage(icon, test_icon, L"EIWV");
		}
		{
			Console << L"Starting the Test compilation..." << IO::NewLineChar;
			if (IsPlatformAvailable(Platform::X86)) {
				auto exit_code = InvokeBuildTool(Console, L"Test/test.project.ini", false, false);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the Test on x86 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
				exit_code = InvokeBuildTool(Console, L"Test/test.project.ini", false, true);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the debuggable Test on x86 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
			}
			if (IsPlatformAvailable(Platform::X64)) {
				auto exit_code = InvokeBuildTool(Console, L"Test/test.project.ini", true, false);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the Test on x64 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
				exit_code = InvokeBuildTool(Console, L"Test/test.project.ini", true, true);
				if (exit_code > 0) {
					Console.SetTextColor(4);
					Console << L"The build tool failed to build the debuggable Test on x64 architecture." << IO::NewLineChar;
					Console.SetTextColor(-1);
				}
				if (exit_code) return 1;
			}
#ifdef ENGINE_WINDOWS
			string exec = L"Test Application.exe";
#endif
#ifdef ENGINE_MACOSX
			string exec = L"Test Application.app/Contents/MacOS/test";
#endif
			if (IsPlatformAvailable(Platform::X86)) {
				if (!RunTest(Console, L"Test/_build/x86/" + exec)) return 1;
				if (!RunTest(Console, L"Test/_build_debug/x86/" + exec)) return 1;
			}
			if (IsPlatformAvailable(Platform::X64)) {
				if (!RunTest(Console, L"Test/_build/x64/" + exec)) return 1;
				if (!RunTest(Console, L"Test/_build_debug/x64/" + exec)) return 1;
			}
		}
	}
	catch (...) {
		Console.SetTextColor(4);
		Console << L"Configuration failed." << IO::NewLineChar;
		Console.SetTextColor(-1);
		return 1;
	}
	return 0;
}