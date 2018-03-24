#include "Clipboard.h"

#include <Windows.h>

namespace Engine
{
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format)
		{
			if (!OpenClipboard(0)) return false;
			BOOL result = false;
			if (format == Format::Text) result = IsClipboardFormatAvailable(CF_UNICODETEXT);
			CloseClipboard();
			return result != 0;
		}
		bool GetData(string & value)
		{
			bool success = false;
			if (OpenClipboard(0)) {
				if (IsClipboardFormatAvailable(CF_UNICODETEXT)) {
					HANDLE text = GetClipboardData(CF_UNICODETEXT);
					widechar * locked = reinterpret_cast<widechar *>(GlobalLock(text));
					if (locked) {
						value = string(locked);
						success = true;
						GlobalUnlock(text);
					}
				}
				CloseClipboard();
			}
			return success;
		}
		bool SetData(const string & value)
		{
			bool success = false;
			if (OpenClipboard(0)) {
				EmptyClipboard();
				HANDLE text = GlobalAlloc(GMEM_MOVEABLE, (value.Length() + 1) << 1);
				if (text) {
					widechar * locked = reinterpret_cast<widechar *>(GlobalLock(text));
					if (locked) {
						value.Encode(locked, Encoding::UTF16, true);
						GlobalUnlock(text);
						if (SetClipboardData(CF_UNICODETEXT, text)) {
							success = true;
						} else {
							GlobalFree(text);
						}
					} else GlobalFree(text);
				}
				CloseClipboard();
			}
			return success;
		}
	}
}