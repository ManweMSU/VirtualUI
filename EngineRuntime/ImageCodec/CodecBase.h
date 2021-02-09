#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Standard image format strings:                                  Support          Windows  Mac OS X
// BMP    - Windows Bitmap (.bmp)                                  WIC, IIO           YES      YES
// PNG    - Portable Network Graphics (.png)                       WIC, IIO           YES      YES
// JPG    - Joint Photographic Image Format (.jpg, .jpeg, .jfif)   WIC, IIO           YES      YES
// GIF    - Graphics Interchange Format (.gif)                     WIC, IIO           YES      YES
// TIF    - Tagged Image File Format (.tif, .tiff)                 WIC, IIO           YES      YES
// DDS    - Direct Draw Surface (.dds)                             WIC                YES      NO
// HEIF   - High Efficiency Image Format (.heif, .heic)            WIC, IIO            NO   READ-ONLY
// ICO    - Windows Icon (.ico)                                    IC                 YES      YES
// CUR    - Windows Cursor (.cur)                                  IC                 YES      YES
// ICNS   - Apple Icon (.icns)                                     IC                 YES      YES
// EIWV   - Engine Image Wide Volume (.eiwv)                       VC                 YES      YES
//
// WIC - Windows Image Component (Windows only)
// IIO - Image I/O Framework (Mac OS X only)
// IC  - Icon Codec (Built-in, universal)
// VC  - Volume Codec (Built-in, universal)
// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

namespace Engine
{
	namespace Codec
	{
		class Frame;
		class Image;

		class ICodec : public Object
		{
		public:
			ICodec(void);
			~ICodec(void) override;

			virtual void EncodeFrame(Streaming::Stream * stream, Frame * frame, const string & format) = 0;
			virtual void EncodeImage(Streaming::Stream * stream, Image * image, const string & format) = 0;
			virtual Frame * DecodeFrame(Streaming::Stream * stream) = 0;
			virtual Image * DecodeImage(Streaming::Stream * stream) = 0;
			
			virtual bool IsImageCodec(void) = 0;
			virtual bool IsFrameCodec(void) = 0;

			virtual string ExamineData(Streaming::Stream * stream) = 0;
			virtual bool CanEncode(const string & format) = 0;
			virtual bool CanDecode(const string & format) = 0;
		};

		ICodec * FindCoder(const string & format);
		ICodec * FindDecoder(const string & format);
		void EncodeFrame(Streaming::Stream * stream, Frame * frame, const string & format);
		void EncodeImage(Streaming::Stream * stream, Image * image, const string & format);
		Frame * DecodeFrame(Streaming::Stream * stream);
		Image * DecodeImage(Streaming::Stream * stream);
		string GetEncodedImageFormat(Streaming::Stream * stream);

		enum class FrameUsage { ColorMap, NormalMap, LightMap };
		enum class AlphaMode { Normal = 0, Straight = 0, Premultiplied = 1 };
		enum class ScanOrigin { TopDown, BottomUp };
		enum class PixelFormat {
			B8G8R8A8, R8G8B8A8, B8G8R8X8, R8G8B8X8,
			B8G8R8, R8G8B8,
			B5G5R5A1, B5G5R5X1, B5G6R5, R5G5B5A1, R5G5B5X1, R5G6B5,
			B4G4R4A4, B4G4R4X4, R4G4B4A4, R4G4B4X4,
			A8, R8, P8, P4, P2, P1
		};

		bool IsPalettePixel(PixelFormat format);
		uint32 GetPaletteVolume(PixelFormat format);
		uint32 GetBitsPerPixel(PixelFormat format);
		uint32 GetRedChannel(uint32 source, PixelFormat source_format, AlphaMode source_alpha);
		uint32 GetGreenChannel(uint32 source, PixelFormat source_format, AlphaMode source_alpha);
		uint32 GetBlueChannel(uint32 source, PixelFormat source_format, AlphaMode source_alpha);
		uint32 GetAlphaChannel(uint32 source, PixelFormat source_format, AlphaMode source_alpha);
		uint32 MakePixel(uint32 r, uint32 g, uint32 b, uint32 a, PixelFormat format, AlphaMode alpha);
		uint32 ConvertPixel(uint32 source, PixelFormat source_format, AlphaMode source_alpha, PixelFormat format, AlphaMode alpha);

