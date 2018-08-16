#include "GroupControls.h"

#include "ButtonControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			ControlGroup::ControlGroup(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ControlGroup::ControlGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"ControlGroup") throw InvalidArgumentException();
				static_cast<Template::Controls::ControlGroup &>(*this) = static_cast<Template::Controls::ControlGroup &>(*Template->Properties);
				Constructor::ConstructChildren(this, Template);
			}
			ControlGroup::~ControlGroup(void) {}
			void ControlGroup::Render(const Box & at)
			{
				auto args = ZeroArgumentProvider();
				if (!_background && Background) _background.SetReference(Background->Initialize(&args));
				if (_background) _background->Render(GetStation()->GetRenderingDevice(), at);
				ParentWindow::Render(at);
			}
			void ControlGroup::ResetCache(void) { _background.SetReference(0); ParentWindow::ResetCache(); }
			void ControlGroup::Enable(bool enable) {}
			bool ControlGroup::IsEnabled(void) { return true; }
			void ControlGroup::Show(bool visible) { Invisible = !visible; }
			bool ControlGroup::IsVisible(void) { return !Invisible; }
			void ControlGroup::SetID(int _ID) { ID = _ID; }
			int ControlGroup::GetID(void) { return ID; }
			void ControlGroup::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle ControlGroup::GetRectangle(void) { return ControlPosition; }
			void ControlGroup::SetInnerControls(Template::ControlTemplate * Template)
			{
				while (ChildrenCount()) Child(0)->Destroy();
				if (Template) Constructor::ConstructChildren(this, Template);
			}

			RadioButtonGroup::RadioButtonGroup(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			RadioButtonGroup::RadioButtonGroup(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"RadioButtonGroup") throw InvalidArgumentException();
				static_cast<Template::Controls::RadioButtonGroup &>(*this) = static_cast<Template::Controls::RadioButtonGroup &>(*Template->Properties);
				for (int i = 0; i < Template->Children.Length(); i++) {
					Station->CreateWindow<RadioButton>(this, &Template->Children[i]);
				}
			}
			RadioButtonGroup::~RadioButtonGroup(void) {}
			void RadioButtonGroup::Render(const Box & at)
			{
				auto args = ZeroArgumentProvider();
				if (!_background && Background) _background.SetReference(Background->Initialize(&args));
				if (_background) _background->Render(GetStation()->GetRenderingDevice(), at);
				ParentWindow::Render(at);
			}
			void RadioButtonGroup::ResetCache(void) { _background.SetReference(0); ParentWindow::ResetCache(); }
			void RadioButtonGroup::Enable(bool enable) { Disabled = !enable; }
			bool RadioButtonGroup::IsEnabled(void) { return !Disabled; }
			void RadioButtonGroup::Show(bool visible) { Invisible = !visible; }
			bool RadioButtonGroup::IsVisible(void) { return !Invisible; }
			void RadioButtonGroup::SetID(int _ID) { ID = _ID; }
			int RadioButtonGroup::GetID(void) { return ID; }
			void RadioButtonGroup::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle RadioButtonGroup::GetRectangle(void) { return ControlPosition; }
			void RadioButtonGroup::CheckRadioButton(Window * window) { for (int i = 0; i < ChildrenCount(); i++) static_cast<RadioButton *>(Child(i))->Checked = window == Child(i); }
			void RadioButtonGroup::CheckRadioButton(int ID) { for (int i = 0; i < ChildrenCount(); i++) static_cast<RadioButton *>(Child(i))->Checked = ID == Child(i)->GetID(); }
			int RadioButtonGroup::GetCheckedButton(void)
			{
				for (int i = 0; i < ChildrenCount(); i++) if (static_cast<RadioButton *>(Child(i))->Checked) return Child(i)->GetID();
				return 0;
			}

			ScrollBoxVirtual::ScrollBoxVirtual(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) {}
			ScrollBoxVirtual::~ScrollBoxVirtual(void) {}
			void ScrollBoxVirtual::SetPosition(const Box & box) { WindowPosition = box; }
			void ScrollBoxVirtual::ArrangeChildren(void) { GetParent()->ArrangeChildren(); }

			ScrollBox::ScrollBox(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				_virtual = Station->CreateWindow<ScrollBoxVirtual>(this);
				ResetCache();
			}
			ScrollBox::ScrollBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"ScrollBox") throw InvalidArgumentException();
				static_cast<Template::Controls::ScrollBox &>(*this) = static_cast<Template::Controls::ScrollBox &>(*Template->Properties);
				_virtual = Station->CreateWindow<ScrollBoxVirtual>(this);
				ResetCache();
				Constructor::ConstructChildren(_virtual, Template);
			}
			ScrollBox::~ScrollBox(void) {}
			void ScrollBox::Render(const Box & at)
			{
				if (!_background && Background) {
					auto args = ZeroArgumentProvider();
					_background.SetReference(Background->Initialize(&args));
				}
				auto device = GetStation()->GetRenderingDevice();
				if (_background) _background->Render(device, at);
				if (_show_vs) {
					auto vs_box = _vertical->GetPosition();
					auto vsr_box = Box(vs_box.Left + at.Left, vs_box.Top + at.Top,
						vs_box.Right + at.Left, vs_box.Bottom + at.Top);
					device->PushClip(vsr_box);
					_vertical->Render(vsr_box);
					device->PopClip();
				}
				if (_show_hs) {
					auto hs_box = _horizontal->GetPosition();
					auto hsr_box = Box(hs_box.Left + at.Left, hs_box.Top + at.Top,
						hs_box.Right + at.Left, hs_box.Bottom + at.Top);
					device->PushClip(hsr_box);
					_horizontal->Render(hsr_box);
					device->PopClip();
				}
				auto box = _virtual->GetPosition();
				auto width = WindowPosition.Right - WindowPosition.Left;
				auto height = WindowPosition.Bottom - WindowPosition.Top;
				auto vbox = Box(Border + at.Left, Border + at.Top,
					width - Border - (_show_vs ? VerticalScrollSize : 0) + at.Left,
					height - Border - (_show_hs ? HorizontalScrollSize : 0) + at.Top);
				auto render_box = Box(box.Left + at.Left, box.Top + at.Top,
					box.Right + at.Left, box.Bottom + at.Top);
				device->PushClip(vbox);
				_virtual->Render(render_box);
				device->PopClip();
			}
			void ScrollBox::ResetCache(void)
			{
				_background.SetReference(0);
				for (int i = 0; i < ChildrenCount(); i++) Child(i)->ResetCache();
				if (_vertical) {
					_vertical->Destroy();
					_vertical = 0;
				}
				if (_horizontal) {
					_horizontal->Destroy();
					_horizontal = 0;
				}
				_vertical = GetStation()->CreateWindow<VerticalScrollBar>(this, static_cast<Template::Controls::ScrollBox *>(this));
				_horizontal = GetStation()->CreateWindow<HorizontalScrollBar>(this, static_cast<Template::Controls::ScrollBox *>(this));
				_vertical->Line = VerticalLine;
				_horizontal->Line = HorizontalLine;
				_vertical->Disabled = Disabled;
				_horizontal->Disabled = Disabled;
				ArrangeChildren();
			}
			void ScrollBox::ArrangeChildren(void)
			{
				int mw = 0, mh = 0;
				for (int i = 0; i < _virtual->ChildrenCount(); i++) {
					auto position = _virtual->Child(i)->GetRectangle();
					if (position.IsValid()) {
						if (position.Right.Anchor == 0.0) {
							int w = position.Right.Absolute + int(position.Right.Zoom * Zoom);
							if (w > mw) mw = w;
						}
						if (position.Bottom.Anchor == 0.0) {
							int h = position.Bottom.Absolute + int(position.Bottom.Zoom * Zoom);
							if (h > mh) mh = h;
						}
					} else {
						auto box = _virtual->Child(i)->GetPosition();
						if (box.Right > mw) mw = box.Right;
						if (box.Bottom > mh) mh = box.Bottom;
					}
				}
				int available_width = WindowPosition.Right - WindowPosition.Left - (Border << 1);
				int available_height = WindowPosition.Bottom - WindowPosition.Top - (Border << 1);
				_show_vs = (mh > available_height) || (mw > available_width && mh > available_height - HorizontalScrollSize);
				_show_hs = (mw > available_width) || (mh > available_height && mw > available_width - VerticalScrollSize);
				if (_show_vs) available_width -= VerticalScrollSize;
				if (_show_hs) available_height -= HorizontalScrollSize;
				_vertical->Show(_show_vs);
				_vertical->SetRange(0, mh - 1);
				_vertical->SetPage(available_height);
				_horizontal->Show(_show_hs);
				_horizontal->SetRange(0, mw - 1);
				_horizontal->SetPage(available_width);
				if (_show_vs) {
					_vertical->SetPosition(Box(Border + available_width, Border,
						Border + available_width + VerticalScrollSize, Border + available_height));
				}
				if (_show_hs) {
					_horizontal->SetPosition(Box(Border, Border + available_height,
						Border + available_width, Border + available_height + HorizontalScrollSize));
				}
				Box align_box = Box(0, 0, available_width, available_height);
				auto dx = -_horizontal->Position;
				auto dy = -_vertical->Position;
				_virtual->SetPosition(Box(Border + dx, Border + dy, Border + dx + mw, Border + dy + mh));
				for (int i = 0; i < _virtual->ChildrenCount(); i++) {
					auto rect = _virtual->Child(i)->GetRectangle();
					if (rect.IsValid()) {
						_virtual->Child(i)->SetPosition(Box(rect, align_box));
					}
				}
			}
			void ScrollBox::Enable(bool enable) { Disabled = !enable; _vertical->Enable(enable); _horizontal->Enable(enable); }
			bool ScrollBox::IsEnabled(void) { return !Disabled; }
			void ScrollBox::Show(bool visible) { Invisible = !visible; }
			bool ScrollBox::IsVisible(void) { return !Invisible; }
			void ScrollBox::SetID(int _ID) { ID = _ID; }
			int ScrollBox::GetID(void) { return ID; }
			void ScrollBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle ScrollBox::GetRectangle(void) { return ControlPosition; }
			void ScrollBox::SetPosition(const Box & box) { WindowPosition = box; ArrangeChildren(); }
			void ScrollBox::RaiseEvent(int ID, Event event, Window * sender)
			{
				if (sender == _vertical || sender == _horizontal) {
					auto pos = _virtual->GetPosition();
					auto vw = pos.Right - pos.Left;
					auto vh = pos.Bottom - pos.Top;
					auto dx = -_horizontal->Position;
					auto dy = -_vertical->Position;
					_virtual->SetPosition(Box(Border + dx, Border + dy, Border + dx + vw, Border + dy + vh));
				} else GetParent()->RaiseEvent(ID, event, sender);
			}
			void ScrollBox::ScrollVertically(double delta) { _vertical->SetScrollerPosition(_vertical->Position + int(delta * double(_vertical->Line))); }
			void ScrollBox::ScrollHorizontally(double delta) { _horizontal->SetScrollerPosition(_horizontal->Position + int(delta * double(_horizontal->Line))); }
			Window * ScrollBox::HitTest(Point at)
			{
				if (Disabled) return this;
				auto vs_box = _vertical->GetPosition();
				if (vs_box.IsInside(at)) {
					return _vertical->HitTest(Point(at.x - vs_box.Left, at.y - vs_box.Top));
				}
				auto hs_box = _horizontal->GetPosition();
				if (hs_box.IsInside(at)) {
					return _horizontal->HitTest(Point(at.x - hs_box.Left, at.y - hs_box.Top));
				}
				auto box = _virtual->GetPosition();
				auto width = WindowPosition.Right - WindowPosition.Left;
				auto height = WindowPosition.Bottom - WindowPosition.Top;
				auto vbox = Box(Border, Border, width - Border - (_show_vs ? VerticalScrollSize : 0), height - Border - (_show_hs ? HorizontalScrollSize : 0));
				if (vbox.IsInside(at)) {
					return _virtual->HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
				return this;
			}
			Window * ScrollBox::GetVirtualGroup(void) { return _virtual; }

			SplitBoxPart::SplitBoxPart(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Size = 0; }
			SplitBoxPart::SplitBoxPart(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"SplitBoxPart") throw InvalidArgumentException();
				static_cast<Template::Controls::SplitBoxPart &>(*this) = static_cast<Template::Controls::SplitBoxPart &>(*Template->Properties);
				Size = 0;
				Constructor::ConstructChildren(this, Template);
			}
			SplitBoxPart::~SplitBoxPart(void) {}
			void SplitBoxPart::SetID(int _ID) { ID = _ID; }
			int SplitBoxPart::GetID(void) { return ID; }
			void SplitBoxPart::SetRectangle(const Rectangle & rect) { ControlPosition = rect; }
			Rectangle SplitBoxPart::GetRectangle(void) { return ControlPosition; }

			VerticalSplitBox::VerticalSplitBox(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); ResetCache(); }
			VerticalSplitBox::VerticalSplitBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"VerticalSplitBox") throw InvalidArgumentException();
				static_cast<Template::Controls::VerticalSplitBox &>(*this) = static_cast<Template::Controls::VerticalSplitBox &>(*Template->Properties);
				ResetCache();
				for (int i = 0; i < Template->Children.Length(); i++) auto part = Station->CreateWindow<SplitBoxPart>(this, &Template->Children[i]);
			}
			VerticalSplitBox::~VerticalSplitBox(void) {}
			void VerticalSplitBox::Render(const Box & at)
			{
				for (int i = 0; i < ChildrenCount() - 1; i++) {
					auto box = Child(i)->GetPosition();
					auto splitter = Box(at.Left, at.Top + box.Bottom, at.Right, at.Top + box.Bottom + SplitterSize);
					auto device = GetStation()->GetRenderingDevice();
					if (_part == i && _state == 2) {
						if (_splitter_pressed) _splitter_pressed->Render(device, splitter);
					} else if (_part == i && _state == 1) {
						if (_splitter_hot) _splitter_hot->Render(device, splitter);
					} else {
						if (_splitter_normal) _splitter_normal->Render(device, splitter);
					}
				}
				ParentWindow::Render(at);
			}
			void VerticalSplitBox::ResetCache(void)
			{
				auto args = ZeroArgumentProvider();
				_splitter_normal.SetReference(ViewSplitterNormal ? ViewSplitterNormal->Initialize(&args) : 0);
				_splitter_hot.SetReference(ViewSplitterHot ? ViewSplitterHot->Initialize(&args) : 0);
				_splitter_pressed.SetReference(ViewSplitterPressed ? ViewSplitterPressed->Initialize(&args) : 0);
				ParentWindow::ResetCache();
			}
			void VerticalSplitBox::Enable(bool enable) { Disabled = !enable; if (Disabled) { _state = 0; _part = -1; } }
			bool VerticalSplitBox::IsEnabled(void) { return !Disabled; }
			void VerticalSplitBox::Show(bool visible) { Invisible = !visible; if (Invisible) { _state = 0; _part = -1; } }
			bool VerticalSplitBox::IsVisible(void) { return !Invisible; }
			void VerticalSplitBox::SetID(int _ID) { ID = _ID; }
			int VerticalSplitBox::GetID(void) { return ID; }
			void VerticalSplitBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle VerticalSplitBox::GetRectangle(void) { return ControlPosition; }
			void VerticalSplitBox::ArrangeChildren(void) {}
			void VerticalSplitBox::SetPosition(const Box & box)
			{
				WindowPosition = box;
				int size = box.Bottom - box.Top;
				int available_size = size - (ChildrenCount() - 1) * SplitterSize;
				int stretch_part = -1;
				if (!_initialized) {
					for (int i = 0; i < ChildrenCount(); i++) {
						auto child = static_cast<SplitBoxPart *>(Child(i));
						auto rect = child->GetRectangle();
						child->SetRectangle(Rectangle::Invalid());
						int part_size = rect.Bottom.Absolute + int(rect.Bottom.Zoom * Zoom) + int(available_size * rect.Bottom.Anchor);
						child->Size = part_size;
					}
					_initialized = true;
				}
				int used_size = 0;
				for (int i = 0; i < ChildrenCount(); i++) {
					auto child = static_cast<SplitBoxPart *>(Child(i));
					used_size += child->Size;
					if (child->Stretch) stretch_part = i;
				}
				if (used_size != available_size) {
					int extra = available_size - used_size;
					if (stretch_part >= 0) {
						auto child = static_cast<SplitBoxPart *>(Child(stretch_part));
						child->Size += extra;
						if (child->Size < child->MinimalSize) {
							extra = child->Size - child->MinimalSize;
							child->Size = child->MinimalSize;
						} else extra = 0;
					}
					if (extra) {
						for (int i = 0; i < ChildrenCount(); i++) {
							auto child = static_cast<SplitBoxPart *>(Child(i));
							int diff = (i == ChildrenCount() - 1) ? (-extra) : (min(child->Size - child->MinimalSize, -extra));
							child->Size -= diff;
							extra += diff;
							if (!extra) break;
						}
					}
				}
				int pos = 0;
				int width = box.Right - box.Left;
				for (int i = 0; i < ChildrenCount(); i++) {
					int part_size = static_cast<SplitBoxPart *>(Child(i))->Size;
					Child(i)->SetPosition(Box(0, pos, width, pos + part_size));
					pos += part_size + SplitterSize;
				}
			}
			void VerticalSplitBox::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = -1; } }
			void VerticalSplitBox::LeftButtonDown(Point at)
			{
				if (_state == 1 && _part != -1) {
					_state = 2;
					_mouse = at.y;
				}
			}
			void VerticalSplitBox::LeftButtonUp(Point at) { ReleaseCapture(); }
			void VerticalSplitBox::MouseMove(Point at)
			{
				if (_state != 2) {
					int pos = 0;
					_part = -1;
					if (GetStation()->HitTest(GetStation()->GetCursorPos()) == this) {
						for (int i = 0; i < ChildrenCount() - 1; i++) {
							auto child = static_cast<SplitBoxPart *>(Child(i));
							if (at.y >= pos + child->Size && at.y < pos + child->Size + SplitterSize) { _part = i; break; }
							pos += child->Size + SplitterSize;
						}
					}
					if (_part != -1) {
						_state = 1;
						SetCapture();
					} else if (_state == 1) ReleaseCapture();
				} else {
					int mouse = at.y;
					int ds = at.y - _mouse;
					auto left = static_cast<SplitBoxPart *>(Child(_part));
					auto right = static_cast<SplitBoxPart *>(Child(_part + 1));
					if (left->Size + ds < left->MinimalSize) {
						mouse += left->MinimalSize - left->Size - ds;
						ds = left->MinimalSize - left->Size;
					} else if (right->Size - ds < right->MinimalSize) {
						mouse += right->Size - right->MinimalSize - ds;
						ds = right->Size - right->MinimalSize;
					}
					if (ds) {
						_mouse = mouse;
						left->Size += ds;
						right->Size -= ds;
						auto left_box = left->GetPosition();
						auto right_box = right->GetPosition();
						left_box.Bottom += ds;
						right_box.Top += ds;
						left->SetPosition(left_box);
						right->SetPosition(right_box);
					}
				}
			}
			void VerticalSplitBox::SetCursor(Point at) { GetStation()->SetCursor(GetStation()->GetSystemCursor(SystemCursor::SizeUpDown)); }

			HorizontalSplitBox::HorizontalSplitBox(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); ResetCache(); }
			HorizontalSplitBox::HorizontalSplitBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station)
			{
				if (Template->Properties->GetTemplateClass() != L"HorizontalSplitBox") throw InvalidArgumentException();
				static_cast<Template::Controls::HorizontalSplitBox &>(*this) = static_cast<Template::Controls::HorizontalSplitBox &>(*Template->Properties);
				ResetCache();
				for (int i = 0; i < Template->Children.Length(); i++) auto part = Station->CreateWindow<SplitBoxPart>(this, &Template->Children[i]);
			}
			HorizontalSplitBox::~HorizontalSplitBox(void) {}
			void HorizontalSplitBox::Render(const Box & at)
			{
				for (int i = 0; i < ChildrenCount() - 1; i++) {
					auto box = Child(i)->GetPosition();
					auto splitter = Box(at.Left + box.Right, at.Top, at.Left + box.Right + SplitterSize, at.Bottom);
					auto device = GetStation()->GetRenderingDevice();
					if (_part == i && _state == 2) {
						if (_splitter_pressed) _splitter_pressed->Render(device, splitter);
					} else if (_part == i && _state == 1) {
						if (_splitter_hot) _splitter_hot->Render(device, splitter);
					} else {
						if (_splitter_normal) _splitter_normal->Render(device, splitter);
					}
				}
				ParentWindow::Render(at);
			}
			void HorizontalSplitBox::ResetCache(void)
			{
				auto args = ZeroArgumentProvider();
				_splitter_normal.SetReference(ViewSplitterNormal ? ViewSplitterNormal->Initialize(&args) : 0);
				_splitter_hot.SetReference(ViewSplitterHot ? ViewSplitterHot->Initialize(&args) : 0);
				_splitter_pressed.SetReference(ViewSplitterPressed ? ViewSplitterPressed->Initialize(&args) : 0);
				ParentWindow::ResetCache();
			}
			void HorizontalSplitBox::Enable(bool enable) { Disabled = !enable; if (Disabled) { _state = 0; _part = -1; } }
			bool HorizontalSplitBox::IsEnabled(void) { return !Disabled; }
			void HorizontalSplitBox::Show(bool visible) { Invisible = !visible; if (Invisible) { _state = 0; _part = -1; } }
			bool HorizontalSplitBox::IsVisible(void) { return !Invisible; }
			void HorizontalSplitBox::SetID(int _ID) { ID = _ID; }
			int HorizontalSplitBox::GetID(void) { return ID; }
			void HorizontalSplitBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle HorizontalSplitBox::GetRectangle(void) { return ControlPosition; }
			void HorizontalSplitBox::ArrangeChildren(void) {}
			void HorizontalSplitBox::SetPosition(const Box & box)
			{
				WindowPosition = box;
				int size = box.Right - box.Left;
				int available_size = size - (ChildrenCount() - 1) * SplitterSize;
				int stretch_part = -1;
				if (!_initialized) {
					for (int i = 0; i < ChildrenCount(); i++) {
						auto child = static_cast<SplitBoxPart *>(Child(i));
						auto rect = child->GetRectangle();
						child->SetRectangle(Rectangle::Invalid());
						int part_size = rect.Right.Absolute + int(rect.Right.Zoom * Zoom) + int(available_size * rect.Right.Anchor);
						child->Size = part_size;
					}
					_initialized = true;
				}
				int used_size = 0;
				for (int i = 0; i < ChildrenCount(); i++) {
					auto child = static_cast<SplitBoxPart *>(Child(i));
					used_size += child->Size;
					if (child->Stretch) stretch_part = i;
				}
				if (used_size != available_size) {
					int extra = available_size - used_size;
					if (stretch_part >= 0) {
						auto child = static_cast<SplitBoxPart *>(Child(stretch_part));
						child->Size += extra;
						if (child->Size < child->MinimalSize) {
							extra = child->Size - child->MinimalSize;
							child->Size = child->MinimalSize;
						} else extra = 0;
					}
					if (extra) {
						for (int i = 0; i < ChildrenCount(); i++) {
							auto child = static_cast<SplitBoxPart *>(Child(i));
							int diff = (i == ChildrenCount() - 1) ? (-extra) : (min(child->Size - child->MinimalSize, -extra));
							child->Size -= diff;
							extra += diff;
							if (!extra) break;
						}
					}
				}
				int pos = 0;
				int height = box.Bottom - box.Top;
				for (int i = 0; i < ChildrenCount(); i++) {
					int part_size = static_cast<SplitBoxPart *>(Child(i))->Size;
					Child(i)->SetPosition(Box(pos, 0, pos + part_size, height));
					pos += part_size + SplitterSize;
				}
			}
			void HorizontalSplitBox::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = -1; } }
			void HorizontalSplitBox::LeftButtonDown(Point at)
			{
				if (_state == 1 && _part != -1) {
					_state = 2;
					_mouse = at.x;
				}
			}
			void HorizontalSplitBox::LeftButtonUp(Point at) { ReleaseCapture(); }
			void HorizontalSplitBox::MouseMove(Point at)
			{
				if (_state != 2) {
					int pos = 0;
					_part = -1;
					if (GetStation()->HitTest(GetStation()->GetCursorPos()) == this) {
						for (int i = 0; i < ChildrenCount() - 1; i++) {
							auto child = static_cast<SplitBoxPart *>(Child(i));
							if (at.x >= pos + child->Size && at.x < pos + child->Size + SplitterSize) { _part = i; break; }
							pos += child->Size + SplitterSize;
						}
					}
					if (_part != -1) {
						_state = 1;
						SetCapture();
					} else if (_state == 1) ReleaseCapture();
				} else {
					int mouse = at.x;
					int ds = at.x - _mouse;
					auto left = static_cast<SplitBoxPart *>(Child(_part));
					auto right = static_cast<SplitBoxPart *>(Child(_part + 1));
					if (left->Size + ds < left->MinimalSize) {
						mouse += left->MinimalSize - left->Size - ds;
						ds = left->MinimalSize - left->Size;
					} else if (right->Size - ds < right->MinimalSize) {
						mouse += right->Size - right->MinimalSize - ds;
						ds = right->Size - right->MinimalSize;
					}
					if (ds) {
						_mouse = mouse;
						left->Size += ds;
						right->Size -= ds;
						auto left_box = left->GetPosition();
						auto right_box = right->GetPosition();
						left_box.Right += ds;
						right_box.Left += ds;
						left->SetPosition(left_box);
						right->SetPosition(right_box);
					}
				}
			}
			void HorizontalSplitBox::SetCursor(Point at) { GetStation()->SetCursor(GetStation()->GetSystemCursor(SystemCursor::SizeLeftRight)); }
		}
	}
}