#include "Drawing.h"

#include "../Interfaces/KeyCodes.h"

namespace Engine
{
	namespace Drawing
	{
		Canvas::Canvas(int width, int height, const Color & color) : res_x(width), res_y(height)
		{
			SafePointer<UI::IObjectFactory> factory = UI::CreateObjectFactory();
			device = factory->CreateTextureRenderingDevice(width, height, color);
			drawing = false;
		}
		Canvas::Canvas(Codec::Frame * frame)
		{
			SafePointer<UI::IObjectFactory> factory = UI::CreateObjectFactory();
			device = factory->CreateTextureRenderingDevice(frame);
			drawing = false;
			res_x = frame->GetWidth();
			res_y = frame->GetHeight();
		}
		Canvas::~Canvas(void) {}
		void Canvas::DrawImage(const Point & left_top, const Point & right_bottom, Image * image)
		{
			if (!drawing) throw InvalidStateException();
			SafePointer<UI::ITextureRenderingInfo> info = device->CreateTextureRenderingInfo(image,
				UI::Box(0, 0, image->GetWidth(), image->GetHeight()), false);
			device->RenderTexture(info, UI::Box(int(left_top.x), int(left_top.y), int(right_bottom.x), int(right_bottom.y)));
		}
		void Canvas::DrawText(const Point & at, const string & text, Font * font, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			SafePointer<UI::ITextRenderingInfo> info = device->CreateTextRenderingInfo(font, text, 0, 0, color);
			int w, h;
			info->GetExtent(w, h);
			device->RenderText(info, UI::Box(int(at.x - w / 2.0), int(at.y - h / 2.0), int(at.x + w / 2.0), int(at.y + h / 2.0)), false);
		}
		void Canvas::DrawLine(const Point & from, const Point & to, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			Point points[] = { from, to };
			device->DrawPolygon(points, 2, color, width);
		}
		void Canvas::DrawPolyline(const Point * verticies, int verticies_count, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			device->DrawPolygon(verticies, verticies_count, color, width);
		}
		void Canvas::DrawPolygon(const Point * verticies, int verticies_count, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			device->FillPolygon(verticies, verticies_count, color);
		}
		void Canvas::DrawRectangleOutline(const Point & left_top, const Point & right_bottom, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			Point points[] = { left_top, Point(right_bottom.x, left_top.y), right_bottom, Point(left_top.x, right_bottom.y), left_top };
			device->DrawPolygon(points, 5, color, width);
		}
		void Canvas::DrawRectangleInterior(const Point & left_top, const Point & right_bottom, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			Point points[] = { left_top, Point(right_bottom.x, left_top.y), right_bottom, Point(left_top.x, right_bottom.y) };
			device->FillPolygon(points, 4, color);
		}
		void Canvas::DrawEllipseOutline(const Point & left_top, const Point & right_bottom, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			double xha = (right_bottom.x - left_top.x) / 2.0;
			double yha = (right_bottom.y - left_top.y) / 2.0;
			double xc = (right_bottom.x + left_top.x) / 2.0;
			double yc = (right_bottom.y + left_top.y) / 2.0;
			double da = 2.0 / (ENGINE_PI * max(max(xha, yha), 1.0));
			Array<Point> points(0x100);
			for (double a = 0.0; a < 2.0 * ENGINE_PI; a += da) {
				points << Point(xc + xha * Math::cos(a), yc + yha * Math::sin(a));
			}
			points << points[0];
			device->DrawPolygon(points.GetBuffer(), points.Length(), color, width);
		}
		void Canvas::DrawEllipseInterior(const Point & left_top, const Point & right_bottom, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			double xha = (right_bottom.x - left_top.x) / 2.0;
			double yha = (right_bottom.y - left_top.y) / 2.0;
			double xc = (right_bottom.x + left_top.x) / 2.0;
			double yc = (right_bottom.y + left_top.y) / 2.0;
			double da = 2.0 / (ENGINE_PI * max(max(xha, yha), 1.0));
			Array<Point> points(0x100);
			for (double a = 0.0; a < 2.0 * ENGINE_PI; a += da) points << Point(xc + xha * Math::cos(a), yc + yha * Math::sin(a));
			points << points[0];
			device->FillPolygon(points.GetBuffer(), points.Length(), color);
		}
		void Canvas::DrawRoundedRectangleOutline(const Point & left_top, const Point & right_bottom, double radius, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			double xha = (right_bottom.x - left_top.x) / 2.0;
			double yha = (right_bottom.y - left_top.y) / 2.0;
			if (radius > xha) radius = xha;
			if (radius > yha) radius = yha;
			double da = 2.0 / (ENGINE_PI * max(radius, 1.0));
			Array<Point> points(0x100);
			points << Point(right_bottom.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(right_bottom.x - radius + radius * Math::cos(a), left_top.y + radius - radius * Math::sin(a));
			}
			points << Point(right_bottom.x - radius, left_top.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(left_top.x + radius - radius * Math::sin(a), left_top.y + radius - radius * Math::cos(a));
			}
			points << Point(left_top.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(left_top.x + radius - radius * Math::cos(a), right_bottom.y - radius + radius * Math::sin(a));
			}
			points << Point(left_top.x + radius, right_bottom.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(right_bottom.x - radius + radius * Math::sin(a), right_bottom.y - radius + radius * Math::cos(a));
			}
			points << Point(right_bottom.x, right_bottom.y - radius);
			points << points[0];
			device->DrawPolygon(points.GetBuffer(), points.Length(), color, width);
		}
		void Canvas::DrawRoundedRectangleInterior(const Point & left_top, const Point & right_bottom, double radius, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			double xha = (right_bottom.x - left_top.x) / 2.0;
			double yha = (right_bottom.y - left_top.y) / 2.0;
			if (radius > xha) radius = xha;
			if (radius > yha) radius = yha;
			double da = 2.0 / (ENGINE_PI * max(radius, 1.0));
			Array<Point> points(0x100);
			points << Point(right_bottom.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(right_bottom.x - radius + radius * Math::cos(a), left_top.y + radius - radius * Math::sin(a));
			}
			points << Point(right_bottom.x - radius, left_top.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(left_top.x + radius - radius * Math::sin(a), left_top.y + radius - radius * Math::cos(a));
			}
			points << Point(left_top.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(left_top.x + radius - radius * Math::cos(a), right_bottom.y - radius + radius * Math::sin(a));
			}
			points << Point(left_top.x + radius, right_bottom.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Point(right_bottom.x - radius + radius * Math::sin(a), right_bottom.y - radius + radius * Math::cos(a));
			}
			points << Point(right_bottom.x, right_bottom.y - radius);
			points << points[0];
			device->FillPolygon(points.GetBuffer(), points.Length(), color);
		}
		int Canvas::GetWidth(void) const noexcept { return res_x; }
		int Canvas::GetHeight(void) const noexcept { return res_y; }
		void Canvas::BeginDraw(void)
		{
			if (drawing) throw InvalidStateException();
			device->BeginDraw();
			drawing = true;
		}
		void Canvas::EndDraw(void)
		{
			if (!drawing) throw InvalidStateException();
			device->EndDraw();
			drawing = false;
		}
		Codec::Frame * Canvas::GetCanvas(void)
		{
			if (drawing) throw InvalidStateException();
			return device->GetRenderTargetAsFrame();
		}
		Image * Canvas::LoadImage(Codec::Frame * frame)
		{
			Image * image = device->LoadTexture(frame);
			if (!image) throw Exception();
			return image;
		}
		Image * Canvas::LoadImage(Streaming::Stream * stream)
		{
			SafePointer<Codec::Frame> frame = Codec::DecodeFrame(stream);
			if (!frame) throw InvalidFormatException();
			return LoadImage(frame);
		}
		Font * Canvas::LoadFont(const string & face, int height, bool bold, bool italic, bool underlined, bool strikedout)
		{
			Font * font = device->LoadFont(face, height, bold ? 700 : 400, italic, underlined, strikedout);
			if (!font) font = device->LoadFont(L"Tahoma", height, bold ? 700 : 400, italic, underlined, strikedout);
			if (!font) throw Exception();
			return font;
		}
		
