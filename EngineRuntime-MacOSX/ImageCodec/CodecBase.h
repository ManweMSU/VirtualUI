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
// HEIF   - High Efficiency Image Format (.heif, .heic)            WIC, IIO            READ-ONLY
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
		enum class PixelFormat { B8G8R8A8, R8G8B8A8, B8G8R8U8, R8G8B8U8, B8G8R8, R8G8B8, P8, P4, P2, P1 };
		enum class AlphaMode { Normal, Premultiplied };
		enum class ScanOrigin { TopDown, BottomUp };

		struct FrameFormat
		{
			PixelFormat Format;
			AlphaMode Alpha;
			ScanOrigin Direction;

			FrameFormat(void);
			FrameFormat(PixelFormat format, AlphaMode alpha, ScanOrigin direction);
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
			ScanOrigin Direction;
			uint8 * RawData;
			Array<uint32> Palette;
		public:
			FrameUsage Usage = FrameUsage::ColorMap;
			int32 HotPointX = 0;
			int32 HotPointY = 0;
			int32 Duration = 0;
			double DpiUsage = 1.0;

			Frame(int32 width, int32 height, int32 scan_line_length, PixelFormat format, AlphaMode alpha, ScanOrigin direction);
			~Frame(void) override;

			int32 GetWidth(void) const;
			int32 GetHeight(void) const;
			int32 GetScanLineLength(void) const;
			PixelFormat GetPixelFormat(void) const;
			AlphaMode GetAlphaMode(void) const;
			ScanOrigin GetScanOrigin(void) const;
			const uint32 * GetPalette(void) const;
			uint32 * GetPalette(void);
			int GetPaletteVolume(void) const;
			void SetPaletteVolume(int volume);
			uint32 GetPixel(int x, int y) const;
			void SetPixel(int x, int y, uint32 v);
			uint8 * GetData(void);
			const uint8 * GetData(void) const;
			uint32 GetBestPaletteIndex(uint32 color) const;

			Frame * ConvertFormat(const FrameFormat & new_format) const;
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