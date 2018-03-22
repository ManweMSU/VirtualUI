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
			VerticalScrollBar::~VerticalScrollBar(void) {}
			void VerticalScrollBar::Render(const Box & at)
			{
				if (_period) {
					uint32 current = GetTimerValue();
					uint32 delta = current - _lasttime;
					if (delta >= _period) {
						int times = delta / _period;
						_lasttime = current - delta % _period;
						_period = Keyboard::GetKeyboardSpeed();
						if (_part == 1) {
							SetScrollerPosition(Position - times * Line);
						} else if (_part == 2) {
							SetScrollerPosition(Position + times * Line);
						} else if (_part == 0) {
							if (_sd) {
								Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
								Box scroller = GetScrollerBox(my);
								if (_sd == 1) SetScrollerPosition(Position - times * max(Page, 1));
								else SetScrollerPosition(Position + times * max(Page, 1));
								scroller = GetScrollerBox(my);
								int sl = scroller.Top;
								int sh = scroller.Bottom;
								if ((_sd == 1 && _mpos >= sl) || (_sd == 2 && _mpos < sh)) { _sd = 0; _period = 0; }
							}
						}
					}
				}
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
						_up_disabled.SetReference(ViewUpButtonDisabled->Initialize(&ZeroArgumentProvider()));
						up_shape = _up_disabled;
					}
					if (_down_disabled) down_shape = _down_disabled;
					else if (ViewDownButtonDisabled) {
						_down_disabled.SetReference(ViewDownButtonDisabled->Initialize(&ZeroArgumentProvider()));
						down_shape = _down_disabled;
					}
					if (_scroller_disabled) scroller_shape = _scroller_disabled;
					else if (ViewScrollerDisabled) {
						_scroller_disabled.SetReference(ViewScrollerDisabled->Initialize(&ZeroArgumentProvider()));
						scroller_shape = _scroller_disabled;
					}
					if (_bar_disabled) bar_shape = _bar_disabled;
					else if (ViewBarDisabled) {
						_bar_disabled.SetReference(ViewBarDisabled->Initialize(&ZeroArgumentProvider()));
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
							*storage = source->Initialize(&ZeroArgumentProvider());
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
							*storage = source->Initialize(&ZeroArgumentProvider());
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
							*storage = source->Initialize(&ZeroArgumentProvider());
							scroller_shape = *storage;
						}
					}
					if (_bar_normal) bar_shape = _bar_normal;
					else if (ViewBarNormal) {
						_bar_normal.SetReference(ViewBarNormal->Initialize(&ZeroArgumentProvider()));
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
			void VerticalScrollBar::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = 0; _period = 0; } }
			void VerticalScrollBar::LeftButtonDown(Point at)
			{
				if (_state == 1) {
					_state = 2;
					if (_part == 1) {
						_period = Keyboard::GetKeyboardDelay();
						_lasttime = GetTimerValue();
						SetScrollerPosition(Position - Line);
					} else if (_part == 2) {
						_period = Keyboard::GetKeyboardDelay();
						_lasttime = GetTimerValue();
						SetScrollerPosition(Position + Line);
					} else if (_part == 0) {
						_lasttime = GetTimerValue();
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
						else _period = Keyboard::GetKeyboardDelay();
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

			HorizontalScrollBar::HorizontalScrollBar(Window * Parent, WindowStation * Station) : Window(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Line = 1; }
			HorizontalScrollBar::HorizontalScrollBar(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : Window(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"HorizontalScrollBar") throw InvalidArgumentException();
				static_cast<Template::Controls::HorizontalScrollBar &>(*this) = static_cast<Template::Controls::HorizontalScrollBar &>(*Template->Properties);
				if (Line < 1) Line = 1;
			}
			HorizontalScrollBar::~HorizontalScrollBar(void) {}
			void HorizontalScrollBar::Render(const Box & at)
			{
				if (_period) {
					uint32 current = GetTimerValue();
					uint32 delta = current - _lasttime;
					if (delta >= _period) {
						int times = delta / _period;
						_lasttime = current - delta % _period;
						_period = Keyboard::GetKeyboardSpeed();
						if (_part == 1) {
							SetScrollerPosition(Position - times * Line);
						} else if (_part == 2) {
							SetScrollerPosition(Position + times * Line);
						} else if (_part == 0) {
							if (_sd) {
								Box my = Box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
								Box scroller = GetScrollerBox(my);
								if (_sd == 1) SetScrollerPosition(Position - times * max(Page, 1));
								else SetScrollerPosition(Position + times * max(Page, 1));
								scroller = GetScrollerBox(my);
								int sl = scroller.Left;
								int sh = scroller.Right;
								if ((_sd == 1 && _mpos >= sl) || (_sd == 2 && _mpos < sh)) { _sd = 0; _period = 0; }
							}
						}
					}
				}
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
						_left_disabled.SetReference(ViewLeftButtonDisabled->Initialize(&ZeroArgumentProvider()));
						left_shape = _left_disabled;
					}
					if (_right_disabled) right_shape = _right_disabled;
					else if (ViewRightButtonDisabled) {
						_right_disabled.SetReference(ViewRightButtonDisabled->Initialize(&ZeroArgumentProvider()));
						right_shape = _right_disabled;
					}
					if (_scroller_disabled) scroller_shape = _scroller_disabled;
					else if (ViewScrollerDisabled) {
						_scroller_disabled.SetReference(ViewScrollerDisabled->Initialize(&ZeroArgumentProvider()));
						scroller_shape = _scroller_disabled;
					}
					if (_bar_disabled) bar_shape = _bar_disabled;
					else if (ViewBarDisabled) {
						_bar_disabled.SetReference(ViewBarDisabled->Initialize(&ZeroArgumentProvider()));
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
							*storage = source->Initialize(&ZeroArgumentProvider());
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
							*storage = source->Initialize(&ZeroArgumentProvider());
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
							*storage = source->Initialize(&ZeroArgumentProvider());
							scroller_shape = *storage;
						}
					}
					if (_bar_normal) bar_shape = _bar_normal;
					else if (ViewBarNormal) {
						_bar_normal.SetReference(ViewBarNormal->Initialize(&ZeroArgumentProvider()));
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
			void HorizontalScrollBar::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = 0; _period = 0; } }
			void HorizontalScrollBar::LeftButtonDown(Point at)
			{
				if (_state == 1) {
					_state = 2;
					if (_part == 1) {
						_period = Keyboard::GetKeyboardDelay();
						_lasttime = GetTimerValue();
						SetScrollerPosition(Position - Line);
					} else if (_part == 2) {
						_period = Keyboard::GetKeyboardDelay();
						_lasttime = GetTimerValue();
						SetScrollerPosition(Position + Line);
					} else if (_part == 0) {
						_lasttime = GetTimerValue();
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
						else _period = Keyboard::GetKeyboardDelay();
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
		}
	}
}