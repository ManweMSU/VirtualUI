#include "Clipboard.h"

#include <Windows.h>

#define ENGINE_CLIPBOARD_FORMAT_ATTRIBUTED_TEXT L"EngineRuntimeAttributedText"
#define ENGINE_CLIPBOARD_FORMAT_CUSTOM L"EngineRuntimeCustomObject"
#define CF_ATTRIBUTEDTEXT ::Engine::Clipboard::AttributedText
#define CF_ENGINECUSTOM ::Engine::Clipboard::Custom

namespace Engine
{
	namespace Clipboard
	{
		UINT AttributedText = 0, Custom = 0;

		bool IsFormatAvailable(Format format)
		{
			if (!OpenClipboard(0)) return false;
			BOOL result = false;
			if (format == Format::Text) result = IsClipboardFormatAvailable(CF_UNICODETEXT);
			else if (format == Format::Image) result = IsClipboardFormatAvailable(CF_BITMAP);
			else if (format == Format::RichText) {
				if (!AttributedText) AttributedText = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_ATTRIBUTED_TEXT);
				result = IsClipboardFormatAvailable(CF_ATTRIBUTEDTEXT);
			} else if (format == Format::Custom) {
				if (!Custom) Custom = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_CUSTOM);
				result = IsClipboardFormatAvailable(CF_ENGINECUSTOM);
			}
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
		bool GetData(Codec::Frame ** value)
		{
			bool success = false;
			if (OpenClipboard(0)) {
				if (IsClipboardFormatAvailable(CF_BITMAP)) {
					HBITMAP picture = reinterpret_cast<HBITMAP>(GetClipboardData(CF_BITMAP));
					HWND desktop = GetDesktopWindow();
					HDC dc = GetDC(desktop);
					BITMAPINFOHEADER hdr;
					hdr.biSize = sizeof(hdr);
					hdr.biBitCount = 0;
					if (GetDIBits(dc, picture, 0, 0, 0, reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS)) {
						Codec::LineDirection direction;
						Codec::PixelFormat format;
						Codec::AlphaFormat alpha = Codec::AlphaFormat::Normal;
						if (hdr.biHeight > 0) direction = Codec::LineDirection::BottomUp;
						else direction = Codec::LineDirection::TopDown;
						if (hdr.biBitCount == 32) {
							format = Codec::PixelFormat::B8G8R8A8;
							hdr.biCompression = 0;
						} else {
							format = Codec::PixelFormat::B8G8R8;
							hdr.biCompression = 0;
							hdr.biBitCount = 24;
						}
						SafePointer<Codec::Frame> frame = new Codec::Frame(hdr.biWidth, hdr.biHeight > 0 ? hdr.biHeight : -hdr.biHeight, -1, format, alpha, direction);
						if (GetDIBits(dc, picture, 0, frame->GetHeight(), frame->GetData(), reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS)) {
							success = true;
							if (format == Codec::PixelFormat::B8G8R8A8) {
								bool no_alpha = true;
								for (int j = 0; j < frame->GetHeight(); j++) {
									for (int i = 0; i < frame->GetWidth(); i++) {
										uint32 clr = frame->GetPixel(i, j);
										if (clr & 0xFF000000) { no_alpha = false; break; }
									}
									if (!no_alpha) break;
								}
								if (no_alpha) {
									for (int j = 0; j < frame->GetHeight(); j++) {
										for (int i = 0; i < frame->GetWidth(); i++) {
											frame->SetPixel(i, j, (frame->GetPixel(i, j) | 0xFF000000));
										}
									}
								}
							}
							*value = frame;
							frame->Retain();
						}
					}
					ReleaseDC(desktop, dc);
				}
				CloseClipboard();
			}
			return success;
		}
		bool GetData(string & value, bool attributed)
		{
			if (attributed) {
				if (!AttributedText) AttributedText = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_ATTRIBUTED_TEXT);
				bool success = false;
				if (OpenClipboard(0)) {
					if (IsClipboardFormatAvailable(CF_ATTRIBUTEDTEXT)) {
						HANDLE text = GetClipboardData(CF_ATTRIBUTEDTEXT);
						void * locked = reinterpret_cast<void *>(GlobalLock(text));
						if (locked) {
							value = string(locked, -1, Encoding::UTF8);
							success = true;
							GlobalUnlock(text);
						}
					}
					CloseClipboard();
				}
				return success;
			} else return GetData(value);
		}
		bool GetData(const string & subclass, Array<uint8> ** value)
		{
			if (!Custom) Custom = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_CUSTOM);
			SafePointer< Array<uint8> > p_array = new Array<uint8>(0x100);
			bool success = false;
			*value = 0;
			if (OpenClipboard(0)) {
				if (IsClipboardFormatAvailable(CF_ENGINECUSTOM)) {
					HANDLE data = GetClipboardData(CF_ENGINECUSTOM);
					uint8 * locked = reinterpret_cast<uint8 *>(GlobalLock(data));
					if (locked) {
						string cls = string(locked, -1, Encoding::UTF8);
						if (cls == subclass) {
							int ofs = cls.GetEncodedLength(Encoding::UTF8) + 1;
							int size = GlobalSize(data) - ofs;
							p_array->SetLength(size);
							MemoryCopy(p_array->GetBuffer(), locked + ofs, size);
							p_array->Retain();
							*value = p_array;
							success = true;
						}
						GlobalUnlock(data);
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
		bool SetData(Codec::Frame * value)
		{
			bool success = false;
			if (OpenClipboard(0)) {
				EmptyClipboard();
				HWND desktop = GetDesktopWindow();
				HDC dc = GetDC(desktop);
				SafePointer<Codec::Frame> frame;
				if (value->GetPixelFormat() == Codec::PixelFormat::B8G8R8A8 && value->GetAlphaFormat() == Codec::AlphaFormat::Normal && value->GetLineDirection() == Codec::LineDirection::BottomUp) {
					frame.SetRetain(value);
				} else {
					frame = value->ConvertFormat(Codec::FrameFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaFormat::Normal, Codec::LineDirection::BottomUp));
				}
				BITMAPINFOHEADER hdr;
				hdr.biSize = sizeof(hdr);
				hdr.biBitCount = 32;
				hdr.biClrImportant = 0;
				hdr.biClrUsed = 0;
				hdr.biCompression = 0;
				hdr.biHeight = frame->GetHeight();
				hdr.biPlanes = 1;
				hdr.biWidth = frame->GetWidth();
				hdr.biXPelsPerMeter = 0;
				hdr.biYPelsPerMeter = 0;
				hdr.biSizeImage = hdr.biWidth * hdr.biHeight * 4;
				HBITMAP picture = CreateDIBitmap(dc, &hdr, CBM_INIT, frame->GetData(), reinterpret_cast<LPBITMAPINFO>(&hdr), DIB_RGB_COLORS);
				if (picture) {
					if (SetClipboardData(CF_BITMAP, picture)) {
						success = true;
					} else {
						DeleteObject(picture);
					}
				}
				ReleaseDC(desktop, dc);
				CloseClipboard();
			}
			return success;
		}
		bool SetData(const string & plain, const string & attributed)
		{
			if (!AttributedText) AttributedText = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_ATTRIBUTED_TEXT);
			bool success = false;
			if (OpenClipboard(0)) {
				EmptyClipboard();
				HANDLE plain_text = GlobalAlloc(GMEM_MOVEABLE, (plain.Length() + 1) << 1);
				if (plain_text) {
					HANDLE attr_text = GlobalAlloc(GMEM_MOVEABLE, attributed.GetEncodedLength(Encoding::UTF8) + 1);
					if (attr_text) {
						widechar * plain_locked = reinterpret_cast<widechar *>(GlobalLock(plain_text));
						if (plain_locked) {
							void * attr_locked = reinterpret_cast<void *>(GlobalLock(attr_text));
							if (attr_locked) {
								plain.Encode(plain_locked, Encoding::UTF16, true);
								attributed.Encode(attr_locked, Encoding::UTF8, true);
								GlobalUnlock(attr_text);
								GlobalUnlock(plain_text);
								if (SetClipboardData(CF_UNICODETEXT, plain_text) && SetClipboardData(CF_ATTRIBUTEDTEXT, attr_text)) {
									success = true;
								} else {
									EmptyClipboard();
									GlobalFree(plain_text);
									GlobalFree(attr_text);
								}
							} else {
								GlobalUnlock(plain_text);
								GlobalFree(plain_text);
								GlobalFree(attr_text);
							}
						} else {
							GlobalFree(plain_text);
							GlobalFree(attr_text);
						}
					} else GlobalFree(plain_text);
				}
				CloseClipboard();
			}
			return success;
		}
		bool SetData(const string & subclass, const void * data, int size)
		{
			if (!Custom) Custom = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_CUSTOM);
			int sclsl = subclass.GetEncodedLength(Encoding::UTF8) + 1;
			int mtlen = size + sclsl;
			bool success = false;
			if (OpenClipboard(0)) {
				EmptyClipboard();
				HANDLE mem = GlobalAlloc(GMEM_MOVEABLE, mtlen);
				if (mem) {
					uint8 * locked = reinterpret_cast<uint8 *>(GlobalLock(mem));
					if (locked) {
						subclass.Encode(locked, Encoding::UTF8, true);
						MemoryCopy(locked + sclsl, data, size);
						GlobalUnlock(mem);
						if (SetClipboardData(CF_ENGINECUSTOM, mem)) {
							success = true;
						} else {
							GlobalFree(mem);
						}
					} else GlobalFree(mem);
				}
				CloseClipboard();
			}
			return success;
		}
		string GetCustomSubclass(void)
		{
			if (!Custom) Custom = RegisterClipboardFormatW(ENGINE_CLIPBOARD_FORMAT_CUSTOM);
			string result = L"";
			if (OpenClipboard(0)) {
				if (IsClipboardFormatAvailable(CF_ENGINECUSTOM)) {
					HANDLE data = GetClipboardData(CF_ENGINECUSTOM);
					void * locked = reinterpret_cast<void *>(GlobalLock(data));
					if (locked) {
						result = string(locked, -1, Encoding::UTF8);
						GlobalUnlock(data);
					}
				}
				CloseClipboard();
			}
			return result;
		}
	}
}