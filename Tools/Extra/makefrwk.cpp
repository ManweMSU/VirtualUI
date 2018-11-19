#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

handle console_output;

void create_directory(const string & path)
{
    try {
        IO::CreateDirectory(path);
    } catch (...) {}
}

int Main(void)
{
    console_output = IO::CloneHandle(IO::GetStandartOutput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;

    SafePointer<RegistryNode> cfg;
    try {
        FileStream src(IO::Path::GetDirectory(IO::GetExecutablePath()) + L"/ertbuild.ini", AccessRead, OpenExisting);
        cfg.SetReference(CompileTextRegistry(&src));
        if (cfg) cfg.SetReference(cfg->OpenNode(L"MacOSX-X64"));
    } catch (...) {}
    if (!cfg) {
        console << L"Failed to load runtime configuration." << IO::NewLineChar;
        return 1;
    }
    string exec_path = IO::Path::GetDirectory(IO::GetExecutablePath());
    string rt_path = IO::ExpandPath(exec_path + L"/" + cfg->GetValueString(L"RuntimePath"));
    string obj_path = IO::ExpandPath(rt_path + L"/" + cfg->GetValueString(L"ObjectPath"));

    Array<string> headers(0x20);
    Array<string> objects(0x20);

    {
        SafePointer< Array<string> > files = IO::Search::GetFiles(obj_path + L"/*." + cfg->GetValueString(L"ObjectExtension"));
        if (!files->Length()) {
            console << L"Object cache is empty. Recompile the runtime." << IO::NewLineChar;
            return 1;
        }
        for (int i = 0; i < files->Length(); i++) objects << obj_path + L"/" + files->ElementAt(i);
    }
    {
        SafePointer< Array<string> > files = IO::Search::GetFiles(rt_path + L"/*.h", true);
        auto op = cfg->GetValueString(L"ObjectPath");
        for (int i = 0; i < files->Length(); i++) {
            if (files->ElementAt(i).FindFirst(op) != -1) continue;
            headers << rt_path + L"/" + files->ElementAt(i);
        }
    }
    {
        Array<string> args(0x10);
        args << L"-rf";
        args << obj_path + L"/EngineRuntime.framework";
        SafePointer<Process> rm = CreateCommandProcess(L"rm", &args);
        if (rm) rm->Wait();
    }
    create_directory(obj_path + L"/EngineRuntime.framework");
    create_directory(obj_path + L"/EngineRuntime.framework/Versions");
    create_directory(obj_path + L"/EngineRuntime.framework/Versions/A");
    create_directory(obj_path + L"/EngineRuntime.framework/Versions/A/Headers");
    create_directory(obj_path + L"/EngineRuntime.framework/Versions/A/Resources");
    try {
        FileStream src(exec_path + L"/rtfrmwk.txt", AccessRead, OpenExisting);
        FileStream out(obj_path + L"/EngineRuntime.framework/Versions/A/Resources/Info.plist", AccessReadWrite, CreateAlways);
        src.CopyTo(&out);
    } catch (...) {
        console << L"Failed to create bundle property list." << IO::NewLineChar;
        return 1;
    }
    for (int i = 0; i < headers.Length(); i++) {
        try {
            string out_name = obj_path + L"/EngineRuntime.framework/Versions/A/Headers" + headers[i].Fragment(rt_path.Length(), -1);
            create_directory(IO::Path::GetDirectory(out_name));
            FileStream src(headers[i], AccessRead, OpenExisting);
            FileStream out(out_name, AccessReadWrite, CreateAlways);
            if (IO::Path::GetFileNameWithoutExtension(out_name) == L"EngineRuntime") {
                TextReader rdr(&src);
                TextWriter wr(&out);
                while (!rdr.EofReached()) {
                    string s = rdr.ReadLine();
                    if (s == L"#pragma once") {
                        wr.WriteLine(L"#pragma once");
                        wr.WriteLine(L"");
                        wr.WriteLine(L"#define ENGINE_UNIX");
                        wr.WriteLine(L"#define ENGINE_MACOSX");
                        wr.WriteLine(L"#define ENGINE_X64");
                    } else wr.WriteLine(s);
                }
            } else src.CopyTo(&out);
        } catch (...) {
            console << L"Failed to copy a header." << IO::NewLineChar;
            return 1;
        }
    }
    {
        string exe = obj_path + L"/EngineRuntime.framework/Versions/A/EngineRuntime";
        string log = obj_path + L"/framework-link.log";
        Array<string> clang_args(0x80);
        clang_args << objects;
        {
            string out_arg = cfg->GetValueString(L"Linker/OutputArgument");
            if (out_arg.FindFirst(L'$') == -1) {
                clang_args << out_arg;
                clang_args << exe;
            } else clang_args << out_arg.Replace(L'$', exe);
        }
        {
            SafePointer<RegistryNode> args_node = cfg->OpenNode(L"Linker/Arguments");
            if (args_node) {
                auto & args_vals = args_node->GetValues();
                for (int i = 0; i < args_vals.Length(); i++) clang_args << args_node->GetValueString(args_vals[i]);
            }
        }
        clang_args << L"-shared";
        clang_args << L"-dynamiclib";
        clang_args << L"-Wl,-install_name,@loader_path/../Frameworks/EngineRuntime.framework/EngineRuntime";
        handle clang_log = IO::CreateFile(log, IO::AccessReadWrite, IO::CreateAlways);
        IO::SetStandartOutput(clang_log);
        IO::SetStandartError(clang_log);
        IO::CloseFile(clang_log);
        SafePointer<Process> linker = CreateCommandProcess(cfg->GetValueString(L"Linker/Path"), &clang_args);
        if (!linker) {
            console << L"Failed to launch the linker (" + cfg->GetValueString(L"Linker/Path") + L")." << IO::NewLineChar;
            return 1;
        }
        linker->Wait();
        if (linker->GetExitCode()) {
            console << L"Linking error!" << IO::NewLineChar;
            Shell::OpenFile(log);
            return 1;
        }
    }
    try {
        IO::CreateSymbolicLink(obj_path + L"/EngineRuntime.framework/Versions/Current", L"A");
        IO::CreateSymbolicLink(obj_path + L"/EngineRuntime.framework/Resources", L"Versions/Current/Resources");
        IO::CreateSymbolicLink(obj_path + L"/EngineRuntime.framework/Headers", L"Versions/Current/Headers");
        IO::CreateSymbolicLink(obj_path + L"/EngineRuntime.framework/EngineRuntime", L"Versions/Current/EngineRuntime");   
    } catch (...) {
        console << L"Symbolic linking failed." << IO::NewLineChar;
        return 1;
    }
    return 0;
}