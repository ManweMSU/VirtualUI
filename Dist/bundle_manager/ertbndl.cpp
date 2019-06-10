#include <EngineRuntime.h>

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
void inst(Archive * arc, const string & at, const string & sys)
{
    IO::Console console;
    IO::SetCurrentDirectory(at);
    for (int i = 1; i <= arc->GetFileCount(); i++) {
        string sys_attr = arc->GetFileAttribute(i, L"System");
        string name = arc->GetFileName(i);
        if (Syntax::MatchFilePattern(sys, sys_attr) && name.Length()) {
            console << L"Installing \"" << name << L"\"...";
            try_create_file_directory(name);
            if (string::CompareIgnoreCase(arc->GetFileAttribute(i, L"Folder"), L"yes") == 0) {
                try_create_directory(name);
            } else {
                SafePointer<Stream> source = arc->QueryFileStream(i);
                SafePointer<Stream> dest = new FileStream(name, AccessReadWrite, CreateAlways);
                source->CopyTo(dest);
            }
            console << L"Succeed." << IO::NewLineChar;
        }
    }
}

int Main(void)
{
    IO::Console console;

    SafePointer< Array<string> > args = GetCommandLine();

    bool where = false;
    bool version = false;
    bool nologo = false;
    bool install = false;
    string package;
    for (int i = 1; i < args->Length(); i++) {
        if (string::CompareIgnoreCase(args->ElementAt(i), L":where") == 0) where = true;
        else if (string::CompareIgnoreCase(args->ElementAt(i), L":version") == 0) version = true;
        else if (string::CompareIgnoreCase(args->ElementAt(i), L":nologo") == 0) nologo = true;
        else if (string::CompareIgnoreCase(args->ElementAt(i), L":install") == 0 && i < args->Length() - 1) {
            install = true;
            package = args->ElementAt(i + 1);
            i++;
        } else return 1;
    }

    if (!nologo) {
        console << ENGINE_VI_APPNAME << IO::NewLineChar;
        console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
        console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    }
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" [:where] [:version] [:install <package>] [:nologo]" << IO::NewLineChar;
        console << L"Where" << IO::NewLineChar;
        console << L"  :where   - prints the full path of the runtime toolset," << IO::NewLineChar;
        console << L"  :version - prints current runtime version and release date," << IO::NewLineChar;
        console << L"  :install - installs a package of extra tools," << IO::NewLineChar;
        console << L"  package  - an .ecsa archive with the extra tools to install," << IO::NewLineChar;
        console << L"  :nologo  - don't print the bunner." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        try {
            string toolset = IO::Path::GetDirectory(IO::GetExecutablePath());
            if (where) {
                console << toolset << IO::NewLineChar;
            }
            if (version) {
                try {
                    SafePointer<Stream> info_stream = new FileStream(toolset + L"/ertbndl.ecs", AccessRead, OpenExisting);
                    SafePointer<Registry> info = LoadRegistry(info_stream);
                    string name = info->GetValueString(L"Name");
                    Time time_stamp = info->GetValueTime(L"Stamp");
                    uint ver_major = info->GetValueInteger(L"VersionMajor");
                    uint ver_minor = info->GetValueInteger(L"VersionMinor");
                    console << name << IO::NewLineChar;
                    console << ver_major << L"." << ver_minor << IO::NewLineChar;
                    console << time_stamp.ToLocal().ToString() << IO::NewLineChar;
                } catch (...) {
                    console << L"Failed to load the bundle configuration." << IO::NewLineChar;
                    return 1;
                }
            }
            if (install) {
                try {
                    SafePointer<Stream> arc_stream = new FileStream(package, AccessRead, OpenExisting);
                    SafePointer<Archive> arc = OpenArchive(arc_stream);
                    if (!arc) throw Exception();
#ifdef ENGINE_WINDOWS
                    string system = L"Windows";
#endif
#ifdef ENGINE_MACOSX
                    string system = L"MacOSX";
#endif
                    inst(arc, toolset, system);
                    console << L"Package installed." << IO::NewLineChar;
                } catch (...) {
                    console << L"Failed to install the package." << IO::NewLineChar;
                    return 1;
                }
            }
        }
        catch (...) {
            console << L"Some shit occured..." << IO::NewLineChar;
            return 1;
        }
    }
    return 0;
}