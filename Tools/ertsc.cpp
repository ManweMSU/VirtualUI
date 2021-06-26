#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;

#define BYTES_PER_ROW	32

int Main(void)
{
	IO::Console console;
	SafePointer< Array<string> > args = GetCommandLine();
	console << ENGINE_VI_APPNAME << IO::LineFeedSequence;
	console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::LineFeedSequence;
	console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::LineFeedSequence << IO::LineFeedSequence;
	if (args->Length() > 1) {
		string source_file = IO::ExpandPath(args->ElementAt(1));
		string air_file = IO::Path::GetDirectory(source_file) + L"/" + IO::Path::GetFileNameWithoutExtension(source_file) + L".air";
		string lib_file = IO::Path::GetDirectory(source_file) + L"/" + IO::Path::GetFileNameWithoutExtension(source_file) + L".metallib";
		string cxx_file = IO::Path::GetDirectory(source_file) + L"/" + IO::Path::GetFileNameWithoutExtension(source_file) + L".cpp";
		Array<string> cmds(8);
		cmds << L"-sdk"; cmds << L"macosx"; cmds << L"metal"; cmds << L"-c";
		cmds << source_file; cmds << L"-o"; cmds << air_file;
		cmds << L"-std=macos-metal2.4";
		cmds << L"--target=air64-apple-macos11.0";
		try {
			SafePointer<Process> process = CreateCommandProcess(L"xcrun", &cmds);
			if (!process) throw Exception();
			process->Wait();
			if (process->GetExitCode()) throw Exception();
		} catch (...) {
			console.SetTextColor(IO::ConsoleColor::Red);
			console.WriteLine(L"Failed to execute the compiler.");
			console.SetTextColor(IO::ConsoleColor::Default);
			return 1;
		}
		cmds.Clear();
		cmds << L"-sdk"; cmds << L"macosx"; cmds << L"metallib";
		cmds << air_file; cmds << L"-o"; cmds << lib_file;
		try {
			SafePointer<Process> process = CreateCommandProcess(L"xcrun", &cmds);
			if (!process) throw Exception();
			process->Wait();
			if (process->GetExitCode()) throw Exception();
		} catch (...) {
			console.SetTextColor(IO::ConsoleColor::Red);
			console.WriteLine(L"Failed to execute the library tool.");
			console.SetTextColor(IO::ConsoleColor::Default);
			return 1;
		}
		try {
			FileStream input(lib_file, AccessRead, OpenExisting);
			FileStream output(cxx_file, AccessReadWrite, CreateAlways);
			TextWriter writer(&output, Encoding::UTF8);
			uint64 length = input.Length();
			SafePointer< Array<uint8> > data = input.ReadAll();
			writer.WriteEncodingSignature();
			writer.WriteLine(L"#include \"MetalDeviceShaders.h\"");
			writer.WriteLine(L"");
			writer.WriteLine(L"namespace Engine");
			writer.WriteLine(L"{");
			writer.WriteLine(L"\tnamespace Cocoa");
			writer.WriteLine(L"\t{");
			writer.WriteLine(L"\t\tconst unsigned char MetalShaderData[] = {");
			for (int i = 0; i < data->Length(); i++) {
				uint32 byte = data->ElementAt(i);
				if (i % BYTES_PER_ROW == 0) writer.Write(L"\t\t\t0x"); else writer.Write(L", 0x");
				writer.Write(string(byte, HexadecimalBase, 2));
				if (i == data->Length() - 1) writer.WriteLine(L"");
				else if (i % BYTES_PER_ROW == BYTES_PER_ROW - 1 || i == data->Length() - 1) writer.WriteLine(L",");
			}
			writer.WriteLine(L"\t\t};");
			writer.WriteLine(L"\t\tconst int MetalShaderSize = " + string(length) + L";");
			writer.WriteLine(L"");
			writer.WriteLine(L"\t\tvoid GetMetalDeviceShaders(const void ** data, int * size) { *data = MetalShaderData; *size = MetalShaderSize; }");
			writer.WriteLine(L"\t}");
			writer.WriteLine(L"}");
		} catch (...) {
			console.SetTextColor(IO::ConsoleColor::Red);
			console.WriteLine(L"Failed to convert library into constant.");
			console.SetTextColor(IO::ConsoleColor::Default);
			return 1;
		}
		console.Write(L"Done successfully! Cleaning intermediate files...");
		try {
			IO::RemoveFile(air_file);
			IO::RemoveFile(lib_file);
		} catch (...) {
			console.SetTextColor(IO::ConsoleColor::Red);
			console.WriteLine(L"Failed.");
			console.SetTextColor(IO::ConsoleColor::Default);
			return 1;
		}
		console.WriteLine(L"Done!");
	} else {
		console << L"Command line syntax:" << IO::LineFeedSequence;
		console << L"  " << ENGINE_VI_APPSYSNAME << L" <shader file.metal>" << IO::LineFeedSequence;
		console << IO::LineFeedSequence;
	}
	return 0;
}