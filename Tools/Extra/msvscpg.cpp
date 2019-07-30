#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

enum class Subsystem { Console, GUI };

handle console_output;

int Main(void)
{
    console_output = IO::CloneHandle(IO::GetStandardOutput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <project config.ini>" << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        string project_name = IO::Path::GetFileNameWithoutExtension(args->ElementAt(1));
        string project_conf = IO::ExpandPath(args->ElementAt(1));
        string project_path = IO::Path::GetDirectory(project_conf);
        if (project_name.FindFirst(L'.') != -1) project_name = project_name.Fragment(0, project_name.FindFirst(L'.'));
        SafePointer<Registry> cfg;
        SafePointer<Registry> rt_cfg;
        try {
            FileStream src(project_conf, AccessRead, OpenExisting);
            cfg.SetReference(CompileTextRegistry(&src));
            FileStream rt_src(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/ertbuild.ini", AccessRead, OpenExisting);
            rt_cfg.SetReference(CompileTextRegistry(&rt_src));
        } catch (...) {}
        if (!cfg) {
            console << L"Failed to load project configuration." << IO::NewLineChar;
            return 1;
        }
        if (!rt_cfg) {
            console << L"Failed to load runtime configuration." << IO::NewLineChar;
            return 1;
        }
        // mac os x specific
        auto include = IO::ExpandPath(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/" + rt_cfg->GetValueString(L"MacOSX-X64/RuntimePath"));
        try {
            FileStream workspace(project_path + L"/" + project_name + L".code-workspace", AccessReadWrite, CreateAlways);
            TextWriter wswr(&workspace, Encoding::UTF8);
            wswr << L"{" << IO::NewLineChar;
            wswr << L"    \"folders\": [" << IO::NewLineChar;
            wswr << L"        {" << IO::NewLineChar;
            wswr << L"            \"path\": \".\"" << IO::NewLineChar;
            wswr << L"        }" << IO::NewLineChar;
            wswr << L"    ]," << IO::NewLineChar;
            wswr << L"    \"settings\": {}" << IO::NewLineChar;
            wswr << L"}";
        } catch (...) { console << L"Failed to write a workspace." << IO::NewLineChar; return 1; }
        try {
            IO::CreateDirectory(project_path + L"/.vscode");
        } catch (IO::DirectoryAlreadyExistsException & e) {} catch (...) { console << L"Failed to create a .vscode directory." << IO::NewLineChar; return 1; }
        Subsystem subsystem = Subsystem::Console;
        if (string::CompareIgnoreCase(cfg->GetValueString(L"Subsystem"), L"GUI") == 0) {
            subsystem = Subsystem::GUI;
        }
        try {
            FileStream isp(project_path + L"/.vscode/c_cpp_properties.json", AccessReadWrite, CreateAlways);
            FileStream isps(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/msvsc_is.txt", AccessRead, OpenExisting);
            TextReader iss(&isps);
            TextWriter is(&isp, Encoding::UTF8);
            string prop = iss.ReadAll();
            string sss = subsystem == Subsystem::Console ? L"ENGINE_SUBSYSTEM_CONSOLE" : L"ENGINE_SUBSYSTEM_GUI";
            prop = prop.Replace(L"$rtinc$", include).Replace(L"$rtss$", sss);
            is.Write(prop);
        } catch (...) { console << L"Failed to write IntelliSense configuration." << IO::NewLineChar; return 1; }
        try {
            FileStream btp(project_path + L"/.vscode/tasks.json", AccessReadWrite, CreateAlways);
            FileStream btps(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/msvsc_bt.txt", AccessRead, OpenExisting);
            TextReader bts(&btps);
            TextWriter bt(&btp, Encoding::UTF8);
            string prop = bts.ReadAll();
            string rtbt = IO::ExpandPath(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/ertbuild");
            prop = prop.Replace(L"$rtbt$", rtbt).Replace(L"$rtprj$", project_conf);
            bt.Write(prop);
        } catch (...) { console << L"Failed to write Tasks configuration." << IO::NewLineChar; return 1; }
        try {
            FileStream rp(project_path + L"/.vscode/launch.json", AccessReadWrite, CreateAlways);
            FileStream rps(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/msvsc_r.txt", AccessRead, OpenExisting);
            TextReader rs(&rps);
            TextWriter r(&rp, Encoding::UTF8);
            string prop = rs.ReadAll();
            string exec = project_path + L"/" + cfg->GetValueString(L"OutputLocation") + L"/";
            string out_macosx = cfg->GetValueString(L"OutputLocationMacOSX64");
            string ext = rt_cfg->GetValueString(L"MacOSX-X64/ExecutableExtension");
            if (subsystem == Subsystem::GUI) ext = L"app";
            if (out_macosx.Length()) exec += out_macosx + L"/";
            exec += cfg->GetValueString(L"OutputName");
            if (ext.Length()) exec += L"." + ext;
            if (subsystem == Subsystem::GUI) {
                exec += L"/Contents/MacOS/";
                string intname = cfg->GetValueString(L"VersionInformation/InternalName");
                if (!intname.Length()) intname = project_name;
                exec += intname;
            }
            prop = prop.Replace(L"$exec$", IO::ExpandPath(exec));
            r.Write(prop);
        } catch (...) { console << L"Failed to write Launch configuration." << IO::NewLineChar; return 1; }
        console << L"Generation completed." << IO::NewLineChar << IO::NewLineChar;
    }
    return 0;
}