#include "OverlappedWindows.h"

#include "GroupControls.h"
#include "ButtonControls.h"
#include "StaticControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			OverlappedWindow::OverlappedWindow(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station), _callback(0), _visible(false), _enabled(true), _mode(0), ControlPosition(Rectangle::Invalid())
			{
				_inner = Station->CreateWindow<ContentFrame>(this);
			}
			OverlappedWindow::OverlappedWindow(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) :
				ParentWindow(Parent, Station), _callback(0), _visible(false), _enabled(true), _mode(0), ControlPosition(Rectangle::Invalid())
			{
				_inner = Station->CreateWindow<ContentFrame>(this, Template);
			}
			OverlappedWindow::~OverlappedWindow(void) {}
			void OverlappedWindow::Render(const Box & at)
			{
				ParentWindow::Render(at);
			}
			void OverlappedWindow::ResetCache(void)
			{
				ParentWindow::ResetCache();
			}
			void OverlappedWindow::Enable(bool enable) { _enabled = enable; }
			bool OverlappedWindow::IsEnabled(void) { return _enabled; }
			void OverlappedWindow::Show(bool visible) { _visible = visible; }
			bool OverlappedWindow::IsVisible(void) { return _visible; }
			bool OverlappedWindow::IsOverlapped(void) { return true; }
			void OverlappedWindow::SetPosition(const Box & box) { ParentWindow::SetPosition(box); if (_callback) _callback->OnFrameEvent(this, FrameEvent::Move); }
			void OverlappedWindow::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle OverlappedWindow::GetRectangle(void) { return ControlPosition; }
			void OverlappedWindow::SetText(const string & text) { _inner->Title = text; ResetCache(); }
			string OverlappedWindow::GetText(void) { return _inner->Title; }
			void OverlappedWindow::RaiseEvent(int ID, Event event, Window * sender)
			{
#pragma message("FILTHER")
				if (_callback) _callback->OnControlEvent(this, ID, event, sender);
			}
			void OverlappedWindow::CaptureChanged(bool got_capture)
			{
			}
			void OverlappedWindow::LeftButtonDown(Point at)
			{
			}
			void OverlappedWindow::LeftButtonUp(Point at)
			{
			}
			void OverlappedWindow::LeftButtonDoubleClick(Point at)
			{
			}
			void OverlappedWindow::MouseMove(Point at)
			{
			}
			void OverlappedWindow::PopupMenuCancelled(void) { if (_callback) _callback->OnFrameEvent(this, FrameEvent::PopupMenuCancelled); }
			void OverlappedWindow::SetCursor(Point at)
			{
#pragma message("TEMPORARY")
				Window::SetCursor(at);
			}
			ContentFrame * OverlappedWindow::GetContentFrame(void) { return _inner; }
			IWindowEventCallback * OverlappedWindow::GetCallback(void) { return _callback; }
			void OverlappedWindow::SetCallback(IWindowEventCallback * callback) { _callback = callback; }

			ContentFrame::ContentFrame(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ContentFrame::ContentFrame(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"DialogFrame") throw InvalidArgumentException();
				static_cast<Template::Controls::DialogFrame &>(*this) = static_cast<Template::Controls::DialogFrame &>(*Template->Properties);
				Constructor::ConstructChildren(this, Template);
			}
			ContentFrame::~ContentFrame(void) {}
			void ContentFrame::Render(const Box & at)
			{
				if (Background) {
					if (!_background) {
						_background.SetReference(Background->Initialize(&ZeroArgumentProvider()));
						_background->Render(GetStation()->GetRenderingDevice(), at);
					} else _background->Render(GetStation()->GetRenderingDevice(), at);
				} else if (DefaultBackground) {
					if (!_background && GetStation()->GetVisualStyles().WindowDefaultBackground) {
						_background.SetReference(GetStation()->GetVisualStyles().WindowDefaultBackground->Initialize(&ZeroArgumentProvider()));
					}
					if (_background) _background->Render(GetStation()->GetRenderingDevice(), at);
				} else {
					if (!_background) {
						_background.SetReference(new BarShape(Rectangle::Entire(), BackgroundColor));
					}
					_background->Render(GetStation()->GetRenderingDevice(), at);
				}
				ParentWindow::Render(at);
			}
			void ContentFrame::ResetCache(void) { _background.SetReference(0); ParentWindow::ResetCache(); }
			void ContentFrame::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle ContentFrame::GetRectangle(void) { return ControlPosition; }

			namespace Constructor
			{
				void ConstructChildren(Window * on, Template::ControlTemplate * source)
				{
					for (int i = 0; i < source->Children.Length(); i++) {
						// Group controls
						if (source->Children[i].Properties->GetTemplateClass() == L"ControlGroup") on->GetStation()->CreateWindow<ControlGroup>(on, &source->Children[i]);
						// Button controls
						else if (source->Children[i].Properties->GetTemplateClass() == L"Button") on->GetStation()->CreateWindow<Button>(on, &source->Children[i]);
						// Static controls
						else if (source->Children[i].Properties->GetTemplateClass() == L"Static") on->GetStation()->CreateWindow<Static>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"ColorView") on->GetStation()->CreateWindow<ColorView>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"ProgressBar") on->GetStation()->CreateWindow<ProgressBar>(on, &source->Children[i]);
#pragma message("REALIZE ALL CONTROLS")
						else throw InvalidArgumentException();
					}
				}
			}
		}
		namespace Windows
		{
			Controls::OverlappedWindow * CreateFramelessDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, Rectangle Position, WindowStation * Station)
			{
				auto window = Station->CreateWindow<Controls::OverlappedWindow>(0, Template);
				Rectangle sizes = Template->Properties->ControlPosition;
				sizes.Right -= sizes.Left;
				sizes.Bottom -= sizes.Top;
				window->GetContentFrame()->SetRectangle(Rectangle::Entire());
				if (Position.IsValid()) {
					window->SetRectangle(Position);
				} else {
					window->SetRectangle(Rectangle(Coordinate(0, 0.0, 0.5) - sizes.Right / 2.0, Coordinate(0, 0.0, 0.5) - sizes.Bottom / 2.0,
						Coordinate(0, 0.0, 0.5) - sizes.Right / 2.0 + sizes.Right, Coordinate(0, 0.0, 0.5) - sizes.Bottom / 2.0 + sizes.Bottom));
				}
				window->SetCallback(Callback);
				if (Callback) Callback->OnInitialized(window);
				window->Show(true);
				return window;
			}
		}
	}
}