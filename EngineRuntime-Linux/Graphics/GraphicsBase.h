#pragma once

#include "../EngineBase.h"
#include "../Streaming.h"
#include "../ImageCodec/CodecBase.h"
#include "../Graphics/Graphics.h"
#include "../Math/Vector.h"

namespace Engine
{
	namespace UI
	{
		extern double CurrentScaleFactor;

		class IRenderingDevice;
		class ITextureRenderingDevice;

		enum RenderingDeviceFeature {
			RenderingDeviceFeatureBlurCapable = 0x00000001,
			RenderingDeviceFeatureInversionCapable = 0x00000002,
			RenderingDeviceFeaturePolygonCapable = 0x00000004,
			RenderingDeviceFeatureLayersCapable = 0x00000008,
			RenderingDeviceFeatureHardware = 0x20000000,
			RenderingDeviceFeatureGraphicsInteropCapable = 0x40000000,
			RenderingDeviceFeatureTextureTarget = 0x80000000
		};

		class Coordinate
		{
		public:
			double Anchor, Zoom;
			int Absolute;

			Coordinate(void) noexcept;
			Coordinate(int shift) noexcept;
			Coordinate(int shift, double zoom, double anchor) noexcept;

			Coordinate friend operator + (const Coordinate & a, const Coordinate & b) noexcept;
			Coordinate friend operator - (const Coordinate & a, const Coordinate & b) noexcept;
			Coordinate friend operator * (const Coordinate & a, double b) noexcept;
			Coordinate friend operator * (double b, const Coordinate & a) noexcept;
			Coordinate friend operator / (const Coordinate & a, double b) noexcept;

			Coordinate & operator += (const Coordinate & a) noexcept;
			Coordinate & operator -= (const Coordinate & a) noexcept;
			Coordinate & operator *= (double a) noexcept;
			Coordinate & operator /= (double a) noexcept;
			Coordinate operator - (void) const noexcept;

			bool friend operator == (const Coordinate & a, const Coordinate & b) noexcept;
			bool friend operator != (const Coordinate & a, const Coordinate & b) noexcept;

			static Coordinate Right() noexcept;
			static Coordinate Bottom() noexcept;
		};
		class Rectangle
		{
		public:
			Coordinate Left, Top, Right, Bottom;

			Rectangle(void) noexcept;
			Rectangle(const Coordinate & left, const Coordinate & top, const Coordinate & right, const Coordinate & bottom) noexcept;

			Rectangle friend operator + (const Rectangle & a, const Rectangle & b) noexcept;
			Rectangle friend operator - (const Rectangle & a, const Rectangle & b) noexcept;
			Rectangle friend operator * (const Rectangle & a, double b) noexcept;
			Rectangle friend operator * (double b, const Rectangle & a) noexcept;
			Rectangle friend operator / (const Rectangle & a, double b) noexcept;

			Rectangle & operator += (const Rectangle & a) noexcept;
			Rectangle & operator -= (const Rectangle & a) noexcept;
			Rectangle & operator *= (double a) noexcept;
			Rectangle & operator /= (double a) noexcept;
			Rectangle operator - (void) const noexcept;

			bool friend operator == (const Rectangle & a, const Rectangle & b) noexcept;
			bool friend operator != (const Rectangle & a, const Rectangle & b) noexcept;

			bool IsValid(void) const noexcept;

			static Rectangle Entire() noexcept;
			static Rectangle Invalid() noexcept;
		};
		class Point
		{
		public:
			int x, y;

			Point(void) noexcept;
			Point(int X, int Y) noexcept;

			bool friend operator == (const Point & a, const Point & b) noexcept;
			bool friend operator != (const Point & a, const Point & b) noexcept;
		};
		class Box
		{
		public:
			int Left, Top, Right, Bottom;

			Box(void) noexcept;
			Box(const Rectangle & source, const Box & outer) noexcept;
			Box(int left, int top, int right, int bottom) noexcept;

			bool friend operator == (const Box & a, const Box & b) noexcept;
			bool friend operator != (const Box & a, const Box & b) noexcept;

			bool IsInside(const Point & p) const noexcept;

			static Box Intersect(const Box & a, const Box & b) noexcept;
		};
		class Color
		{
		public:
			union {
				struct { uint8 r, g, b, a; };
				uint32 Value;
			};

			Color(void) noexcept;
			Color(uint8 sr, uint8 sg, uint8 sb, uint8 sa = 0xFF) noexcept;
			Color(int sr, int sg, int sb, int sa = 0xFF) noexcept;
			Color(float sr, float sg, float sb, float sa = 1.0) noexcept;
			Color(double sr, double sg, double sb, double sa = 1.0) noexcept;
			Color(uint32 code) noexcept;

			operator uint32 (void) const noexcept;

			bool friend operator == (const Color & a, const Color & b) noexcept;
			bool friend operator != (const Color & a, const Color & b) noexcept;
		};
		class GradientPoint
		{
		public:
			Color Color;
			double Position;

			GradientPoint(void) noexcept;
			GradientPoint(const UI::Color & color) noexcept;
			GradientPoint(const UI::Color & color, double position) noexcept;

			bool friend operator == (const GradientPoint & a, const GradientPoint & b) noexcept;
			bool friend operator != (const GradientPoint & a, const GradientPoint & b) noexcept;
		};
	
