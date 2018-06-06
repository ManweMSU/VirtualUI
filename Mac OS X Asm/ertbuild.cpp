#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

int main(void)
{
    handle console_output = IO::CloneHandle(IO::GetStandartInput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();
    
    if (args->Length() < 2) {
        console << L"Engine Runtime Mac OS X builder" << IO::NewLineChar;
        console << L"Copyright (C) Engine Software. 2018" << IO::NewLineChar << IO::NewLineChar;
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  ertbuild <project config.ini>" << IO::NewLineChar;
        console << L"    to build your project" << IO::NewLineChar;
        console << L"  ertbuild :asmrt" << IO::NewLineChar;
        console << L"    to rebuild runtime object cache" << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        if (string::CompareIgnoreCase(args->ElementAt(1), L":asmrt") == 0) {
            try {
                SafePointer<Registry> sys_cfg;
                try {
                    FileStream sys_cfg_src(IO::GetExecutablePath() + L".ini", AccessRead, OpenExisting);
                    sys_cfg = CompileTextRegistry(&sys_cfg_src);
                    if (!sys_cfg) throw Exception();
                }
                catch (...) {
                    console << L"Failed to open runtime configuration." << IO::NewLineChar;
                    return 1;
                }
                Array<string> clang_src(0x80);
                string sys_cfg_path = IO::Path::GetDirectory(IO::GetExecutablePath());
                string rt_path = IO::ExpandPath(sys_cfg_path + L"/" + sys_cfg->GetValueString(L"RuntimePath"));
                if (!sys_cfg->GetValueBoolean(L"UseWhitelist")) {
                    SafePointer< Array<string> > files = IO::Search::GetFiles(rt_path + L"/*.c;*.cpp;*.m;*.mm", true);
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
                    SafePointer< Array<string> > files = IO::Search::GetFiles(asm_path + L"/*.o;*.log");
                    for (int i = 0; i < files->Length(); i++) {
                        IO::RemoveFile(asm_path + L"/" + files->ElementAt(i));
                    }
                }
                for (int i = 0; i < clang_src.Length(); i++) {
                    Array<string> clang_args(0x80);
                    clang_args << L"-c";
                    clang_args << clang_src[i];
                    string out_path = IO::ExpandPath(asm_path + L"/" + IO::Path::GetFileNameWithoutExtension(clang_src[i]) + L".o");
                    clang_args << L"-I" + rt_path;
                    clang_args << L"-o";
                    clang_args << out_path;
                    clang_args << L"-O3";
                    clang_args << L"-std=c++14";
                    clang_args << L"-v";
                    clang_args << L"-DENGINE_UNIX=1";
                    clang_args << L"-DENGINE_X64=1";
                    clang_args << L"-DENGINE_MACOSX=1";
                    clang_args << L"-fmodules";
                    clang_args << L"-fcxx-modules";
                    handle clang_log = IO::CreateFile(asm_path + L"/" + IO::Path::GetFileNameWithoutExtension(clang_src[i]) + L".log", IO::AccessReadWrite, IO::CreateAlways);
                    IO::SetStandartOutput(clang_log);
                    IO::SetStandartError(clang_log);
                    IO::CloseFile(clang_log);
                    SafePointer<Process> compiler = CreateProcess(sys_cfg->GetValueString(L"CompilerPath"), &clang_args);
                    if (!compiler) {
                        console << L"Failed to launch the compiler (" + sys_cfg->GetValueString(L"CompilerPath") + L")." << IO::NewLineChar;
                        return 1;
                    }
                    compiler->Wait();
                    if (compiler->GetExitCode()) {
                        console << L"Compilation error!" << IO::NewLineChar;
                        Shell::OpenFile(asm_path + L"/" + IO::Path::GetFileNameWithoutExtension(clang_src[i]) + L".log");
                        return 1;
                    }
                    IO::CloseFile(clang_log);
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
        } else {
            try {
                SafePointer<Registry> sys_cfg;
                SafePointer<Registry> prj_cfg;
                try {
                    FileStream sys_cfg_src(IO::GetExecutablePath() + L".ini", AccessRead, OpenExisting);
                    FileStream prj_cfg_src(IO::ExpandPath(args->ElementAt(1)), AccessRead, OpenExisting);
                    sys_cfg = CompileTextRegistry(&sys_cfg_src);
                    prj_cfg = CompileTextRegistry(&prj_cfg_src);
                    if (!sys_cfg | !prj_cfg) throw Exception();
                }
                catch (...) {
                    console << L"Failed to open project or runtime configuration." << IO::NewLineChar;
                    return 1;
                }
                Array<string> clang_args(0x80);
                string prj_cfg_path = IO::Path::GetDirectory(IO::ExpandPath(args->ElementAt(1)));
                string sys_cfg_path = IO::Path::GetDirectory(IO::GetExecutablePath());
                string prj_path = IO::ExpandPath(prj_cfg_path + L"/" + prj_cfg->GetValueString(L"CompileAt"));
                string rt_path = IO::ExpandPath(sys_cfg_path + L"/" + sys_cfg->GetValueString(L"RuntimePath"));
                {
                    string obj_path = rt_path + L"/" + sys_cfg->GetValueString(L"ObjectPath");
                    SafePointer< Array<string> > files = IO::Search::GetFiles(obj_path + L"/*.o", true);
                    if (!files->Length()) {
                        console << L"No object files in Runtime cache! Recompile Runtime! (ertbuild :asmrt)" << IO::NewLineChar;
                    }
                    for (int i = 0; i < files->Length(); i++) {
                        clang_args << obj_path + L"/" + files->ElementAt(i);
                    }
                }
                if (prj_cfg->GetValueBoolean(L"CompileAll")) {
                    SafePointer< Array<string> > files = IO::Search::GetFiles(prj_path + L"/*.c;*.cpp;*.m;*.mm", true);
                    for (int i = 0; i < files->Length(); i++) {
                        clang_args << prj_path + L"/" + files->ElementAt(i);
                    }
                } else {
                    SafePointer<RegistryNode> list = prj_cfg->OpenNode(L"CompileList");
                    if (list) {
                        auto & paths = list->GetValues();
                        for (int i = 0; i < paths.Length(); i++) {
                            clang_args << IO::ExpandPath(prj_path + L"/" + list->GetValueString(paths[i]));
                        }
                    }
                }
                string out_path = IO::ExpandPath(prj_cfg_path + L"/" + prj_cfg->GetValueString(L"OutputLocation"));
                clang_args << L"-I" + rt_path;
                clang_args << L"-o";
                clang_args << out_path + L"/" + prj_cfg->GetValueString(L"OutputName");
                clang_args << L"-O3";
                clang_args << L"-std=c++14";
                clang_args << L"-v";
                clang_args << L"-DENGINE_UNIX=1";
                clang_args << L"-DENGINE_X64=1";
                clang_args << L"-DENGINE_MACOSX=1";
                clang_args << L"-fmodules";
                clang_args << L"-fcxx-modules";
                for (int i = 1; i < out_path.Length(); i++) {
                    if (out_path[i] == L'/') {
                        try { IO::CreateDirectory(out_path.Fragment(0, i)); }
                        catch (...) {}
                    }
                }
                try { IO::CreateDirectory(out_path); }
                catch (...) {}
                try {
                    try { IO::CreateDirectory(out_path + L"/_buildlog"); } catch (...) {}
                    FileStream argv_stream(out_path + L"/_buildlog/clang-args.log", AccessReadWrite, CreateAlways);
                    TextWriter argv_writer(&argv_stream);
                    argv_writer.WriteEncodingSignature();
                    for (int i = 0; i < clang_args.Length(); i++) {
                        argv_writer << clang_args[i] << IO::NewLineChar;   
                    }
                }
                catch (...) {}
                handle clang_log = IO::CreateFile(out_path + L"/_buildlog/clang-output.log", IO::AccessReadWrite, IO::CreateAlways);
                IO::SetStandartOutput(clang_log);
                IO::SetStandartError(clang_log);
                IO::CloseFile(clang_log);
                SafePointer<Process> compiler = CreateProcess(sys_cfg->GetValueString(L"CompilerPath"), &clang_args);
                if (!compiler) {
                    console << L"Failed to launch the compiler (" + sys_cfg->GetValueString(L"CompilerPath") + L")." << IO::NewLineChar;
                    return 1;
                }
                compiler->Wait();
                if (compiler->GetExitCode()) {
                    console << L"Compilation error!" << IO::NewLineChar;
                    Shell::OpenFile(out_path + L"/_buildlog/clang-output.log");
                    return 1;
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