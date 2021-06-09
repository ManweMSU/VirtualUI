#include "../Interfaces/Console.h"
#include "../Interfaces/KeyCodes.h"
#include "../Miscellaneous/DynamicString.h"

#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <err.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <signal.h>

using namespace Engine::Streaming;

namespace Engine
{
	namespace IO
	{
		volatile bool _console_size_changed = false;
		class ConsoleObject : public Object
		{
		public:
			SafePointer< Array<uint32> > unprocessed;
            SafePointer< Array<uint8> > iostat;
            SafePointer<TextWriter> writer;
			SafePointer<TextReader> reader;
			uint32 buffered_code;
			int surrogate_pending;
            int file_out;
			int file_in;
            bool alt_buffer;

			static void on_size_changed(int) { _console_size_changed = true; }

			void _init_console(void)
			{
				alt_buffer = false;
				writer = new TextWriter(SafePointer<Stream>(new FileStream(reinterpret_cast<handle>(file_out))), TextFileEncoding);
				reader = new TextReader(SafePointer<Stream>(new FileStream(reinterpret_cast<handle>(file_in))), TextFileEncoding);
				unprocessed = new Array<uint32>(0x100);
				buffered_code = 0;
				surrogate_pending = 0;
				signal(SIGWINCH, on_size_changed);
				siginterrupt(SIGWINCH, 1);
			}
			uint32 _read_raw(void)
			{
				if (unprocessed->Length()) {
					auto result = unprocessed->FirstElement();
					unprocessed->RemoveFirst();
					return result;
				} else {
					while (true) {
						int code = 0;
						int status = read(file_in, &code, 1);
						if (status == -1) {
							if (errno == EINTR) return 0xFFFFFFFE;
							else throw Exception();
						} else if (status == 0) return 0xFFFFFFFF;
						if (surrogate_pending) {
							buffered_code <<= 6;
							buffered_code |= code & 0x3F;
							surrogate_pending--;
							if (!surrogate_pending) return buffered_code;
						} else {
							if (code & 0x80) {
								code &= 0x7F;
								if (code & 0x20) {
									code &= 0x1F;
									if (code & 0x10) {
										buffered_code = code & 0x07;
										surrogate_pending = 3;
									} else {
										buffered_code = code & 0x0F;
										surrogate_pending = 2;
									}
								} else {
									buffered_code = code & 0x1F;
									surrogate_pending = 1;
								}
							} else return code;
						}
					}
				}
			}
			uint32 _read_raw_nointr(void)
			{
				while (true) {
					auto result = _read_raw();
					if (result != 0xFFFFFFFE) return result;
				}
			}
			ConsoleObject(void) : file_out(reinterpret_cast<intptr>(GetStandardOutput())), file_in(reinterpret_cast<intptr>(GetStandardInput())) { _init_console(); }
			ConsoleObject(handle output) : file_out(reinterpret_cast<intptr>(output)), file_in(reinterpret_cast<intptr>(GetStandardInput())) { _init_console(); }
			ConsoleObject(handle output, handle input) : file_out(reinterpret_cast<intptr>(output)), file_in(reinterpret_cast<intptr>(input)) { _init_console(); }
			virtual ~ConsoleObject(void) override {}
		};
		Console::Console(void) : internal(new ConsoleObject) {}
		Console::Console(handle output) : internal(new ConsoleObject(output)) {}
		Console::Console(handle output, handle input) : internal(new ConsoleObject(output, input)) {}
		Console::~Console(void) {}
		void Console::Write(const string & text) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (object.iostat) {
				SafePointer< Array<uint8> > utf8 = text.EncodeSequence(Encoding::UTF8, false);
				for (auto & c : *utf8) {
					if (c == L'\n') write(object.file_out, "\n\r", 2);
					else if (c != L'\r') write(object.file_out, &c, 1);
				}
			} else object.writer->Write(text);
		}
		void Console::WriteLine(const string & text) const { Write(text + L"\n"); }
		void Console::WriteEncodingSignature(void) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (!isatty(object.file_out)) object.writer->WriteEncodingSignature();
		}
		uint32 Console::ReadChar(void) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_in)) {
				while (true) {
					ConsoleEventDesc event;
					ReadEvent(event);
					if (event.Event == ConsoleEvent::CharacterInput) return event.CharacterCode;
					else if (event.Event == ConsoleEvent::EndOfFile) return 0xFFFFFFFF;
				}
			} else {
				auto result = object.reader->ReadChar();
				if (result == 0xFFFFFFFF) eof = true;
				return result;
			}
		}
		void Console::ReadEvent(ConsoleEventDesc & event) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_in)) {
				while (true) {
					if (_console_size_changed) {
						_console_size_changed = false;
						int w, h;
						GetScreenBufferDimensions(w, h);
						event.Event = ConsoleEvent::ConsoleResized;
						event.Width = w;
						event.Height = h;
						return;
					}
					auto chr = object._read_raw();
					if (chr != 0xFFFFFFFE) {
						if (chr == 0xFFFFFFFF) {
							eof = true;
							event.Event = ConsoleEvent::EndOfFile;
							return;
						} else if (chr == 0) {
							event.Event = ConsoleEvent::KeyInput;
							event.KeyCode = KeyCodes::Space;
							event.KeyFlags = ConsoleKeyFlagControl;
							return;
						} else if (chr == 9 || chr == 10 || chr == 13) {
							event.Event = ConsoleEvent::CharacterInput;
							event.CharacterCode = chr;
							return;
						} else if (chr < 27) {
							event.Event = ConsoleEvent::KeyInput;
							event.KeyCode = 'A' - 1 + chr;
							event.KeyFlags = ConsoleKeyFlagControl;
							return;
						} else if (chr == 27) {
							pollfd fd;
							fd.fd = object.file_in;
							fd.events = POLLIN;
							int status = poll(&fd, 1, 0);
							if (status == 0) {
								event.Event = ConsoleEvent::KeyInput;
								event.KeyCode = KeyCodes::Escape;
								event.KeyFlags = 0;
								return;
							} else {
								chr = object._read_raw_nointr();
								if (chr == L'O' || chr == L'[') {
									auto ini = chr;
									DynamicString strd;
									while (true) {
										chr = object._read_raw_nointr();
										if (chr >= L'A' && chr <= L'Z') { strd << chr; break; }
										else if (chr == L'~') { break; }
										else if (chr == 0xFFFFFFFF) { eof = true; break; }
										else if (chr > 32) { strd << chr; }
									}
									auto str = strd.ToString();
									event.Event = ConsoleEvent::KeyInput;
									event.KeyCode = 0;
									event.KeyFlags = 0;
									if (ini == L'O') {
										if (str == L"A") event.KeyCode = KeyCodes::Up;
										else if (str == L"B") event.KeyCode = KeyCodes::Down;
										else if (str == L"C") event.KeyCode = KeyCodes::Right;
										else if (str == L"D") event.KeyCode = KeyCodes::Left;
										else if (str == L"H") event.KeyCode = KeyCodes::Home;
										else if (str == L"F") event.KeyCode = KeyCodes::End;
										else if (str == L"P") event.KeyCode = KeyCodes::F1;
										else if (str == L"Q") event.KeyCode = KeyCodes::F2;
										else if (str == L"R") event.KeyCode = KeyCodes::F3;
										else if (str == L"S") event.KeyCode = KeyCodes::F4;
										else continue;
									} else {
										if (str == L"1") event.KeyCode = KeyCodes::Home;
										else if (str == L"2") event.KeyCode = KeyCodes::Insert;
										else if (str == L"3") event.KeyCode = KeyCodes::Delete;
										else if (str == L"4") event.KeyCode = KeyCodes::End;
										else if (str == L"5") event.KeyCode = KeyCodes::PageUp;
										else if (str == L"6") event.KeyCode = KeyCodes::PageDown;
										else if (str == L"7") event.KeyCode = KeyCodes::Home;
										else if (str == L"8") event.KeyCode = KeyCodes::End;
										else if (str == L"11") event.KeyCode = KeyCodes::F1;
										else if (str == L"12") event.KeyCode = KeyCodes::F2;
										else if (str == L"13") event.KeyCode = KeyCodes::F3;
										else if (str == L"14") event.KeyCode = KeyCodes::F4;
										else if (str == L"15") event.KeyCode = KeyCodes::F5;
										else if (str == L"17") event.KeyCode = KeyCodes::F6;
										else if (str == L"18") event.KeyCode = KeyCodes::F7;
										else if (str == L"19") event.KeyCode = KeyCodes::F8;
										else if (str == L"20") event.KeyCode = KeyCodes::F9;
										else if (str == L"21") event.KeyCode = KeyCodes::F10;
										else if (str == L"23") event.KeyCode = KeyCodes::F11;
										else if (str == L"24") event.KeyCode = KeyCodes::F12;
										else if (str == L"25") event.KeyCode = KeyCodes::F13;
										else if (str == L"26") event.KeyCode = KeyCodes::F14;
										else if (str == L"28") event.KeyCode = KeyCodes::F15;
										else if (str == L"29") event.KeyCode = KeyCodes::F16;
										else if (str == L"31") event.KeyCode = KeyCodes::F17;
										else if (str == L"32") event.KeyCode = KeyCodes::F18;
										else if (str == L"33") event.KeyCode = KeyCodes::F19;
										else if (str == L"34") event.KeyCode = KeyCodes::F20;
										else if (str == L"A") event.KeyCode = KeyCodes::Up;
										else if (str == L"B") event.KeyCode = KeyCodes::Down;
										else if (str == L"C") event.KeyCode = KeyCodes::Right;
										else if (str == L"D") event.KeyCode = KeyCodes::Left;
										else if (str == L"F") event.KeyCode = KeyCodes::End;
										else if (str == L"H") event.KeyCode = KeyCodes::Home;
										else if (str == L"1P") event.KeyCode = KeyCodes::F1;
										else if (str == L"1Q") event.KeyCode = KeyCodes::F2;
										else if (str == L"1R") event.KeyCode = KeyCodes::F3;
										else if (str == L"1S") event.KeyCode = KeyCodes::F4;
										else if (str == L"1;5A") { event.KeyCode = KeyCodes::Up; event.KeyFlags |= ConsoleKeyFlagControl; }
										else if (str == L"1;5B") { event.KeyCode = KeyCodes::Down; event.KeyFlags |= ConsoleKeyFlagControl; }
										else if (str == L"1;5C") { event.KeyCode = KeyCodes::Right; event.KeyFlags |= ConsoleKeyFlagControl; }
										else if (str == L"1;5D") { event.KeyCode = KeyCodes::Left; event.KeyFlags |= ConsoleKeyFlagControl; }
										else continue;
									}
									return;
								} else {
									object.unprocessed->Insert(chr, 0);
									event.Event = ConsoleEvent::KeyInput;
									event.KeyCode = KeyCodes::Escape;
									event.KeyFlags = 0;
									return;
								}
							}
						} else if (chr < 32) {
						} else if (chr == 0x7F) {
							event.Event = ConsoleEvent::KeyInput;
							event.KeyCode = KeyCodes::Back;
							event.KeyFlags = 0;
							return;
						} else {
							event.Event = ConsoleEvent::CharacterInput;
							event.CharacterCode = chr;
							return;
						}
					}
				}
			} else throw FileAccessException(Error::NotImplemented);
		}
		bool Console::WaitEvent(uint timeout) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_in)) {
				if (_console_size_changed) return true;
				pollfd fd;
				fd.fd = object.file_in;
				fd.events = POLLIN;
				while (true) {
					int status = poll(&fd, 1, timeout);
					if (status == -1) {
						if (errno != EINTR) throw Exception();
						else if (_console_size_changed) return true;
					} else if (status == 0) return false;
					else return true;
				}
			} else throw FileAccessException(Error::NotImplemented);
		}
		void Console::WaitEvent(void) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_in)) {
				if (_console_size_changed) return;
				pollfd fd;
				fd.fd = object.file_in;
				fd.events = POLLIN;
				while (true) {
					int status = poll(&fd, 1, -1);
					if (status == -1) {
						if (errno != EINTR) throw Exception();
						else if (_console_size_changed) return;
					} else break;
				}
			} else throw FileAccessException(Error::NotImplemented);
		}
		void Console::SetTextColor(int color) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) {
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
		void Console::SetTextColor(ConsoleColor color) const { SetTextColor(int(color)); }
		void Console::SetBackgroundColor(int color) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) {
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
		void Console::SetBackgroundColor(ConsoleColor color) const { SetBackgroundColor(int(color)); }
		void Console::ClearScreen(void) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) Write(L"\e[2J\e[1;1H");
		}
		void Console::ClearLine(void) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) Write(L"\e[2K\e[1G");
		}
		void Console::MoveCaret(int x, int y) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) Write(L"\e[" + string(y + 1) + L";" + string(x + 1) + L"H");
		}
		void Console::SetTitle(const string & title) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) { string val = L"\e]0;" + title + L"\a"; Write(val); }
		}
		void Console::SetInputMode(ConsoleInputMode mode) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_in)) {
				tcdrain(isatty(object.file_out));
				auto in = object.file_in;
				if (mode == ConsoleInputMode::Echo) {
					if (object.iostat) {
						termios tio;
						MemoryCopy(&tio, object.iostat->GetBuffer(), sizeof(tio));
						tcflush(in, TCIOFLUSH);
						tcsetattr(in, TCSANOW, &tio);
						object.iostat.SetReference(0);
					}
				} else if (mode == ConsoleInputMode::Raw) {
					if (!object.iostat) {
						termios old_tio, tio;
						tcflush(in, TCIOFLUSH);
						tcgetattr(in, &tio);
						tcgetattr(in, &old_tio);
						cfmakeraw(&tio);
						tcsetattr(in, TCSANOW, &tio);
						object.iostat = new DataBlock(0x100);
						object.iostat->SetLength(sizeof(old_tio));
						MemoryCopy(object.iostat->GetBuffer(), &old_tio, sizeof(old_tio));
					}
				}
			}
		}
		void Console::AlternateScreenBuffer(bool alternate) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) {
				tcdrain(isatty(object.file_out));
				if (alternate) {
					if (!object.alt_buffer) {
						Write(L"\e[?1049h");
						object.alt_buffer = true;
					}
				} else {
					if (object.alt_buffer) {
						Write(L"\e[?1049l");
						object.alt_buffer = false;
					}
				}
			}
		}
		void Console::GetCaretPosition(int & x, int & y) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out) && isatty(object.file_in)) {
				tcdrain(isatty(object.file_out));
				auto in = object.file_in;
				termios old_tio, tio;
				tcflush(in, TCIOFLUSH);
				tcgetattr(in, &tio);
				tcgetattr(in, &old_tio);
				cfmakeraw(&tio);
				tcsetattr(in, TCSANOW, &tio);
				Write(L"\e[6n");
				while (true) {
					auto chr = ReadChar();
					if (chr == 27) {
						Array<uint32> cmd(0x10);
						while (chr != L'R' && chr != 0xFFFFFFFF) { cmd << chr; chr = ReadChar(); }
						if (chr == 0xFFFFFFFF) object.unprocessed->Append(chr);
						if (cmd.Length() < 3) { x = y = 0; tcsetattr(in, TCSANOW, &old_tio); return; }
						string conv = string(cmd.GetBuffer() + 2, cmd.Length() - 2, Encoding::UTF32);
						int sep = conv.FindFirst(L';');
						if (sep == -1) { x = y = 0; tcsetattr(in, TCSANOW, &old_tio); return; }
						try {
							y = conv.Fragment(0, sep).ToInt32() - 1;
							x = conv.Fragment(sep + 1, -1).ToInt32() - 1;
						} catch (...) { x = y = 0; }
						tcsetattr(in, TCSANOW, &old_tio);
						return;
					} else if (chr == 0xFFFFFFFF) { object.unprocessed->Append(chr); x = y = 0; tcsetattr(in, TCSANOW, &old_tio); return; }
					else object.unprocessed->Append(chr);
				}
			} else { x = y = 0; }
		}
		void Console::GetScreenBufferDimensions(int & w, int & h) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			if (isatty(object.file_out)) {
				auto out = object.file_out;
				winsize size;
				ioctl(out, TIOCGWINSZ, &size);
				w = size.ws_col;
				h = size.ws_row;
			} else { w = h = 0; }
		}
		bool Console::IsConsoleDevice(void) const
		{
			auto & object = reinterpret_cast<ConsoleObject &>(*internal.Inner());
			return isatty(object.file_out) && isatty(object.file_in);
		}
	}
}