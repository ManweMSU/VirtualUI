#include "Drawing.h"

#include "../Interfaces/KeyCodes.h"
#include "../Interfaces/SystemGraphics.h"

namespace Engine
{
	namespace Drawing
	{
		Canvas::Canvas(int width, int height, const Color & color) : res_x(width), res_y(height)
		{
			SafePointer<Graphics::I2DDeviceContextFactory> factory = Graphics::CreateDeviceContextFactory();
			bitmap = factory->CreateBitmap(width, height, color);
			device = factory->CreateBitmapContext();
			drawing = false;
		}
		Canvas::Canvas(Codec::Frame * frame)
		{
			SafePointer<Graphics::I2DDeviceContextFactory> factory = Graphics::CreateDeviceContextFactory();
			bitmap = factory->LoadBitmap(frame);
			device = factory->CreateBitmapContext();
			drawing = false;
			res_x = frame->GetWidth();
			res_y = frame->GetHeight();
		}
		Canvas::~Canvas(void) {}
		void Canvas::DrawImage(const Point & left_top, const Point & right_bottom, Picture * picture)
		{
			if (!drawing) throw InvalidStateException();
			if (!picture) throw InvalidArgumentException();
			SafePointer<Graphics::IBitmapBrush> info = device->CreateBitmapBrush(picture, Box(0, 0, picture->GetWidth(), picture->GetHeight()), false);
			device->Render(info, Box(left_top.x, left_top.y, right_bottom.x, right_bottom.y));
		}
		void Canvas::DrawText(const Point & at, const string & text, Font * font, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			if (!font) throw InvalidArgumentException();
			SafePointer<Graphics::ITextBrush> info = device->CreateTextBrush(font, text, 0, 0, color);
			int w, h;
			info->GetExtents(w, h);
			device->Render(info, Box(at.x - w / 2, at.y - h / 2, at.x + w - w / 2, at.y + h - h / 2), false);
		}
		void Canvas::DrawLine(const Point & from, const Point & to, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			Math::Vector2 points[] = { Math::Vector2(from.x, from.y), Math::Vector2(to.x, to.y) };
			device->RenderPolyline(points, 2, color, width);
		}
		void Canvas::DrawPolyline(const Point * verticies, int verticies_count, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			Array<Math::Vector2> points(1);
			points.SetLength(verticies_count);
			for (int i = 0; i < verticies_count; i++) points[i] = Math::Vector2(verticies[i].x, verticies[i].y);
			device->RenderPolyline(points, verticies_count, color, width);
		}
		void Canvas::DrawPolygon(const Point * verticies, int verticies_count, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			Array<Math::Vector2> points(1);
			points.SetLength(verticies_count);
			for (int i = 0; i < verticies_count; i++) points[i] = Math::Vector2(verticies[i].x, verticies[i].y);
			device->RenderPolygon(points, verticies_count, color);
		}
		void Canvas::DrawRectangleOutline(const Point & left_top, const Point & right_bottom, const Color & color, double width)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			Math::Vector2 points[] = {
				Math::Vector2(left_top.x, left_top.y),
				Math::Vector2(right_bottom.x, left_top.y),
				Math::Vector2(right_bottom.x, right_bottom.y),
				Math::Vector2(left_top.x, right_bottom.y),
				Math::Vector2(left_top.x, left_top.y)
			};
			device->RenderPolyline(points, 5, color, width);
		}
		void Canvas::DrawRectangleInterior(const Point & left_top, const Point & right_bottom, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			if (left_top.x > right_bottom.x) return;
			if (left_top.y > right_bottom.y) return;
			Math::Vector2 points[] = {
				Math::Vector2(left_top.x, left_top.y),
				Math::Vector2(right_bottom.x, left_top.y),
				Math::Vector2(right_bottom.x, right_bottom.y),
				Math::Vector2(left_top.x, right_bottom.y)
			};
			device->RenderPolygon(points, 4, color);
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
			Array<Math::Vector2> points(0x100);
			for (double a = 0.0; a < 2.0 * ENGINE_PI; a += da) {
				points << Math::Vector2(xc + xha * Math::cos(a), yc + yha * Math::sin(a));
			}
			points << points[0];
			device->RenderPolyline(points.GetBuffer(), points.Length(), color, width);
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
			Array<Math::Vector2> points(0x100);
			for (double a = 0.0; a < 2.0 * ENGINE_PI; a += da) points << Math::Vector2(xc + xha * Math::cos(a), yc + yha * Math::sin(a));
			points << points[0];
			device->RenderPolygon(points.GetBuffer(), points.Length(), color);
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
			Array<Math::Vector2> points(0x100);
			points << Math::Vector2(right_bottom.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(right_bottom.x - radius + radius * Math::cos(a), left_top.y + radius - radius * Math::sin(a));
			}
			points << Math::Vector2(right_bottom.x - radius, left_top.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(left_top.x + radius - radius * Math::sin(a), left_top.y + radius - radius * Math::cos(a));
			}
			points << Math::Vector2(left_top.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(left_top.x + radius - radius * Math::cos(a), right_bottom.y - radius + radius * Math::sin(a));
			}
			points << Math::Vector2(left_top.x + radius, right_bottom.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(right_bottom.x - radius + radius * Math::sin(a), right_bottom.y - radius + radius * Math::cos(a));
			}
			points << Math::Vector2(right_bottom.x, right_bottom.y - radius);
			points << points[0];
			device->RenderPolyline(points.GetBuffer(), points.Length(), color, width);
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
			Array<Math::Vector2> points(0x100);
			points << Math::Vector2(right_bottom.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(right_bottom.x - radius + radius * Math::cos(a), left_top.y + radius - radius * Math::sin(a));
			}
			points << Math::Vector2(right_bottom.x - radius, left_top.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(left_top.x + radius - radius * Math::sin(a), left_top.y + radius - radius * Math::cos(a));
			}
			points << Math::Vector2(left_top.x, left_top.y + radius);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(left_top.x + radius - radius * Math::cos(a), right_bottom.y - radius + radius * Math::sin(a));
			}
			points << Math::Vector2(left_top.x + radius, right_bottom.y);
			for (double a = da; a < ENGINE_PI / 2.0; a += da) {
				points << Math::Vector2(right_bottom.x - radius + radius * Math::sin(a), right_bottom.y - radius + radius * Math::cos(a));
			}
			points << Math::Vector2(right_bottom.x, right_bottom.y - radius);
			points << points[0];
			device->RenderPolygon(points.GetBuffer(), points.Length(), color);
		}
		int Canvas::GetWidth(void) const noexcept { return res_x; }
		int Canvas::GetHeight(void) const noexcept { return res_y; }
		void Canvas::BeginDraw(void)
		{
			if (drawing) throw InvalidStateException();
			if (!device->BeginRendering(bitmap)) throw Exception();
			drawing = true;
		}
		void Canvas::EndDraw(void)
		{
			if (!drawing) throw InvalidStateException();
			if (!device->EndRendering()) throw Exception();
			drawing = false;
		}
		Codec::Frame * Canvas::GetCanvas(void)
		{
			if (drawing) throw InvalidStateException();
			return bitmap->QueryFrame();
		}
		Picture * Canvas::LoadPicture(Codec::Frame * frame)
		{
			Picture * image = device->GetParentFactory()->LoadBitmap(frame);
			if (!image) throw OutOfMemoryException();
			return image;
		}
		Picture * Canvas::LoadPicture(Streaming::Stream * stream)
		{
			SafePointer<Codec::Frame> frame = Codec::DecodeFrame(stream);
			if (!frame) throw InvalidFormatException();
			return LoadPicture(frame);
		}
		Font * Canvas::LoadFont(const string & face, int height, bool bold, bool italic, bool underlined, bool strikedout)
		{
			Font * font = device->GetParentFactory()->LoadFont(face, height, bold ? 700 : 400, italic, underlined, strikedout);
			if (!font) font = device->GetParentFactory()->LoadFont(Graphics::SystemSansSerifFont, height, bold ? 700 : 400, italic, underlined, strikedout);
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
		
		CanvasWindow::CanvasWindow(void) : CanvasWindow(Color(0, 0, 0)) {}
		CanvasWindow::CanvasWindow(const Color & color) : CanvasWindow(color, 640, 480) {}
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y) : CanvasWindow(color, res_x, res_y, SafePointer<Windows::IScreen>(Windows::GetDefaultScreen())->GetDpiScale()) {}
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y, double window_scale_factor) : Canvas(res_x, res_y, color)
		{
			if (res_x < 1 || res_y < 1 || window_scale_factor < 1.0) throw InvalidArgumentException();
			SafePointer<Windows::IScreen> screen = Windows::GetDefaultScreen();
			SafePointer<CanvasCallback> _callback = new CanvasCallback(this);
			callback.SetRetain(_callback);
			Windows::CreateWindowDesc desc;
			desc.Flags = Windows::WindowFlagCloseButton | Windows::WindowFlagHasTitle | Windows::WindowFlagMinimizeButton;
			desc.Callback = _callback;
			desc.MinimalConstraints = desc.MaximalConstraints = Point(0, 0);
			desc.ParentWindow = 0;
			auto resolution = screen->GetResolution();
			auto size = Windows::GetWindowSystem()->ConvertClientToWindow(Point(int(res_x * window_scale_factor), int(res_y * window_scale_factor)), desc.Flags);
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
			if (res_x < 1 || res_y < 1 || window_width < 1 || window_height < 1) throw InvalidArgumentException();
			SafePointer<Windows::IScreen> screen = Windows::GetDefaultScreen();
			SafePointer<CanvasCallback> _callback = new CanvasCallback(this);
			callback.SetRetain(_callback);
			Windows::CreateWindowDesc desc;
			desc.Flags = Windows::WindowFlagCloseButton | Windows::WindowFlagHasTitle | Windows::WindowFlagMinimizeButton;
			desc.Callback = _callback;
			desc.MinimalConstraints = desc.MaximalConstraints = Point(0, 0);
			desc.ParentWindow = 0;
			auto resolution = screen->GetResolution();
			auto size = Windows::GetWindowSystem()->ConvertClientToWindow(Point(window_width, window_height), desc.Flags);
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