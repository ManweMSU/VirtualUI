#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

int Main(void)
{
    handle console_output = IO::CloneHandle(IO::GetStandartInput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;

    if (args->Length() < 2 || string::CompareIgnoreCase(args->ElementAt(1), L":sync")) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" :sync" << IO::NewLineChar;
        console << L"    to syncronize the runtime." << IO::NewLineChar;
        console << IO::NewLineChar;
        return 0;
    }

    IO::SetCurrentDirectory(IO::Path::GetDirectory(IO::GetExecutablePath()));
    SafePointer<RegistryNode> rt_reg;
    SafePointer<Registry> sync_reg;
    try {
        FileStream rt_stream(L"ertbuild.ini", AccessRead, OpenExisting);
        FileStream sync_stream(L"ertsync.ini", AccessRead, OpenExisting);
        rt_reg = CompileTextRegistry(&rt_stream);
        sync_reg = CompileTextRegistry(&sync_stream);
        if (!rt_reg) throw Exception();
        SafePointer<RegistryNode> conf = rt_reg->OpenNode(L"MacOSX-X64");
        rt_reg.SetRetain(conf);
        if (!rt_reg || !sync_reg) throw Exception();
    }
    catch (...) {
        console << L"Failed to load configuration" << IO::NewLineChar;
        return 1;
    }
    string source_path = IO::ExpandPath(sync_reg->GetValueString(L"BaseRuntimePath"));
    string dest_path = IO::ExpandPath(rt_reg->GetValueString(L"RuntimePath"));
    string obj_path = dest_path + L"/" + rt_reg->GetValueString(L"ObjectPath");

    {
        console << L"Cleaning object cache:" << IO::NewLineChar;
        SafePointer< Array<string> > files = IO::Search::GetFiles(obj_path + L"/*.o;*.log");
        for (int i = 0; i < files->Length(); i++) {
            string name = obj_path + L"/" + files->ElementAt(i);
            console << L"Removing " << name << L"...";
            IO::RemoveFile(name);
            console << L"OK" << IO::NewLineChar;
        }
        console << IO::NewLineChar;
    }
    SafePointer< Array<string> > sfiles = IO::Search::GetFiles(source_path + L"/" + sync_reg->GetValueString(L"Syncronize"), true);
    {
        SafePointer<RegistryNode> exclude_node = sync_reg->OpenNode(L"Exclude");
        Array<string> exf(0x10);
        if (exclude_node) {
            auto & vals = exclude_node->GetValues();
            for (int i = 0; i < vals.Length(); i++) exf << exclude_node->GetValueString(vals[i]);
        }
        for (int i = sfiles->Length() - 1; i >= 0; i--) {
            bool excl = false;
            for (int j = 0; j < exf.Length(); j++) {
                if (Syntax::MatchFilePattern(sfiles->ElementAt(i), exf[j])) { excl = true; break; }
            }
            if (excl) sfiles->Remove(i);
        }
    }

    Array<string> diff(0x80);
    console << L"Syncronizing:" << IO::NewLineChar;
    for (int i = 0; i < sfiles->Length(); i++) {
        FileStream source(source_path + L"/" + sfiles->ElementAt(i), AccessRead, OpenExisting);
        string dest_name = dest_path + L"/" + sfiles->ElementAt(i);
        if (IO::FileExists(dest_name)) {
            console << L"Checking " << dest_name << L"...";
            try {
                FileStream dest(dest_name, AccessReadWrite, OpenExisting);
                Array<uint8> src_data;
                Array<uint8> dest_data;
                src_data.SetLength(source.Length());
                dest_data.SetLength(dest.Length());
                source.Read(src_data.GetBuffer(), src_data.Length());
                dest.Read(dest_data.GetBuffer(), dest_data.Length());
                if (src_data.Length() != dest_data.Length() || MemoryCompare(src_data.GetBuffer(), dest_data.GetBuffer(), dest_data.Length())) {
                    console << L"Coping...";
                    diff << dest_name;
                    dest.Seek(0, Begin);
                    dest.SetLength(0);
                    dest.Write(src_data.GetBuffer(), src_data.Length());
                }
            }
            catch (...) {
                console << L"Failed" << IO::NewLineChar;
                return 1;
            }
            console << L"OK" << IO::NewLineChar;
        } else {
            console << L"File " << dest_name << L" not found. Coping...";
            for (int j = 1; j < dest_name.Length(); j++) {
                if (dest_name[j] == L'/' || dest_name[j] == 0) {
                    string folder = dest_name.Fragment(0, j);
                    try { IO::CreateDirectory(folder); } catch(...) {}
                }
            }
            try {
                FileStream dest(dest_name, AccessWrite, CreateNew);
                source.CopyTo(&dest);
            }
            catch (...) {
                console << L"Failed" << IO::NewLineChar;
                return 1;
            }
            diff << dest_name;
            console << L"OK" << IO::NewLineChar;
        }
    }
    console << IO::NewLineChar;
    console << L"Finished!" << IO::NewLineChar << IO::NewLineChar;

    if (diff.Length()) {
        console << L"Changes:" << IO::NewLineChar;
        for (int i = 0; i < diff.Length(); i++) {
            console << diff[i] << IO::NewLineChar;
        }
        console << IO::NewLineChar;
    }

    return 0;
}