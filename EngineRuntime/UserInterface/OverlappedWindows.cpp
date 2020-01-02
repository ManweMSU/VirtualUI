#include "OverlappedWindows.h"

#include "GroupControls.h"
#include "ButtonControls.h"
#include "StaticControls.h"
#include "ScrollableControls.h"
#include "EditControls.h"
#include "ListControls.h"
#include "CombinedControls.h"

#include "../PlatformDependent/NativeStation.h"
#include "../PlatformDependent/KeyCodes.h"

using namespace Engine::UI::Windows;

namespace Engine
{
	namespace UI
	{
		namespace Accelerators
		{
			AcceleratorCommand::AcceleratorCommand(void) {}
			AcceleratorCommand::AcceleratorCommand(int invoke_command, uint on_key, bool control, bool shift, bool alternative) :
				CommandID(invoke_command), KeyCode(on_key), Control(control), Shift(shift), Alternative(alternative), SystemCommand(false) {}
			AcceleratorCommand::AcceleratorCommand(AcceleratorSystemCommand command, uint on_key, bool control, bool shift, bool alternative) :
				CommandID(static_cast<int>(command)), KeyCode(on_key), Control(control), Shift(shift), Alternative(alternative), SystemCommand(true) {}
		}
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
				_minwidth(0), _minheight(0), _btnwidth(0), ControlPosition(Rectangle::Invalid()), _close(0), _maximize(0), _minimize(0), _help(0), _accels(0x10)
			{
				_inner = Station->CreateWindow<ContentFrame>(this);
			}
			OverlappedWindow::OverlappedWindow(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) :
				ParentWindow(Parent, Station), _callback(0), _visible(false), _enabled(true), _mode(0), _border(0), _caption(0), _state(0), _minwidth(0), _minheight(0), _btnwidth(0),
				ControlPosition(Rectangle::Invalid()), _close(0), _maximize(0), _minimize(0), _help(0), _accels(0x10)
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
						auto provider = ArgumentService::OverlappedWindowArgumentProvider(this);
						*shape = temp->Initialize(&provider);
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
			void OverlappedWindow::Enable(bool enable)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					NativeWindows::EnableWindow(GetStation(), enable);
				} else {
					_enabled = enable;
				}
			}
			bool OverlappedWindow::IsEnabled(void)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					return NativeWindows::IsWindowEnabled(GetStation());
				} else {
					return _enabled;
				}
			}
			void OverlappedWindow::Show(bool visible)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					NativeWindows::ShowWindow(GetStation(), visible);
				} else {
					_visible = visible;
				}
			}
			bool OverlappedWindow::IsVisible(void)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					return NativeWindows::IsWindowVisible(GetStation());
				} else {
					return _visible;
				}
			}
			bool OverlappedWindow::IsOverlapped(void) { return true; }
			bool OverlappedWindow::IsNeverActive(void) { return !_overlaps; }
			void OverlappedWindow::SetPosition(const Box & box)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					NativeWindows::SetWindowPosition(GetStation(), box);
				} else {
					ParentWindow::SetPosition(box);
					if (_callback) _callback->OnFrameEvent(this, FrameEvent::Move);
				}
			}
			Box OverlappedWindow::GetPosition(void)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					return NativeWindows::GetWindowPosition(GetStation());
				} else {
					return ParentWindow::GetPosition();
				}
			}
			void OverlappedWindow::SetRectangle(const Rectangle & rect)
			{
				ControlPosition = rect;
				Window * Parent = GetParent();
				if (Parent) Parent->ArrangeChildren();
				else SetPosition(Box(ControlPosition, NativeWindows::GetScreenDimensions()));
			}
			Rectangle OverlappedWindow::GetRectangle(void) { return ControlPosition; }
			void OverlappedWindow::SetText(const string & text)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					NativeWindows::SetWindowTitle(GetStation(), text);
				} else {
					_inner->Title = text; ResetCache();
				}
			}
			string OverlappedWindow::GetText(void)
			{
				if (GetStation()->IsNativeStationWrapper()) {
					return NativeWindows::GetWindowTitle(GetStation());
				} else {
					return _inner->Title;
				}
			}
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
			bool OverlappedWindow::TranslateAccelerators(int key_code)
			{
				for (int i = 0; i < _accels.Length(); i++) {
					if (key_code == _accels[i].KeyCode &&
						Keyboard::IsKeyPressed(KeyCodes::Shift) == _accels[i].Shift &&
						Keyboard::IsKeyPressed(KeyCodes::Control) == _accels[i].Control &&
						Keyboard::IsKeyPressed(KeyCodes::Alternative) == _accels[i].Alternative) {
						if (_accels[i].SystemCommand) {
							if (_accels[i].CommandID == static_cast<int>(Accelerators::AcceleratorSystemCommand::WindowClose)) {
								if (_callback) _callback->OnFrameEvent(this, Windows::FrameEvent::Close);
							} else if (_accels[i].CommandID == static_cast<int>(Accelerators::AcceleratorSystemCommand::WindowInvokeHelp)) {
								if (_callback) _callback->OnFrameEvent(this, Windows::FrameEvent::Help);
							} else if (_accels[i].CommandID == static_cast<int>(Accelerators::AcceleratorSystemCommand::WindowNextControl)) {
								Window * Focus = GetStation()->GetFocus();
								if (!Focus) Focus = this;
								auto New = Focus->GetNextTabStopControl();
								if (New) New->SetFocus();
							} else if (_accels[i].CommandID == static_cast<int>(Accelerators::AcceleratorSystemCommand::WindowPreviousControl)) {
								Window * Focus = GetStation()->GetFocus();
								if (!Focus) Focus = this;
								auto New = Focus->GetPreviousTabStopControl();
								if (New) New->SetFocus();
							}
						} else {
							if (_callback) _callback->OnControlEvent(this, _accels[i].CommandID, Event::AcceleratorCommand, 0);
						}
						return true;
					}
				}
				return false;
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
			void OverlappedWindow::Timer(void) { if (_callback) _callback->OnFrameEvent(this, FrameEvent::Timer); }
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
			void OverlappedWindow::RaiseFrameEvent(Windows::FrameEvent event) { if (_callback) _callback->OnFrameEvent(this, event); }
			void OverlappedWindow::SetBackground(Template::Shape * shape)
			{
				GetContentFrame()->Background.SetRetain(shape);
				GetContentFrame()->ResetCache();
			}
			Array<Accelerators::AcceleratorCommand>& OverlappedWindow::GetAcceleratorTable(void) { return _accels; }
			const Array<Accelerators::AcceleratorCommand>& OverlappedWindow::GetAcceleratorTable(void) const { return _accels; }
			void OverlappedWindow::AddDialogStandardAccelerators(void)
			{
				_accels << Accelerators::AcceleratorCommand(1, KeyCodes::Return, false, false, false);
				_accels << Accelerators::AcceleratorCommand(2, KeyCodes::Escape, false, false, false);
				_accels << Accelerators::AcceleratorCommand(Accelerators::AcceleratorSystemCommand::WindowNextControl,
					KeyCodes::Tab, false, false, false);
				_accels << Accelerators::AcceleratorCommand(Accelerators::AcceleratorSystemCommand::WindowPreviousControl,
					KeyCodes::Tab, false, true, false);
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
						auto provider = ZeroArgumentProvider();
						_background.SetReference(Background->Initialize(&provider));
						_background->Render(GetStation()->GetRenderingDevice(), at);
					} else _background->Render(GetStation()->GetRenderingDevice(), at);
				} else if (DefaultBackground) {
					if (!_background && GetStation()->GetVisualStyles().WindowDefaultBackground) {
						auto provider = ZeroArgumentProvider();
						_background.SetReference(GetStation()->GetVisualStyles().WindowDefaultBackground->Initialize(&provider));
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
			void ContentFrame::ArrangeChildren(void) { if (static_cast<OverlappedWindow *>(GetParent())->_initialized) ParentWindow::ArrangeChildren(); }
			void ContentFrame::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle ContentFrame::GetRectangle(void) { return ControlPosition; }

			namespace Constructor
			{
				void ConstructChildren(Window * on, Template::ControlTemplate * source)
				{
					for (int i = 0; i < source->Children.Length(); i++) CreateChildWindow(on, &source->Children[i]);
				}
				Window * CreateChildWindow(Window * on, Template::ControlTemplate * child)
				{
					// Group controls
					if (child->Properties->GetTemplateClass() == L"ControlGroup") return on->GetStation()->CreateWindow<ControlGroup>(on, child);
					else if (child->Properties->GetTemplateClass() == L"RadioButtonGroup") return on->GetStation()->CreateWindow<RadioButtonGroup>(on, child);
					else if (child->Properties->GetTemplateClass() == L"ScrollBox") return on->GetStation()->CreateWindow<ScrollBox>(on, child);
					else if (child->Properties->GetTemplateClass() == L"VerticalSplitBox") return on->GetStation()->CreateWindow<VerticalSplitBox>(on, child);
					else if (child->Properties->GetTemplateClass() == L"HorizontalSplitBox") return on->GetStation()->CreateWindow<HorizontalSplitBox>(on, child);
					else if (child->Properties->GetTemplateClass() == L"BookmarkView") return on->GetStation()->CreateWindow<BookmarkView>(on, child);
					// Button controls
					else if (child->Properties->GetTemplateClass() == L"Button") return on->GetStation()->CreateWindow<Button>(on, child);
					else if (child->Properties->GetTemplateClass() == L"CheckBox") return on->GetStation()->CreateWindow<CheckBox>(on, child);
					else if (child->Properties->GetTemplateClass() == L"ToolButton") return on->GetStation()->CreateWindow<ToolButton>(on, child);
					// Static controls
					else if (child->Properties->GetTemplateClass() == L"Static") return on->GetStation()->CreateWindow<Static>(on, child);
					else if (child->Properties->GetTemplateClass() == L"ColorView") return on->GetStation()->CreateWindow<ColorView>(on, child);
					else if (child->Properties->GetTemplateClass() == L"ProgressBar") return on->GetStation()->CreateWindow<ProgressBar>(on, child);
					// Scrollable controls
					else if (child->Properties->GetTemplateClass() == L"VerticalScrollBar") return on->GetStation()->CreateWindow<VerticalScrollBar>(on, child);
					else if (child->Properties->GetTemplateClass() == L"HorizontalScrollBar") return on->GetStation()->CreateWindow<HorizontalScrollBar>(on, child);
					else if (child->Properties->GetTemplateClass() == L"VerticalTrackBar") return on->GetStation()->CreateWindow<VerticalTrackBar>(on, child);
					else if (child->Properties->GetTemplateClass() == L"HorizontalTrackBar") return on->GetStation()->CreateWindow<HorizontalTrackBar>(on, child);
					// Edit controls
					else if (child->Properties->GetTemplateClass() == L"Edit") return on->GetStation()->CreateWindow<Edit>(on, child);
					else if (child->Properties->GetTemplateClass() == L"MultiLineEdit") return on->GetStation()->CreateWindow<MultiLineEdit>(on, child);
					// List controls
					else if (child->Properties->GetTemplateClass() == L"ListBox") return on->GetStation()->CreateWindow<ListBox>(on, child);
					else if (child->Properties->GetTemplateClass() == L"TreeView") return on->GetStation()->CreateWindow<TreeView>(on, child);
					else if (child->Properties->GetTemplateClass() == L"ListView") return on->GetStation()->CreateWindow<ListView>(on, child);
					// Combined controls
					else if (child->Properties->GetTemplateClass() == L"ComboBox") return on->GetStation()->CreateWindow<ComboBox>(on, child);
					else if (child->Properties->GetTemplateClass() == L"TextComboBox") return on->GetStation()->CreateWindow<TextComboBox>(on, child);
#pragma message("REALIZE ALL CONTROLS")
					else throw InvalidArgumentException();

					/*
					NOT IMPLEMENTED:

					RichEdit
					CustomControl ???

					FUTURE CONTROLS:

					MenuBar

					*/
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
				window->_initialized = true;
				if (Position.IsValid()) {
					window->SetRectangle(Position);
				} else {
					window->SetRectangle(Rectangle(Coordinate(0, 0.0, 0.5) - sizes.Right / 2.0, Coordinate(0, 0.0, 0.5) - sizes.Bottom / 2.0,
						Coordinate(0, 0.0, 0.5) - sizes.Right / 2.0 + sizes.Right, Coordinate(0, 0.0, 0.5) - sizes.Bottom / 2.0 + sizes.Bottom));
				}
				window->SetCallback(Callback);
				if (Callback) Callback->OnInitialized(window);
				return window;
			}
			Controls::OverlappedWindow * CreateFramedDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station)
			{
				Controls::OverlappedWindow * window = 0;
				if (!Station || Station->IsNativeStationWrapper()) {
					WindowStation * new_native = NativeWindows::CreateOverlappedWindow(Template, Position, Station);
					new_native->GetVisualStyles().CaretWidth = int(Zoom);
					window = new_native->GetDesktop()->As<Controls::OverlappedWindow>();
					window->_visible = true;
					window->_initialized = true;
					window->ArrangeChildren();
				} else {
					window = Station->CreateWindow<Controls::OverlappedWindow>(0, Template);
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
					window->_initialized = true;
					window->SetPosition(Box((outer.Right - outer.Left - sizes.Right) / 2 + outer.Left, (outer.Bottom - outer.Top - sizes.Bottom) / 2 + outer.Top,
						(outer.Right - outer.Left - sizes.Right) / 2 + outer.Left + sizes.Right, (outer.Bottom - outer.Top - sizes.Bottom) / 2 + outer.Top + sizes.Bottom));
				}
				window->SetCallback(Callback);
				if (Callback) Callback->OnInitialized(window);
				return window;
			}
			Controls::OverlappedWindow * CreatePopupDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station)
			{
				Controls::OverlappedWindow * window = 0;
				if (!Station || Station->IsNativeStationWrapper()) {
					WindowStation * new_native = NativeWindows::CreatePopupWindow(Template, Position, Station);
					new_native->GetVisualStyles().CaretWidth = int(Zoom);
					window = new_native->GetDesktop()->As<Controls::OverlappedWindow>();
					window->_visible = true;
					window->_initialized = true;
					window->ArrangeChildren();
				} else {
					window = Template ? Station->CreateWindow<Controls::OverlappedWindow>(0, Template) :
						Station->CreateWindow<Controls::OverlappedWindow>(0);
					window->_overlaps = false;
					Rectangle sizes = Template ? Template->Properties->ControlPosition : Rectangle::Entire();
					sizes.Right -= sizes.Left;
					sizes.Bottom -= sizes.Top;
					window->GetContentFrame()->SetRectangle(Rectangle::Entire());
					window->_initialized = true;
					if (Position.IsValid()) {
						window->SetRectangle(Position);
					} else {
						window->SetRectangle(Rectangle(Coordinate(0, 0.0, 0.5) - sizes.Right / 2.0, Coordinate(0, 0.0, 0.5) - sizes.Bottom / 2.0,
							Coordinate(0, 0.0, 0.5) - sizes.Right / 2.0 + sizes.Right, Coordinate(0, 0.0, 0.5) - sizes.Bottom / 2.0 + sizes.Bottom));
					}
				}
				window->SetCallback(Callback);
				if (Callback) Callback->OnInitialized(window);
				return window;
			}
			void InitializeCodecCollection(void) { NativeWindows::InitializeCodecCollection(); }
			IResourceLoader * CreateNativeCompatibleResourceLoader(void) { return NativeWindows::CreateCompatibleResourceLoader(); }
			Drawing::ITextureRenderingDevice * CreateNativeCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color) { return NativeWindows::CreateCompatibleTextureRenderingDevice(width, height, color); }
			Box GetScreenDimensions(void) { return NativeWindows::GetScreenDimensions(); }
			double GetScreenScale(void) { return NativeWindows::GetScreenScale(); }
			void RunMessageLoop(void) { NativeWindows::RunMainMessageLoop(); }
			void ExitMessageLoop(void) { NativeWindows::ExitMainLoop(); }
			Array<string>* GetFontFamilies(void) { return NativeWindows::GetFontFamilies(); }
			void SetApplicationIcon(Codec::Image * icon) { NativeWindows::SetApplicationIcon(icon); }
		}
	}
}