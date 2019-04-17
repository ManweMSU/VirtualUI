#pragma once

#include "../Streaming.h"

namespace Engine
{
    namespace IO
    {
        class Console final : public Streaming::ITextWriter
        {
            Streaming::TextWriter writer;
            handle file;
        public:
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
            virtual ~Console(void) override;
			virtual void Write(const string & text) const override;
			virtual void WriteLine(const string & text) const override;
			virtual void WriteEncodingSignature(void) const override;
			virtual string ToString(void) const override;

            Streaming::TextWriter & GetWriter(void);
            void SetTextColor(int color);
            void SetBackgroundColor(int color);
            void ClearScreen(void);
            void ClearLine(void);
            void MoveCaret(int x, int y);

			Console & operator << (const string & text);
			const Console & operator << (const string & text) const;
        };
    }
}