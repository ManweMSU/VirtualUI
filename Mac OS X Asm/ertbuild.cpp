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
};

bool clean = false;
string rt_path;
SafePointer<Registry> sys_cfg;
SafePointer<Registry> prj_cfg;
VersionInfo prj_ver;
Time proj_time = 0;

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
    }
    console << L"Compiling " << IO::Path::GetFileName(source) << L"...";
    string out_path = object;
    Array<string> clang_args(0x80);
    clang_args << source;
    clang_args << sys_cfg->GetValueString(L"Compiler/IncludeArgument");
    clang_args << rt_path;
    clang_args << sys_cfg->GetValueString(L"Compiler/OutputArgument");
    clang_args << out_path;
    if (prj_ver.UseDefines) {
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_APPNAME=L\"" + prj_ver.AppName.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_COMPANY=L\"" + prj_ver.CompanyName.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_COPYRIGHT=L\"" + prj_ver.Copyright.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_APPSYSNAME=L\"" + prj_ver.InternalName.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\"") + L"\"";
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_APPSHORTVERSION=L\"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"\"";
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_APPVERSION=L\"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"." + string(prj_ver.Subversion) + L"." + string(prj_ver.Build) + L"\"";
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_VERSIONMAJOR=" + string(prj_ver.VersionMajor);
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_VERSIONMINOR=" + string(prj_ver.VersionMinor);
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_SUBVERSION=" + string(prj_ver.Subversion);
        clang_args << sys_cfg->GetValueString(L"Compiler/DefineArgument");
        clang_args << L"ENGINE_VI_BUILD=" + string(prj_ver.Build);
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
    IO::CloseFile(clang_log);
    console << L"Succeed" << IO::NewLineChar;
    return true;
}
bool link(const Array<string> & objs, const string & exe, const string & log, TextWriter & console)
{
    console << L"Linking " << IO::Path::GetFileName(exe) << L"...";
    Array<string> clang_args(0x80);
    clang_args << objs;
    clang_args << sys_cfg->GetValueString(L"Linker/OutputArgument");
    clang_args << exe;
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
            FileStream sys_cfg_src(IO::GetExecutablePath() + L".ini", AccessRead, OpenExisting);
            sys_cfg = CompileTextRegistry(&sys_cfg_src);
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
        try { IO::CreateDirectory(asm_path); }
        catch (...) {}
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

int main(void)
{
    handle console_output = IO::CloneHandle(IO::GetStandartInput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();
    
    if (args->Length() < 2) {
        console << ENGINE_VI_APPNAME << IO::NewLineChar;
        console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'©', L"(C)") << IO::NewLineChar;
        console << L"Version " << ENGINE_VI_VERSIONMAJOR << L"." << ENGINE_VI_VERSIONMINOR << L"." << ENGINE_VI_SUBVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <project config.ini> [:clean]" << IO::NewLineChar;
        console << L"    to build your project" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" :asmrt [:clean]" << IO::NewLineChar;
        console << L"    to rebuild runtime object cache" << IO::NewLineChar;
        console << L"  use :clean to recompile all the sources." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        for (int i = 1; i < args->Length(); i++) if (string::CompareIgnoreCase(args->ElementAt(i), L":clean") == 0) clean = true;
        if (string::CompareIgnoreCase(args->ElementAt(1), L":asmrt") == 0) {
            prj_ver.UseDefines = false;
            if (!BuildRuntime(console)) return 1;
        } else {
            try {
                try {
                    FileStream sys_cfg_src(IO::GetExecutablePath() + L".ini", AccessRead, OpenExisting);
                    FileStream prj_cfg_src(IO::ExpandPath(args->ElementAt(1)), AccessRead, OpenExisting);
                    sys_cfg = CompileTextRegistry(&sys_cfg_src);
                    prj_cfg = CompileTextRegistry(&prj_cfg_src);
                    if (!sys_cfg | !prj_cfg) throw Exception();
                    proj_time = IO::DateTime::GetFileAlterTime(prj_cfg_src.Handle());
                }
                catch (...) {
                    console << L"Failed to open project or runtime configuration." << IO::NewLineChar;
                    return 1;
                }
                if (prj_cfg->GetValueBoolean(L"UseVersionDefines")) {
                    prj_ver.UseDefines = true;
                    prj_ver.AppName = prj_cfg->GetValueString(L"VersionInformation/ApplicationName");
                    prj_ver.CompanyName = prj_cfg->GetValueString(L"VersionInformation/CompanyName");
                    prj_ver.Copyright = prj_cfg->GetValueString(L"VersionInformation/Copyright");
                    prj_ver.InternalName = prj_cfg->GetValueString(L"VersionInformation/InternalName");
                    try {
                        auto verind = prj_cfg->GetValueString(L"VersionInformation/Version").Split(L'.');
                        if (verind.Length() > 0) prj_ver.VersionMajor = verind[0].ToUInt32();
                        if (verind.Length() > 1) prj_ver.VersionMinor = verind[1].ToUInt32();
                        if (verind.Length() > 2) prj_ver.Subversion = verind[2].ToUInt32();
                        if (verind.Length() > 3) prj_ver.Build = verind[3].ToUInt32();
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
                string out_path = IO::ExpandPath(prj_cfg_path + L"/" + prj_cfg->GetValueString(L"OutputLocation"));
                for (int i = 1; i < out_path.Length(); i++) {
                    if (out_path[i] == L'/') {
                        try { IO::CreateDirectory(out_path.Fragment(0, i)); }
                        catch (...) {}
                    }
                }
                try { IO::CreateDirectory(out_path); } catch (...) {}
                try { IO::CreateDirectory(out_path + L"/_obj"); } catch (...) {}
                for (int i = 0; i < source_list.Length(); i++) {
                    string obj = out_path + L"/_obj/" + IO::Path::GetFileNameWithoutExtension(source_list[i]) + L".";
                    string log = obj + L"log";
                    obj += sys_cfg->GetValueString(L"ObjectExtension");
                    if (!compile(source_list[i], obj, log, console)) return 1;
                    object_list << obj;
                }
                if (!link(object_list, out_path + L"/" + prj_cfg->GetValueString(L"OutputName"), out_path + L"/_obj/linker-output.log", console)) return 1;
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