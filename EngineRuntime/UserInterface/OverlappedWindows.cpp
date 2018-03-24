#include "OverlappedWindows.h"

#include "GroupControls.h"
#include "ButtonControls.h"
#include "StaticControls.h"
#include "ScrollableControls.h"

using namespace Engine::UI::Windows;

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class OverlappedWindowArgumentProvider : public IArgumentProvider
				{
				public:
					OverlappedWindow * Owner;
					OverlappedWindowArgumentProvider(OverlappedWindow * owner) : Owner(owner) {}
					virtual void GetArgument(const string & name, int * value) override
					{
						if (name == L"Border") *value = Owner->GetBorderWidth();
						else if (name == L"Caption") *value = Owner->GetCaptionWidth();
						else if (name == L"ButtonsWidth") *value = Owner->GetButtonsWidth();
						else *value = 0;
					}
					virtual void GetArgument(const string & name, double * value) override
					{
						if (name == L"Border") *value = double(Owner->GetBorderWidth());
						else if (name == L"Caption") *value = double(Owner->GetCaptionWidth());
						else if (name == L"ButtonsWidth") *value = double(Owner->GetButtonsWidth());
						else *value = 0.0;
					}
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = Owner->GetText();
						else *value = L"";
					}
					virtual void GetArgument(const string & name, ITexture ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, IFont ** value) override { *value = 0; }
				};
			}

			OverlappedWindow::OverlappedWindow(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station), _callback(0), _visible(false), _enabled(true), _mode(0), _border(0), _caption(0), _state(0),
				_minwidth(0), _minheight(0), _btnwidth(0), ControlPosition(Rectangle::Invalid()), _close(0), _maximize(0), _minimize(0), _help(0)
			{
				_inner = Station->CreateWindow<ContentFrame>(this);
			}
			OverlappedWindow::OverlappedWindow(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) :
				ParentWindow(Parent, Station), _callback(0), _visible(false), _enabled(true), _mode(0), _border(0), _caption(0), _state(0), _minwidth(0), _minheight(0), _btnwidth(0),
				ControlPosition(Rectangle::Invalid()), _close(0), _maximize(0), _minimize(0), _help(0)
			{
				_inner = Station->CreateWindow<ContentFrame>(this, Template);
			}
			OverlappedWindow::~OverlappedWindow(void) {}
			void OverlappedWindow::Render(const Box & at)
			{
				if (_mode) {
					Shape ** shape = 0;
					Template::Shape * temp = 0;
					if (GetStation()->GetActiveWindow() == this) {
						shape = _active.InnerRef();
						temp = _inner->ToolWindow ? GetStation()->GetVisualStyles().WindowSmallActiveView : GetStation()->GetVisualStyles().WindowActiveView;
					} else {
						shape = _inactive.InnerRef();
						temp = _inner->ToolWindow ? GetStation()->GetVisualStyles().WindowSmallInactiveView : GetStation()->GetVisualStyles().WindowInactiveView;
					}
					if (!(*shape) && temp) {
						*shape = temp->Initialize(&ArgumentService::OverlappedWindowArgumentProvider(this));
					}
					if (*shape) (*shape)->Render(GetStation()->GetRenderingDevice(), at);
				}
				ParentWindow::Render(at);
			}
			void OverlappedWindow::ResetCache(void)
			{
				_active.SetReference(0);
				_inactive.SetReference(0);
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
				if (event == Event::Command) {
					if (sender == _close) {
						if (_callback) _callback->OnFrameEvent(this, FrameEvent::Close);
						return;
					} else if (sender == _maximize) {
						if (_callback) _callback->OnFrameEvent(this, FrameEvent::Maximize);
						return;
					} else if (sender == _minimize) {
						if (_callback) _callback->OnFrameEvent(this, FrameEvent::Minimize);
						return;
					} else if (sender == _help) {
						if (_callback) _callback->OnFrameEvent(this, FrameEvent::Help);
						return;
					}
				}
				if (_callback) _callback->OnControlEvent(this, ID, event, sender);
			}
			void OverlappedWindow::CaptureChanged(bool got_capture) { if (!got_capture) _state = 0; }
			void OverlappedWindow::LeftButtonDown(Point at)
			{
				int Part = GetPart(at);
				if (Part != -1) {
					_state = 0xFFFF;
					_size.lp = GetStation()->GetCursorPos();
					SetCapture();
				}
			}
			void OverlappedWindow::LeftButtonUp(Point at) { if (_state & 0xF00) ReleaseCapture(); }
			void OverlappedWindow::LeftButtonDoubleClick(Point at)
			{
				int Part = GetPart(at);
				if (Part == 0 && _callback) _callback->OnFrameEvent(this, FrameEvent::Maximize);
			}
			void OverlappedWindow::MouseMove(Point at)
			{
				if (_state & 0xF00) {
					Point np = GetStation()->GetCursorPos();
					auto dp = Point(np.x - _size.lp.x, np.y - _size.lp.y);
					Box New = WindowPosition;
					New.Left += _size.sl * dp.x;
					New.Right += _size.sr * dp.x;
					New.Top += _size.st * dp.y;
					New.Bottom += _size.sb * dp.y;
					if (New.Right - New.Left < _minwidth) {
						int inss = _minwidth - (New.Right - New.Left);
						if (_size.sr) {
							dp.x += inss;
							New.Right = New.Left + _minwidth;
						} else {
							dp.x -= inss;
							New.Left = New.Right - _minwidth;
						}
					}
					if (New.Bottom - New.Top < _minheight) {
						int inss = _minheight - (New.Bottom - New.Top);
						if (_size.sb) {
							dp.y += inss;
							New.Bottom = New.Top + _minheight;
						} else {
							dp.y -= inss;
							New.Top = New.Bottom - _minheight;
						}
					}
					_size.lp = Point(_size.lp.x + dp.x, _size.lp.y + dp.y);
					SetPosition(New);
				} else {
					GetPart(at);
				}
			}
			void OverlappedWindow::PopupMenuCancelled(void) { if (_callback) _callback->OnFrameEvent(this, FrameEvent::PopupMenuCancelled); }
			void OverlappedWindow::SetCursor(Point at)
			{
				SystemCursor Cursor;
				int Part;
				if (_state & 0xF00) {
					Part = _size.part;
				} else {
					Part = GetPart(at);
				}
				if (Part == -1 || Part == 0) Cursor = SystemCursor::Arrow;
				else if (_size.sl && _size.st) Cursor = SystemCursor::SizeLeftUpRightDown;
				else if (_size.sl && _size.sb) Cursor = SystemCursor::SizeLeftDownRightUp;
				else if (_size.sr && _size.st) Cursor = SystemCursor::SizeLeftDownRightUp;
				else if (_size.sr && _size.sb) Cursor = SystemCursor::SizeLeftUpRightDown;
				else if (_size.sl || _size.sr) Cursor = SystemCursor::SizeLeftRight;
				else Cursor = SystemCursor::SizeUpDown;
				GetStation()->SetCursor(GetStation()->GetSystemCursor(Cursor));
			}
			ContentFrame * OverlappedWindow::GetContentFrame(void) { return _inner; }
			IWindowEventCallback * OverlappedWindow::GetCallback(void) { return _callback; }
			void OverlappedWindow::SetCallback(IWindowEventCallback * callback) { _callback = callback; }
			int OverlappedWindow::GetBorderWidth(void) { return _border; }
			int OverlappedWindow::GetCaptionWidth(void) { return _caption; }
			int OverlappedWindow::GetButtonsWidth(void) { return _btnwidth; }
			int OverlappedWindow::GetPart(Point cursor)
			{
				_size.part = -1;
				if (cursor.x < 0 || cursor.y < 0 || cursor.x >= WindowPosition.Right - WindowPosition.Left || cursor.y >= WindowPosition.Bottom - WindowPosition.Top) return -1;
				if (cursor.x >= _border && cursor.y >= _border + _caption && cursor.x < WindowPosition.Right - WindowPosition.Left - _border && cursor.y < WindowPosition.Bottom - WindowPosition.Top - _border) return -1;
				if (cursor.y >= _border && cursor.y < _border + _caption && cursor.x >= _border && cursor.x < WindowPosition.Right - WindowPosition.Left - _border - _btnwidth) {
					_size.sl = 1; _size.st = 1; _size.sr = 1; _size.sb = 1; _size.part = 0;
					return 0;
				}
				if (!_inner->Sizeble) return -1;
				if (cursor.x < _border) { _size.sl = 1; _size.sr = 0; } else if (cursor.x >= WindowPosition.Right - WindowPosition.Left - _border) { _size.sl = 0; _size.sr = 1; } else { _size.sl = 0; _size.sr = 0; }
				if (cursor.y < _border) { _size.st = 1; _size.sb = 0; } else if (cursor.y >= WindowPosition.Bottom - WindowPosition.Top - _border) { _size.st = 0; _size.sb = 1; } else { _size.st = 0; _size.sb = 0; }
				_size.part = 1;
				return 1;
			}

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
						else if (source->Children[i].Properties->GetTemplateClass() == L"RadioButtonGroup") on->GetStation()->CreateWindow<RadioButtonGroup>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"ScrollBox") on->GetStation()->CreateWindow<ScrollBox>(on, &source->Children[i]);
						// Button controls
						else if (source->Children[i].Properties->GetTemplateClass() == L"Button") on->GetStation()->CreateWindow<Button>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"CheckBox") on->GetStation()->CreateWindow<CheckBox>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"ToolButton") on->GetStation()->CreateWindow<ToolButton>(on, &source->Children[i]);
						// Static controls
						else if (source->Children[i].Properties->GetTemplateClass() == L"Static") on->GetStation()->CreateWindow<Static>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"ColorView") on->GetStation()->CreateWindow<ColorView>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"ProgressBar") on->GetStation()->CreateWindow<ProgressBar>(on, &source->Children[i]);
						// Scrollable controls
						else if (source->Children[i].Properties->GetTemplateClass() == L"VerticalScrollBar") on->GetStation()->CreateWindow<VerticalScrollBar>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"HorizontalScrollBar") on->GetStation()->CreateWindow<HorizontalScrollBar>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"VerticalTrackBar") on->GetStation()->CreateWindow<VerticalTrackBar>(on, &source->Children[i]);
						else if (source->Children[i].Properties->GetTemplateClass() == L"HorizontalTrackBar") on->GetStation()->CreateWindow<HorizontalTrackBar>(on, &source->Children[i]);
