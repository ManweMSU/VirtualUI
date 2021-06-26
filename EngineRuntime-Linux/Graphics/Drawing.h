#pragma once

#include "GraphicsBase.h"
#include "../Interfaces/SystemWindows.h"
#include "../Math/Color.h"

namespace Engine
{
	namespace Drawing
	{
		typedef UI::ITexture Image;
		typedef UI::IFont Font;
		typedef Math::Vector2 Point;
		typedef Math::Color Color;
		typedef Math::StandardColor StandardColor;
		typedef Math::CGAColor CGAColor;

		class Canvas : public Object
		{
		protected:
			SafePointer<UI::ITextureRenderingDevice> device;
			bool drawing;
			int res_x, res_y;
		public:
			Canvas(int width, int height, const Color & color);
			Canvas(Codec::Frame * frame);
			virtual ~Canvas(void) override;

			void DrawImage(const Point & left_top, const Point & right_bottom, Image * image);
			void DrawText(const Point & at, const string & text, Font * font, const Color & color);
			void DrawLine(const Point & from, const Point & to, const Color & color, double width);
			void DrawPolyline(const Point * verticies, int verticies_count, const Color & color, double width);
			void DrawPolygon(const Point * verticies, int verticies_count, const Color & color);
			void DrawRectangleOutline(const Point & left_top, const Point & right_bottom, const Color & color, double width);
			void DrawRectangleInterior(const Point & left_top, const Point & right_bottom, const Color & color);
			void DrawEllipseOutline(const Point & left_top, const Point & right_bottom, const Color & color, double width);
			void DrawEllipseInterior(const Point & left_top, const Point & right_bottom, const Color & color);
			void DrawRoundedRectangleOutline(const Point & left_top, const Point & right_bottom, double radius, const Color & color, double width);
			void DrawRoundedRectangleInterior(const Point & left_top, const Point & right_bottom, double radius, const Color & color);

			int GetWidth(void) const noexcept;
			int GetHeight(void) const noexcept;

			virtual void BeginDraw(void);
			virtual void EndDraw(void);

			Codec::Frame * GetCanvas(void);
			
			Image * LoadImage(Codec::Frame * frame);
			Image * LoadImage(Streaming::Stream * stream);
			Font * LoadFont(const string & face, int height, bool bold = false, bool italic = false, bool underlined = false, bool strikedout = false);
		};

		class CanvasWindow : public Canvas
		{
			Windows::IWindow * window;
			SafePointer<Object> callback;
		public:
			CanvasWindow(void);
			CanvasWindow(const Color & color);
			CanvasWindow(const Color & color, int res_x, int res_y);
			CanvasWindow(const Color & color, int res_x, int res_y, double window_scale_factor);
			CanvasWindow(const Color & color, int res_x, int res_y, int window_width, int window_height);
			virtual ~CanvasWindow(void) override;

			virtual void BeginDraw(void) override;
			virtual void EndDraw(void) override;

			void SetTitle(const string & text);
			uint32 ReadKey(void);
		};
	}
}