#include "Console.h"

#include "unistd.h"

namespace Engine
{
    namespace IO
    {
        Console::Console(void) : writer(new Streaming::FileStream(GetStandartOutput())), file(GetStandartOutput()) {}
        Console::Console(handle output) : writer(new Streaming::FileStream(output)), file(output) {}
        Console::~Console(void) {}
        void Console::Write(const string & text) const { writer.Write(text); }
        void Console::WriteLine(const string & text) const { writer.WriteLine(text); }
        void Console::WriteEncodingSignature(void) const { writer.WriteEncodingSignature(); }
        string Console::ToString(void) const { return L"Console"; }

        Streaming::TextWriter & Console::GetWriter(void) { return writer; }
        void Console::SetTextColor(int color)
        {
            if (isatty(reinterpret_cast<intptr>(file))) {
                string val;
                if (color == -1) val = L"\e[39m"; // revert
                else if (color == 0) val = L"\e[38;5;0m"; // black
                else if (color == 1) val = L"\e[38;5;4m"; // dark blue
                else if (color == 2) val = L"\e[38;5;2m"; // dark green
                else if (color == 3) val = L"\e[38;5;6m"; // dark cyan
                else if (color == 4) val = L"\e[38;5;1m"; // dark red
                else if (color == 5) val = L"\e[38;5;5m"; // dark magenta
                else if (color == 6) val = L"\e[38;5;3m"; // dark yellow
                else if (color == 7) val = L"\e[38;5;7m"; // grey
                else if (color == 8) val = L"\e[38;5;8m"; // dark grey
                else if (color == 9) val = L"\e[38;5;12m"; // blue
                else if (color == 10) val = L"\e[38;5;10m"; // green
                else if (color == 11) val = L"\e[38;5;14m"; // cyan
                else if (color == 12) val = L"\e[38;5;9m"; // red
                else if (color == 13) val = L"\e[38;5;13m"; // magenta
                else if (color == 14) val = L"\e[38;5;11m"; // yellow
                else if (color == 15) val = L"\e[38;5;15m"; // white
                Write(val);
            }
        }
        void Console::SetBackgroundColor(int color)
        {
            if (isatty(reinterpret_cast<intptr>(file))) {
                string val;
                if (color == -1) val = L"\e[49m"; // revert
                else if (color == 0) val = L"\e[48;5;0m"; // black
                else if (color == 1) val = L"\e[48;5;4m"; // dark blue
                else if (color == 2) val = L"\e[48;5;2m"; // dark green
                else if (color == 3) val = L"\e[48;5;6m"; // dark cyan
                else if (color == 4) val = L"\e[48;5;1m"; // dark red
                else if (color == 5) val = L"\e[48;5;5m"; // dark magenta
                else if (color == 6) val = L"\e[48;5;3m"; // dark yellow
                else if (color == 7) val = L"\e[48;5;7m"; // gray
                else if (color == 8) val = L"\e[48;5;8m"; // dark gray
                else if (color == 9) val = L"\e[48;5;12m"; // blue
                else if (color == 10) val = L"\e[48;5;10m"; // green
                else if (color == 11) val = L"\e[48;5;14m"; // cyan
                else if (color == 12) val = L"\e[48;5;9m"; // red
                else if (color == 13) val = L"\e[48;5;13m"; // magenta
                else if (color == 14) val = L"\e[48;5;11m"; // yellow
                else if (color == 15) val = L"\e[48;5;15m"; // white
                Write(val);
            }
        }
        void Console::ClearScreen(void)
        {
            if (isatty(reinterpret_cast<intptr>(file))) {
                Write(L"\e[2J\e[1;1H");
            }
        }
        void Console::ClearLine(void)
        {
            if (isatty(reinterpret_cast<intptr>(file))) {
                Write(L"\e[2K\e[1G");
            }
        }
        void Console::MoveCaret(int x, int y)
        {
            if (isatty(reinterpret_cast<intptr>(file))) {
                Write(L"\e[" + string(y + 1) + L";" + string(x + 1) + L"H");
            }
        }

        Console & Console::operator << (const string & text) { writer << text; return *this; }
        const Console & Console::operator << (const string & text) const { writer << text; return *this; }
    }
}