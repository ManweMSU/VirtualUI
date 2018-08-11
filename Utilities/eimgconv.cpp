#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Codec;

bool Save(Image * image, const string & as, const string & codec, TextWriter & console)
{
    try {
        console << L"Writing \"" << as << L"\"...";
        SafePointer<Stream> stream;
        try {
            stream = new FileStream(as, AccessReadWrite, CreateAlways);
        }
        catch (...) {
            console << L"Failed." << IO::NewLineChar;
            console << L"Can not write the file." << IO::NewLineChar;
            throw;
        }
        try {
            EncodeImage(stream, image, codec);
        }
        catch (...) {
            console << L"Failed." << IO::NewLineChar;
            console << L"Can not encode the image." << IO::NewLineChar;
            throw;
        }
        console << L"Succeed." << IO::NewLineChar;
    } catch (...) { return false; }
    return true;
}
int Main(void)
{
    FileStream console_stream(IO::GetStandartOutput());
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <input> <format> [:decompose] <output>" << IO::NewLineChar;
        console << L"Where" << IO::NewLineChar;
        console << L"  input      - file (or files written by file filter) to convert," << IO::NewLineChar;
        console << L"  format     - format (codec) to use to save images," << IO::NewLineChar;
        console << L"  :decompose - save images in separate files instead of multi-frame image file," << IO::NewLineChar;
        console << L"  output     - output file name or directory if :decompose specified." << IO::NewLineChar;
        console << L"Available image formats:" << IO::NewLineChar;
        console << L"  :bmp       - Windows Bitmap," << IO::NewLineChar;
        console << L"  :png       - Portable Network Graphics," << IO::NewLineChar;
        console << L"  :jpg       - Joint Photographic Image Format," << IO::NewLineChar;
        console << L"  :gif       - Graphics Interchange Format," << IO::NewLineChar;
        console << L"  :tif       - Tagged Image File Format," << IO::NewLineChar;
        console << L"  :ico       - Windows Icon," << IO::NewLineChar;
        console << L"  :cur       - Windows Cursor," << IO::NewLineChar;
        console << L"  :icns      - Apple Icon," << IO::NewLineChar;
        console << L"  :eiwv      - Engine Image Volume." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        string source;
        string output;
        string codec;
        bool decompose = false;
        for (int i = 1; i < args->Length(); i++) {
            if (args->ElementAt(i)[0] == L':') {
                if (string::CompareIgnoreCase(args->ElementAt(i), L":decompose") == 0) {
                    decompose = true;
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":bmp") == 0) {
                    codec = L"BMP";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":png") == 0) {
                    codec = L"PNG";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":jpg") == 0) {
                    codec = L"JPG";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":gif") == 0) {
                    codec = L"GIF";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":tif") == 0) {
                    codec = L"TIF";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":ico") == 0) {
                    codec = L"ICO";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":cur") == 0) {
                    codec = L"CUR";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":icns") == 0) {
                    codec = L"ICNS";
                } else if (string::CompareIgnoreCase(args->ElementAt(i), L":eiwv") == 0) {
                    codec = L"EIWV";
                } else {
                    console << L"Invalid command line argument." << IO::NewLineChar;
                    return 1;
                }
            } else {
                if (!source.Length()) {
                    source = args->ElementAt(i);
                } else if (!output.Length()) {
                    output = args->ElementAt(i);
                } else {
                    console << L"Invalid command line argument." << IO::NewLineChar;
                    return 1;
                }
            }
        }
        if (!source.Length()) {
            console << L"No input in command line." << IO::NewLineChar;
            return 1;
        }
        if (!codec.Length()) {
            console << L"No format in command line." << IO::NewLineChar;
            return 1;
        }
        if (!output.Length()) {
            console << L"No output in command line." << IO::NewLineChar;
            return 1;
        }
        UI::Windows::InitializeCodecCollection();
        try {
            SafePointer<Image> image = new Image;
            SafePointer< Array<string> > files = IO::Search::GetFiles(source);
            if (!files->Length()) {
                console << L"No files found." << IO::NewLineChar;
                return 1;
            }
            for (int i = 0; i < files->Length(); i++) {
                string name = IO::Path::GetDirectory(source) + L"/" + files->ElementAt(i);
                console << L"Importing \"" + name + L"\"...";
                SafePointer<Stream> input;
                SafePointer<Image> decoded;
                try {
                    input = new FileStream(name, AccessRead, OpenExisting);
                }
                catch (...) {
                    console << L"Failed." << IO::NewLineChar;
                    console << L"Can not access the file." << IO::NewLineChar;
                    return 1;
                }
                try {
                    decoded = DecodeImage(input);
                }
                catch (...) {
                    console << L"Failed." << IO::NewLineChar;
                    console << L"Can not decode the file." << IO::NewLineChar;
                    return 1;
                }
                for (int j = 0; j < decoded->Frames.Length(); j++) image->Frames.Append(decoded->Frames.ElementAt(j));
                console << L"Succeed." << IO::NewLineChar;
            }
            if (decompose) {
                try { IO::CreateDirectory(output); } catch (...) {}
                for (int i = 0; i < image->Frames.Length(); i++) {
                    SafePointer<Image> fake = new Image;
                    fake->Frames.Append(image->Frames.ElementAt(i));
                    if (!Save(fake, output + L"/frame_" + string(uint32(i), L"0123456789", 3) + L"." + codec.LowerCase(), codec, console)) return 1;
                }
            } else {
                if (!Save(image, output, codec, console)) return 1;
            }
        }
        catch (...) {
            console << L"Internal error." << IO::NewLineChar;
            return 1;
        }
    }
    return 0;
}