		class CanvasCallback : public Windows::IWindowCallback, public Object
		{
			Windows::IWindow * _window;
			CanvasWindow * _owner;
			Array<uint32> _input_queue;
		public:
			CanvasCallback(CanvasWindow * owner) : _window(0), _owner(owner), _input_queue(0x100) {}
			virtual ~CanvasCallback(void) override {}
			virtual void Created(Windows::IWindow * window) { _window = window; }
			virtual void WindowClose(Windows::IWindow * window) override
			{
				_input_queue << KeyCodes::Escape;
				Windows::GetWindowSystem()->ExitMainLoop();
			}
			virtual bool KeyDown(Windows::IWindow * window, int key_code) override
			{
				_input_queue << key_code;
				Windows::GetWindowSystem()->ExitMainLoop();
				return true;
			}
			void UpdateImage(Codec::Frame * frame) { _window->SetBackbufferedRenderingDevice(frame); }
			uint32 DequeueKey(void)
			{
				if (_input_queue.Length()) {
					auto result = _input_queue.FirstElement();
					_input_queue.RemoveFirst();
					return result;
				} else return 0;
			}
		};
		
		CanvasWindow::CanvasWindow(void) : CanvasWindow(Color(0.0, 0.0, 0.0)) {}
		CanvasWindow::CanvasWindow(const Color & color) : CanvasWindow(color, 640, 480) {}
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y) : CanvasWindow(color, res_x, res_y, SafePointer<Windows::IScreen>(Windows::GetDefaultScreen())->GetDpiScale()) {}
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y, double window_scale_factor) : Canvas(res_x, res_y, color)
		{
			SafePointer<Windows::IScreen> screen = Windows::GetDefaultScreen();
			SafePointer<CanvasCallback> _callback = new CanvasCallback(this);
			callback.SetRetain(_callback);
			Windows::CreateWindowDesc desc;
			desc.Flags = Windows::WindowFlagCloseButton | Windows::WindowFlagHasTitle | Windows::WindowFlagMinimizeButton;
			desc.Callback = _callback;
			desc.MinimalConstraints = desc.MaximalConstraints = UI::Point(0, 0);
			desc.ParentWindow = 0;
			auto resolution = screen->GetResolution();
			auto size = Windows::GetWindowSystem()->ConvertClientToWindow(UI::Point(res_x * window_scale_factor, res_y * window_scale_factor), desc.Flags);
			desc.Position.Left = (resolution.x - size.x) / 2;
			desc.Position.Top = (resolution.y - size.y) / 2;
			desc.Position.Right = desc.Position.Left + size.x;
			desc.Position.Bottom = desc.Position.Top + size.y;
			desc.Screen = screen;
			desc.Title = IO::Path::GetFileName(IO::GetExecutablePath());
			window = Windows::GetWindowSystem()->CreateWindow(desc);
			if (!window) throw Exception();
			SafePointer<Codec::Frame> initial = GetCanvas();
			_callback->UpdateImage(initial);
			window->Show(true);
		}
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y, int window_width, int window_height) : Canvas(res_x, res_y, color)
		{
			SafePointer<Windows::IScreen> screen = Windows::GetDefaultScreen();
			SafePointer<CanvasCallback> _callback = new CanvasCallback(this);
			callback.SetRetain(_callback);
			Windows::CreateWindowDesc desc;
			desc.Flags = Windows::WindowFlagCloseButton | Windows::WindowFlagHasTitle | Windows::WindowFlagMinimizeButton;
			desc.Callback = _callback;
			desc.MinimalConstraints = desc.MaximalConstraints = UI::Point(0, 0);
			desc.ParentWindow = 0;
			auto resolution = screen->GetResolution();
			auto size = Windows::GetWindowSystem()->ConvertClientToWindow(UI::Point(window_width, window_height), desc.Flags);
			desc.Position.Left = (resolution.x - size.x) / 2;
			desc.Position.Top = (resolution.y - size.y) / 2;
			desc.Position.Right = desc.Position.Left + size.x;
			desc.Position.Bottom = desc.Position.Top + size.y;
			desc.Screen = screen;
			desc.Title = IO::Path::GetFileName(IO::GetExecutablePath());
			window = Windows::GetWindowSystem()->CreateWindow(desc);
			if (!window) throw Exception();
			SafePointer<Codec::Frame> initial = GetCanvas();
			_callback->UpdateImage(initial);
			window->Show(true);
		}
		CanvasWindow::~CanvasWindow(void) { window->Destroy(); }
		void CanvasWindow::BeginDraw(void) { Canvas::BeginDraw(); }
		void CanvasWindow::EndDraw(void)
		{
			Canvas::EndDraw();
			SafePointer<Codec::Frame> frame = GetCanvas();
			static_cast<CanvasCallback *>(callback.Inner())->UpdateImage(frame);
		}
		void CanvasWindow::SetTitle(const string & text) { window->SetText(text); }
		uint32 CanvasWindow::ReadKey(void)
		{
			if (drawing) throw InvalidStateException();
			auto _callback = static_cast<CanvasCallback *>(callback.Inner());
			while (true) {
				auto key = _callback->DequeueKey();
				if (!key) {
					Windows::GetWindowSystem()->RunMainLoop();
					key = _callback->DequeueKey();
				}
				if (key) return key;
			}
		}
	}
}