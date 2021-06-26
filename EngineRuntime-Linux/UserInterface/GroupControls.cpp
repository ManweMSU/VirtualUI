#include "GroupControls.h"

#include "ButtonControls.h"
#include "../Interfaces/KeyCodes.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace ArgumentService
			{
				class BookmarkArgumentProvider : public IArgumentProvider
				{
				public:
					IFont * font;
					const string * text;
					BookmarkArgumentProvider(IFont * fnt, const string * txt) : font(fnt), text(txt) {}
					virtual void GetArgument(const string & name, int * value) override { *value = 0; }
					virtual void GetArgument(const string & name, double * value) override { *value = 0.0; }
					virtual void GetArgument(const string & name, Color * value) override { *value = 0; }
					virtual void GetArgument(const string & name, string * value) override
					{
						if (name == L"Text") *value = *text;
						else *value = L"";
					}
					virtual void GetArgument(const string & name, ITexture ** value) override { *value = 0; }
					virtual void GetArgument(const string & name, IFont ** value) override
					{
						if (name == L"Font" && font) {
							*value = font;
							(*value)->Retain();
						} else *value = 0;
					}
				};
			}
			ControlGroup::ControlGroup(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			ControlGroup::ControlGroup(Template::ControlTemplate * Template, IControlFactory * Factory)
			{
				if (Template->Properties->GetTemplateClass() != L"ControlGroup") throw InvalidArgumentException();
				static_cast<Template::Controls::ControlGroup &>(*this) = static_cast<Template::Controls::ControlGroup &>(*Template->Properties);
				CreateChildrenControls(this, Template, Factory);
			}
			ControlGroup::~ControlGroup(void) {}
			void ControlGroup::Render(IRenderingDevice * device, const Box & at)
			{
				auto args = ZeroArgumentProvider();
				if (!_background && Background) _background.SetReference(Background->Initialize(&args));
				if (_background) _background->Render(device, at);
				device->PushClip(at);
				ParentControl::Render(device, at);
				device->PopClip();
			}
			void ControlGroup::ResetCache(void) { _background.SetReference(0); ParentControl::ResetCache(); }
			void ControlGroup::Enable(bool enable) {}
			bool ControlGroup::IsEnabled(void) { return true; }
			void ControlGroup::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool ControlGroup::IsVisible(void) { return !Invisible; }
			void ControlGroup::SetID(int _ID) { ID = _ID; }
			int ControlGroup::GetID(void) { return ID; }
			void ControlGroup::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle ControlGroup::GetRectangle(void) { return ControlPosition; }
			string ControlGroup::GetControlClass(void) { return L"ControlGroup"; }
			void ControlGroup::SetInnerControls(Template::ControlTemplate * Template, IControlFactory * Factory)
			{
				while (ChildrenCount()) RemoveChildAt(0);
				if (Template) CreateChildrenControls(this, Template, Factory);
			}

			RadioButtonGroup::RadioButtonGroup(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			RadioButtonGroup::RadioButtonGroup(Template::ControlTemplate * Template)
			{
				if (Template->Properties->GetTemplateClass() != L"RadioButtonGroup") throw InvalidArgumentException();
				static_cast<Template::Controls::RadioButtonGroup &>(*this) = static_cast<Template::Controls::RadioButtonGroup &>(*Template->Properties);
				for (int i = 0; i < Template->Children.Length(); i++) {
					SafePointer<Control> child = new RadioButton(&Template->Children[i]);
					AddChild(child);
				}
			}
			RadioButtonGroup::~RadioButtonGroup(void) {}
			void RadioButtonGroup::Render(IRenderingDevice * device, const Box & at)
			{
				auto args = ZeroArgumentProvider();
				if (!_background && Background) _background.SetReference(Background->Initialize(&args));
				if (_background) _background->Render(device, at);
				ParentControl::Render(device, at);
			}
			void RadioButtonGroup::ResetCache(void) { _background.SetReference(0); ParentControl::ResetCache(); }
			void RadioButtonGroup::Enable(bool enable) { Disabled = !enable; Invalidate(); }
			bool RadioButtonGroup::IsEnabled(void) { return !Disabled; }
			void RadioButtonGroup::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool RadioButtonGroup::IsVisible(void) { return !Invisible; }
			void RadioButtonGroup::SetID(int _ID) { ID = _ID; }
			int RadioButtonGroup::GetID(void) { return ID; }
			void RadioButtonGroup::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle RadioButtonGroup::GetRectangle(void) { return ControlPosition; }
			string RadioButtonGroup::GetControlClass(void) { return L"RadioButtonGroup"; }
			void RadioButtonGroup::CheckRadioButton(Control * window) { for (int i = 0; i < ChildrenCount(); i++) static_cast<RadioButton *>(Child(i))->Checked = window == Child(i); Invalidate(); }
			void RadioButtonGroup::CheckRadioButton(int ID) { for (int i = 0; i < ChildrenCount(); i++) static_cast<RadioButton *>(Child(i))->Checked = ID == Child(i)->GetID(); Invalidate(); }
			int RadioButtonGroup::GetCheckedButton(void)
			{
				for (int i = 0; i < ChildrenCount(); i++) if (static_cast<RadioButton *>(Child(i))->Checked) return Child(i)->GetID();
				return 0;
			}

			ScrollBoxVirtual::ScrollBoxVirtual(void) {}
			ScrollBoxVirtual::~ScrollBoxVirtual(void) {}
			void ScrollBoxVirtual::SetPosition(const Box & box) { ControlBoundaries = box; }
			void ScrollBoxVirtual::ArrangeChildren(void) { if (GetParent()) GetParent()->ArrangeChildren(); }
			string ScrollBoxVirtual::GetControlClass(void) { return L"ScrollBoxVirtual"; }

			ScrollBox::ScrollBox(void)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				SafePointer<ScrollBoxVirtual> virt = new ScrollBoxVirtual;
				AddChild(virt);
				_virtual = virt;
				ResetCache();
			}
			ScrollBox::ScrollBox(Template::ControlTemplate * Template, IControlFactory * Factory)
			{
				if (Template->Properties->GetTemplateClass() != L"ScrollBox") throw InvalidArgumentException();
				static_cast<Template::Controls::ScrollBox &>(*this) = static_cast<Template::Controls::ScrollBox &>(*Template->Properties);
				SafePointer<ScrollBoxVirtual> virt = new ScrollBoxVirtual;
				AddChild(virt);
				_virtual = virt;
				ResetCache();
				CreateChildrenControls(_virtual, Template, Factory);
			}
			ScrollBox::~ScrollBox(void) {}
			void ScrollBox::Render(IRenderingDevice * device, const Box & at)
			{
				if (!_background && Background) {
					auto args = ZeroArgumentProvider();
					_background.SetReference(Background->Initialize(&args));
				}
				if (_background) _background->Render(device, at);
				auto box = _virtual->GetPosition();
				auto width = ControlBoundaries.Right - ControlBoundaries.Left;
				auto height = ControlBoundaries.Bottom - ControlBoundaries.Top;
				auto vbox = Box(Border + at.Left, Border + at.Top,
					width - Border - (_show_vs ? VerticalScrollSize : 0) + at.Left,
					height - Border - (_show_hs ? HorizontalScrollSize : 0) + at.Top);
				auto render_box = Box(box.Left + at.Left, box.Top + at.Top, box.Right + at.Left, box.Bottom + at.Top);
				device->PushClip(vbox);
				_virtual->Render(device, render_box);
				device->PopClip();
				if (_show_vs) {
					auto vs_box = _vertical->GetPosition();
					auto vsr_box = Box(vs_box.Left + at.Left, vs_box.Top + at.Top, vs_box.Right + at.Left, vs_box.Bottom + at.Top);
					_vertical->Render(device, vsr_box);
				}
				if (_show_hs) {
					auto hs_box = _horizontal->GetPosition();
					auto hsr_box = Box(hs_box.Left + at.Left, hs_box.Top + at.Top, hs_box.Right + at.Left, hs_box.Bottom + at.Top);
					_horizontal->Render(device, hsr_box);
				}
			}
			void ScrollBox::ResetCache(void)
			{
				_background.SetReference(0);
				for (int i = 0; i < ChildrenCount(); i++) Child(i)->ResetCache();
				if (_vertical) {
					_vertical->RemoveFromParent();
					_vertical = 0;
				}
				if (_horizontal) {
					_horizontal->RemoveFromParent();
					_horizontal = 0;
				}
				SafePointer<VerticalScrollBar> vert = new VerticalScrollBar(static_cast<Template::Controls::ScrollBox *>(this));
				SafePointer<HorizontalScrollBar> horz = new HorizontalScrollBar(static_cast<Template::Controls::ScrollBox *>(this));
				AddChild(vert); AddChild(horz);
				_vertical = vert;
				_horizontal = horz;
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
							int w = position.Right.Absolute + int(position.Right.Zoom * CurrentScaleFactor);
							if (w > mw) mw = w;
						}
						if (position.Bottom.Anchor == 0.0) {
							int h = position.Bottom.Absolute + int(position.Bottom.Zoom * CurrentScaleFactor);
							if (h > mh) mh = h;
						}
					} else {
						auto box = _virtual->Child(i)->GetPosition();
						if (box.Right > mw) mw = box.Right;
						if (box.Bottom > mh) mh = box.Bottom;
					}
				}
				int available_width = ControlBoundaries.Right - ControlBoundaries.Left - (Border << 1);
				int available_height = ControlBoundaries.Bottom - ControlBoundaries.Top - (Border << 1);
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
					if (rect.IsValid()) _virtual->Child(i)->SetPosition(Box(rect, align_box));
				}
				Invalidate();
			}
			void ScrollBox::Enable(bool enable) { Disabled = !enable; _vertical->Enable(enable); _horizontal->Enable(enable); Invalidate(); }
			bool ScrollBox::IsEnabled(void) { return !Disabled; }
			void ScrollBox::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool ScrollBox::IsVisible(void) { return !Invisible; }
			void ScrollBox::SetID(int _ID) { ID = _ID; }
			int ScrollBox::GetID(void) { return ID; }
			void ScrollBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle ScrollBox::GetRectangle(void) { return ControlPosition; }
			void ScrollBox::SetPosition(const Box & box) { ControlBoundaries = box; ArrangeChildren(); }
			void ScrollBox::RaiseEvent(int ID, ControlEvent event, Control * sender)
			{
				if (sender == _vertical || sender == _horizontal) {
					auto pos = _virtual->GetPosition();
					auto vw = pos.Right - pos.Left;
					auto vh = pos.Bottom - pos.Top;
					auto dx = -_horizontal->Position;
					auto dy = -_vertical->Position;
					_virtual->SetPosition(Box(Border + dx, Border + dy, Border + dx + vw, Border + dy + vh));
					Invalidate();
				} else if (GetParent()) GetParent()->RaiseEvent(ID, event, sender);
			}
			void ScrollBox::ScrollVertically(double delta) { _vertical->SetScrollerPosition(_vertical->Position + int(delta * double(_vertical->Line))); }
			void ScrollBox::ScrollHorizontally(double delta) { _horizontal->SetScrollerPosition(_horizontal->Position + int(delta * double(_horizontal->Line))); }
			Control * ScrollBox::HitTest(Point at)
			{
				if (Disabled) return this;
				auto vs_box = _vertical->GetPosition();
				if (_show_vs && vs_box.IsInside(at)) {
					return _vertical->HitTest(Point(at.x - vs_box.Left, at.y - vs_box.Top));
				}
				auto hs_box = _horizontal->GetPosition();
				if (_show_hs && hs_box.IsInside(at)) {
					return _horizontal->HitTest(Point(at.x - hs_box.Left, at.y - hs_box.Top));
				}
				auto box = _virtual->GetPosition();
				auto width = ControlBoundaries.Right - ControlBoundaries.Left;
				auto height = ControlBoundaries.Bottom - ControlBoundaries.Top;
				auto vbox = Box(Border, Border, width - Border - (_show_vs ? VerticalScrollSize : 0), height - Border - (_show_hs ? HorizontalScrollSize : 0));
				if (vbox.IsInside(at)) {
					return _virtual->HitTest(Point(at.x - box.Left, at.y - box.Top));
				}
				return this;
			}
			string ScrollBox::GetControlClass(void) { return L"ScrollBox"; }
			Control * ScrollBox::GetVirtualGroup(void) { return _virtual; }

			SplitBoxPart::SplitBoxPart(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); Size = 0; }
			SplitBoxPart::SplitBoxPart(Template::ControlTemplate * Template, IControlFactory * Factory)
			{
				if (Template->Properties->GetTemplateClass() != L"SplitBoxPart") throw InvalidArgumentException();
				static_cast<Template::Controls::SplitBoxPart &>(*this) = static_cast<Template::Controls::SplitBoxPart &>(*Template->Properties);
				Size = 0;
				CreateChildrenControls(this, Template, Factory);
			}
			SplitBoxPart::~SplitBoxPart(void) {}
			void SplitBoxPart::Render(IRenderingDevice * device, const Box & at)
			{
				device->PushClip(at);
				ParentControl::Render(device, at);
				device->PopClip();
			}
			void SplitBoxPart::Show(bool visible) { Invisible = !visible; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			bool SplitBoxPart::IsVisible(void){ return !Invisible; }
			void SplitBoxPart::SetID(int _ID) { ID = _ID; }
			int SplitBoxPart::GetID(void) { return ID; }
			void SplitBoxPart::SetRectangle(const Rectangle & rect) { ControlPosition = rect; }
			Rectangle SplitBoxPart::GetRectangle(void) { return ControlPosition; }
			string SplitBoxPart::GetControlClass(void) { return L"SplitBoxPart"; }

			VerticalSplitBox::VerticalSplitBox(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); ResetCache(); }
			VerticalSplitBox::VerticalSplitBox(Template::ControlTemplate * Template, IControlFactory * Factory)
			{
				if (Template->Properties->GetTemplateClass() != L"VerticalSplitBox") throw InvalidArgumentException();
				static_cast<Template::Controls::VerticalSplitBox &>(*this) = static_cast<Template::Controls::VerticalSplitBox &>(*Template->Properties);
				ResetCache();
				for (int i = 0; i < Template->Children.Length(); i++) {
					SafePointer<SplitBoxPart> part = new SplitBoxPart(&Template->Children[i], Factory);
					AddChild(part);
				}
			}
			VerticalSplitBox::~VerticalSplitBox(void) {}
			void VerticalSplitBox::Render(IRenderingDevice * device, const Box & at)
			{
				int index = 0;
				for (int i = 0; i < ChildrenCount() - 1; i++) {
					if (!Child(i)->IsVisible()) continue;
					auto box = Child(i)->GetPosition();
					auto splitter = Box(at.Left, at.Top + box.Bottom, at.Right, at.Top + box.Bottom + SplitterSize);
					if (_part == index && _state == 2) {
						if (_splitter_pressed) _splitter_pressed->Render(device, splitter);
					} else if (_part == i && _state == 1) {
						if (_splitter_hot) _splitter_hot->Render(device, splitter);
					} else {
						if (_splitter_normal) _splitter_normal->Render(device, splitter);
					}
					index++;
				}
				ParentControl::Render(device, at);
			}
			void VerticalSplitBox::ResetCache(void)
			{
				auto args = ZeroArgumentProvider();
				_splitter_normal.SetReference(ViewSplitterNormal ? ViewSplitterNormal->Initialize(&args) : 0);
				_splitter_hot.SetReference(ViewSplitterHot ? ViewSplitterHot->Initialize(&args) : 0);
				_splitter_pressed.SetReference(ViewSplitterPressed ? ViewSplitterPressed->Initialize(&args) : 0);
				ParentControl::ResetCache();
			}
			void VerticalSplitBox::Enable(bool enable) { Disabled = !enable; if (Disabled) { _state = 0; _part = -1; } Invalidate(); }
			bool VerticalSplitBox::IsEnabled(void) { return !Disabled; }
			void VerticalSplitBox::Show(bool visible) { Invisible = !visible; if (Invisible) { _state = 0; _part = -1; } Invalidate(); }
			bool VerticalSplitBox::IsVisible(void) { return !Invisible; }
			void VerticalSplitBox::SetID(int _ID) { ID = _ID; }
			int VerticalSplitBox::GetID(void) { return ID; }
			void VerticalSplitBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle VerticalSplitBox::GetRectangle(void) { return ControlPosition; }
			void VerticalSplitBox::ArrangeChildren(void)
			{
				auto & box = ControlBoundaries;
				int visible_count = 0;
				for (int i = 0; i < ChildrenCount(); i++) if (Child(i)->IsVisible()) visible_count++;
				int size = box.Bottom - box.Top;
				int available_size = size - (visible_count - 1) * SplitterSize;
				int stretch_part = -1;
				if (!_initialized) {
					for (int i = 0; i < ChildrenCount(); i++) {
						auto child = static_cast<SplitBoxPart *>(Child(i));
						auto rect = child->GetRectangle();
						child->SetRectangle(Rectangle::Invalid());
						int part_size = rect.Bottom.Absolute + int(rect.Bottom.Zoom * CurrentScaleFactor) + int(available_size * rect.Bottom.Anchor);
						child->Size = part_size;
					}
					_initialized = true;
				}
				int used_size = 0;
				for (int i = 0; i < ChildrenCount(); i++) {
					auto child = static_cast<SplitBoxPart *>(Child(i));
					if (!child->IsVisible()) continue;
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
							if (!child->IsVisible()) continue;
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
					if (!Child(i)->IsVisible()) continue;
					int part_size = static_cast<SplitBoxPart *>(Child(i))->Size;
					Child(i)->SetPosition(Box(0, pos, width, pos + part_size));
					pos += part_size + SplitterSize;
				}
				Invalidate();
			}
			void VerticalSplitBox::SetPosition(const Box & box) { ControlBoundaries = box; ArrangeChildren(); Invalidate(); }
			void VerticalSplitBox::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = -1; Invalidate(); } }
			void VerticalSplitBox::LeftButtonDown(Point at)
			{
				if (_state == 1 && _part != -1) {
					_state = 2;
					_mouse = at.y;
					Invalidate();
				}
			}
			void VerticalSplitBox::LeftButtonUp(Point at) { ReleaseCapture(); }
			void VerticalSplitBox::MouseMove(Point at)
			{
				if (_state != 2) {
					int part = _part, state = _state;
					int pos = 0;
					_part = -1;
					if (IsHovered()) {
						int index = 0;
						for (int i = 0; i < ChildrenCount() - 1; i++) {
							auto child = static_cast<SplitBoxPart *>(Child(i));
							if (!child->IsVisible()) continue;
							if (at.y >= pos + child->Size && at.y < pos + child->Size + SplitterSize) { _part = index; break; }
							pos += child->Size + SplitterSize;
							index++;
						}
					}
					if (_part != -1) {
						_state = 1;
						SetCapture();
					} else if (_state == 1) ReleaseCapture();
					if (part != _part || state != _state) Invalidate();
				} else {
					int mouse = at.y;
					int ds = at.y - _mouse;
					SplitBoxPart * left = 0;
					SplitBoxPart * right = 0;
					int index = 0;
					for (int i = 0; i < ChildrenCount(); i++) {
						if (!Child(i)->IsVisible()) continue;
						if (index == _part) left = static_cast<SplitBoxPart *>(Child(i));
						else if (index == _part + 1) { right = static_cast<SplitBoxPart *>(Child(i)); break; }
						index++;
					}
					if (!left || !right) return;
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
						Invalidate();
					}
				}
			}
			void VerticalSplitBox::SetCursor(Point at) { SelectCursor(Windows::SystemCursorClass::SizeUpDown); }
			string VerticalSplitBox::GetControlClass(void) { return L"VerticalSplitBox"; }

			HorizontalSplitBox::HorizontalSplitBox(void) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); ResetCache(); }
			HorizontalSplitBox::HorizontalSplitBox(Template::ControlTemplate * Template, IControlFactory * Factory)
			{
				if (Template->Properties->GetTemplateClass() != L"HorizontalSplitBox") throw InvalidArgumentException();
				static_cast<Template::Controls::HorizontalSplitBox &>(*this) = static_cast<Template::Controls::HorizontalSplitBox &>(*Template->Properties);
				ResetCache();
				for (int i = 0; i < Template->Children.Length(); i++) {
					SafePointer<SplitBoxPart> part = new SplitBoxPart(&Template->Children[i], Factory);
					AddChild(part);
				}
			}
			HorizontalSplitBox::~HorizontalSplitBox(void) {}
			void HorizontalSplitBox::Render(IRenderingDevice * device, const Box & at)
			{
				int index = 0;
				for (int i = 0; i < ChildrenCount() - 1; i++) {
					if (!Child(i)->IsVisible()) continue;
					auto box = Child(i)->GetPosition();
					auto splitter = Box(at.Left + box.Right, at.Top, at.Left + box.Right + SplitterSize, at.Bottom);
					if (_part == index && _state == 2) {
						if (_splitter_pressed) _splitter_pressed->Render(device, splitter);
					} else if (_part == i && _state == 1) {
						if (_splitter_hot) _splitter_hot->Render(device, splitter);
					} else {
						if (_splitter_normal) _splitter_normal->Render(device, splitter);
					}
					index++;
				}
				ParentControl::Render(device, at);
			}
			void HorizontalSplitBox::ResetCache(void)
			{
				auto args = ZeroArgumentProvider();
				_splitter_normal.SetReference(ViewSplitterNormal ? ViewSplitterNormal->Initialize(&args) : 0);
				_splitter_hot.SetReference(ViewSplitterHot ? ViewSplitterHot->Initialize(&args) : 0);
				_splitter_pressed.SetReference(ViewSplitterPressed ? ViewSplitterPressed->Initialize(&args) : 0);
				ParentControl::ResetCache();
			}
			void HorizontalSplitBox::Enable(bool enable) { Disabled = !enable; if (Disabled) { _state = 0; _part = -1; } Invalidate(); }
			bool HorizontalSplitBox::IsEnabled(void) { return !Disabled; }
			void HorizontalSplitBox::Show(bool visible) { Invisible = !visible; if (Invisible) { _state = 0; _part = -1; } Invalidate(); }
			bool HorizontalSplitBox::IsVisible(void) { return !Invisible; }
			void HorizontalSplitBox::SetID(int _ID) { ID = _ID; }
			int HorizontalSplitBox::GetID(void) { return ID; }
			void HorizontalSplitBox::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle HorizontalSplitBox::GetRectangle(void) { return ControlPosition; }
			void HorizontalSplitBox::ArrangeChildren(void)
			{
				auto & box = ControlBoundaries;
				int visible_count = 0;
				for (int i = 0; i < ChildrenCount(); i++) if (Child(i)->IsVisible()) visible_count++;
				int size = box.Right - box.Left;
				int available_size = size - (visible_count - 1) * SplitterSize;
				int stretch_part = -1;
				if (!_initialized) {
					for (int i = 0; i < ChildrenCount(); i++) {
						auto child = static_cast<SplitBoxPart *>(Child(i));
						auto rect = child->GetRectangle();
						child->SetRectangle(Rectangle::Invalid());
						int part_size = rect.Right.Absolute + int(rect.Right.Zoom * CurrentScaleFactor) + int(available_size * rect.Right.Anchor);
						child->Size = part_size;
					}
					_initialized = true;
				}
				int used_size = 0;
				for (int i = 0; i < ChildrenCount(); i++) {
					auto child = static_cast<SplitBoxPart *>(Child(i));
					if (!child->IsVisible()) continue;
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
							if (!child->IsVisible()) continue;
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
					if (!Child(i)->IsVisible()) continue;
					int part_size = static_cast<SplitBoxPart *>(Child(i))->Size;
					Child(i)->SetPosition(Box(pos, 0, pos + part_size, height));
					pos += part_size + SplitterSize;
				}
				Invalidate();
			}
			void HorizontalSplitBox::SetPosition(const Box & box) { ControlBoundaries = box; ArrangeChildren(); Invalidate(); }
			void HorizontalSplitBox::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; _part = -1; Invalidate(); } }
			void HorizontalSplitBox::LeftButtonDown(Point at)
			{
				if (_state == 1 && _part != -1) {
					_state = 2;
					_mouse = at.x;
					Invalidate();
				}
			}
			void HorizontalSplitBox::LeftButtonUp(Point at) { ReleaseCapture(); }
			void HorizontalSplitBox::MouseMove(Point at)
			{
				if (_state != 2) {
					int part = _part, state = _state;
					int pos = 0;
					_part = -1;
					if (IsHovered()) {
						int index = 0;
						for (int i = 0; i < ChildrenCount() - 1; i++) {
							auto child = static_cast<SplitBoxPart *>(Child(i));
							if (!child->IsVisible()) continue;
							if (at.x >= pos + child->Size && at.x < pos + child->Size + SplitterSize) { _part = index; break; }
							pos += child->Size + SplitterSize;
							index++;
						}
					}
					if (_part != -1) {
						_state = 1;
						SetCapture();
					} else if (_state == 1) ReleaseCapture();
					if (part != _part || state != _state) Invalidate();
				} else {
					int mouse = at.x;
					int ds = at.x - _mouse;
					SplitBoxPart * left = 0;
					SplitBoxPart * right = 0;
					int index = 0;
					for (int i = 0; i < ChildrenCount(); i++) {
						if (!Child(i)->IsVisible()) continue;
						if (index == _part) left = static_cast<SplitBoxPart *>(Child(i));
						else if (index == _part + 1) { right = static_cast<SplitBoxPart *>(Child(i)); break; }
						index++;
					}
					if (!left || !right) return;
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
						Invalidate();
					}
				}
			}
			void HorizontalSplitBox::SetCursor(Point at) { SelectCursor(Windows::SystemCursorClass::SizeLeftRight); }
			string HorizontalSplitBox::GetControlClass(void) { return L"HorizontalSplitBox"; }

			BookmarkView::BookmarkView(void) : _bookmarks(4) { ControlPosition = Rectangle::Invalid(); Reflection::PropertyZeroInitializer Initializer; EnumerateProperties(Initializer); }
			BookmarkView::BookmarkView(Template::ControlTemplate * Template, IControlFactory * Factory) : _bookmarks(4)
			{
				if (Template->Properties->GetTemplateClass() != L"BookmarkView") throw InvalidArgumentException();
				static_cast<Template::Controls::BookmarkView &>(*this) = static_cast<Template::Controls::BookmarkView &>(*Template->Properties);
				for (int i = 0; i < Template->Children.Length(); i++) {
					auto & bmt = Template->Children[i];
					if (bmt.Children.Length() > 1 || bmt.Properties->GetTemplateClass() != L"Bookmark") throw InvalidArgumentException();
					auto & props = static_cast<Template::Controls::Bookmark &>(*bmt.Properties);
					int width = props.ControlPosition.Right.Absolute + int(props.ControlPosition.Right.Zoom * CurrentScaleFactor);
					if (bmt.Children.Length()) {
						AddBookmark(width, props.ID, props.Text, bmt.Children.FirstElement(), Factory);
					} else {
						AddBookmark(width, props.ID, props.Text);
					}
				}
				if (_bookmarks.Length()) ActivateBookmark(0);
			}
			BookmarkView::~BookmarkView(void) {}
			void BookmarkView::Render(IRenderingDevice * device, const Box & at)
			{
				int tabs_width = 0;
				int active_center = -1;
				for (int i = 0; i < _bookmarks.Length(); i++) {
					if (i == _active) active_center = tabs_width + _bookmarks[i]._width / 2;
					tabs_width += _bookmarks[i]._width;
				}
				if (tabs_width > at.Right - at.Left) {
					_shift = active_center - (at.Right - at.Left) / 2;
					if (_shift < 0) _shift = 0;
					if (tabs_width - _shift < at.Right - at.Left) _shift = tabs_width - at.Right + at.Left;
				} else _shift = 0;
				int last_edge = at.Left - _shift;
				for (int i = 0; i < _bookmarks.Length(); i++) {
					int next_edge = last_edge + _bookmarks[i]._width;
					Box tab_box(last_edge, at.Top, next_edge, at.Top + TabHeight);
					Template::Shape * templ = 0;
					Shape ** shape = 0;
					if (i != _active) {
						if (i == _hot) {
							templ = ViewTabHot;
							shape = _bookmarks[i]._view_hot.InnerRef();
						} else {
							templ = ViewTabNormal;
							shape = _bookmarks[i]._view_normal.InnerRef();
						}
					} else { last_edge = next_edge; continue; }
					if (!(*shape) && templ) {
						ArgumentService::BookmarkArgumentProvider provider(Font, &_bookmarks[i].Text);
						*shape = templ->Initialize(&provider);
					}
					if (*shape) (*shape)->Render(device, tab_box);
					last_edge = next_edge;
				}
				if (!_view_background && ViewBackground) {
					ZeroArgumentProvider provider;
					_view_background.SetReference(ViewBackground->Initialize(&provider));
				}
				if (_view_background) {
					Box bk_box(at.Left, at.Top + TabHeight, at.Right, at.Bottom);
					_view_background->Render(device, bk_box);
				}
				device->PushClip(at);
				ParentControl::Render(device, at);
				device->PopClip();
				last_edge = at.Left - _shift;
				for (int i = 0; i < _bookmarks.Length(); i++) {
					int next_edge = last_edge + _bookmarks[i]._width;
					Box tab_box(last_edge, at.Top, next_edge, at.Top + TabHeight);
					Template::Shape * templ = 0;
					Shape ** shape = 0;
					if (i == _active) {
						if (GetFocus() == this) {
							templ = ViewTabActiveFocused;
							shape = _bookmarks[i]._view_active_focused.InnerRef();
						} else {
							templ = ViewTabActive;
							shape = _bookmarks[i]._view_active.InnerRef();
						}
					} else { last_edge = next_edge; continue; }
					if (!(*shape) && templ) {
						ArgumentService::BookmarkArgumentProvider provider(Font, &_bookmarks[i].Text);
						*shape = templ->Initialize(&provider);
					}
					if (*shape) (*shape)->Render(device, tab_box);
					last_edge = next_edge;
				}
			}
			void BookmarkView::ResetCache(void)
			{
				_view_background.SetReference(0);
				for (int i = 0; i < _bookmarks.Length(); i++) {
					_bookmarks[i]._view_normal.SetReference(0);
					_bookmarks[i]._view_hot.SetReference(0);
					_bookmarks[i]._view_active.SetReference(0);
					_bookmarks[i]._view_active_focused.SetReference(0);
				}
				ParentControl::ResetCache();
			}
			void BookmarkView::ArrangeChildren(void)
			{
				Box inner = Box(0, TabHeight, ControlBoundaries.Right - ControlBoundaries.Left, ControlBoundaries.Bottom - ControlBoundaries.Top);
				for (int i = 0; i < ChildrenCount(); i++) {
					auto rect = Child(i)->GetRectangle();
					if (rect.IsValid()) Child(i)->SetPosition(Box(rect, inner));
				}
				Invalidate();
			}
			void BookmarkView::Show(bool visible) { Invisible = !visible; Invalidate(); }
			bool BookmarkView::IsVisible(void) { return !Invisible; }
			bool BookmarkView::IsTabStop(void) { return true; }
			void BookmarkView::SetID(int _ID) { ID = _ID; }
			int BookmarkView::GetID(void) { return ID; }
			void BookmarkView::SetRectangle(const Rectangle & rect) { ControlPosition = rect; if (GetParent()) GetParent()->ArrangeChildren(); Invalidate(); }
			Rectangle BookmarkView::GetRectangle(void) { return ControlPosition; }
			void BookmarkView::SetPosition(const Box & box) { _hot = -1; ParentControl::SetPosition(box); Invalidate(); }
			void BookmarkView::FocusChanged(bool got_focus) { Invalidate(); }
			void BookmarkView::CaptureChanged(bool got_capture) { if (!got_capture) { _hot = -1; Invalidate(); }}
			void BookmarkView::LeftButtonDown(Point at) { if (_hot >= 0) { ActivateBookmark(_hot); ReleaseCapture(); } }
			void BookmarkView::MouseMove(Point at)
			{
				int w = ControlBoundaries.Right - ControlBoundaries.Left;
				int h = ControlBoundaries.Bottom - ControlBoundaries.Top;
				if (at.x >= 0 && at.x < w && at.y >= 0 && at.y < h && at.y < TabHeight && IsHovered()) {
					int x = at.x + _shift;
					int oh = _hot; _hot = -1;
					int right = 0;
					for (int i = 0; i < _bookmarks.Length(); i++) {
						right += _bookmarks[i]._width;
						if (x < right) { _hot = i; break; }
					}
					if (oh == -1 && _hot != -1) SetCapture();
					else if (oh != -1 && _hot == -1) ReleaseCapture();
					if (oh != _hot) Invalidate();
				} else {
					if (_hot != -1) ReleaseCapture();
				}
			}
			bool BookmarkView::KeyDown(int key_code)
			{
				if (key_code == KeyCodes::Left) {
					if (_active > 0) ActivateBookmark(_active - 1);
					else if (_active == -1 && _bookmarks.Length()) ActivateBookmark(0);
					return true;
				} else if (key_code == KeyCodes::Right) {
					if (_active >= 0 && _active < _bookmarks.Length() - 1) ActivateBookmark(_active + 1);
					else if (_active == -1 && _bookmarks.Length()) ActivateBookmark(0);
					return true;
				}
				return false;
			}
			string BookmarkView::GetControlClass(void) { return L"BookmarkView"; }
			int BookmarkView::FindBookmark(int _ID) { for (int i = 0; i < _bookmarks.Length(); i++) if (_bookmarks[i].ID == _ID) return i; return -1; }
			int BookmarkView::GetBookmarkCount(void) { return _bookmarks.Length(); }
			int BookmarkView::GetBookmarkID(int index) { return _bookmarks[index].ID; }
			string BookmarkView::GetBookmarkText(int index) { return _bookmarks[index].Text; }
			int BookmarkView::GetBookmarkWidth(int index) { return _bookmarks[index]._width; }
			Control * BookmarkView::GetBookmarkView(int index) { return _bookmarks[index].Group; }
			void BookmarkView::SetBookmarkID(int index, int _ID) { _bookmarks[index].ID = _ID; }
			void BookmarkView::SetBookmarkText(int index, const string & text)
			{
				_bookmarks[index].Text = text;
				_bookmarks[index]._view_normal.SetReference(0);
				_bookmarks[index]._view_hot.SetReference(0);
				_bookmarks[index]._view_active.SetReference(0);
				_bookmarks[index]._view_active_focused.SetReference(0);
				Invalidate();
			}
			void BookmarkView::SetBookmarkWidth(int index, int width) { _bookmarks[index]._width = width; _hot = -1; Invalidate(); }
			void BookmarkView::OrderBookmark(int index, int new_index)
			{
				if (_active == index) _active = new_index;
				_hot = -1;
				if (new_index < index) {
					for (int i = index - 1; i >= new_index; i--) _bookmarks.SwapAt(i, i + 1);
				} else if (new_index > index) {
					for (int i = index; i < new_index; i++) _bookmarks.SwapAt(i, i + 1);
				}
				Invalidate();
			}
			void BookmarkView::RemoveBookmark(int index)
			{
				if (_active == index) _active = -1;
				_hot = -1;
				_bookmarks[index].Group->RemoveFromParent();
				_bookmarks.Remove(index);
				Invalidate();
			}
			void BookmarkView::AddBookmark(int width, int _ID, const string & text)
			{
				SafePointer<Control> group = new ControlGroup;
				AddChild(group);
				_bookmarks << Bookmark();
				_bookmarks.LastElement()._width = width;
				_bookmarks.LastElement().ID = _ID;
				_bookmarks.LastElement().Text = text;
				_bookmarks.LastElement().Group = group;
				_bookmarks.LastElement().Group->Show(false);
				_hot = -1;
				Invalidate();
			}
			void BookmarkView::AddBookmark(int width, int _ID, const string & text, Template::ControlTemplate * view_template, IControlFactory * factory)
			{
				SafePointer<Control> group;
				if (factory) group = factory->CreateControl(view_template, factory);
				if (!group) group = CreateStandardControl(view_template, factory);
				if (!group) throw InvalidArgumentException();
				AddChild(group);
				_bookmarks << Bookmark();
				_bookmarks.LastElement()._width = width;
				_bookmarks.LastElement().ID = _ID;
				_bookmarks.LastElement().Text = text;
				_bookmarks.LastElement().Group = group;
				_bookmarks.LastElement().Group->Show(false);
				_hot = -1;
				Invalidate();
			}
			int BookmarkView::GetActiveBookmark(void) { return _active; }
			void BookmarkView::ActivateBookmark(int index)
			{
				if (index < 0 || index >= _bookmarks.Length()) return;
				_active = index;
				_hot = -1;
				for (int i = 0; i < _bookmarks.Length(); i++) _bookmarks[i].Group->Show(i == index);
				Invalidate();
			}
		}
	}
}