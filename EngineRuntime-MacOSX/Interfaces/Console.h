#pragma once

#include "../Streaming.h"

namespace Engine
{
	namespace IO
	{
		enum class ConsoleColor {
			Default = -1, Black = 0, DarkBlue = 1, DarkGreen = 2, DarkCyan = 3,
			DarkRed = 4, DarkMagenta = 5, DarkYellow = 6, Gray = 7,
			DarkGray = 8, Blue = 9, Green = 10, Cyan = 11,
			Red = 12, Magenta = 13, Yellow = 14, White = 15
		};
		enum class ConsoleInputMode { Echo, Raw };
		enum class ConsoleEvent { CharacterInput, KeyInput, ConsoleResized, EndOfFile };
		enum ConsoleKeyFlags {
			ConsoleKeyFlagShift = 0x01,
			ConsoleKeyFlagControl = 0x02,
			ConsoleKeyFlagAlternative = 0x04
		};
		struct ConsoleEventDesc
		{
			ConsoleEvent Event;
			union {
				struct { uint CharacterCode; };
				struct { uint KeyCode, KeyFlags; };
				struct { uint Width, Height; };
			};
		};
		class Console final : virtual public Streaming::ITextWriter, virtual public Streaming::ITextReader
		{
			SafePointer<Object> internal;
		public:
			Console(void);
			Console(handle output);
			Console(handle output, handle input);
			virtual ~Console(void) override;

			virtual void Write(const string & text) const override;
			virtual void WriteLine(const string & text) const override;
			virtual void WriteEncodingSignature(void) const override;

			virtual uint32 ReadChar(void) const override;
			void ReadEvent(ConsoleEventDesc & event) const;
			bool WaitEvent(uint timeout) const;
			void WaitEvent(void) const;

			void SetTextColor(int color) const;
			void SetTextColor(ConsoleColor color) const;
			void SetBackgroundColor(int color) const;
			void SetBackgroundColor(ConsoleColor color) const;
			void ClearScreen(void) const;
			void ClearLine(void) const;
			void MoveCaret(int x, int y) const;
			void SetTitle(const string & title) const;
			void SetInputMode(ConsoleInputMode mode) const;
			void AlternateScreenBuffer(bool alternate) const;
			void GetCaretPosition(int & x, int & y) const;
			void GetScreenBufferDimensions(int & w, int & h) const;
			bool IsConsoleDevice(void) const;
		};
	}
}