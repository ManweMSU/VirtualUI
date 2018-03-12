#include "../VirtualUI/EngineBase.h"
#include "../VirtualUI/Streaming.h"
#include "../VirtualUI/PlatformDependent/FileApi.h"

using namespace Engine;

#include <stdio.h>

int main(int argc, char ** argv)
{
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    SafePointer<Streaming::FileStream> Input = new Streaming::FileStream(L"in.txt", Streaming::AccessRead, Streaming::OpenExisting);
    SafePointer<Streaming::FileStream> LogOutput = new Streaming::FileStream(L"пидор 🌹.txt", Streaming::AccessWrite, Streaming::CreateAlways);
    Streaming::TextWriter Console(ConsoleOutStream);
    Streaming::TextWriter Log(LogOutput);

    Array<uint8> chars;
    chars.SetLength(Input->Length());
    Input->Read(chars.GetBuffer(), Input->Length());
    string str = string(chars.GetBuffer(), chars.Length(), Encoding::UTF8);

    Console << str << IO::NewLineChar << L"корневген пидор" << IO::NewLineChar;

    Log.WriteEncodingSignature();
    Log << str << IO::NewLineChar << L"Пидор 🌹";

    return 0;
}