		class Frame : public Object
		{
			int32 Width;
			int32 Height;
			int32 ScanLineLength;
			PixelFormat Format;
			AlphaMode Alpha;
			ScanOrigin Origin;
			uint8 * RawData;
			Array<uint32> Palette;
		public:
			FrameUsage Usage = FrameUsage::ColorMap;
			int32 HotPointX = 0;
			int32 HotPointY = 0;
			int32 Duration = 0;
			double DpiUsage = 1.0;

			Frame(const Frame & src);
			Frame(const Frame * src);
			Frame(int32 width, int32 height, PixelFormat format);
			Frame(int32 width, int32 height, PixelFormat format, AlphaMode alpha);
			Frame(int32 width, int32 height, PixelFormat format, ScanOrigin origin);
			Frame(int32 width, int32 height, PixelFormat format, AlphaMode alpha, ScanOrigin origin);
			Frame(int32 width, int32 height, int32 scan_line_length, PixelFormat format);
			Frame(int32 width, int32 height, int32 scan_line_length, PixelFormat format, AlphaMode alpha);
			Frame(int32 width, int32 height, int32 scan_line_length, PixelFormat format, ScanOrigin origin);
			Frame(int32 width, int32 height, int32 scan_line_length, PixelFormat format, AlphaMode alpha, ScanOrigin origin);
			~Frame(void) override;

			int32 GetWidth(void) const;
			int32 GetHeight(void) const;
			int32 GetScanLineLength(void) const;
			PixelFormat GetPixelFormat(void) const;
			AlphaMode GetAlphaMode(void) const;
			ScanOrigin GetScanOrigin(void) const;

			uint32 GetPixel(int x, int y) const;
			void SetPixel(int x, int y, uint32 v);
			uint8 * GetData(void);
			const uint8 * GetData(void) const;

			const uint32 * GetPalette(void) const;
			uint32 * GetPalette(void);
			int GetPaletteVolume(void) const;
			void SetPaletteVolume(int volume);
			uint32 GetBestPaletteIndex(uint32 color) const;

			uint32 ReadPixel(int x, int y, PixelFormat format = PixelFormat::R8G8B8A8, AlphaMode alpha = AlphaMode::Normal) const;
			void WritePixel(int x, int y, uint32 v, PixelFormat format = PixelFormat::R8G8B8A8, AlphaMode alpha = AlphaMode::Normal);
			uint32 ReadPalette(int index, PixelFormat format = PixelFormat::R8G8B8A8, AlphaMode alpha = AlphaMode::Normal) const;
			void WritePalette(int index, uint32 v, PixelFormat format = PixelFormat::R8G8B8A8, AlphaMode alpha = AlphaMode::Normal);

			Frame * ConvertFormat(PixelFormat format, int scan_line = -1) const;
			Frame * ConvertFormat(AlphaMode alpha, int scan_line = -1) const;
			Frame * ConvertFormat(ScanOrigin origin, int scan_line = -1) const;
			Frame * ConvertFormat(PixelFormat format, AlphaMode alpha, int scan_line = -1) const;
			Frame * ConvertFormat(PixelFormat format, ScanOrigin origin, int scan_line = -1) const;
			Frame * ConvertFormat(AlphaMode alpha, ScanOrigin origin, int scan_line = -1) const;
			Frame * ConvertFormat(PixelFormat format, AlphaMode alpha, ScanOrigin origin, int scan_line = -1) const;
		};
		class Image : public Object
		{
		public:
			ObjectArray<Frame> Frames;

			Image(void);
			~Image(void) override;

			Frame * GetFrameBestSizeFit(int32 w, int32 h) const;
			Frame * GetFrameBestDpiFit(double dpi) const;
			Frame * GetFrameByUsage(FrameUsage usage) const;
			Frame * GetFramePreciseSize(int32 w, int32 h) const;
		};
	}
}