		class IBarRenderingInfo : public Object
		{
		public:
			virtual ~IBarRenderingInfo(void);
		};
		class IBlurEffectRenderingInfo : public Object
		{
		public:
			virtual ~IBlurEffectRenderingInfo(void);
		};
		class IInversionEffectRenderingInfo : public Object
		{
		public:
			virtual ~IInversionEffectRenderingInfo(void);
		};
		class ITextureRenderingInfo : public Object
		{
		public:
			virtual ~ITextureRenderingInfo(void);
		};
		class ITextRenderingInfo : public Object
		{
		public:
			virtual void GetExtent(int & width, int & height) noexcept = 0;
			virtual void SetHighlightColor(const Color & color) noexcept = 0;
			virtual void HighlightText(int Start, int End) noexcept = 0;
			virtual int TestPosition(int point) noexcept = 0;
			virtual int EndOfChar(int Index) noexcept = 0;
			virtual int GetStringLength(void) noexcept = 0;
			virtual void SetCharPalette(const Array<Color> & colors) = 0;
			virtual void SetCharColors(const Array<uint8> & indicies) = 0;
			virtual void SetCharAdvances(const double * advances) = 0;
			virtual void GetCharAdvances(double * advances) noexcept = 0;
			virtual ~ITextRenderingInfo(void);
		};

		class ITexture : public Object
		{
		public:
			virtual int GetWidth(void) const noexcept = 0;
			virtual int GetHeight(void) const noexcept = 0;
			virtual void VersionWasDestroyed(ITexture * texture) noexcept = 0;
			virtual void DeviceWasDestroyed(IRenderingDevice * device) noexcept = 0;
			virtual void AddDeviceVersion(IRenderingDevice * device, ITexture * texture) noexcept = 0;
			virtual bool IsDeviceSpecific(void) const noexcept = 0;
			virtual IRenderingDevice * GetParentDevice(void) const noexcept = 0;
			virtual ITexture * GetDeviceVersion(IRenderingDevice * target_device) noexcept = 0;
			virtual void Reload(Codec::Frame * source) = 0;
			virtual void Reload(ITexture * device_independent) = 0;
		};
		class IFont : public Object
		{
		public:
			virtual int GetWidth(void) const noexcept = 0;
			virtual int GetHeight(void) const noexcept = 0;
			virtual int GetLineSpacing(void) const noexcept = 0;
			virtual int GetBaselineOffset(void) const noexcept = 0;
		};
		class IResourceLoader : public Object
		{
		public:
			virtual ITexture * LoadTexture(Codec::Frame * source) noexcept = 0;
			virtual IFont * LoadFont(const string & face_name, int height, int weight, bool italic, bool underline, bool strikeout) noexcept = 0;
			virtual Array<string> * GetFontFamilies(void) noexcept = 0;
		};
		class IObjectFactory : public IResourceLoader
		{
		public:
			virtual ITextureRenderingDevice * CreateTextureRenderingDevice(int width, int height, Color color) noexcept = 0;
			virtual ITextureRenderingDevice * CreateTextureRenderingDevice(Codec::Frame * frame) noexcept = 0;
		};
		class IRenderingDevice : public IObjectFactory
		{
		public:
			virtual void TextureWasDestroyed(ITexture * texture) noexcept = 0;

			virtual void GetImplementationInfo(string & tech, uint32 & version) noexcept = 0;
			virtual uint32 GetFeatureList(void) noexcept = 0;

			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint> & gradient, double angle) noexcept = 0;
			virtual IBarRenderingInfo * CreateBarRenderingInfo(Color color) noexcept = 0;
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) noexcept = 0;
			virtual IInversionEffectRenderingInfo * CreateInversionEffectRenderingInfo(void) noexcept = 0;
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(ITexture * texture, const Box & take_area, bool fill_pattern) noexcept = 0;
			virtual ITextureRenderingInfo * CreateTextureRenderingInfo(Graphics::ITexture * texture) noexcept = 0;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const string & text, int horizontal_align, int vertical_align, const Color & color) noexcept = 0;
			virtual ITextRenderingInfo * CreateTextRenderingInfo(IFont * font, const Array<uint32> & text, int horizontal_align, int vertical_align, const Color & color) noexcept = 0;

			virtual Graphics::ITexture * CreateIntermediateRenderTarget(Graphics::PixelFormat format, int width, int height) = 0;

			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void RenderTexture(ITextureRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void RenderText(ITextRenderingInfo * Info, const Box & At, bool Clip) noexcept = 0;
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) noexcept = 0;
			virtual void ApplyInversion(IInversionEffectRenderingInfo * Info, const Box & At, bool Blink) noexcept = 0;

			virtual void DrawPolygon(const Math::Vector2 * points, int count, Color color, double width) noexcept = 0;
			virtual void FillPolygon(const Math::Vector2 * points, int count, Color color) noexcept = 0;

			virtual void PushClip(const Box & Rect) noexcept = 0;
			virtual void PopClip(void) noexcept = 0;
			virtual void BeginLayer(const Box & Rect, double Opacity) noexcept = 0;
			virtual void EndLayer(void) noexcept = 0;
			virtual void SetTimerValue(uint32 time) noexcept = 0;
			virtual uint32 GetCaretBlinkHalfTime(void) noexcept = 0;
			virtual bool CaretShouldBeVisible(void) noexcept = 0;
			virtual void ClearCache(void) noexcept = 0;
			virtual ~IRenderingDevice(void);
		};
		class ITextureRenderingDevice : public IRenderingDevice
		{
		public:
			virtual void BeginDraw(void) noexcept = 0;
			virtual void EndDraw(void) noexcept = 0;
			virtual ITexture * GetRenderTargetAsTexture(void) noexcept = 0;
			virtual Codec::Frame * GetRenderTargetAsFrame(void) noexcept = 0;
		};

		IObjectFactory * CreateObjectFactory(void);
	}
}