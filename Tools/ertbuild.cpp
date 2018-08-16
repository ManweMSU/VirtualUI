#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

struct VersionInfo
{
    bool UseDefines;
    string AppName;
    string CompanyName;
    string Copyright;
    string InternalName;
    uint32 VersionMajor;
    uint32 VersionMinor;
    uint32 Subversion;
    uint32 Build;
    string AppIdent;
    string ComIdent;
    string Description;
};

handle console_output;
bool clean = false;
string rt_path;
SafePointer<RegistryNode> sys_cfg;
SafePointer<RegistryNode> prj_cfg;
VersionInfo prj_ver;
Time proj_time = 0;
#ifdef ENGINE_MACOSX
string compile_system = L"macosx";
string compile_architecture = L"x64";
string compile_subsystem = L"console";
#endif
#ifdef ENGINE_WINDOWS
string compile_system = L"windows";
string compile_architecture = L"x86";
string compile_subsystem = L"console";
#endif

void try_create_directory(const string & path)
{
    try { IO::CreateDirectory(path); } catch (...) {}
}
void try_create_directory_full(const string & path)
{
    for (int i = 1; i < path.Length(); i++) {
        if (path[i] == L'/' || path[i] == L'\\') {
            try_create_directory(path.Fragment(0, i));
        }
    }
    try_create_directory(path);
}
void clear_directory(const string & path)
{
    try {
        SafePointer< Array<string> > files = IO::Search::GetFiles(path + L"/*");
        for (int i = 0; i < files->Length(); i++) IO::RemoveFile(path + L"/" + files->ElementAt(i));
        SafePointer< Array<string> > dirs = IO::Search::GetDirectories(path + L"/*");
        for (int i = 0; i < dirs->Length(); i++) {
            string p = path + L"/" + dirs->ElementAt(i);
            clear_directory(p);
            IO::RemoveDirectory(p);
        }
    }
    catch (...) {}
}
bool copy_file_nothrow(const string & source, const string & dest)
{
    try {
        FileStream src(source, AccessRead, OpenExisting);
        FileStream out(dest, AccessWrite, CreateAlways);
        src.CopyTo(&out);
    }
    catch (...) { return false; }
    return true;
}
bool compile(const string & source, const string & object, const string & log, TextWriter & console)
{
    bool vcheck = true;
    handle source_handle;
    handle object_handle;
    try {
        source_handle = IO::CreateFile(source, IO::AccessRead, IO::OpenExisting);
    } catch (...) { return false; }
    try {
        object_handle = IO::CreateFile(object, IO::AccessRead, IO::OpenExisting);
    } catch (...) { vcheck = false; IO::CloseFile(source_handle); }
    if (vcheck && !clean) {
        bool update;
        Time source_time = IO::DateTime::GetFileAlterTime(source_handle);
        Time object_time = IO::DateTime::GetFileAlterTime(object_handle);
        IO::CloseFile(source_handle);
        IO::CloseFile(object_handle);
        update = object_time < source_time || object_time < proj_time;
        if (!update) return true;
        if (object_time < source_time) {
            console << IO::Path::GetFileName(source) << L" renewed (" << source_time.ToLocal().ToString() << L" against " << object_time.ToLocal().ToString() << L")" << IO::NewLineChar;
        }
    } else if (clean) {
        IO::CloseFile(source_handle);
        IO::CloseFile(object_handle);
    }
    console << L"Compiling " << IO::Path::GetFileName(source) << L"...";
    string out_path = object;
    string argdef = sys_cfg->GetValueString(L"Compiler/DefineArgument");
    Array<string> clang_args(0x80);
    clang_args << source;
    clang_args << sys_cfg->GetValueString(L"Compiler/IncludeArgument");
    clang_args << rt_path;
    {
        string out_arg = sys_cfg->GetValueString(L"Compiler/OutputArgument");
        if (out_arg.FindFirst(L'$') == -1) {
            clang_args << out_arg;
            clang_args << out_path;
        } else clang_args << out_arg.Replace(L'$', out_path);
    }
    if (prj_ver.UseDefines) {
        clang_args << argdef;
        clang_args << L"ENGINE_VI_APPNAME=L\"" + prj_ver.AppName.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_COMPANY=L\"" + prj_ver.CompanyName.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_COPYRIGHT=L\"" + prj_ver.Copyright.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_APPSYSNAME=L\"" + prj_ver.InternalName.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_APPSHORTVERSION=L\"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_APPVERSION=L\"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"." + string(prj_ver.Subversion) + L"." + string(prj_ver.Build) + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_VERSIONMAJOR=" + string(prj_ver.VersionMajor);
        clang_args << argdef;
        clang_args << L"ENGINE_VI_VERSIONMINOR=" + string(prj_ver.VersionMinor);
        clang_args << argdef;
        clang_args << L"ENGINE_VI_SUBVERSION=" + string(prj_ver.Subversion);
        clang_args << argdef;
        clang_args << L"ENGINE_VI_BUILD=" + string(prj_ver.Build);
        clang_args << argdef;
        clang_args << L"ENGINE_VI_APPIDENT=L\"" + prj_ver.AppIdent.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_COMPANYIDENT=L\"" + prj_ver.ComIdent.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << argdef;
        clang_args << L"ENGINE_VI_DESCRIPTION=L\"" + prj_ver.Description.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
    }
    if (compile_system == L"windows") {
        clang_args << argdef;
        clang_args << L"ENGINE_WINDOWS=1";
    } else if (compile_system == L"macosx") {
        clang_args << argdef;
        clang_args << L"ENGINE_UNIX=1";
        clang_args << argdef;
        clang_args << L"ENGINE_MACOSX=1";
    }
    if (compile_architecture == L"x64") {
        clang_args << argdef;
        clang_args << L"ENGINE_X64=1";
    }
    if (compile_subsystem == L"console") {
        clang_args << argdef;
        clang_args << L"ENGINE_SUBSYSTEM_CONSOLE=1";
    } else {
        clang_args << argdef;
        clang_args << L"ENGINE_SUBSYSTEM_GUI=1";
    }
    {
        SafePointer<RegistryNode> args_node = sys_cfg->OpenNode(L"Compiler/Arguments");
        if (args_node) {
            auto & args_vals = args_node->GetValues();
            for (int i = 0; i < args_vals.Length(); i++) clang_args << args_node->GetValueString(args_vals[i]);
        }
    }
    handle clang_log = IO::CreateFile(log, IO::AccessReadWrite, IO::CreateAlways);
    IO::SetStandartOutput(clang_log);
    IO::SetStandartError(clang_log);
    IO::CloseFile(clang_log);
    SafePointer<Process> compiler = CreateCommandProcess(sys_cfg->GetValueString(L"Compiler/Path"), &clang_args);
    if (!compiler) {
        console << L"Failed" << IO::NewLineChar;
        console << L"Failed to launch the compiler (" + sys_cfg->GetValueString(L"Compiler/Path") + L")." << IO::NewLineChar;
        return false;
    }
    compiler->Wait();
    if (compiler->GetExitCode()) {
        console << L"Failed" << IO::NewLineChar;
        Shell::OpenFile(log);
        return false;
    }
    console << L"Succeed" << IO::NewLineChar;
    return true;
}
bool link(const Array<string> & objs, const string & exe, const string & real_exe, const string & log, TextWriter & console)
{
    console << L"Linking " << IO::Path::GetFileName(real_exe) << L"...";
    Array<string> clang_args(0x80);
    clang_args << objs;
    {
        string out_arg = sys_cfg->GetValueString(L"Linker/OutputArgument");
        if (out_arg.FindFirst(L'$') == -1) {
            clang_args << out_arg;
            clang_args << exe;
        } else clang_args << out_arg.Replace(L'$', exe);
    }
    {
        SafePointer<RegistryNode> args_node = sys_cfg->OpenNode(L"Linker/Arguments");
        if (args_node) {
            auto & args_vals = args_node->GetValues();
            for (int i = 0; i < args_vals.Length(); i++) clang_args << args_node->GetValueString(args_vals[i]);
        }
    }
    {
        SafePointer<RegistryNode> args_node = sys_cfg->OpenNode(L"Linker/Arguments" + compile_subsystem);
        if (args_node) {
            auto & args_vals = args_node->GetValues();
            for (int i = 0; i < args_vals.Length(); i++) clang_args << args_node->GetValueString(args_vals[i]);
        }
    }
    handle clang_log = IO::CreateFile(log, IO::AccessReadWrite, IO::CreateAlways);
    IO::SetStandartOutput(clang_log);
    IO::SetStandartError(clang_log);
    IO::CloseFile(clang_log);
    SafePointer<Process> linker = CreateCommandProcess(sys_cfg->GetValueString(L"Linker/Path"), &clang_args);
    if (!linker) {
        console << L"Failed" << IO::NewLineChar;
        console << L"Failed to launch the linker (" + sys_cfg->GetValueString(L"Linker/Path") + L")." << IO::NewLineChar;
        return false;
    }
    linker->Wait();
    if (linker->GetExitCode()) {
        console << L"Failed" << IO::NewLineChar;
        console << L"Linking error!" << IO::NewLineChar;
        Shell::OpenFile(log);
        return false;
    }
    console << L"Succeed" << IO::NewLineChar;
    return true;
}
bool BuildRuntime(TextWriter & console)
{
    try {
        try {
            FileStream sys_cfg_src(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/ertbuild.ini", AccessRead, OpenExisting);
            sys_cfg = CompileTextRegistry(&sys_cfg_src);
            SafePointer<RegistryNode> target = sys_cfg->OpenNode(compile_system + L"-" + compile_architecture);
            sys_cfg.SetRetain(target);
            if (!sys_cfg) throw Exception();
        }
        catch (...) {
            console << L"Failed to open runtime configuration." << IO::NewLineChar;
            return false;
        }
        Array<string> clang_src(0x80);
        string sys_cfg_path = IO::Path::GetDirectory(IO::GetExecutablePath());
        rt_path = IO::ExpandPath(sys_cfg_path + L"/" + sys_cfg->GetValueString(L"RuntimePath"));
        if (!sys_cfg->GetValueBoolean(L"UseWhitelist")) {
            SafePointer< Array<string> > files = IO::Search::GetFiles(rt_path + L"/" + sys_cfg->GetValueString(L"CompileFilter"), true);
            for (int i = 0; i < files->Length(); i++) {
                clang_src << rt_path + L"/" + files->ElementAt(i);
            }
        } else {
            SafePointer<RegistryNode> whitelist = sys_cfg->OpenNode(L"Whitelist");
            if (whitelist) {
                auto & paths = whitelist->GetValues();
                for (int i = 0; i < paths.Length(); i++) {
                    clang_src << IO::ExpandPath(rt_path + L"/" + whitelist->GetValueString(paths[i]));
                }
            }
        }
        string asm_path = rt_path + L"/" + sys_cfg->GetValueString(L"ObjectPath");
        try_create_directory_full(asm_path);
        {
            if (clean) {
                SafePointer< Array<string> > files = IO::Search::GetFiles(asm_path + L"/*." + sys_cfg->GetValueString(L"ObjectExtension") + L";*.log");
                for (int i = 0; i < files->Length(); i++) {
                    IO::RemoveFile(asm_path + L"/" + files->ElementAt(i));
                }
            }
        }
        for (int i = 0; i < clang_src.Length(); i++) {
            if (!compile(clang_src[i], IO::ExpandPath(asm_path + L"/" + IO::Path::GetFileNameWithoutExtension(clang_src[i]) + L"." + sys_cfg->GetValueString(L"ObjectExtension")),
                asm_path + L"/" + IO::Path::GetFileNameWithoutExtension(clang_src[i]) + L".log", console)) return 1;
        }
    }
    catch (Exception & ex) {
        console << L"Some shit occured: Engine Exception " << ex.ToString() << IO::NewLineChar;
        return false;
    }
    catch (...) {
        console << L"Some shit occured..." << IO::NewLineChar;
        return false;
    }
    return true;
}
bool run_restool(const string & prj, const string & out_path, const string & bundle_path, TextWriter & console)
{
    Array<string> rt_args(0x10);
    rt_args << prj;
    rt_args << out_path;
    if (compile_system == L"windows") rt_args << L":winres";
    else if (compile_system == L"macosx") rt_args << L":macres";
    if (clean) rt_args << L":clean";
    if (bundle_path.Length()) {
        rt_args << L":bundle";
        rt_args << bundle_path;
    }
    console << L"Starting native resource generator..." << IO::NewLineChar << IO::NewLineChar;
    IO::SetStandartOutput(console_output);
    IO::SetStandartError(console_output);
    SafePointer<Process> restool = CreateCommandProcess(sys_cfg->GetValueString(L"ResourceTool"), &rt_args);
    if (!restool) {
        console << L"Failed to start resource generator (" + sys_cfg->GetValueString(L"ResourceTool") + L")." << IO::NewLineChar;
        return false;
    }
    restool->Wait();
    if (restool->GetExitCode()) {
        console << L"Resource generator failed." << IO::NewLineChar;
        return false;
    }
    console << IO::NewLineChar;
    return true;
}
bool copy_attachments(const string & out_path, const string & base_path, const string & obj_path, TextWriter & console, RegistryNode * node)
{
    auto & res = node->GetSubnodes();
    for (int i = 0; i < res.Length(); i++) {
        string src = node->GetValueString(res[i] + L"/From");
        string dest = node->GetValueString(res[i] + L"/To");
        if (string::CompareIgnoreCase(src.Fragment(0, 9), L"<objpath>") == 0) {
            src = IO::ExpandPath(obj_path + src.Fragment(9, -1));
        } else {
            src = IO::ExpandPath(base_path + L"/" + src);
        }
        string view = dest;
        dest = IO::ExpandPath(out_path + L"/" + dest);
        console << L"Copying attachment \"" + view + L"\"...";
        try_create_directory_full(IO::Path::GetDirectory(dest));
        if (!copy_file_nothrow(src, dest)) {
            console << L"Failed" << IO::NewLineChar;
            return false;
        }
        console << L"Succeed" << IO::NewLineChar;
    }
    return true;
}
bool copy_attachments(const string & out_path, const string & base_path, const string & obj_path, TextWriter & console)
{
    SafePointer<RegistryNode> node = prj_cfg->OpenNode(L"Attachments");
    SafePointer<RegistryNode> node_os_local = prj_cfg->OpenNode(L"Attachments-" + compile_system);
    SafePointer<RegistryNode> node_os_arc_local = prj_cfg->OpenNode(L"Attachments-" + compile_system + L"-" + compile_architecture);
    SafePointer<RegistryNode> node_arc_local = prj_cfg->OpenNode(L"Attachments-" + compile_architecture);
    if (node) {
        if (!copy_attachments(out_path, base_path, obj_path, console, node)) return false;
    }
    if (node_os_local) {
        if (!copy_attachments(out_path, base_path, obj_path, console, node_os_local)) return false;
    }
    if (node_os_arc_local) {
        if (!copy_attachments(out_path, base_path, obj_path, console, node_os_arc_local)) return false;
    }
    if (node_arc_local) {
        if (!copy_attachments(out_path, base_path, obj_path, console, node_arc_local)) return false;
    }
    return true;
}
bool invoke(RegistryNode * node, const string & base_path, const string & obj_path, TextWriter & console)
{
    Array<string> args(0x10);
    string source = node->GetValueString(L"Source");
    string dest = node->GetValueString(L"Dest");
    string verb = node->GetValueString(L"Verb");
    auto & vals = node->GetValues();
    for (int i = 0; i < vals.Length(); i++) if (vals[i][0] == L'_') args << node->GetValueString(vals[i]);
    handle manifest;
    try {
        manifest = IO::CreateFile(base_path + L"/" + verb + L".command.ini", IO::AccessRead, IO::OpenExisting);
    }
    catch (...) {
        try {
            manifest = IO::CreateFile(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + verb + L".command.ini", IO::AccessRead, IO::OpenExisting);
        }
        catch (...) { return false; }
    }
    FileStream manifest_stream(manifest, true);
    SafePointer<Registry> manifest_reg = CompileTextRegistry(&manifest_stream);
    if (!manifest_reg) {
        console << L"Unrecognized command \"" << verb << L"\"." << IO::NewLineChar;
        return false;
    }
    Array<string> command_line(0x10);
    string server = manifest_reg->GetValueString(L"Server");
    SafePointer<RegistryNode> cmdl = manifest_reg->OpenNode(L"CommandLine");
    if (cmdl) {
        auto & cmdla = cmdl->GetValues();
        for (int i = 0; i < cmdla.Length(); i++) {
            string val = cmdl->GetValueString(cmdla[i]);
            if (val == L"$args$") command_line << args;
            else {
                val = val.Replace(L"$source$", source);
                val = val.Replace(L"$dest$", dest);
                command_line << val;
            }
        }
    }
    IO::SetStandartOutput(console_output);
    IO::SetStandartError(console_output);
    SafePointer<Process> process = CreateCommandProcess(server, &command_line);
    if (!process) {
        process.SetReference(CreateProcess(base_path + L"/" + server, &command_line));
    }
    if (!process) {
        console << L"Failed to launch the server \"" << server << L"\"." << IO::NewLineChar;
        return false;
    }
    process->Wait();
    return true;
}
bool do_invokations(const string & base_path, const string & obj_path, TextWriter & console)
{
    SafePointer<RegistryNode> base = prj_cfg->OpenNode(L"Invoke");
    if (base) {
        auto & cmds = base->GetSubnodes();
        for (int i = 0; i < cmds.Length(); i++) {
            SafePointer<RegistryNode> cmd = base->OpenNode(cmds[i]);
            if (cmd) {
                if (!invoke(cmd, base_path, obj_path, console)) return false;
            }
        }
    }
    return true;
}

int Main(void)
{
    console_output = IO::CloneHandle(IO::GetStandartOutput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <project config.ini> [:clean] [:x64]" << IO::NewLineChar;
        console << L"    to build your project" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" :asmrt [:clean] [:x64]" << IO::NewLineChar;
        console << L"    to rebuild runtime object cache" << IO::NewLineChar;
        console << L"  use :clean to recompile all the sources." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        for (int i = 1; i < args->Length(); i++) {
            if (string::CompareIgnoreCase(args->ElementAt(i), L":clean") == 0) clean = true;
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":x64") == 0) compile_architecture = L"x64";
        }
        if (string::CompareIgnoreCase(args->ElementAt(1), L":asmrt") == 0) {
            prj_ver.UseDefines = false;
            if (!BuildRuntime(console)) return 1;
        } else {
            try {
                console << L"Building " << IO::Path::GetFileName(args->ElementAt(1)) << L" on ";
                if (compile_architecture == L"x64") console << L"64-bit "; else console << L"32-bit ";
                if (compile_system == L"windows") console << L"Windows";
                else if (compile_system == L"macosx") console << L"Mac OS X";
                console << IO::NewLineChar;
                string bootstrapper;
                try {
                    FileStream sys_cfg_src(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/ertbuild.ini", AccessRead, OpenExisting);
                    FileStream prj_cfg_src(IO::ExpandPath(args->ElementAt(1)), AccessRead, OpenExisting);
                    sys_cfg = CompileTextRegistry(&sys_cfg_src);
                    prj_cfg = CompileTextRegistry(&prj_cfg_src);
                    SafePointer<RegistryNode> target = sys_cfg->OpenNode(compile_system + L"-" + compile_architecture);
                    sys_cfg.SetRetain(target);
                    if (!sys_cfg | !prj_cfg) throw Exception();
                    proj_time = IO::DateTime::GetFileAlterTime(prj_cfg_src.Handle());
                    string ss = prj_cfg->GetValueString(L"Subsystem");
                    if (string::CompareIgnoreCase(ss, L"console") == 0 || ss.Length() == 0) compile_subsystem = L"console";
                    else if (string::CompareIgnoreCase(ss, L"gui") == 0) compile_subsystem = L"gui";
                    else if (string::CompareIgnoreCase(ss, L"object") == 0) compile_subsystem = L"object";
                    else {
                        console << L"Unknown subsystem \"" + ss + L"\". Use CONSOLE or GUI." << IO::NewLineChar;
                        return 1;
                    }
                    bootstrapper = IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + sys_cfg->GetValueString(L"Bootstrapper");
                }
                catch (...) {
                    console << L"Failed to open project or runtime configuration." << IO::NewLineChar;
                    return 1;
                }
                prj_ver.InternalName = prj_cfg->GetValueString(L"VersionInformation/InternalName");
                if (prj_cfg->GetValueBoolean(L"UseVersionDefines")) {
                    prj_ver.UseDefines = true;
                    prj_ver.AppName = prj_cfg->GetValueString(L"VersionInformation/ApplicationName");
                    prj_ver.CompanyName = prj_cfg->GetValueString(L"VersionInformation/CompanyName");
                    prj_ver.Copyright = prj_cfg->GetValueString(L"VersionInformation/Copyright");
                    prj_ver.AppIdent = prj_cfg->GetValueString(L"VersionInformation/ApplicationIdentifier");
                    prj_ver.ComIdent = prj_cfg->GetValueString(L"VersionInformation/CompanyIdentifier");
                    prj_ver.Description = prj_cfg->GetValueString(L"VersionInformation/Description");
                    try {
                        auto verind = prj_cfg->GetValueString(L"VersionInformation/Version").Split(L'.');
                        if (verind.Length() > 0) prj_ver.VersionMajor = verind[0].ToUInt32(); else prj_ver.VersionMajor = 0;
                        if (verind.Length() > 1) prj_ver.VersionMinor = verind[1].ToUInt32(); else prj_ver.VersionMinor = 0;
                        if (verind.Length() > 2) prj_ver.Subversion = verind[2].ToUInt32(); else prj_ver.Subversion = 0;
                        if (verind.Length() > 3) prj_ver.Build = verind[3].ToUInt32(); else prj_ver.Build = 0;
                    }
                    catch (...) {
                        console << L"Invalid application version notation." << IO::NewLineChar;
                        return 1;
                    }
                    if (!prj_ver.InternalName.Length()) {
                        prj_ver.InternalName = IO::Path::GetFileNameWithoutExtension(args->ElementAt(1));
                        int dot = prj_ver.InternalName.FindFirst(L'.');
                        if (dot != -1) prj_ver.InternalName = prj_ver.InternalName.Fragment(0, dot);
                    }
                } else prj_ver.UseDefines = false;
                Array<string> object_list(0x80);
                Array<string> source_list(0x80);
                string prj_cfg_path = IO::Path::GetDirectory(IO::ExpandPath(args->ElementAt(1)));
                string sys_cfg_path = IO::Path::GetDirectory(IO::GetExecutablePath());
                string prj_path = IO::ExpandPath(prj_cfg_path + L"/" + prj_cfg->GetValueString(L"CompileAt"));
                rt_path = IO::ExpandPath(sys_cfg_path + L"/" + sys_cfg->GetValueString(L"RuntimePath"));
                {
                    string obj_path = rt_path + L"/" + sys_cfg->GetValueString(L"ObjectPath");
                    SafePointer< Array<string> > files = IO::Search::GetFiles(obj_path + L"/*." + sys_cfg->GetValueString(L"ObjectExtension"), true);
                    if (!files->Length()) {
                        console << L"No object files in Runtime cache! Recompile Runtime! (ertbuild :asmrt)" << IO::NewLineChar;
                    }
                    for (int i = 0; i < files->Length(); i++) {
                        object_list << obj_path + L"/" + files->ElementAt(i);
                    }
                }
                if (prj_cfg->GetValueBoolean(L"CompileAll")) {
                    SafePointer< Array<string> > files = IO::Search::GetFiles(prj_path + L"/" + sys_cfg->GetValueString(L"CompileFilter"), true);
                    for (int i = 0; i < files->Length(); i++) {
                        source_list << prj_path + L"/" + files->ElementAt(i);
                    }
                } else {
                    SafePointer<RegistryNode> list = prj_cfg->OpenNode(L"CompileList");
                    if (list) {
                        auto & paths = list->GetValues();
                        for (int i = 0; i < paths.Length(); i++) {
                            source_list << IO::ExpandPath(prj_path + L"/" + list->GetValueString(paths[i]));
                        }
                    }
                }
                string out_path = prj_cfg_path + L"/" + prj_cfg->GetValueString(L"OutputLocation");
                string bitness = compile_architecture == L"x64" ? L"64" : L"32";
                string platform_path = prj_cfg->GetValueString(L"OutputLocation" + compile_system + bitness);
                if (platform_path.Length()) out_path += L"/" + platform_path;
                out_path = IO::ExpandPath(out_path);
                try_create_directory_full(out_path);
                if (clean) clear_directory(out_path);
                try_create_directory(out_path + L"/_obj");
                if (compile_subsystem != L"object") {
                    string obj = out_path + L"/_obj/_bootstrapper." + sys_cfg->GetValueString(L"ObjectExtension");
                    string log = out_path + L"/_obj/_bootstrapper.log";
                    if (!compile(bootstrapper, obj, log, console)) return 1;
                    object_list << obj;
                }
                for (int i = 0; i < source_list.Length(); i++) {
                    string obj;
                    if (compile_subsystem != L"object") {
                        obj = out_path + L"/_obj/" + IO::Path::GetFileNameWithoutExtension(source_list[i]) + L".";
                    } else obj = out_path + L"/" + IO::Path::GetFileNameWithoutExtension(source_list[i]) + L".";
                    string log = obj + L"log";
                    obj += sys_cfg->GetValueString(L"ObjectExtension");
                    if (!compile(source_list[i], obj, log, console)) return 1;
                    object_list << obj;
                }
                if (compile_subsystem != L"object") {
                    if (!do_invokations(prj_path, out_path + L"/_obj", console)) return 1;
                    string out_file = out_path + L"/" + prj_cfg->GetValueString(L"OutputName");
                    string exe_ext = sys_cfg->GetValueString(L"ExecutableExtension");
                    if (exe_ext.Length() && string::CompareIgnoreCase(IO::Path::GetExtension(out_file), exe_ext)) out_file += L"." + exe_ext;
                    if (sys_cfg->GetValueString(L"ResourceTool").Length()) {
                        if (compile_system == L"windows") {
                            string project = IO::ExpandPath(args->ElementAt(1));
                            if (!run_restool(project, out_path + L"/_obj", L"", console)) return 1;
                            object_list << out_path + L"/_obj/" + IO::Path::GetFileNameWithoutExtension(project) + L".res";
                        } else if (compile_system == L"macosx" && compile_subsystem == L"gui") {
                            string project = IO::ExpandPath(args->ElementAt(1));
                            string bundle = out_file;
                            if (string::CompareIgnoreCase(IO::Path::GetExtension(out_file), L"app")) out_file += L".app";
                            if (!run_restool(project, out_path + L"/_obj", out_file, console)) return 1;
                            out_file += L"/Contents/MacOS/" + prj_ver.InternalName;
                        } else if (compile_system == L"macosx" && compile_subsystem == L"console") {
                            SafePointer<RegistryNode> res = prj_cfg->OpenNode(L"Resources");
                            if (res) {
                                console << L"NOTE: Assembly Resources are not supported on console Unix applications!" << IO::NewLineChar;
                            }
                        }
                    } else {
                        SafePointer<RegistryNode> res = prj_cfg->OpenNode(L"Resources");
                        if (res) {
                            console << L"NOTE: Assembly Resources are not available without Resource Tool!" << IO::NewLineChar;
                        }
                        if (compile_system == L"macosx" && compile_subsystem == L"gui") {
                            console << L"NOTE: Can't build Mac OS X Application Bundle without Resource Tool!" << IO::NewLineChar;
                        }
                    }
                    string out_internal = out_path + L"/_obj/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".tmp";
                    if (!copy_attachments(IO::Path::GetDirectory(out_file), prj_path, out_path + L"/_obj", console)) return 1;
                    if (!link(object_list, out_internal, out_file, out_path + L"/_obj/linker-output.log", console)) return 1;
                    try {
                        FileStream src(out_internal, AccessRead, OpenExisting);
                        FileStream out(out_file, AccessReadWrite, CreateAlways);
                        src.CopyTo(&out);
                    } catch (...) {
                        console << L"Failed to substitute the executable." << IO::NewLineChar;
                        return 1;
                    }
                }
            }
            catch (Exception & ex) {
                console << L"Some shit occured: Engine Exception " << ex.ToString() << IO::NewLineChar;
                return 1;
            }
            catch (...) {
                console << L"Some shit occured..." << IO::NewLineChar;
                return 1;
            }
        }
        return 0;
    }
}