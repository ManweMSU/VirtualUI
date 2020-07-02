#include <EngineRuntime.h>

#include <PlatformSpecific/WindowsRegistry.h>

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
void local_configure(RegistryNode * node, const string & rt_path, const string & tools, bool x64)
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
    node->SetValue(L"ObjectPath", string(L"_build/win") + (x64 ? string(L"64") : string(L"32")));
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
    compiler->CreateValue(L"Arguments/03", RegistryValueType::String);
    compiler->SetValue(L"Arguments/03", string(L"/GL"));
    compiler->CreateValue(L"Arguments/04", RegistryValueType::String);
    compiler->SetValue(L"Arguments/04", string(L"/W3"));
    compiler->CreateValue(L"Arguments/05", RegistryValueType::String);
    compiler->SetValue(L"Arguments/05", string(L"/Gy"));
    compiler->CreateValue(L"Arguments/06", RegistryValueType::String);
    compiler->SetValue(L"Arguments/06", string(L"/Gm-"));
    compiler->CreateValue(L"Arguments/07", RegistryValueType::String);
    compiler->SetValue(L"Arguments/07", string(L"/O2"));
    compiler->CreateValue(L"Arguments/08", RegistryValueType::String);
    compiler->SetValue(L"Arguments/08", string(L"/WX-"));
    compiler->CreateValue(L"Arguments/09", RegistryValueType::String);
    compiler->SetValue(L"Arguments/09", string(L"/Gd"));
    compiler->CreateValue(L"Arguments/10", RegistryValueType::String);
    compiler->SetValue(L"Arguments/10", string(L"/Oy-"));
    compiler->CreateValue(L"Arguments/11", RegistryValueType::String);
    compiler->SetValue(L"Arguments/11", string(L"/Oi"));
    compiler->CreateValue(L"Arguments/12", RegistryValueType::String);
    compiler->SetValue(L"Arguments/12", string(L"/Zc:wchar_t"));
    compiler->CreateValue(L"Arguments/13", RegistryValueType::String);
    compiler->SetValue(L"Arguments/13", string(L"/Zc:forScope"));
    compiler->CreateValue(L"Arguments/14", RegistryValueType::String);
    compiler->SetValue(L"Arguments/14", string(L"/Zc:inline"));
    compiler->CreateValue(L"Arguments/15", RegistryValueType::String);
    compiler->SetValue(L"Arguments/15", string(L"/MT"));
    compiler->CreateValue(L"Arguments/16", RegistryValueType::String);
    compiler->SetValue(L"Arguments/16", string(L"/errorReport:none"));
    compiler->CreateValue(L"Arguments/17", RegistryValueType::String);
    compiler->SetValue(L"Arguments/17", string(L"/fp:precise"));
    compiler->CreateValue(L"Arguments/18", RegistryValueType::String);
    compiler->SetValue(L"Arguments/18", string(L"/EHsc"));
    compiler->CreateValue(L"Arguments/19", RegistryValueType::String);
    compiler->SetValue(L"Arguments/19", string(L"/DWIN32"));
    if (x64) {
        compiler->CreateValue(L"Arguments/20", RegistryValueType::String);
        compiler->SetValue(L"Arguments/20", string(L"/D_WIN64"));
    }
    compiler->CreateValue(L"Arguments/21", RegistryValueType::String);
    compiler->SetValue(L"Arguments/21", string(L"/DNDEBUG"));
    compiler->CreateValue(L"Arguments/22", RegistryValueType::String);
    compiler->SetValue(L"Arguments/22", string(L"/D_UNICODE"));
    compiler->CreateValue(L"Arguments/23", RegistryValueType::String);
    compiler->SetValue(L"Arguments/23", string(L"/DUNICODE"));
    {
        uint num = 24;
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
    linker->CreateValue(L"Arguments/05", RegistryValueType::String);
    linker->SetValue(L"Arguments/05", string(L"/OPT:REF"));
    if (!x64) {
        linker->CreateValue(L"Arguments/06", RegistryValueType::String);
        linker->SetValue(L"Arguments/06", string(L"/SAFESEH"));
    }
    linker->CreateValue(L"Arguments/07", RegistryValueType::String);
    linker->SetValue(L"Arguments/07", string(L"/OPT:ICF"));
    linker->CreateValue(L"Arguments/08", RegistryValueType::String);
    linker->SetValue(L"Arguments/08", string(L"/ERRORREPORT:NONE"));
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
        local_configure(x86_node, rt_path, tools, false);
        if (IsPlatformAvailable(Platform::X64)) {
            ertbuild->CreateNode(L"Windows-X64");
            SafePointer<RegistryNode> x64_node = ertbuild->OpenNode(L"Windows-X64");
            local_configure(x64_node, rt_path, tools, true);
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

int Main(void)
{
    SafePointer< Array<string> > args = GetCommandLine();
    if (args->Length() > 2 && string::CompareIgnoreCase(args->ElementAt(1), L":path") == 0) {
        IO::Console Console;
        try {
            Console.SetTextColor(15);
            Console.WriteLine(L"Accessing the environment registry key...");
            // SafePointer<WindowsSpecific::RegistryKey> local_machine = WindowsSpecific::OpenRootRegistryKey(WindowsSpecific::RegistryRootKey::LocalMachine);
            // SafePointer<WindowsSpecific::RegistryKey> system = local_machine ? local_machine->OpenKey(L"System", WindowsSpecific::RegistryKeyAccess::Full) : 0;
            // SafePointer<WindowsSpecific::RegistryKey> control_set = system ? system->OpenKey(L"CurrentControlSet", WindowsSpecific::RegistryKeyAccess::Full) : 0;
            // SafePointer<WindowsSpecific::RegistryKey> control = control_set ? control_set->OpenKey(L"Control", WindowsSpecific::RegistryKeyAccess::Full) : 0;
            // SafePointer<WindowsSpecific::RegistryKey> session = control ? control->OpenKey(L"Session Manager", WindowsSpecific::RegistryKeyAccess::Full) : 0;
            // SafePointer<WindowsSpecific::RegistryKey> environment = session ? session->OpenKey(L"Environment", WindowsSpecific::RegistryKeyAccess::Full) : 0;
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
    }
    handle OutputClone = IO::CloneHandle(IO::GetStandardOutput());
    IO::Console Console(OutputClone);
    try {
        UI::Windows::InitializeCodecCollection();
        FileStream InputStream(IO::GetStandardInput());
        TextReader Input(&InputStream, Encoding::UTF8);
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
        string path = Input.ReadLine();
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
        Console << L"Configuring...";
        configure(L"../Runtime", path + L"/Tools");
        Console << L"Succeed!" << IO::NewLineChar;
        Console << L"The configuration was produces by a robot." << IO::NewLineChar;
        Console << L"Please, validate Tools\\ertbuild.ini and Tools\\ertres.ini." << IO::NewLineChar;
        Console << L"Press \"RETURN\" to continue..." << IO::NewLineChar;
        Input.ReadLine();
        {
            IO::SetStandardOutput(OutputClone);
            IO::SetStandardError(OutputClone);
            Console << L"Starting the Runtime Compilation..." << IO::NewLineChar;
            Array<string> args(1);
            args << L":asmrt";
            SafePointer<Process> builder = CreateProcess(path + L"/Tools/ertbuild", &args);
            if (!builder) {
                Console.SetTextColor(4);
                Console << L"Failed to launch the builder tool." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            builder->Wait();
            if (builder->GetExitCode()) {
                Console.SetTextColor(4);
                Console << L"The build tool failed to build the Runtime on 32-bit Windows." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            if (IsPlatformAvailable(Platform::X64)) {
                args << L":x64";
                SafePointer<Process> builder = CreateProcess(path + L"/Tools/ertbuild", &args);
                if (!builder) {
                    Console.SetTextColor(4);
                    Console << L"Failed to launch the builder tool." << IO::NewLineChar;
                    Console.SetTextColor(-1);
                    return 1;
                }
                builder->Wait();
                if (builder->GetExitCode()) {
                    Console.SetTextColor(4);
                    Console << L"The build tool failed to build the Runtime on 64-bit Windows." << IO::NewLineChar;
                    Console.SetTextColor(-1);
                    return 1;
                }
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
            frame = prod_frame(256);
            test_icon->Frames.Append(frame);
            SafePointer<Stream> icon = new FileStream(L"Test/test.eiwv", AccessReadWrite, CreateAlways);
            Codec::EncodeImage(icon, test_icon, L"EIWV");
        }
        {
            Console << L"Starting the Test compilation..." << IO::NewLineChar;
            Array<string> args(1);
            args << IO::ExpandPath(L"Test/test.project.ini");
            SafePointer<Process> builder = CreateProcess(path + L"/Tools/ertbuild", &args);
            if (!builder) {
                Console.SetTextColor(4);
                Console << L"Failed to launch the builder tool." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            builder->Wait();
            if (builder->GetExitCode()) {
                Console.SetTextColor(4);
                Console << L"The build tool failed to build the x86 Test." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            if (IsPlatformAvailable(Platform::X64)) {
                args << L":x64";
                SafePointer<Process> builder = CreateProcess(path + L"/Tools/ertbuild", &args);
                if (!builder) {
                    Console.SetTextColor(4);
                    Console << L"Failed to launch the builder tool." << IO::NewLineChar;
                    Console.SetTextColor(-1);
                    return 1;
                }
                builder->Wait();
                if (builder->GetExitCode()) {
                    Console.SetTextColor(4);
                    Console << L"The build tool failed to build the x64 Test." << IO::NewLineChar;
                    Console.SetTextColor(-1);
                    return 1;
                }
            }
            SafePointer<Process> test = CreateProcess(path + L"/Test/_build/win32/test.exe");
            if (!test) {
                Console.SetTextColor(4);
                Console << L"Failed to launch the test." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            test->Wait();
            if (IsPlatformAvailable(Platform::X64)) {
                SafePointer<Process> test = CreateProcess(path + L"/Test/_build/win64/test.exe");
                if (!test) {
                    Console.SetTextColor(4);
                    Console << L"Failed to launch the test." << IO::NewLineChar;
                    Console.SetTextColor(-1);
                    return 1;
                }
                test->Wait();
            }
        }
        Console << L"The Runtime has been successfully installed and tested." << IO::NewLineChar;
        Console << L"Do you want to install the Runtime tools into the PATH environment variable? (yes/no)" << IO::NewLineChar;
		Console << L"In case of 'YES' please restart your user session after the installation's end." << IO::NewLineChar;
        string ans = Input.ReadLine();
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