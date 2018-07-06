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
bool link(const Array<string> & objs, const string & exe, const string & log, TextWriter & console)
{
    console << L"Linking " << IO::Path::GetFileName(exe) << L"...";
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
        try { IO::CreateDirectory(asm_path); } catch (...) {}
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

int Main(void)
{
    console_output = IO::CloneHandle(IO::GetStandartOutput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();
    
    if (args->Length() < 2) {
        console << ENGINE_VI_APPNAME << IO::NewLineChar;
        console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
        console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <project config.ini> [:clean] [:x64]" << IO::NewLineChar;
        console << L"    to build your project" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" :asmrt [:clean] [:x64]" << IO::NewLineChar;
        console << L"    to rebuild runtime object cache" << IO::NewLineChar;
        console << L"  use :clean to recompile all the sources." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        console << ENGINE_VI_APPNAME << IO::NewLineChar;
        console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
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
                for (int i = 1; i < out_path.Length(); i++) {
                    if (out_path[i] == L'/' || out_path[i] == L'\\') {
                        try { IO::CreateDirectory(out_path.Fragment(0, i)); } catch (...) {}
                    }
                }
                try { IO::CreateDirectory(out_path); } catch (...) {}
                try { IO::CreateDirectory(out_path + L"/_obj"); } catch (...) {}
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
					string out_file = out_path + L"/" + prj_cfg->GetValueString(L"OutputName");
					string exe_ext = sys_cfg->GetValueString(L"ExecutableExtension");
					if (exe_ext.Length() && string::CompareIgnoreCase(IO::Path::GetExtension(out_file), exe_ext)) out_file += L"." + exe_ext;
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
                    }
					if (!link(object_list, out_file, out_path + L"/_obj/linker-output.log", console)) return 1;
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