#include <EngineRuntime.h>
#include <Storage/TextRegistryGrammar.h>

using namespace Engine;
using namespace Engine::Streaming;
using namespace Engine::Storage;

enum class Operation { View, ToText, ToBinary };

int Main(void)
{
    handle console_output = IO::CloneHandle(IO::GetStandardOutput());
    FileStream console_stream(console_output);
    TextWriter console(&console_stream);

    SafePointer< Array<string> > args = GetCommandLine();

    console << ENGINE_VI_APPNAME << IO::NewLineChar;
    console << L"Copyright " << string(ENGINE_VI_COPYRIGHT).Replace(L'\xA9', L"(C)") << IO::NewLineChar;
    console << L"Version " << ENGINE_VI_APPVERSION << L", build " << ENGINE_VI_BUILD << IO::NewLineChar << IO::NewLineChar;
    
    if (args->Length() < 2) {
        console << L"Command line syntax:" << IO::NewLineChar;
        console << L"  " << ENGINE_VI_APPSYSNAME << L" <registry> [:text <text file> [:enc <encoding>] | :binary <binary file>]" << IO::NewLineChar;
        console << L"Where" << IO::NewLineChar;
        console << L"  registry    - source binary or text registry file," << IO::NewLineChar;
        console << L"  :text       - produce text registry notation," << IO::NewLineChar;
        console << L"  :binary     - produce binary registry file," << IO::NewLineChar;
        console << L"  text file   - output text registry file (.txt, .ini, .eini)," << IO::NewLineChar;
        console << L"  binary file - output binary registry file (.ecs, .ecsr)," << IO::NewLineChar;
        console << L"  :enc        - specify text file encoding," << IO::NewLineChar;
        console << L"  encoding    - the required encoding, one of the following:" << IO::NewLineChar;
        console << L"    ascii     - basic 1-byte text encoding," << IO::NewLineChar;
        console << L"    utf8      - Unicode 1-byte surrogate notation, the default one," << IO::NewLineChar;
        console << L"    utf16     - Unicode 2-byte surrogate notation," << IO::NewLineChar;
        console << L"    utf32     - pure Unicode 4-byte notation." << IO::NewLineChar;
        console << L"Started only with the source registry name, writes it on the standard output." << IO::NewLineChar;
        console << IO::NewLineChar;
    } else {
        string source = args->ElementAt(1);
        string output;
        Encoding encoding = Encoding::UTF8;
        Operation operation = Operation::View;
        for (int i = 2; i < args->Length(); i++) {
            if (string::CompareIgnoreCase(args->ElementAt(i), L":text") == 0 && i < args->Length() - 1) {
                output = args->ElementAt(i + 1);
                operation = Operation::ToText;
                i++;
            } else if (string::CompareIgnoreCase(args->ElementAt(i), L":binary") == 0 && i < args->Length() - 1) {
                output = args->ElementAt(i + 1);
                operation = Operation::ToBinary;
                i++;
            } else if (string::CompareIgnoreCase(args->ElementAt(i), L":enc") == 0 && i < args->Length() - 1) {
                if (string::CompareIgnoreCase(args->ElementAt(i + 1), L"ascii") == 0) {
                    encoding = Encoding::ANSI;
                } else if (string::CompareIgnoreCase(args->ElementAt(i + 1), L"utf8") == 0) {
                    encoding = Encoding::UTF8;
                } else if (string::CompareIgnoreCase(args->ElementAt(i + 1), L"utf16") == 0) {
                    encoding = Encoding::UTF16;
                } else if (string::CompareIgnoreCase(args->ElementAt(i + 1), L"utf32") == 0) {
                    encoding = Encoding::UTF32;
                } else {
                    console << L"Unknown text encoding \"" + args->ElementAt(i + 1) + L"\"." << IO::NewLineChar;
                    return 1;
                }
                i++;
            } else {
                console << L"Invalid command line syntax." << IO::NewLineChar;
                return 1;
            }
        }
        SafePointer<Registry> reg;
        SafePointer< Array<Syntax::Token> > tokens;
        string inner;
        try {
            FileStream file(source, AccessRead, OpenExisting);
            reg = LoadRegistry(&file);
            if (!reg) {
                file.Seek(0, Begin);
                reg = CompileTextRegistry(&file);
                if (!reg) {
                    file.Seek(0, Begin);
                    TextReader reader(&file);
                    inner = reader.ReadAll();
                    Syntax::Spelling spelling;
                    Syntax::Grammar grammar;
                    CreateTextRegistryGrammar(grammar);
                    CreateTextRegistrySpelling(spelling);
                    tokens = Syntax::ParseText(inner, spelling);
                    SafePointer< Syntax::SyntaxTree > tree = new Syntax::SyntaxTree(*tokens, grammar);
                    throw InvalidFormatException();
                }
            }
        }
        catch (Syntax::ParserSpellingException & e) {
            console << L"Syntax error: " << e.Comments << IO::NewLineChar;
            int line = 1;
            int lbegin = 0;
            for (int i = 0; i < e.Position; i++) if (inner[i] == L'\n') { line++; lbegin = i + 1; }
            int lend = e.Position;
            while (lend < inner.Length() && inner[lend] != L'\n') lend++;
            if (lend < inner.Length()) lend--;
            string errstr = inner.Fragment(lbegin, lend - lbegin).Replace(L'\t', L' ');
            console << errstr << IO::NewLineChar;
            int posrel = e.Position - lbegin;
            for (int i = 0; i < posrel; i++) console << L" ";
            console << L"^" << IO::NewLineChar;
            console << L"At line #" << line << IO::NewLineChar;
            return 1;
        }
        catch (Syntax::ParserSyntaxException & e) {
            console << L"Syntax error: " << e.Comments << IO::NewLineChar;
            int token_begin = tokens->ElementAt(e.Position).SourcePosition;
            int token_end = inner.Length();
            if (e.Position < tokens->Length() - 1) token_end = tokens->ElementAt(e.Position + 1).SourcePosition;
            int line = 1;
            int lbegin = 0;
            for (int i = 0; i < token_begin; i++) if (inner[i] == L'\n') { line++; lbegin = i + 1; }
            int lend = token_begin;
            while (lend < inner.Length() && inner[lend] != L'\n') lend++;
            if (lend < inner.Length()) lend--;
            string errstr = inner.Fragment(lbegin, lend - lbegin).Replace(L'\t', L' ');
            console << errstr << IO::NewLineChar;
            int posrel = token_begin - lbegin;
            for (int i = 0; i < posrel; i++) console << L" ";
            console << L"^";
            for (int i = 0; i < token_end - token_begin - 1; i++) console << L"~";
            console << IO::NewLineChar;
            console << L"At line #" << line << IO::NewLineChar;
            return 1;
        }
        catch (InvalidFormatException & e) {
            console << L"Invalid input file format." << IO::NewLineChar;
            return 1;
        }
        catch (IO::FileAccessException & e) {
            console << L"Error accessing the input file." << IO::NewLineChar;
            return 1;
        }
        catch (...) {
            console << L"Internal application error." << IO::NewLineChar;
            return 1;
        }
        try {
            if (operation == Operation::View) {
                RegistryToText(reg, &console);
            } else if (operation == Operation::ToText) {
                console << L"Writing text registry...";
                FileStream file(output, AccessReadWrite, CreateAlways);
                TextWriter writer(&file, encoding);
                writer.WriteEncodingSignature();
                RegistryToText(reg, &writer);
                console << L"Succeed" << IO::NewLineChar;
            } else if (operation == Operation::ToBinary) {
                console << L"Writing binary registry...";
                FileStream file(output, AccessReadWrite, CreateAlways);
                reg->Save(&file);
                console << L"Succeed" << IO::NewLineChar;
            }
        }
        catch (IO::FileAccessException & e) {
            console << L"Error accessing the output file." << IO::NewLineChar;
            return 1;
        }
        catch (...) {
            console << L"Internal application error." << IO::NewLineChar;
            return 1;
        }
    }
    return 0;
}