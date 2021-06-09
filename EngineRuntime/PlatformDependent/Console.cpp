#include "../Interfaces/Console.h"

#include <Windows.h>

using namespace Engine::Streaming;

namespace Engine
{
	namespace IO
	{
		class ConsoleObject : public Object
		{
		public:
			SafePointer<TextWriter> writer;
			SafePointer<TextReader> reader;
			handle file_out;
			handle file_in;
			handle main_buffer;
			uint16 attr;
			uint32 cached_char;
			bool is_true_console;
			int console_in_mode;

			void _console_init(void)
			{
				cached_char = 0;
				if (file_out == INVALID_HANDLE_VALUE) throw FileAccessException(Error::InvalidHandle);
				writer = new TextWriter(SafePointer<Stream>(new FileStream(file_out)), TextFileEncoding);
				reader = new TextReader(SafePointer<Stream>(new FileStream(file_in)), TextFileEncoding);
				DWORD mode;
				if (GetConsoleMode(file_out, &mode)) {
					is_true_console = true;
					CONSOLE_SCREEN_BUFFER_INFO info;
					GetConsoleScreenBufferInfo(file_out, &info);
					attr = info.wAttributes;
				} else {
					is_true_console = false;
					attr = 0;
				}
				if (file_in == INVALID_HANDLE_VALUE) console_in_mode = 0;
				else if (GetConsoleMode(file_in, &mode)) console_in_mode = 1;
				else console_in_mode = 2;
			}
			ConsoleObject(void) : file_out(GetStandardOutput()), file_in(GetStandardInput()), main_buffer(INVALID_HANDLE_VALUE) { _console_init(); }
			ConsoleObject(handle output) : file_out(output), file_in(GetStandardInput()), main_buffer(INVALID_HANDLE_VALUE) { _console_init(); }
			ConsoleObject(handle output, handle input) : file_out(output), file_in(input), main_buffer(INVALID_HANDLE_VALUE) { _console_init(); }
			virtual ~ConsoleObject(void) override {}
		};
		Console::Console(void) : internal(new ConsoleObject) {}
		Console::Console(handle output) : internal(new ConsoleObject(output)) {}
		Console::Console(handle output, handle input) : internal(new ConsoleObject(output, input)) {}
		Console::~Console(void) {}
		void Console::Write(const string & text) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) WriteConsoleW(object.file_out, text, text.Length(), 0, 0);
			else object.writer->Write(text);
		}
		void Console::WriteLine(const string & text) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				WriteConsoleW(object.file_out, text, text.Length(), 0, 0);
				WriteConsoleW(object.file_out, L"\n", 1, 0, 0);
			} else object.writer->WriteLine(text);
		}
		void Console::WriteEncodingSignature(void) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (!object.is_true_console) object.writer->WriteEncodingSignature();
		}
		uint32 Console::ReadChar(void) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.console_in_mode) {
				if (object.console_in_mode == 1) {
					while (true) {
						DWORD read;
						widechar char_read;
						if (!ReadConsoleW(object.file_in, &char_read, 1, &read, 0)) throw FileAccessException(Error::ReadFailure);
						if (read == 1) {
							if ((char_read & 0xFC00) == 0xD800) {
								object.cached_char = char_read & 0x03FF;
								object.cached_char <<= 10;
							} else if ((char_read & 0xFC00) == 0xDC00) {
								object.cached_char |= char_read & 0x03FF;
								object.cached_char += 0x10000;
								return object.cached_char;
							} else return char_read;
						} else { eof = true; return 0xFFFFFFFF; }
					}
				} else {
					auto char_read = object.reader->ReadChar();
					eof = object.reader->EofReached();
					return char_read;
				}
			} else throw FileAccessException(Error::NotImplemented);
		}
		void Console::ReadEvent(ConsoleEventDesc & event) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.console_in_mode == 1) {
				DWORD read;
				INPUT_RECORD input;
				while (true) {
					if (!ReadConsoleInputW(object.file_in, &input, 1, &read)) throw FileAccessException(Error::ReadFailure);
					if (read) {
						if (input.EventType == KEY_EVENT) {
							if (input.Event.KeyEvent.bKeyDown) {
								if (input.Event.KeyEvent.uChar.UnicodeChar) {
									if ((input.Event.KeyEvent.uChar.UnicodeChar & 0xFC00) == 0xD800) {
										object.cached_char = input.Event.KeyEvent.uChar.UnicodeChar & 0x03FF;
										object.cached_char <<= 10;
									} else if ((input.Event.KeyEvent.uChar.UnicodeChar & 0xFC00) == 0xDC00) {
										object.cached_char |= input.Event.KeyEvent.uChar.UnicodeChar & 0x03FF;
										object.cached_char += 0x10000;
										event.Event = ConsoleEvent::CharacterInput;
										event.CharacterCode = object.cached_char;
										return;
									} else {
										event.Event = ConsoleEvent::CharacterInput;
										event.CharacterCode = input.Event.KeyEvent.uChar.UnicodeChar;
										return;
									}
								} else {
									event.Event = ConsoleEvent::KeyInput;
									event.KeyCode = input.Event.KeyEvent.wVirtualKeyCode;
									event.KeyFlags = 0;
									if (input.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED) event.KeyFlags |= ConsoleKeyFlagShift;
									else if (input.Event.KeyEvent.dwControlKeyState & LEFT_CTRL_PRESSED) event.KeyFlags |= ConsoleKeyFlagControl;
									else if (input.Event.KeyEvent.dwControlKeyState & RIGHT_CTRL_PRESSED) event.KeyFlags |= ConsoleKeyFlagControl;
									else if (input.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED) event.KeyFlags |= ConsoleKeyFlagAlternative;
									else if (input.Event.KeyEvent.dwControlKeyState & RIGHT_ALT_PRESSED) event.KeyFlags |= ConsoleKeyFlagAlternative;
									return;
								}
							}
						} else if (input.EventType == WINDOW_BUFFER_SIZE_EVENT) {
							event.Event = ConsoleEvent::ConsoleResized;
							event.Width = input.Event.WindowBufferSizeEvent.dwSize.X;
							event.Height = input.Event.WindowBufferSizeEvent.dwSize.Y;
							return;
						}
					} else {
						eof = true;
						event.Event = ConsoleEvent::EndOfFile;
						return;
					}
				}
			} else throw FileAccessException(Error::NotImplemented);
		}
		bool Console::WaitEvent(uint timeout) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.console_in_mode == 1) {
				auto status = WaitForSingleObject(object.file_in, timeout);
				if (status) {
					if (status == WAIT_TIMEOUT) return false;
					else throw Exception();
				} else return true;
			} else throw FileAccessException(Error::NotImplemented);
		}
		void Console::WaitEvent(void) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.console_in_mode == 1) {
				if (WaitForSingleObject(object.file_in, INFINITE)) throw Exception();
			} else throw FileAccessException(Error::NotImplemented);
		}
		void Console::SetTextColor(int color) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				if (!GetConsoleScreenBufferInfo(object.file_out, &info)) throw Exception();
				if (color >= 0 && color <= 15) SetConsoleTextAttribute(object.file_out, (info.wAttributes & 0xF0) | uint16(color));
				else SetConsoleTextAttribute(object.file_out, (info.wAttributes & 0xF0) | (object.attr & 0x0F));
			}
		}
		void Console::SetTextColor(ConsoleColor color) const { SetTextColor(int(color)); }
		void Console::SetBackgroundColor(int color) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				if (!GetConsoleScreenBufferInfo(object.file_out, &info)) throw Exception();
				if (color >= 0 && color <= 15) SetConsoleTextAttribute(object.file_out, (info.wAttributes & 0x0F) | (uint16(color) << 4));
				else SetConsoleTextAttribute(object.file_out, (info.wAttributes & 0x0F) | (object.attr & 0xF0));
			}
		}
		void Console::SetBackgroundColor(ConsoleColor color) const { SetBackgroundColor(int(color)); }
		void Console::ClearScreen(void) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				if (!GetConsoleScreenBufferInfo(object.file_out, &info)) throw Exception();
				DWORD Written; COORD Begin; Begin.X = Begin.Y = 0;
				FillConsoleOutputCharacterW(object.file_out, L' ', info.dwSize.X * info.dwSize.Y, Begin, &Written);
				FillConsoleOutputAttribute(object.file_out, info.wAttributes, info.dwSize.X * info.dwSize.Y, Begin, &Written);
				SetConsoleCursorPosition(object.file_out, Begin);
			}
		}
		void Console::ClearLine(void) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				if (!GetConsoleScreenBufferInfo(object.file_out, &info)) throw Exception();
				DWORD Written; COORD Begin; Begin.X = 0; Begin.Y = info.dwCursorPosition.Y;
				FillConsoleOutputCharacterW(object.file_out, L' ', info.dwSize.X, Begin, &Written);
				FillConsoleOutputAttribute(object.file_out, info.wAttributes, info.dwSize.X, Begin, &Written);
				SetConsoleCursorPosition(object.file_out, Begin);
			}
		}
		void Console::MoveCaret(int x, int y) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				COORD position; position.X = x; position.Y = y;
				SetConsoleCursorPosition(object.file_out, position);
			}
		}
		void Console::SetTitle(const string & title) const { SetConsoleTitleW(title); }
		void Console::SetInputMode(ConsoleInputMode mode) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.console_in_mode == 1) {
				DWORD mode_var;
				GetConsoleMode(object.file_in, &mode_var);
				if (mode == ConsoleInputMode::Echo) {
					mode_var |= ENABLE_ECHO_INPUT;
					mode_var |= ENABLE_LINE_INPUT;
				} else if (mode == ConsoleInputMode::Raw) {
					mode_var &= ~ENABLE_ECHO_INPUT;
					mode_var &= ~ENABLE_LINE_INPUT;
				}
				FlushConsoleInputBuffer(object.file_in);
				SetConsoleMode(object.file_in, mode_var);
			}
		}
		void Console::AlternateScreenBuffer(bool alternate) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				if (alternate) {
					if (object.main_buffer == INVALID_HANDLE_VALUE) {
						object.main_buffer = object.file_out;
						object.file_out = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, 0, CONSOLE_TEXTMODE_BUFFER, 0);
						SetConsoleActiveScreenBuffer(object.file_out);
					}
				} else {
					if (object.main_buffer != INVALID_HANDLE_VALUE) {
						SetConsoleActiveScreenBuffer(object.main_buffer);
						CloseHandle(object.file_out);
						object.file_out = object.main_buffer;
						object.main_buffer = INVALID_HANDLE_VALUE;
					}
				}
			}
		}
		void Console::GetCaretPosition(int & x, int & y) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				if (!GetConsoleScreenBufferInfo(object.file_out, &info)) throw Exception();
				x = info.dwCursorPosition.X;
				y = info.dwCursorPosition.Y;
			} else { x = y = 0; }
		}
		void Console::GetScreenBufferDimensions(int & w, int & h) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			if (object.is_true_console) {
				CONSOLE_SCREEN_BUFFER_INFO info;
				if (!GetConsoleScreenBufferInfo(object.file_out, &info)) throw Exception();
				w = info.dwSize.X;
				h = info.dwSize.Y;
			} else { w = h = 0; }
		}
		bool Console::IsConsoleDevice(void) const
		{
			auto & object = static_cast<ConsoleObject &>(*internal.Inner());
			return object.is_true_console && object.console_in_mode == 1;
		}
	}
}