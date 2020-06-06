#include "ScrollableControls.h"

#include "../PlatformDependent/KeyCodes.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			VerticalScrollBar::VerticalScrollBar(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Line = 1; }
			VerticalScrollBar::VerticalScrollBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"VerticalScrollBar") throw InvalidArgumentException();
				static_cast<Template::Controls::VerticalScrollBar &>(*this) = static_cast<Template::Controls::VerticalScrollBar &>(*Template->Properties);
				if (Line < 1) Line = 1;
			}
			VerticalScrollBar::VerticalScrollBar(Window * Parent, WindowStation * Station, Template::Controls::Scrollable * Template) : Window(Parent, Station)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				Line = 1;
				ViewBarNormal = Template->ViewVerticalScrollBarBarNormal;
				ViewBarDisabled = Template->ViewVerticalScrollBarBarDisabled;
				ViewUpButtonNormal = Template->ViewVerticalScrollBarUpButtonNormal;
				ViewUpButtonHot = Template->ViewVerticalScrollBarUpButtonHot;
				ViewUpButtonPressed = Template->ViewVerticalScrollBarUpButtonPressed;
				ViewUpButtonDisabled = Template->ViewVerticalScrollBarUpButtonDisabled;
				ViewDownButtonNormal = Template->ViewVerticalScrollBarDownButtonNormal;
				ViewDownButtonHot = Template->ViewVerticalScrollBarDownButtonHot;
				ViewDownButtonPressed = Template->ViewVerticalScrollBarDownButtonPressed;
				ViewDownButtonDisabled = Template->ViewVerticalScrollBarDownButtonDisabled;
				ViewScrollerNormal = Template->ViewVerticalScrollBarScrollerNormal;
				ViewScrollerHot = Template->ViewVerticalScrollBarScrollerHot;
				ViewScrollerPressed = Template->ViewVerticalScrollBarScrollerPressed;
				ViewScrollerDisabled = Template->ViewVerticalScrollBarScrollerDisabled;
			}
			VerticalScrollBar::VerticalScrollBar(Window * Parent, WindowStation * Station, Template::Controls::VerticallyScrollable * Template) : Window(Parent, Station)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				Line = 1;
				ViewBarNormal = Template->ViewScrollBarBarNormal;
				ViewBarDisabled = Template->ViewScrollBarBarDisabled;
				ViewUpButtonNormal = Template->ViewScrollBarUpButtonNormal;
				ViewUpButtonHot = Template->ViewScrollBarUpButtonHot;
				ViewUpButtonPressed = Template->ViewScrollBarUpButtonPressed;
				ViewUpButtonDisabled = Template->ViewScrollBarUpButtonDisabled;
				ViewDownButtonNormal = Template->ViewScrollBarDownButtonNormal;
				ViewDownButtonHot = Template->ViewScrollBarDownButtonHot;
				ViewDownButtonPressed = Template->ViewScrollBarDownButtonPressed;
				ViewDownButtonDisabled = Template->ViewScrollBarDownButtonDisabled;
				ViewScrollerNormal = Template->ViewScrollBarScrollerNormal;
				ViewScrollerHot = Template->ViewScrollBarScrollerHot;
				ViewScrollerPressed = Template->ViewScrollBarScrollerPressed;
				ViewScrollerDisabled = Template->ViewScrollBarScrollerDisabled;
			}
			VerticalScrollBar::~VerticalScrollBar(void) {}
			void VerticalScrollBar::Render(const Box & at)
			{
				Box up = Box(at.Left, at.Top, at.Right, at.Top + at.Right - at.Left);
				Box down = Box(at.Left, at.Bottom - at.Right + at.Left, at.Right, at.Bottom);
				Box scroller = GetScrollerBox(at);
				Shape * up_shape = 0;
				Shape * down_shape = 0;
				Shape * scroller_shape = 0;
				Shape * bar_shape = 0;
				if (Disabled) {
					if (_up_disabled) up_shape = _up_disabled;
					else if (ViewUpButtonDisabled) {
						auto provider = ZeroArgumentProvider();
						_up_disabled.SetReference(ViewUpButtonDisabled->Initialize(&provider));
						up_shape = _up_disabled;
					}
					if (_down_disabled) down_shape = _down_disabled;
					else if (ViewDownButtonDisabled) {
						auto provider = ZeroArgumentProvider();
						_down_disabled.SetReference(ViewDownButtonDisabled->Initialize(&provider));
						down_shape = _down_disabled;
					}
					if (Position > RangeMinimal || Position + max(Page, 1) - 1 < RangeMaximal) {
						if (_scroller_disabled) scroller_shape = _scroller_disabled;
						else if (ViewScrollerDisabled) {
							auto provider = ZeroArgumentProvider();
							_scroller_disabled.SetReference(ViewScrollerDisabled->Initialize(&provider));
							scroller_shape = _scroller_disabled;
						}
					}
					if (_bar_disabled) bar_shape = _bar_disabled;
					else if (ViewBarDisabled) {
						auto provider = ZeroArgumentProvider();
						_bar_disabled.SetReference(ViewBarDisabled->Initialize(&provider));
						bar_shape = _bar_disabled;
					}
				} else {
					{
						Shape ** storage = 0; Template::Shape * source = 0;
						if (_part == 1) {
							if (_state == 1) {
								storage = _up_hot.InnerRef();
								source = ViewUpButtonHot.Inner();
							} else if (_state == 2) {
								storage = _up_pressed.InnerRef();
								source = ViewUpButtonPressed.Inner();
							} else {
								storage = _up_normal.InnerRef();
								source = ViewUpButtonNormal.Inner();
							}
						} else {
							storage = _up_normal.InnerRef();
							source = ViewUpButtonNormal.Inner();
						}
						if (*storage) up_shape = *storage;
						else if (source) {
							auto provider = ZeroArgumentProvider();
							*storage = source->Initialize(&provider);
							up_shape = *storage;
						}
					}
					{
						Shape ** storage = 0; Template::Shape * source = 0;
						if (_part == 2) {
							if (_state == 1) {
								storage = _down_hot.InnerRef();
								source = ViewDownButtonHot.Inner();
							} else if (_state == 2) {
								storage = _down_pressed.InnerRef();
								source = ViewDownButtonPressed.Inner();
							} else {
								storage = _down_normal.InnerRef();
								source = ViewDownButtonNormal.Inner();
							}
						} else {
							storage = _down_normal.InnerRef();
							source = ViewDownButtonNormal.Inner();
						}
						if (*storage) down_shape = *storage;
						else if (source) {
							auto provider = ZeroArgumentProvider();
							*storage = source->Initialize(&provider);
							down_shape = *storage;
						}
					}
					{
						Shape ** storage = 0; Template::Shape * source = 0;
						if (_part == 3) {
							if (_state == 1) {
								storage = _scroller_hot.InnerRef();
								source = ViewScrollerHot.Inner();
							} else if (_state == 2) {
								storage = _scroller_pressed.InnerRef();
								source = ViewScrollerPressed.Inner();
							} else {
								storage = _scroller_normal.InnerRef();
								source = ViewScrollerNormal.Inner();
							}
						} else {
							storage = _scroller_normal.InnerRef();
							source = ViewScrollerNormal.Inner();
						}
						if (*storage) scroller_shape = *storage;
						else if (source) {
							auto provider = ZeroArgumentProvider();
							*storage = source->Initialize(&provider);
							scroller_shape = *storage;
						}
					}
					if (_bar_normal) bar_shape = _bar_normal;
					else if (ViewBarNormal) {
						auto provider = ZeroArgumentProvider();
						_bar_normal.SetReference(ViewBarNormal->Initialize(&provider));
						bar_shape = _bar_normal;
					}
				}
				auto device = GetStation()->GetRenderingDevice();
				if (bar_shape) bar_shape->Render(device, at);
				if (up_shape) up_shape->Render(device, up);
				if (down_shape) down_shape->Render(device, down);
				if (scroller_shape) scroller_shape->Render(device, scroller);
			}
			void VerticalScrollBar::ResetCache(void)
			{
				_up_normal.SetReference(0);
				_up_hot.SetReference(0);
				_up_pressed.SetReference(0);
				_up_disabled.SetReference(0);
				_down_normal.SetReference(0);
				_down_hot.SetReference(0);
				_down_pressed.SetReference(0);
				_down_disabled.SetReference(0);
				_scroller_normal.SetReference(0);
				_scroller_hot.SetReference(0);
				_scroller_pressed.SetReference(0);
				_scroller_disabled.SetReference(0);
				_bar_normal.SetReference(0);
				_bar_disabled.SetReference(0);
			}
			void VerticalScrollBar::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; }
			bool VerticalScrollBar::IsEnabled(void) { return !Disabled; }
			void VerticalScrollBar::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; }
			bool VerticalScrollBar::IsVisible(void) { return !Invisible; }
			void VerticalScrollBar::SetID(int _ID) { ID = _ID; }
			int VerticalScrollBar::GetID(void) { return ID; }
			Window * VerticalScrollBar::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void VerticalScrollBar::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle VerticalScrollBar::GetRectangle(void) { return ControlPosition; }
			void VerticalScrollBar::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = 0; GetStation()->SetTimer(this, 0); } }
			void VerticalScrollBar::LeftButtonDown(Point at)
			{
				if (_state == 1) {
					_state = 2;
					if (_part == 1) {
						GetStation()->SetTimer(this, Keyboard::GetKeyboardDelay());
						SetScrollerPosition(Position - Line);
					} else if (_part == 2) {
						GetStation()->SetTimer(this, Keyboard::GetKeyboardDelay());
						SetScrollerPosition(Position + Line);
					} else if (_part == 0) {
						Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
						Box scroller = GetScrollerBox(my);
						_mpos = at.y;
						_sd = (_mpos >= scroller.Top) ? 2 : 1;
						if (_sd == 1) SetScrollerPosition(Position - max(Page, 1));
						else SetScrollerPosition(Position + max(Page, 1));
						scroller = GetScrollerBox(my);
						int sl = scroller.Top;
						int sh = scroller.Bottom;
						if ((_sd == 1 && (_mpos >= sl)) || (_sd == 2 && (_mpos < sh))) _sd = 0;
						else GetStation()->SetTimer(this, Keyboard::GetKeyboardDelay());
					} else if (_part == 3) {
						_mpos = at.y;
						_sd = _mpos;
						_ipos = Position;
					}
				}
			}
			void VerticalScrollBar::LeftButtonUp(Point at) { if (_state == 2) ReleaseCapture(); }
			void VerticalScrollBar::MouseMove(Point at)
			{
				Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				Box up = Box(my.Left, my.Top, my.Right, my.Top + my.Right - my.Left);
				Box down = Box(my.Left, my.Bottom - my.Right + my.Left, my.Right, my.Bottom);
				Box scroller = GetScrollerBox(my);
				if (_state == 0 || _state == 1) {
					SetCapture();
					_state = 1;
					if (GetStation()->HitTest(GetStation()->GetCursorPos()) != this) {
						ReleaseCapture();
					} else {
						if (up.IsInside(at)) _part = 1;
						else if (down.IsInside(at)) _part = 2;
						else if (scroller.IsInside(at)) _part = 3;
						else _part = 0;
					}
				} else {
					if (_part == 0) {
						_mpos = at.y;
					} else if (_part == 3) {
						if (RangeMaximal == RangeMinimal) return;
						int w = my.Right;
						int h = my.Bottom;
						_mpos = at.y;
						int dx = _mpos - _sd;
						double unit = Page ? (double(h - w - w) / (RangeMaximal - RangeMinimal + 1)) : (double(h - w - w - w) / (RangeMaximal - RangeMinimal));
						double hpos = double(_ipos) + double(dx) / unit;
						SetScrollerPosition(int(hpos + 0.5));
					}
				}
			}
			void VerticalScrollBar::Timer(void)
			{
				GetStation()->SetTimer(this, Keyboard::GetKeyboardSpeed());
				if (_part == 1) {
					SetScrollerPosition(Position - Line);
				} else if (_part == 2) {
					SetScrollerPosition(Position + Line);
				} else if (_part == 0) {
					if (_sd) {
						Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
						Box scroller = GetScrollerBox(my);
						if (_sd == 1) SetScrollerPosition(Position - max(Page, 1));
						else SetScrollerPosition(Position + max(Page, 1));
						scroller = GetScrollerBox(my);
						int sl = scroller.Top;
						int sh = scroller.Bottom;
						if ((_sd == 1 && _mpos >= sl) || (_sd == 2 && _mpos < sh)) { _sd = 0; GetStation()->SetTimer(this, 0); }
					}
				}
			}
			string VerticalScrollBar::GetControlClass(void) { return L"VerticalScrollBar"; }
			Box VerticalScrollBar::GetScrollerBox(const Box & at)
			{
				if (_state == 2 && _part == 3) {
					int width = at.Right - at.Left;
					int pmin = at.Top + width, pmax = at.Bottom - width;
					Box scroller;
					if (Page == 0) {
						if (RangeMinimal == RangeMaximal) scroller = Box(at.Left, pmin, at.Right, pmin + width);
						else {
							int lpos = pmin + int32(int64(pmax - pmin - width) * (_ipos - RangeMinimal) / (RangeMaximal - RangeMinimal)) + (_mpos - _sd);
							scroller = Box(at.Left, lpos, at.Right, lpos + width);
						}
					} else {
						int lpos = pmin + int32(int64(pmax - pmin) * (_ipos - RangeMinimal) / (RangeMaximal - RangeMinimal + 1)) + (_mpos - _sd);
						int hpos = pmin + int32(int64(pmax - pmin) * (min(_ipos + Page, RangeMaximal + 1) - RangeMinimal) / (RangeMaximal - RangeMinimal + 1)) + (_mpos - _sd);
						scroller = Box(at.Left, lpos, at.Right, hpos);
					}
					if (scroller.Bottom >= at.Bottom - width) {
						int d = scroller.Bottom - at.Bottom + width;
						scroller.Top -= d;
						scroller.Bottom -= d;
					}
					if (scroller.Top < at.Top + width) {
						int d = at.Top + width - scroller.Top;
						scroller.Top += d;
						scroller.Bottom += d;
					}
					return scroller;
				} else {
					int pmin = at.Right - at.Left, pmax = at.Bottom - at.Top - at.Right + at.Left;
					if (Page == 0) {
						if (RangeMaximal == RangeMinimal) {
							return Box(at.Left, at.Top + pmin, at.Right, at.Top + pmin + pmin);
						}
						int lpos = pmin + int32(int64(pmax - pmin - pmin) * (Position - RangeMinimal) / (RangeMaximal - RangeMinimal));
						return Box(at.Left, at.Top + lpos, at.Right, at.Top + lpos + pmin);
					} else {
						int lpos = pmin + int32(int64(pmax - pmin) * (Position - RangeMinimal) / (RangeMaximal - RangeMinimal + 1));
						int hpos = pmin + int32(int64(pmax - pmin) * (min(Position + Page, RangeMaximal + 1) - RangeMinimal) / (RangeMaximal - RangeMinimal + 1));
						return Box(at.Left, at.Top + lpos, at.Right, at.Top + hpos);
					}
				}
			}
			void VerticalScrollBar::SetScrollerPosition(int position)
			{
				int op = Position;
				int _page = max(Page, 1);
				Position = max(min(const_cast<const int &>(position), RangeMaximal - _page + 1), RangeMinimal);
				if (Position != op) GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			void VerticalScrollBar::SetPage(int page)
			{
				Page = page;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) { Position = pos; GetParent()->RaiseEvent(ID, Event::ValueChange, this); }
			}
			void VerticalScrollBar::SetRange(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) { Position = pos; GetParent()->RaiseEvent(ID, Event::ValueChange, this); }
			}
			void VerticalScrollBar::SetScrollerPositionSilent(int position)
			{
				int op = Position;
				int _page = max(Page, 1);
				Position = max(min(const_cast<const int &>(position), RangeMaximal - _page + 1), RangeMinimal);
			}
			void VerticalScrollBar::SetPageSilent(int page)
			{
				Page = page;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) Position = pos;
			}
			void VerticalScrollBar::SetRangeSilent(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) Position = pos;
			}

			HorizontalScrollBar::HorizontalScrollBar(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Line = 1; }
			HorizontalScrollBar::HorizontalScrollBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"HorizontalScrollBar") throw InvalidArgumentException();
				static_cast<Template::Controls::HorizontalScrollBar &>(*this) = static_cast<Template::Controls::HorizontalScrollBar &>(*Template->Properties);
				if (Line < 1) Line = 1;
			}
			HorizontalScrollBar::HorizontalScrollBar(Window * Parent, WindowStation * Station, Template::Controls::Scrollable * Template) : Window(Parent, Station)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				Line = 1;
				ViewBarNormal = Template->ViewHorizontalScrollBarBarNormal;
				ViewBarDisabled = Template->ViewHorizontalScrollBarBarDisabled;
				ViewLeftButtonNormal = Template->ViewHorizontalScrollBarLeftButtonNormal;
				ViewLeftButtonHot = Template->ViewHorizontalScrollBarLeftButtonHot;
				ViewLeftButtonPressed = Template->ViewHorizontalScrollBarLeftButtonPressed;
				ViewLeftButtonDisabled = Template->ViewHorizontalScrollBarLeftButtonDisabled;
				ViewRightButtonNormal = Template->ViewHorizontalScrollBarRightButtonNormal;
				ViewRightButtonHot = Template->ViewHorizontalScrollBarRightButtonHot;
				ViewRightButtonPressed = Template->ViewHorizontalScrollBarRightButtonPressed;
				ViewRightButtonDisabled = Template->ViewHorizontalScrollBarRightButtonDisabled;
				ViewScrollerNormal = Template->ViewHorizontalScrollBarScrollerNormal;
				ViewScrollerHot = Template->ViewHorizontalScrollBarScrollerHot;
				ViewScrollerPressed = Template->ViewHorizontalScrollBarScrollerPressed;
				ViewScrollerDisabled = Template->ViewHorizontalScrollBarScrollerDisabled;
			}
			HorizontalScrollBar::~HorizontalScrollBar(void) {}
			void HorizontalScrollBar::Render(const Box & at)
			{
				Box left = Box(at.Left, at.Top, at.Left + at.Bottom - at.Top, at.Bottom);
				Box right = Box(at.Right - at.Bottom + at.Top, at.Top, at.Right, at.Bottom);
				Box scroller = GetScrollerBox(at);
				Shape * left_shape = 0;
				Shape * right_shape = 0;
				Shape * scroller_shape = 0;
				Shape * bar_shape = 0;
				if (Disabled) {
					if (_left_disabled) left_shape = _left_disabled;
					else if (ViewLeftButtonDisabled) {
						auto provider = ZeroArgumentProvider();
						_left_disabled.SetReference(ViewLeftButtonDisabled->Initialize(&provider));
						left_shape = _left_disabled;
					}
					if (_right_disabled) right_shape = _right_disabled;
					else if (ViewRightButtonDisabled) {
						auto provider = ZeroArgumentProvider();
						_right_disabled.SetReference(ViewRightButtonDisabled->Initialize(&provider));
						right_shape = _right_disabled;
					}
					if (Position > RangeMinimal || Position + max(Page, 1) - 1 < RangeMaximal) {
						if (_scroller_disabled) scroller_shape = _scroller_disabled;
						else if (ViewScrollerDisabled) {
							auto provider = ZeroArgumentProvider();
							_scroller_disabled.SetReference(ViewScrollerDisabled->Initialize(&provider));
							scroller_shape = _scroller_disabled;
						}
					}
					if (_bar_disabled) bar_shape = _bar_disabled;
					else if (ViewBarDisabled) {
						auto provider = ZeroArgumentProvider();
						_bar_disabled.SetReference(ViewBarDisabled->Initialize(&provider));
						bar_shape = _bar_disabled;
					}
				} else {
					{
						Shape ** storage = 0; Template::Shape * source = 0;
						if (_part == 1) {
							if (_state == 1) {
								storage = _left_hot.InnerRef();
								source = ViewLeftButtonHot.Inner();
							} else if (_state == 2) {
								storage = _left_pressed.InnerRef();
								source = ViewLeftButtonPressed.Inner();
							} else {
								storage = _left_normal.InnerRef();
								source = ViewLeftButtonNormal.Inner();
							}
						} else {
							storage = _left_normal.InnerRef();
							source = ViewLeftButtonNormal.Inner();
						}
						if (*storage) left_shape = *storage;
						else if (source) {
							auto provider = ZeroArgumentProvider();
							*storage = source->Initialize(&provider);
							left_shape = *storage;
						}
					}
					{
						Shape ** storage = 0; Template::Shape * source = 0;
						if (_part == 2) {
							if (_state == 1) {
								storage = _right_hot.InnerRef();
								source = ViewRightButtonHot.Inner();
							} else if (_state == 2) {
								storage = _right_pressed.InnerRef();
								source = ViewRightButtonPressed.Inner();
							} else {
								storage = _right_normal.InnerRef();
								source = ViewRightButtonNormal.Inner();
							}
						} else {
							storage = _right_normal.InnerRef();
							source = ViewRightButtonNormal.Inner();
						}
						if (*storage) right_shape = *storage;
						else if (source) {
							auto provider = ZeroArgumentProvider();
							*storage = source->Initialize(&provider);
							right_shape = *storage;
						}
					}
					{
						Shape ** storage = 0; Template::Shape * source = 0;
						if (_part == 3) {
							if (_state == 1) {
								storage = _scroller_hot.InnerRef();
								source = ViewScrollerHot.Inner();
							} else if (_state == 2) {
								storage = _scroller_pressed.InnerRef();
								source = ViewScrollerPressed.Inner();
							} else {
								storage = _scroller_normal.InnerRef();
								source = ViewScrollerNormal.Inner();
							}
						} else {
							storage = _scroller_normal.InnerRef();
							source = ViewScrollerNormal.Inner();
						}
						if (*storage) scroller_shape = *storage;
						else if (source) {
							auto provider = ZeroArgumentProvider();
							*storage = source->Initialize(&provider);
							scroller_shape = *storage;
						}
					}
					if (_bar_normal) bar_shape = _bar_normal;
					else if (ViewBarNormal) {
						auto provider = ZeroArgumentProvider();
						_bar_normal.SetReference(ViewBarNormal->Initialize(&provider));
						bar_shape = _bar_normal;
					}
				}
				auto device = GetStation()->GetRenderingDevice();
				if (bar_shape) bar_shape->Render(device, at);
				if (left_shape) left_shape->Render(device, left);
				if (right_shape) right_shape->Render(device, right);
				if (scroller_shape) scroller_shape->Render(device, scroller);
			}
			void HorizontalScrollBar::ResetCache(void)
			{
				_left_normal.SetReference(0);
				_left_hot.SetReference(0);
				_left_pressed.SetReference(0);
				_left_disabled.SetReference(0);
				_right_normal.SetReference(0);
				_right_hot.SetReference(0);
				_right_pressed.SetReference(0);
				_right_disabled.SetReference(0);
				_scroller_normal.SetReference(0);
				_scroller_hot.SetReference(0);
				_scroller_pressed.SetReference(0);
				_scroller_disabled.SetReference(0);
				_bar_normal.SetReference(0);
				_bar_disabled.SetReference(0);
			}
			void HorizontalScrollBar::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; }
			bool HorizontalScrollBar::IsEnabled(void) { return !Disabled; }
			void HorizontalScrollBar::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; }
			bool HorizontalScrollBar::IsVisible(void) { return !Invisible; }
			void HorizontalScrollBar::SetID(int _ID) { ID = _ID; }
			int HorizontalScrollBar::GetID(void) { return ID; }
			Window * HorizontalScrollBar::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void HorizontalScrollBar::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle HorizontalScrollBar::GetRectangle(void) { return ControlPosition; }
			void HorizontalScrollBar::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = 0; GetStation()->SetTimer(this, 0); } }
			void HorizontalScrollBar::LeftButtonDown(Point at)
			{
				if (_state == 1) {
					_state = 2;
					if (_part == 1) {
						GetStation()->SetTimer(this, Keyboard::GetKeyboardDelay());
						SetScrollerPosition(Position - Line);
					} else if (_part == 2) {
						GetStation()->SetTimer(this, Keyboard::GetKeyboardDelay());
						SetScrollerPosition(Position + Line);
					} else if (_part == 0) {
						Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
						Box scroller = GetScrollerBox(my);
						_mpos = at.x;
						_sd = (_mpos >= scroller.Left) ? 2 : 1;
						if (_sd == 1) SetScrollerPosition(Position - max(Page, 1));
						else SetScrollerPosition(Position + max(Page, 1));
						scroller = GetScrollerBox(my);
						int sl = scroller.Left;
						int sh = scroller.Right;
						if ((_sd == 1 && (_mpos >= sl)) || (_sd == 2 && (_mpos < sh))) _sd = 0;
						else GetStation()->SetTimer(this, Keyboard::GetKeyboardDelay());
					} else if (_part == 3) {
						_mpos = at.x;
						_sd = _mpos;
						_ipos = Position;
					}
				}
			}
			void HorizontalScrollBar::LeftButtonUp(Point at) { if (_state == 2) ReleaseCapture(); }
			void HorizontalScrollBar::MouseMove(Point at)
			{
				Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				Box left = Box(my.Left, my.Top, my.Left + my.Bottom - my.Top, my.Bottom);
				Box right = Box(my.Right - my.Bottom + my.Top, my.Top, my.Right, my.Bottom);
				Box scroller = GetScrollerBox(my);
				if (_state == 0 || _state == 1) {
					SetCapture();
					_state = 1;
					if (GetStation()->HitTest(GetStation()->GetCursorPos()) != this) {
						ReleaseCapture();
					} else {
						if (left.IsInside(at)) _part = 1;
						else if (right.IsInside(at)) _part = 2;
						else if (scroller.IsInside(at)) _part = 3;
						else _part = 0;
					}
				} else {
					if (_part == 0) {
						_mpos = at.x;
					} else if (_part == 3) {
						if (RangeMaximal == RangeMinimal) return;
						int w = my.Right;
						int h = my.Bottom;
						_mpos = at.x;
						int dx = _mpos - _sd;
						double unit = Page ? (double(w - h - h) / (RangeMaximal - RangeMinimal + 1)) : (double(w - h - h - h) / (RangeMaximal - RangeMinimal));
						double hpos = double(_ipos) + double(dx) / unit;
						SetScrollerPosition(int(hpos + 0.5));
					}
				}
			}
			void HorizontalScrollBar::Timer(void)
			{
				GetStation()->SetTimer(this, Keyboard::GetKeyboardSpeed());
				if (_part == 1) {
					SetScrollerPosition(Position - Line);
				} else if (_part == 2) {
					SetScrollerPosition(Position + Line);
				} else if (_part == 0) {
					if (_sd) {
						Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
						Box scroller = GetScrollerBox(my);
						if (_sd == 1) SetScrollerPosition(Position - max(Page, 1));
						else SetScrollerPosition(Position + max(Page, 1));
						scroller = GetScrollerBox(my);
						int sl = scroller.Left;
						int sh = scroller.Right;
						if ((_sd == 1 && _mpos >= sl) || (_sd == 2 && _mpos < sh)) { _sd = 0; GetStation()->SetTimer(this, 0); }
					}
				}
			}
			string HorizontalScrollBar::GetControlClass(void) { return L"HorizontalScrollBar"; }
			Box HorizontalScrollBar::GetScrollerBox(const Box & at)
			{
				if (_state == 2 && _part == 3) {
					int width = at.Bottom - at.Top;
					int pmin = at.Left + width, pmax = at.Right - width;
					Box scroller;
					if (Page == 0) {
						if (RangeMinimal == RangeMaximal) scroller = Box(pmin, at.Top, pmin + width, at.Bottom);
						else {
							int lpos = pmin + int32(int64(pmax - pmin - width) * (_ipos - RangeMinimal) / (RangeMaximal - RangeMinimal)) + (_mpos - _sd);
							scroller = Box(lpos, at.Top, lpos + width, at.Bottom);
						}
					} else {
						int lpos = pmin + int32(int64(pmax - pmin) * (_ipos - RangeMinimal) / (RangeMaximal - RangeMinimal + 1)) + (_mpos - _sd);
						int hpos = pmin + int32(int64(pmax - pmin) * (min(_ipos + Page, RangeMaximal + 1) - RangeMinimal) / (RangeMaximal - RangeMinimal + 1)) + (_mpos - _sd);
						scroller = Box(lpos, at.Top, hpos, at.Bottom);
					}
					if (scroller.Right >= at.Right - width) {
						int d = scroller.Right - at.Right + width;
						scroller.Left -= d;
						scroller.Right -= d;
					}
					if (scroller.Left < at.Left + width) {
						int d = at.Left + width - scroller.Left;
						scroller.Left += d;
						scroller.Right += d;
					}
					return scroller;
				} else {
					int pmin = at.Bottom - at.Top, pmax = at.Right - at.Left - at.Bottom + at.Top;
					if (Page == 0) {
						if (RangeMaximal == RangeMinimal) {
							return Box(at.Left + pmin, at.Top, at.Left + pmin + pmin, at.Bottom);
						}
						int lpos = pmin + int32(int64(pmax - pmin - pmin) * (Position - RangeMinimal) / (RangeMaximal - RangeMinimal));
						return Box(at.Left + lpos, at.Top, at.Left + lpos + pmin, at.Bottom);
					} else {
						int lpos = pmin + int32(int64(pmax - pmin) * (Position - RangeMinimal) / (RangeMaximal - RangeMinimal + 1));
						int hpos = pmin + int32(int64(pmax - pmin) * (min(Position + Page, RangeMaximal + 1) - RangeMinimal) / (RangeMaximal - RangeMinimal + 1));
						return Box(at.Left + lpos, at.Top, at.Left + hpos, at.Bottom);
					}
				}
			}
			void HorizontalScrollBar::SetScrollerPosition(int position)
			{
				int op = Position;
				int _page = max(Page, 1);
				Position = max(min(const_cast<const int &>(position), RangeMaximal - _page + 1), RangeMinimal);
				if (Position != op) GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			void HorizontalScrollBar::SetPage(int page)
			{
				Page = page;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) { Position = pos; GetParent()->RaiseEvent(ID, Event::ValueChange, this); }
			}
			void HorizontalScrollBar::SetRange(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) { Position = pos; GetParent()->RaiseEvent(ID, Event::ValueChange, this); }
			}
			void HorizontalScrollBar::SetScrollerPositionSilent(int position)
			{
				int op = Position;
				int _page = max(Page, 1);
				Position = max(min(const_cast<const int &>(position), RangeMaximal - _page + 1), RangeMinimal);
			}
			void HorizontalScrollBar::SetPageSilent(int page)
			{
				Page = page;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) Position = pos;
			}
			void HorizontalScrollBar::SetRangeSilent(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int _page = max(Page, 1);
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal - _page + 1), RangeMinimal);
				if (pos != Position) Position = pos;
			}

			VerticalTrackBar::VerticalTrackBar(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Step = 1; }
			VerticalTrackBar::VerticalTrackBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"VerticalTrackBar") throw InvalidArgumentException();
				static_cast<Template::Controls::VerticalTrackBar &>(*this) = static_cast<Template::Controls::VerticalTrackBar &>(*Template->Properties);
				if (Step < 1) Step = 1;
			}
			VerticalTrackBar::~VerticalTrackBar(void) {}
			void VerticalTrackBar::Render(const Box & at)
			{
				Box tracker_pos = GetTrackerPosition(at);
				Shape * tracker = 0;
				Shape * bar = 0;
				if (Disabled) {
					if (_tracker_disabled) tracker = _tracker_disabled;
					else if (ViewTrackerDisabled) {
						auto provider = ZeroArgumentProvider();
						_tracker_disabled.SetReference(ViewTrackerDisabled->Initialize(&provider));
						tracker = _tracker_disabled;
					}
					if (_bar_disabled) bar = _bar_disabled;
					else if (ViewBarDisabled) {
						auto provider = ZeroArgumentProvider();
						_bar_disabled.SetReference(ViewBarDisabled->Initialize(&provider));
						bar = _bar_disabled;
					}
				} else {
					Shape ** storage = 0; Template::Shape * source = 0;
					if (_state == 1) {
						storage = _tracker_hot.InnerRef();
						source = ViewTrackerHot.Inner();
					} else if (_state == 2) {
						storage = _tracker_pressed.InnerRef();
						source = ViewTrackerPressed.Inner();
					} else {
						if (GetFocus() == this) {
							storage = _tracker_focused.InnerRef();
							source = ViewTrackerFocused.Inner();
						} else {
							storage = _tracker_normal.InnerRef();
							source = ViewTrackerNormal.Inner();
						}
					}
					if (*storage) tracker = *storage;
					else if (source) {
						auto provider = ZeroArgumentProvider();
						*storage = source->Initialize(&provider);
						tracker = *storage;
					}
					if (_bar_normal) bar = _bar_normal;
					else if (ViewBarNormal) {
						auto provider = ZeroArgumentProvider();
						_bar_normal.SetReference(ViewBarNormal->Initialize(&provider));
						bar = _bar_normal;
					}
				}
				auto device = GetStation()->GetRenderingDevice();
				if (bar) bar->Render(device, at);
				if (tracker) tracker->Render(device, tracker_pos);
			}
			void VerticalTrackBar::ResetCache(void)
			{
				_tracker_normal.SetReference(0);
				_tracker_focused.SetReference(0);
				_tracker_hot.SetReference(0);
				_tracker_pressed.SetReference(0);
				_tracker_disabled.SetReference(0);
				_bar_normal.SetReference(0);
				_bar_disabled.SetReference(0);
			}
			void VerticalTrackBar::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; }
			bool VerticalTrackBar::IsEnabled(void) { return !Disabled; }
			void VerticalTrackBar::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; }
			bool VerticalTrackBar::IsVisible(void) { return !Invisible; }
			bool VerticalTrackBar::IsTabStop(void) { return true; }
			void VerticalTrackBar::SetID(int _ID) { ID = _ID; }
			int VerticalTrackBar::GetID(void) { return ID; }
			Window * VerticalTrackBar::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void VerticalTrackBar::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle VerticalTrackBar::GetRectangle(void) { return ControlPosition; }
			void VerticalTrackBar::CaptureChanged(bool got_capture) { if (!got_capture) _state = 0; }
			void VerticalTrackBar::LeftButtonDown(Point at)
			{
				SetFocus();
				Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				Box tracker = GetTrackerPosition(my);
				if (_state == 1 && tracker.IsInside(at)) {
					_state = 2;
					_mouse = at.y - GetTrackerShift(my);
				}
			}
			void VerticalTrackBar::LeftButtonUp(Point at) { ReleaseCapture(); }
			void VerticalTrackBar::MouseMove(Point at)
			{
				Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				Box tracker = GetTrackerPosition(my);
				if (_state == 0) {
					if (tracker.IsInside(at) && GetStation()->HitTest(GetStation()->GetCursorPos()) == this) {
						_state = 1;
						SetCapture();
					}
				} else if (_state == 1) {
					if (!tracker.IsInside(at) || GetStation()->HitTest(GetStation()->GetCursorPos()) != this) {
						ReleaseCapture();
					}
				} else if (_state == 2) {
					int np = MouseToTracker(my, at.y - _mouse);
					SetTrackerPosition(np);
				}
			}
			bool VerticalTrackBar::KeyDown(int key_code)
			{
				if (key_code == KeyCodes::Up) {
					SetTrackerPosition(Position - Step);
					return true;
				} else if (key_code == KeyCodes::Down) {
					SetTrackerPosition(Position + Step);
					return true;
				}
				return false;
			}
			string VerticalTrackBar::GetControlClass(void) { return L"VerticalTrackBar"; }
			Box VerticalTrackBar::GetTrackerPosition(const Box & at)
			{
				int le = TrackerWidth >> 1;
				int re = TrackerWidth - le;
				int pos = int32(int64(at.Bottom - at.Top - TrackerWidth) * (Position - RangeMinimal) / max(RangeMaximal - RangeMinimal, 1)) + le;
				return Box(at.Left, at.Top + pos - le, at.Right, at.Top + pos + re);
			}
			int VerticalTrackBar::GetTrackerShift(const Box & at)
			{
				int le = TrackerWidth >> 1;
				return int32(int64(at.Bottom - at.Top - TrackerWidth) * (Position - RangeMinimal) / max(RangeMaximal - RangeMinimal, 1)) + le;
			}
			int VerticalTrackBar::MouseToTracker(const Box & at, int mouse)
			{
				int le = TrackerWidth >> 1;
				int hd = (RangeMaximal == RangeMinimal) ? 0 : (((at.Bottom - at.Top - TrackerWidth) / (RangeMaximal - RangeMinimal)) / 2);
				return min(max(int32(int64(mouse - le + hd) * max(RangeMaximal - RangeMinimal, 1) / (at.Bottom - at.Top - TrackerWidth)) + RangeMinimal, RangeMinimal), RangeMaximal);
			}
			void VerticalTrackBar::SetTrackerPosition(int position)
			{
				int op = Position;
				Position = max(min(const_cast<const int &>(position), RangeMaximal), RangeMinimal);
				if (Position != op) GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			void VerticalTrackBar::SetRange(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal), RangeMinimal);
				if (pos != Position) { Position = pos; GetParent()->RaiseEvent(ID, Event::ValueChange, this); }
			}
			void VerticalTrackBar::SetTrackerPositionSilent(int position)
			{
				int op = Position;
				Position = max(min(const_cast<const int &>(position), RangeMaximal), RangeMinimal);
			}
			void VerticalTrackBar::SetRangeSilent(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal), RangeMinimal);
				if (pos != Position) Position = pos;
			}

			HorizontalTrackBar::HorizontalTrackBar(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Step = 1; }
			HorizontalTrackBar::HorizontalTrackBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"HorizontalTrackBar") throw InvalidArgumentException();
				static_cast<Template::Controls::HorizontalTrackBar &>(*this) = static_cast<Template::Controls::HorizontalTrackBar &>(*Template->Properties);
				if (Step < 1) Step = 1;
			}
			HorizontalTrackBar::~HorizontalTrackBar(void) {}
			void HorizontalTrackBar::Render(const Box & at)
			{
				Box tracker_pos = GetTrackerPosition(at);
				Shape * tracker = 0;
				Shape * bar = 0;
				if (Disabled) {
					if (_tracker_disabled) tracker = _tracker_disabled;
					else if (ViewTrackerDisabled) {
						auto provider = ZeroArgumentProvider();
						_tracker_disabled.SetReference(ViewTrackerDisabled->Initialize(&provider));
						tracker = _tracker_disabled;
					}
					if (_bar_disabled) bar = _bar_disabled;
					else if (ViewBarDisabled) {
						auto provider = ZeroArgumentProvider();
						_bar_disabled.SetReference(ViewBarDisabled->Initialize(&provider));
						bar = _bar_disabled;
					}
				} else {
					Shape ** storage = 0; Template::Shape * source = 0;
					if (_state == 1) {
						storage = _tracker_hot.InnerRef();
						source = ViewTrackerHot.Inner();
					} else if (_state == 2) {
						storage = _tracker_pressed.InnerRef();
						source = ViewTrackerPressed.Inner();
					} else {
						if (GetFocus() == this) {
							storage = _tracker_focused.InnerRef();
							source = ViewTrackerFocused.Inner();
						} else {
							storage = _tracker_normal.InnerRef();
							source = ViewTrackerNormal.Inner();
						}
					}
					if (*storage) tracker = *storage;
					else if (source) {
						auto provider = ZeroArgumentProvider();
						*storage = source->Initialize(&provider);
						tracker = *storage;
					}
					if (_bar_normal) bar = _bar_normal;
					else if (ViewBarNormal) {
						auto provider = ZeroArgumentProvider();
						_bar_normal.SetReference(ViewBarNormal->Initialize(&provider));
						bar = _bar_normal;
					}
				}
				auto device = GetStation()->GetRenderingDevice();
				if (bar) bar->Render(device, at);
				if (tracker) tracker->Render(device, tracker_pos);
			}
			void HorizontalTrackBar::ResetCache(void)
			{
				_tracker_normal.SetReference(0);
				_tracker_focused.SetReference(0);
				_tracker_hot.SetReference(0);
				_tracker_pressed.SetReference(0);
				_tracker_disabled.SetReference(0);
				_bar_normal.SetReference(0);
				_bar_disabled.SetReference(0);
			}
			void HorizontalTrackBar::Enable(bool enable) { Disabled = !enable; if (Disabled) _state = 0; }
			bool HorizontalTrackBar::IsEnabled(void) { return !Disabled; }
			void HorizontalTrackBar::Show(bool visible) { Invisible = !visible; if (Invisible) _state = 0; }
			bool HorizontalTrackBar::IsVisible(void) { return !Invisible; }
			bool HorizontalTrackBar::IsTabStop(void) { return true; }
			void HorizontalTrackBar::SetID(int _ID) { ID = _ID; }
			int HorizontalTrackBar::GetID(void) { return ID; }
			Window * HorizontalTrackBar::FindChild(int _ID)
			{
				if (ID == _ID && ID != 0) return this;
				else return 0;
			}
			void HorizontalTrackBar::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle HorizontalTrackBar::GetRectangle(void) { return ControlPosition; }
			void HorizontalTrackBar::CaptureChanged(bool got_capture) { if (!got_capture) _state = 0; }
			void HorizontalTrackBar::LeftButtonDown(Point at)
			{
				SetFocus();
				Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				Box tracker = GetTrackerPosition(my);
				if (_state == 1 && tracker.IsInside(at)) {
					_state = 2;
					_mouse = at.x - GetTrackerShift(my);
				}
			}
			void HorizontalTrackBar::LeftButtonUp(Point at) { ReleaseCapture(); }
			void HorizontalTrackBar::MouseMove(Point at)
			{
				Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				Box tracker = GetTrackerPosition(my);
				if (_state == 0) {
					if (tracker.IsInside(at) && GetStation()->HitTest(GetStation()->GetCursorPos()) == this) {
						_state = 1;
						SetCapture();
					}
				} else if (_state == 1) {
					if (!tracker.IsInside(at) || GetStation()->HitTest(GetStation()->GetCursorPos()) != this) {
						ReleaseCapture();
					}
				} else if (_state == 2) {
					int np = MouseToTracker(my, at.x - _mouse);
					SetTrackerPosition(np);
				}
			}
			bool HorizontalTrackBar::KeyDown(int key_code)
			{
				if (key_code == KeyCodes::Left) {
					SetTrackerPosition(Position - Step);
					return true;
				} else if (key_code == KeyCodes::Right) {
					SetTrackerPosition(Position + Step);
					return true;
				}
				return false;
			}
			string HorizontalTrackBar::GetControlClass(void) { return L"HorizontalTrackBar"; }
			Box HorizontalTrackBar::GetTrackerPosition(const Box & at)
			{
				int le = TrackerWidth >> 1;
				int re = TrackerWidth - le;
				int pos = int32(int64(at.Right - at.Left - TrackerWidth) * (Position - RangeMinimal) / max(RangeMaximal - RangeMinimal, 1)) + le;
				return Box(at.Left + pos - le, at.Top, at.Left + pos + re, at.Bottom);
			}
			int HorizontalTrackBar::GetTrackerShift(const Box & at)
			{
				int le = TrackerWidth >> 1;
				return int32(int64(at.Right - at.Left - TrackerWidth) * (Position - RangeMinimal) / max(RangeMaximal - RangeMinimal, 1)) + le;
			}
			int HorizontalTrackBar::MouseToTracker(const Box & at, int mouse)
			{
				int le = TrackerWidth >> 1;
				int hd = (RangeMaximal == RangeMinimal) ? 0 : (((at.Right - at.Left - TrackerWidth) / (RangeMaximal - RangeMinimal)) / 2);
				return min(max(int32(int64(mouse - le + hd) * max(RangeMaximal - RangeMinimal, 1) / (at.Right - at.Left - TrackerWidth)) + RangeMinimal, RangeMinimal), RangeMaximal);
			}
			void HorizontalTrackBar::SetTrackerPosition(int position)
			{
				int op = Position;
				Position = max(min(const_cast<const int &>(position), RangeMaximal), RangeMinimal);
				if (Position != op) GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			void HorizontalTrackBar::SetRange(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal), RangeMinimal);
				if (pos != Position) { Position = pos; GetParent()->RaiseEvent(ID, Event::ValueChange, this); }
			}
			void HorizontalTrackBar::SetTrackerPositionSilent(int position)
			{
				int op = Position;
				Position = max(min(const_cast<const int &>(position), RangeMaximal), RangeMinimal);
			}
			void HorizontalTrackBar::SetRangeSilent(int range_min, int range_max)
			{
				RangeMinimal = range_min; RangeMaximal = range_max;
				int pos = max(min(const_cast<const int &>(Position), RangeMaximal), RangeMinimal);
				if (pos != Position) Position = pos;
			}
		}
	}
}