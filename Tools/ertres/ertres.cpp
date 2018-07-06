#include <EngineRuntime.h>
#include "ertrescodec.h"

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;
using namespace Engine::Codec;

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

string target_system;
SafePointer<RegistryNode> sys_cfg;
SafePointer<RegistryNode> prj_cfg;
Time prj_time;
VersionInfo prj_ver;
bool clean = false;

string make_lexem(const string & str) { return str.Replace(L'\\', L"\\\\").Replace(L'\"', L"\\\""); }
bool compile_resource(const string & rc, const string & res, TextWriter & console)
{
    try {
        Time src_time = 0;
        Time out_time = 0;
        try {
            FileStream src(rc, AccessRead, OpenExisting);
            FileStream out(res, AccessRead, OpenExisting);
            src_time = IO::DateTime::GetFileAlterTime(src.Handle());
            out_time = IO::DateTime::GetFileAlterTime(out.Handle());
        }
        catch (...) {}
        if (src_time < out_time && !clean) return true;

    console << L"Compiling resource script " << IO::Path::GetFileName(rc) << L"...";
    Array<string> rc_args(0x80);
    rc_args << rc;
    {
        string out_arg = sys_cfg->GetValueString(L"Compiler/OutputArgument");
        if (out_arg.FindFirst(L'$') == -1) {
            rc_args << out_arg;
            rc_args << res;
        } else rc_args << out_arg.Replace(L'$', res);
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
    IO::CloseFile(clang_log);
    console << L"Succeed" << IO::NewLineChar;

    }
    catch (...) { return false; }
    return true;
}
bool asm_icon(const string & source, const string & output, TextWriter & console)
{
    try {
        Time src_time = 0;
        Time out_time = 0;
        try {
            FileStream src(source, AccessRead, OpenExisting);
            FileStream date(output, AccessRead, OpenExisting);
            src_time = IO::DateTime::GetFileAlterTime(src.Handle());
            out_time = IO::DateTime::GetFileAlterTime(date.Handle());
        }
        catch (...) {}
        if (src_time < out_time && !clean) return true;
        console << L"Converting icon file " << IO::Path::GetFileName(source) << L"...";
        try {
            FileStream src(source, AccessRead, OpenExisting);
            SafePointer<Image> icon = DecodeImage(&src);
            Array<int> sizes(0x10);
            SafePointer<RegistryNode> sizes_node = sys_cfg->OpenNode(L"IconFormats");
            if (sizes_node) {
                auto & names = sizes_node->GetValues();
                for (int i = 0; i < names.Length(); i++) sizes << sizes_node->GetValueInteger(names[i]);
            }
            for (int i = icon->Frames.Length() - 1; i >= 0; i--) {
                bool match = false;
                for (int j = 0; j < sizes.Length(); j++) {
                    if (icon->Frames[i].GetWidth() == sizes[j] && icon->Frames[i].GetHeight() == sizes[j]) {
                        match = true;
                        break;
                    }
                }
                if (!match) icon->Frames.Remove(i);
            }
            if (icon->Frames.Length()) {
                FileStream out(output, AccessReadWrite, CreateAlways);
                EncodeImage(&out, icon, sys_cfg->GetValueString(L"IconCodec"));
            }
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool asm_resscript(const string & manifest, const Array<string> & icons, const string & rc, TextWriter & console)
{
    try {
        Time max_time = 0;
        Time rc_time = 0;
        string wd = IO::Path::GetDirectory(rc) + L"/";
        try {
            FileStream date(wd + manifest, AccessRead, OpenExisting);
            FileStream rcs(wd + rc, AccessRead, OpenExisting);
            max_time = IO::DateTime::GetFileAlterTime(date.Handle());
            for (int i = 0; i < icons.Length(); i++) {
                FileStream icon(wd + icons[i], AccessRead, OpenExisting);
                Time it = IO::DateTime::GetFileAlterTime(icon.Handle());
                if (it > max_time) max_time = it;
            }
            if (prj_time > max_time) max_time = prj_time;
            rc_time = IO::DateTime::GetFileAlterTime(rcs.Handle());
        }
        catch (...) {}
        if (max_time < rc_time && !clean) return true;
        console << L"Writing resource script file " << IO::Path::GetFileName(rc) << L"...";
        try {
            FileStream rc_file(rc, AccessWrite, CreateAlways);
            TextWriter script(&rc_file, Encoding::UTF16);
            script.WriteEncodingSignature();
            script << L"#include <Windows.h>" << IO::NewLineChar << IO::NewLineChar;
            script << L"CREATEPROCESS_MANIFEST_RESOURCE_ID RT_MANIFEST \"" + manifest + L"\"" << IO::NewLineChar << IO::NewLineChar;
            if (icons.Length()) {
                for (int i = 0; i < icons.Length(); i++) {
                    script << string(i + 1) + L" ICON \"" + icons[i] + L"\"" << IO::NewLineChar;
                }
                script << IO::NewLineChar;
            }
            script << L"1 VERSIONINFO" << IO::NewLineChar;
            script << L"FILEVERSION " + string(prj_ver.VersionMajor) + L", " + string(prj_ver.VersionMinor) + L", " +
                string(prj_ver.Subversion) + L", " + string(prj_ver.Build) << IO::NewLineChar;
            script << L"PRODUCTVERSION " + string(prj_ver.VersionMajor) + L", " + string(prj_ver.VersionMinor) + L", " +
                string(prj_ver.Subversion) + L", " + string(prj_ver.Build) << IO::NewLineChar;
            script << L"FILEFLAGSMASK 0x3fL" << IO::NewLineChar;
            script << L"FILEFLAGS 0x0L" << IO::NewLineChar;
            script << L"FILEOS 0x40004L" << IO::NewLineChar;
            script << L"FILETYPE 0x1L" << IO::NewLineChar;
            script << L"FILESUBTYPE 0x0L" << IO::NewLineChar;
            script << L"BEGIN" << IO::NewLineChar;
            script << L"\tBLOCK \"StringFileInfo\"" << IO::NewLineChar;
            script << L"\tBEGIN" << IO::NewLineChar;
            script << L"\t\tBLOCK \"041904b0\"" << IO::NewLineChar;
            script << L"\t\tBEGIN" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"CompanyName\", \"" + make_lexem(prj_ver.CompanyName) + L"\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"FileDescription\", \"" + make_lexem(prj_ver.AppName) + L"\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"FileVersion\", \"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"InternalName\", \"" + make_lexem(prj_ver.InternalName) + L"\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"LegalCopyright\", \"" + make_lexem(prj_ver.Copyright) + L"\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"OriginalFilename\", \"" + make_lexem(prj_ver.InternalName) + L".exe\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"ProductName\", \"" + make_lexem(prj_ver.AppName) + L"\"" << IO::NewLineChar;
            script << L"\t\t\tVALUE \"ProductVersion\", \"" + string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"\"" << IO::NewLineChar;
            script << L"\t\tEND" << IO::NewLineChar;
            script << L"\tEND" << IO::NewLineChar;
            script << L"\tBLOCK \"VarFileInfo\"" << IO::NewLineChar;
            script << L"\tBEGIN" << IO::NewLineChar;
            script << L"\t\tVALUE \"Translation\", 0x419, 1200" << IO::NewLineChar;
            script << L"\tEND" << IO::NewLineChar;
            script << L"END";
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}
bool asm_manifest(const string & output, TextWriter & console)
{
    try {
        Time man_time = 0;
        try {
            FileStream date(output, AccessRead, OpenExisting);
            man_time = IO::DateTime::GetFileAlterTime(date.Handle());
        }
        catch (...) {}
        if (prj_time < man_time && !clean) return true;
        console << L"Writing manifest file " << IO::Path::GetFileName(output) << L"...";
        try {
            FileStream man_file(output, AccessWrite, CreateAlways);
            TextWriter manifest(&man_file, Encoding::UTF8);
            manifest << L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>" << IO::NewLineChar;
            manifest << L"<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">" << IO::NewLineChar;
            manifest << L"<assemblyIdentity version=\"" +
                string(prj_ver.VersionMajor) + L"." + string(prj_ver.VersionMinor) + L"." + string(prj_ver.Subversion) + L"." + string(prj_ver.Build) +
                L"\" processorArchitecture=\"*\" name=\"" +
                prj_ver.ComIdent + L"." + prj_ver.AppIdent +
                L"\" type=\"win32\"/>" << IO::NewLineChar;
            manifest << L"<description>" + prj_ver.Description + L"</description>" << IO::NewLineChar;
            // Common Controls 6.0 support
            manifest << L"<dependency><dependentAssembly>" << IO::NewLineChar;
            manifest << L"\t<assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\" processorArchitecture=\"*\" "
                L"publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>" << IO::NewLineChar;
            manifest << L"</dependentAssembly></dependency>" << IO::NewLineChar;
            // Make application DPI-aware, enable very long paths
            manifest << L"<application xmlns=\"urn:schemas-microsoft-com:asm.v3\">" << IO::NewLineChar;
            manifest << L"\t<windowsSettings>" << IO::NewLineChar;
            manifest << L"\t\t<dpiAware xmlns=\"http://schemas.microsoft.com/SMI/2005/WindowsSettings\">true</dpiAware>" << IO::NewLineChar;
            manifest << L"\t</windowsSettings>" << IO::NewLineChar;
            manifest << L"\t<windowsSettings xmlns:ws2=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">" << IO::NewLineChar;
            manifest << L"\t\t<ws2:longPathAware>true</ws2:longPathAware>" << IO::NewLineChar;
            manifest << L"\t</windowsSettings>" << IO::NewLineChar;
            manifest << L"</application>" << IO::NewLineChar;
            // UAC Execution level
            manifest << L"<trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v2\"><security><requestedPrivileges>" << IO::NewLineChar;
            manifest << L"\t<requestedExecutionLevel level=\"asInvoker\" uiAccess=\"FALSE\"></requestedExecutionLevel>" << IO::NewLineChar;
            manifest << L"</requestedPrivileges></security></trustInfo>" << IO::NewLineChar;
            // Finilize
            manifest << L"</assembly>";
        }
        catch (...) { console << L"Failed" << IO::NewLineChar; throw; }
        console << L"Succeed" << IO::NewLineChar;
    }
    catch (...) { return false; }
    return true;
}

int Main(void)
{
    InitializeCodecs();
    handle console_output = IO::CloneHandle(IO::GetStandartOutput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <project config.ini> <object path> <:winres|:macres> [:clean]" << IO::NewLineChar;
        console << L"Where" << IO::NewLineChar;
        console << L"  project config.ini  - project configuration to take data," << IO::NewLineChar;
        console << L"  object path         - path to folder to store intermediate and final output," << IO::NewLineChar;
        console << L"  :winres             - build .ico, .manifest, .rc and .res resources," << IO::NewLineChar;
        console << L"  :macres             - build .icns and info.plist resources," << IO::NewLineChar;
        console << L"  :clean              - rebuild all outputs." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        if (args->Length() < 4) {
            console << L"Not enough arguments." << IO::NewLineChar;
            return 1;
        }
        for (int i = 3; i < args->Length(); i++) {
            if (string::CompareIgnoreCase(args->ElementAt(i), L":winres") == 0) target_system = L"windows";
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":macres") == 0) target_system = L"macosx";
            else if (string::CompareIgnoreCase(args->ElementAt(i), L":clean") == 0) clean = true;
        }
        try {
            try {
                FileStream sys_reg_stream(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + string(ENGINE_VI_APPSYSNAME) + L".ini", AccessRead, OpenExisting);
                FileStream prj_reg_stream(args->ElementAt(1), AccessRead, OpenExisting);
                SafePointer<RegistryNode> sys_reg = CompileTextRegistry(&sys_reg_stream);
                prj_cfg = CompileTextRegistry(&prj_reg_stream);
                if (sys_reg) sys_cfg = sys_reg->OpenNode(target_system);
                if (!sys_cfg || !prj_cfg) throw Exception();
                prj_time = IO::DateTime::GetFileAlterTime(prj_reg_stream.Handle());
                {
                    prj_ver.UseDefines = true;
                    prj_ver.AppName = prj_cfg->GetValueString(L"VersionInformation/ApplicationName");
                    prj_ver.CompanyName = prj_cfg->GetValueString(L"VersionInformation/CompanyName");
                    prj_ver.Copyright = prj_cfg->GetValueString(L"VersionInformation/Copyright");
                    prj_ver.InternalName = prj_cfg->GetValueString(L"VersionInformation/InternalName");
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
                }
            }
            catch (...) {
                console << L"Failed to load project or tool configuration." << IO::NewLineChar;
                return 1;
            }
            if (target_system == L"windows") {
                string manifest = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".manifest";
                if (!asm_manifest(manifest, console)) return 1;
                Array<string> icons(0x10);
                string app_icon = prj_cfg->GetValueString(L"ApplicationIcon");
                if (app_icon.Length()) {
                    app_icon = IO::ExpandPath(IO::Path::GetDirectory(args->ElementAt(1)) + L"/" + app_icon);
                    string conv_icon = IO::Path::GetFileNameWithoutExtension(app_icon) + L"." + sys_cfg->GetValueString(L"IconExtension");
                    conv_icon = args->ElementAt(2) + L"/" + conv_icon;
                    if (!asm_icon(app_icon, conv_icon, console)) return 1;
                    app_icon = conv_icon;
                    icons << IO::Path::GetFileName(app_icon);
                }
                string rc = args->ElementAt(2) + L"/" + IO::Path::GetFileNameWithoutExtension(args->ElementAt(1)) + L".rc";
                manifest = IO::Path::GetFileName(manifest);
                if (!asm_resscript(manifest, icons, rc, console)) return 1;
            } else if (target_system == L"macosx") {
                // make plist
                string app_icon = prj_cfg->GetValueString(L"ApplicationIcon");
                if (app_icon.Length()) {
                    app_icon = IO::ExpandPath(IO::Path::GetDirectory(args->ElementAt(1)) + L"/" + app_icon);
                    string conv_icon = IO::Path::GetFileNameWithoutExtension(app_icon) + L"." + sys_cfg->GetValueString(L"IconExtension");
                    conv_icon = args->ElementAt(2) + L"/" + conv_icon;
                    if (!asm_icon(app_icon, conv_icon, console)) return 1;
                }
            } else {
                console << L"Invalid target system." << IO::NewLineChar;
                return 1;
            }
        }
        catch (...) {
            console << L"Some shit occured..." << IO::NewLineChar;
            return 1;
        }
    }
    return 0;
}