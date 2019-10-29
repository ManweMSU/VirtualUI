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
			else if (format == Format::Image) result = IsClipboardFormatAvailable(CF_BITMAP);
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
	}
}