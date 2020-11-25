#pragma once

#include "../Streaming.h"

namespace Engine
{
	namespace IO
	{
		class ControlToken
		{
		public:
			uint32 value;
			ControlToken(uint32 arg);
		};
		class Console final : virtual public Streaming::ITextWriter, virtual public Streaming::ITextReader
		{
			Streaming::TextWriter writer;
			Streaming::TextReader reader;
			handle file;
			handle file_in;
			handle main_buffer;
			uint16 attr;
			bool is_true_console;
			int console_in_mode;
		public:
			enum class InputMode { Echo, Raw };
			enum ConsoleColor {
				ColorDefault = -1,
				ColorDarkBlack = 0,
				ColorDarkBlue = 1,
				ColorDarkGreen = 2,
				ColorDarkCyan = 3,
				ColorDarkRed = 4,
				ColorDarkMagenta = 5,
				ColorDarkYellow = 6,
				ColorGray = 7,
				ColorDarkGray = 8,
				ColorBlue = 9,
				ColorGreen = 10,
				ColorCyan = 11,
				ColorRed = 12,
				ColorMagenta = 13,
				ColorYellow = 14,
				ColorWhite = 15
			};

			Console(void);
			Console(handle output);
			Console(handle output, handle input);
			virtual ~Console(void) override;

			virtual void Write(const string & text) const override;
			virtual void WriteLine(const string & text) const override;
			virtual void WriteEncodingSignature(void) const override;
			virtual string ToString(void) const override;

			void SetTextColor(int color);
			void SetBackgroundColor(int color);
			void ClearScreen(void);
			void ClearLine(void);
			void MoveCaret(int x, int y);
			void SetTitle(const string & title);
			void SetInputMode(InputMode mode);
			void AlternateScreenBuffer(bool alternate);
			void GetCaretPosition(int & x, int & y);
			void GetScreenBufferDimensions(int & w, int & h);
			bool IsConsoleDevice(void);

			virtual uint32 ReadChar(void) const override;

			Console & operator << (const string & text);
			Console & operator << (ControlToken token);
			Console & operator >> (string & str);
		};

		namespace ConsoleControl
		{
			typedef Engine::IO::Console Console;
			ControlToken LineFeed(void);
			ControlToken TextColor(int color);
			ControlToken TextColorDefault(void);
			ControlToken TextBackground(int color);
			ControlToken TextBackgroundDefault(void);
		};
	}
}