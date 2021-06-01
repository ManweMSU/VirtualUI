#include "Drawing.h"

#include "../UserInterface/OverlappedWindows.h"
#include "../PlatformDependent/KeyCodes.h"

namespace Engine
{
	namespace Drawing
	{
		Color GetStandardColor(StandardColor color)
		{
			if (color == StandardColor::Null) return Color(0.0, 0.0, 0.0, 0.0);
			else if (color == StandardColor::Black) return Color(0.0, 0.0, 0.0);
			else if (color == StandardColor::DarkBlue) return Color(0.0, 0.0, 0.5);
			else if (color == StandardColor::DarkGreen) return Color(0.0, 0.5, 0.0);
			else if (color == StandardColor::DarkCyan) return Color(0.0, 0.5, 0.5);
			else if (color == StandardColor::DarkRed) return Color(0.5, 0.0, 0.0);
			else if (color == StandardColor::DarkMagenta) return Color(0.5, 0.0, 0.5);
			else if (color == StandardColor::DarkYellow) return Color(0.5, 0.5, 0.0);
			else if (color == StandardColor::Gray) return Color(0.75, 0.75, 0.75);
			else if (color == StandardColor::DarkGray) return Color(0.5, 0.5, 0.5);
			else if (color == StandardColor::Blue) return Color(0.0, 0.0, 1.0);
			else if (color == StandardColor::Green) return Color(0.0, 1.0, 0.0);
			else if (color == StandardColor::Cyan) return Color(0.0, 1.0, 1.0);
			else if (color == StandardColor::Red) return Color(1.0, 0.0, 0.0);
			else if (color == StandardColor::Magenta) return Color(1.0, 0.0, 1.0);
			else if (color == StandardColor::Yellow) return Color(1.0, 1.0, 0.0);
			else if (color == StandardColor::White) return Color(1.0, 1.0, 1.0);
			else throw InvalidArgumentException();
		}

