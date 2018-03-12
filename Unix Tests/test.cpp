#include "../VirtualUI/EngineBase.h"
#include "../VirtualUI/Streaming.h"
#include "../VirtualUI/PlatformDependent/FileApi.h"

using namespace Engine;

#include <stdio.h>
#include <unistd.h>

int main(int argc, char ** argv)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    SafePointer<Streaming::FileStream> Input = new Streaming::FileStream(L"in.txt", Streaming::AccessRead, Streaming::OpenExisting);
    SafePointer<Streaming::FileStream> LogOutput = new Streaming::FileStream(L"пидор 🌹.txt", Streaming::AccessWrite, Streaming::CreateAlways);
    Streaming::TextWriter Console(ConsoleOutStream);
    Streaming::TextWriter Log(LogOutput);

    Console << int(sizeof(off_t)) << IO::NewLineChar;
    Console << IO::ExpandPath(L"MacOSX\\FileApi.cpp") << IO::NewLineChar;

    Array<uint8> chars;
    chars.SetLength(Input->Length());
    Input->Read(chars.GetBuffer(), Input->Length());
    string str = string(chars.GetBuffer(), chars.Length(), Encoding::UTF8);

    IO::SetCurrentDirectory(L"MacOSX");
    Console << str << IO::NewLineChar << L"корневген пидор" << IO::NewLineChar << IO::GetCurrentDirectory() << IO::NewLineChar;

    Log.WriteEncodingSignature();
    Log << str << IO::NewLineChar << L"Пидор 🌹" << IO::NewLineChar << IO::GetCurrentDirectory();

    return 0;
}