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
void configure(const string & rt_path, const string & tools)
{
    {
        SafePointer<Registry> ertbuild = CreateRegistry();
        ertbuild->CreateNode(L"MacOSX-X64");
        SafePointer<RegistryNode> x64_node = ertbuild->OpenNode(L"MacOSX-X64");
        x64_node->CreateValue(L"RuntimePath", RegistryValueType::String);
        x64_node->SetValue(L"RuntimePath", rt_path);
        x64_node->CreateValue(L"UseWhitelist", RegistryValueType::Boolean);
        x64_node->SetValue(L"UseWhitelist", false);
        x64_node->CreateValue(L"ResourceTool", RegistryValueType::String);
        x64_node->SetValue(L"ResourceTool", string(L"ertres"));
        x64_node->CreateValue(L"Bootstrapper", RegistryValueType::String);
        x64_node->SetValue(L"Bootstrapper", string(L"bootstrapper.cpp"));
        x64_node->CreateValue(L"CompileFilter", RegistryValueType::String);
        x64_node->SetValue(L"CompileFilter", string(L"*.c;*.cpp;*.cxx;*.m;*.mm"));
        x64_node->CreateValue(L"ObjectExtension", RegistryValueType::String);
        x64_node->SetValue(L"ObjectExtension", string(L"o"));
        x64_node->CreateValue(L"ObjectPath", RegistryValueType::String);
        x64_node->SetValue(L"ObjectPath", string(L"_build"));
        x64_node->CreateValue(L"ExecutableExtension", RegistryValueType::String);
        x64_node->SetValue(L"ExecutableExtension", string(L""));
        x64_node->CreateValue(L"LibraryExtension", RegistryValueType::String);
        x64_node->SetValue(L"LibraryExtension", string(L"dylib"));
        x64_node->CreateNode(L"Compiler");
        x64_node->CreateNode(L"Linker");
        SafePointer<RegistryNode> compiler = x64_node->OpenNode(L"Compiler");
        SafePointer<RegistryNode> linker = x64_node->OpenNode(L"Linker");
        compiler->CreateValue(L"Path", RegistryValueType::String);
        compiler->SetValue(L"Path", string(L"clang++"));
        compiler->CreateValue(L"IncludeArgument", RegistryValueType::String);
        compiler->SetValue(L"IncludeArgument", string(L"-I"));
        compiler->CreateValue(L"OutputArgument", RegistryValueType::String);
        compiler->SetValue(L"OutputArgument", string(L"-o"));
        compiler->CreateValue(L"DefineArgument", RegistryValueType::String);
        compiler->SetValue(L"DefineArgument", string(L"-D"));
        compiler->CreateNode(L"Arguments");
        compiler->CreateValue(L"Arguments/01", RegistryValueType::String);
        compiler->SetValue(L"Arguments/01", string(L"-c"));
        compiler->CreateValue(L"Arguments/02", RegistryValueType::String);
        compiler->SetValue(L"Arguments/02", string(L"-O3"));
        compiler->CreateValue(L"Arguments/03", RegistryValueType::String);
        compiler->SetValue(L"Arguments/03", string(L"-std=c++14"));
        compiler->CreateValue(L"Arguments/04", RegistryValueType::String);
        compiler->SetValue(L"Arguments/04", string(L"-v"));
        compiler->CreateValue(L"Arguments/05", RegistryValueType::String);
        compiler->SetValue(L"Arguments/05", string(L"-fvisibility=hidden"));
        compiler->CreateValue(L"Arguments/06", RegistryValueType::String);
        compiler->SetValue(L"Arguments/06", string(L"-fmodules"));
        compiler->CreateValue(L"Arguments/07", RegistryValueType::String);
        compiler->SetValue(L"Arguments/07", string(L"-fcxx-modules"));
        linker->CreateValue(L"Path", RegistryValueType::String);
        linker->SetValue(L"Path", string(L"clang++"));
        linker->CreateValue(L"OutputArgument", RegistryValueType::String);
        linker->SetValue(L"OutputArgument", string(L"-o"));
        linker->CreateNode(L"Arguments");
        linker->CreateValue(L"Arguments/01", RegistryValueType::String);
        linker->SetValue(L"Arguments/01", string(L"-O3"));
        linker->CreateValue(L"Arguments/02", RegistryValueType::String);
        linker->SetValue(L"Arguments/02", string(L"-v"));
        linker->CreateValue(L"Arguments/03", RegistryValueType::String);
        linker->SetValue(L"Arguments/03", string(L"-s"));
        linker->CreateValue(L"Arguments/04", RegistryValueType::String);
        linker->SetValue(L"Arguments/04", string(L"-fvisibility=hidden"));
        linker->CreateNode(L"ArgumentsLibrary");
        linker->CreateValue(L"ArgumentsLibrary/01", RegistryValueType::String);
        linker->SetValue(L"ArgumentsLibrary/01", string(L"-dynamiclib"));
        SafePointer<Stream> bld_stream = new FileStream(tools + L"/ertbuild.ini", AccessReadWrite, CreateAlways);
        RegistryToText(ertbuild, bld_stream, Encoding::UTF8);
    }
    {
        SafePointer<Registry> ertres = CreateRegistry();
        ertres->CreateNode(L"MacOSX");
        SafePointer<RegistryNode> mac_node = ertres->OpenNode(L"MacOSX");
        mac_node->CreateValue(L"IconCodec", RegistryValueType::String);
        mac_node->SetValue(L"IconCodec", string(L"ICNS"));
        mac_node->CreateValue(L"IconExtension", RegistryValueType::String);
        mac_node->SetValue(L"IconExtension", string(L"icns"));
        mac_node->CreateNode(L"IconFormats");
        mac_node->CreateValue(L"IconFormats/01", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/01", 16);
        mac_node->CreateValue(L"IconFormats/02", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/02", 32);
        mac_node->CreateValue(L"IconFormats/03", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/03", 64);
        mac_node->CreateValue(L"IconFormats/04", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/04", 128);
        mac_node->CreateValue(L"IconFormats/05", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/05", 256);
        mac_node->CreateValue(L"IconFormats/06", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/06", 512);
        mac_node->CreateValue(L"IconFormats/07", RegistryValueType::Integer);
        mac_node->SetValue(L"IconFormats/07", 1024);
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
        string path = args->ElementAt(2);
        IO::SetCurrentDirectory(L"/etc/paths.d");
        SafePointer<Stream> file = new FileStream(L"engine_runtime", AccessReadWrite, CreateAlways);
        TextWriter writer(file, Encoding::UTF8);
        writer.WriteLine(path);
        return 0;
    }
    IO::Console Console;
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
        {
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
                Console << L"The build tool failed to build the Runtime." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            Console << L"The Runtime object cache has been built successfully." << IO::NewLineChar;
        }
        {
            SafePointer<Codec::Image> test_icon = new Codec::Image;
            SafePointer<Codec::Frame> frame;
            frame = prod_frame(16);
            test_icon->Frames.Append(frame);
            frame = prod_frame(32);
            test_icon->Frames.Append(frame);
            frame = prod_frame(64);
            test_icon->Frames.Append(frame);
            frame = prod_frame(128);
            test_icon->Frames.Append(frame);
            frame = prod_frame(256);
            test_icon->Frames.Append(frame);
            frame = prod_frame(512);
            test_icon->Frames.Append(frame);
            frame = prod_frame(1024);
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
                Console << L"The build tool failed to build the Test." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            SafePointer<Process> test = CreateProcess(path + L"/Test/_build/Test Application.app/Contents/MacOS/testapp");
            if (!test) {
                Console.SetTextColor(4);
                Console << L"Failed to launch the test." << IO::NewLineChar;
                Console.SetTextColor(-1);
                return 1;
            }
            test->Wait();
        }
        Console << L"The Runtime has been successfully installed and tested." << IO::NewLineChar;
        Console << L"Do you want to install the Runtime tools into the PATH environment variable? (yes/no)" << IO::NewLineChar;
		Console << L"In case of 'YES' please restart your terminal session after the installation's end." << IO::NewLineChar;
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