		Canvas::Canvas(int width, int height, const Color & color)
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
		}
		Canvas::~Canvas(void) {}
		void Canvas::DrawImage(const Point & left_top, const Point & right_bottom, Image * image)
		{
			if (!drawing) throw InvalidStateException();
			SafePointer<UI::ITextureRenderingInfo> info = device->CreateTextureRenderingInfo(image,
				UI::Box(0, 0, image->GetWidth(), image->GetHeight()), false);
			device->RenderTexture(info, UI::Box(left_top.x, left_top.y, right_bottom.x, right_bottom.y));
		}
		void Canvas::DrawText(const Point & at, const string & text, Font * font, const Color & color)
		{
			if (!drawing) throw InvalidStateException();
			SafePointer<UI::ITextRenderingInfo> info = device->CreateTextRenderingInfo(font, text, 0, 0, color);
			int w, h;
			info->GetExtent(w, h);
			device->RenderText(info, UI::Box(at.x - w / 2, at.y - h / 2, at.x + w / 2, at.y + h / 2), false);
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
		
		class CanvasView : public UI::Window, public UI::Windows::IWindowEventCallback
		{
			SafePointer<Image> image;
			SafePointer<UI::ITextureRenderingInfo> info;
		public:
			Array<uint32> key_pressed_pool;
			CanvasView(UI::Window * parent, UI::WindowStation * station) : Window(parent, station), key_pressed_pool(0x100) {}
			virtual ~CanvasView(void) override {}

			virtual void Render(const UI::Box & at)
			{
				auto device = GetRenderingDevice();
				if (!info) info = device->CreateTextureRenderingInfo(image, UI::Box(0, 0, image->GetWidth(), image->GetHeight()), false);
				if (info) device->RenderTexture(info, at);
			}
			virtual void ResetCache(void) override { info.SetReference(0); }
			virtual UI::Rectangle GetRectangle(void) override { return UI::Rectangle::Entire(); }
			virtual bool KeyDown(int key_code) override
			{
				key_pressed_pool << key_code;
				UI::Windows::ExitMessageLoop();
				return true;
			}
			virtual void OnInitialized(UI::Window * window) override {}
			virtual void OnControlEvent(UI::Window * window, int ID, UI::Window::Event event, UI::Window * sender) override {}
			virtual void OnFrameEvent(UI::Window * window, UI::Windows::FrameEvent event) override
			{
				if (event == UI::Windows::FrameEvent::Activate) {
					SetFocus();
				} else if (event == UI::Windows::FrameEvent::Close) {
					key_pressed_pool << KeyCodes::Escape;
					UI::Windows::ExitMessageLoop();
				}
			}
			void UpdateImage(Image * frame)
			{
				image.SetRetain(frame);
				info.SetReference(0);
			}
		};
		
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y) : Canvas(res_x, res_y, color)
		{
			SafePointer<UI::Template::ControlTemplate> frame_template = UI::Controls::CreateOverlappedWindowTemplate(
				IO::Path::GetFileName(IO::GetExecutablePath()), UI::Rectangle(0, 0, res_x, res_y),
				UI::Controls::OverlappedWindowCaptioned | UI::Controls::OverlappedWindowCloseButton |
				UI::Controls::OverlappedWindowMinimizeButton);
			window = UI::Windows::CreateFramedDialog(frame_template, 0, UI::Rectangle::Entire());
			view = window->GetStation()->CreateWindow<CanvasView>(window->As<UI::Controls::OverlappedWindow>()->GetContentFrame());
			window->As<UI::Controls::OverlappedWindow>()->GetContentFrame()->ArrangeChildren();
			SafePointer<Codec::Frame> initial = GetCanvas();
			SafePointer<Image> image = LoadImage(initial);
			view->As<CanvasView>()->UpdateImage(image);
			window->As<UI::Controls::OverlappedWindow>()->SetCallback(view->As<CanvasView>());
			window->Show(true);
		}
		CanvasWindow::CanvasWindow(const Color & color, int res_x, int res_y, int window_width, int window_height) : Canvas(res_x, res_y, color)
		{
			SafePointer<UI::Template::ControlTemplate> frame_template = UI::Controls::CreateOverlappedWindowTemplate(
				IO::Path::GetFileName(IO::GetExecutablePath()), UI::Rectangle(0, 0, window_width, window_height),
				UI::Controls::OverlappedWindowCaptioned | UI::Controls::OverlappedWindowCloseButton |
				UI::Controls::OverlappedWindowMinimizeButton);
			window = UI::Windows::CreateFramedDialog(frame_template, 0, UI::Rectangle::Entire());
			view = window->GetStation()->CreateWindow<CanvasView>(window->As<UI::Controls::OverlappedWindow>()->GetContentFrame());
			window->As<UI::Controls::OverlappedWindow>()->GetContentFrame()->ArrangeChildren();
			SafePointer<Codec::Frame> initial = GetCanvas();
			SafePointer<Image> image = LoadImage(initial);
			view->As<CanvasView>()->UpdateImage(image);
			window->As<UI::Controls::OverlappedWindow>()->SetCallback(view->As<CanvasView>());
			window->Show(true);
		}
		CanvasWindow::~CanvasWindow(void) { window->Destroy(); }
		void CanvasWindow::BeginDraw(void) { Canvas::BeginDraw(); }
		void CanvasWindow::EndDraw(void)
		{
			Canvas::EndDraw();
			SafePointer<Codec::Frame> frame = GetCanvas();
			SafePointer<Image> image = LoadImage(frame);
			view->As<CanvasView>()->UpdateImage(image);
			window->RequireRedraw();
		}
		void CanvasWindow::SetTitle(const string & text) { window->SetText(text); }
		uint32 CanvasWindow::ReadKey(void)
		{
			if (drawing) throw InvalidStateException();
			auto _view = view->As<CanvasView>();
			while (true) {
				if (!_view->key_pressed_pool.Length()) UI::Windows::RunMessageLoop();
				if (_view->key_pressed_pool.Length()) {
					uint32 result = _view->key_pressed_pool[0];
					_view->key_pressed_pool.RemoveFirst();
					return result;
				}
			}
		}
	}
}