#pragma message("REALIZE ALL CONTROLS")
						else throw InvalidArgumentException();

						/*
						NOT IMPLEMENTED:

						Edit
						HorizontalSplitBox
						VerticalSplitBox
						SplitBoxPart

						ListBox
						TreeView
						ListView
						ListViewColumn
						MultiLineEdit

						ComboBox
						TextComboBox
						CustomControl
						
						*/
					}
				}
			}
		}
		namespace Windows
		{
			Controls::OverlappedWindow * CreateFramelessDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station)
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
			Controls::OverlappedWindow * CreateFramedDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station)
			{
				auto window = Station->CreateWindow<Controls::OverlappedWindow>(0, Template);
				window->_mode = 1;
				auto props = static_cast<Template::Controls::DialogFrame *>(Template->Properties);
				Box outer = Position.IsValid() ? Box(Position, Station->GetBox()) : Station->GetBox();
				Box sizes = Box(Template->Properties->ControlPosition, outer);
				Box margins;
				sizes.Right -= sizes.Left;
				sizes.Bottom -= sizes.Top;
				int border = props->Sizeble ? Station->GetVisualStyles().WindowSizableBorder : Station->GetVisualStyles().WindowFixedBorder;
				int caption = props->Captioned ? (props->ToolWindow ? Station->GetVisualStyles().WindowSmallCaptionHeight : Station->GetVisualStyles().WindowCaptionHeight) : 0;
				window->_border = border;
				window->_caption = caption;
				margins.Left = margins.Right = margins.Bottom = border;
				margins.Top = caption + border;
				sizes.Right += margins.Left + margins.Right;
				sizes.Bottom += margins.Top + margins.Bottom;
				window->GetContentFrame()->SetRectangle(Rectangle(Coordinate(margins.Left, 0.0, 0.0), Coordinate(margins.Top, 0.0, 0.0), Coordinate(-margins.Right, 0.0, 1.0), Coordinate(-margins.Bottom, 0.0, 1.0)));
				window->_minwidth = 2 * border + props->MinimalWidth;
				window->_minheight = 2 * border + caption + props->MinimalHeight;
				if (props->Captioned) {
					Coordinate hdrtop = Coordinate(border, 0.0, 0.0);
					Coordinate hdrbottom = Coordinate(border + caption, 0.0, 0.0);
					Coordinate delta = Coordinate(caption, 0.0, 0.0);
					Coordinate pos = Coordinate(-border, 0.0, 1.0);
					Template::ControlTemplate * temp;
					if (props->CloseButton && (temp = props->ToolWindow ? Station->GetVisualStyles().WindowSmallCloseButton : Station->GetVisualStyles().WindowCloseButton)) {
						auto frame = Station->CreateWindow<Controls::ToolButton>(window);
						frame->SetRectangle(Rectangle(pos - delta, hdrtop, pos, hdrbottom));
						pos -= delta;
						auto button = Station->CreateWindow<Controls::ToolButtonPart>(frame, temp);
						button->SetRectangle(Rectangle::Entire());
						button->Checked = false;
						button->Disabled = false;
						button->DropDownMenu.SetReference(0);
						button->ID = 0;
						window->_close = button;
						window->_btnwidth += caption;
					}
					if (props->MaximizeButton && (temp = props->ToolWindow ? Station->GetVisualStyles().WindowSmallMaximizeButton : Station->GetVisualStyles().WindowMaximizeButton)) {
						auto frame = Station->CreateWindow<Controls::ToolButton>(window);
						frame->SetRectangle(Rectangle(pos - delta, hdrtop, pos, hdrbottom));
						pos -= delta;
						auto button = Station->CreateWindow<Controls::ToolButtonPart>(frame, temp);
						button->SetRectangle(Rectangle::Entire());
						button->Checked = false;
						button->Disabled = false;
						button->DropDownMenu.SetReference(0);
						button->ID = 0;
						window->_maximize = button;
						window->_btnwidth += caption;
					}
					if (props->MinimizeButton && (temp = props->ToolWindow ? Station->GetVisualStyles().WindowSmallMinimizeButton : Station->GetVisualStyles().WindowMinimizeButton)) {
						auto frame = Station->CreateWindow<Controls::ToolButton>(window);
						frame->SetRectangle(Rectangle(pos - delta, hdrtop, pos, hdrbottom));
						pos -= delta;
						auto button = Station->CreateWindow<Controls::ToolButtonPart>(frame, temp);
						button->SetRectangle(Rectangle::Entire());
						button->Checked = false;
						button->Disabled = false;
						button->DropDownMenu.SetReference(0);
						button->ID = 0;
						window->_minimize = button;
						window->_btnwidth += caption;
					}
					if (props->HelpButton && (temp = props->ToolWindow ? Station->GetVisualStyles().WindowSmallHelpButton : Station->GetVisualStyles().WindowHelpButton)) {
						auto frame = Station->CreateWindow<Controls::ToolButton>(window);
						frame->SetRectangle(Rectangle(pos - delta, hdrtop, pos, hdrbottom));
						pos -= delta;
						auto button = Station->CreateWindow<Controls::ToolButtonPart>(frame, temp);
						button->SetRectangle(Rectangle::Entire());
						button->Checked = false;
						button->Disabled = false;
						button->DropDownMenu.SetReference(0);
						button->ID = 0;
						window->_help = button;
						window->_btnwidth += caption;
					}
				}
				window->SetPosition(Box((outer.Right - outer.Left - sizes.Right) / 2 + outer.Left, (outer.Bottom - outer.Top - sizes.Bottom) / 2 + outer.Top,
					(outer.Right - outer.Left - sizes.Right) / 2 + outer.Left + sizes.Right, (outer.Bottom - outer.Top - sizes.Bottom) / 2 + outer.Top + sizes.Bottom));
				window->SetCallback(Callback);
				if (Callback) Callback->OnInitialized(window);
				window->Show(true);
				return window;
			}
		}
	}
}