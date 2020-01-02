#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "GroupControls.h"
#include "ScrollableControls.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class ListBox : public ParentWindow, public Template::Controls::ListBox
			{
			private:
				struct Element
				{
					SafePointer<Shape> ViewNormal;
					SafePointer<Shape> ViewDisabled;
					void * User;
					bool Selected;
				};
				VerticalScrollBar * _scroll = 0;
				ControlGroup * _editor = 0;
				bool _svisible = false;
				Array<Element> _elements;
				int _current = -1;
				int _hot = -1;
				SafePointer<Shape> _view_normal;
				SafePointer<Shape> _view_disabled;
				SafePointer<Shape> _view_focused;
				SafePointer<Shape> _view_element_hot;
				SafePointer<Shape> _view_element_selected;

				void reset_scroll_ranges(void);
				void scroll_to_current(void);
				void select(int index, bool down);
				void move_selection(int to);
			public:
				ListBox(Window * Parent, WindowStation * Station);
				ListBox(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ListBox(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void ScrollVertically(double delta) override;
				virtual bool KeyDown(int key_code) override;
				virtual Window * HitTest(Point at) override;

				void AddItem(const string & text, void * user = 0);
				void AddItem(IArgumentProvider * provider, void * user = 0);
				void AddItem(Reflection::Reflected & object, void * user = 0);
				void InsertItem(const string & text, int at, void * user = 0);
				void InsertItem(IArgumentProvider * provider, int at, void * user = 0);
				void InsertItem(Reflection::Reflected & object, int at, void * user = 0);
				void ResetItem(int index, const string & text);
				void ResetItem(int index, IArgumentProvider * provider);
				void ResetItem(int index, Reflection::Reflected & object);
				void SwapItems(int i, int j);
				void RemoveItem(int index);
				void ClearItems(void);
				int ItemCount(void);
				void * GetItemUserData(int index);
				void SetItemUserData(int index, void * user);
				int GetSelectedIndex(void);
				void SetSelectedIndex(int index, bool scroll_to_view = false);
				bool IsItemSelected(int index);
				void SelectItem(int index, bool select);
				Window * CreateEmbeddedEditor(Template::ControlTemplate * Template, const Rectangle & Position = Rectangle::Entire());
				Window * GetEmbeddedEditor(void);
				void CloseEmbeddedEditor(void);
			};
			class TreeView : public ParentWindow, public Template::Controls::TreeView
			{
				friend class TreeViewItem;
			public:
				class TreeViewItem
				{
					friend class TreeView;
				private:
					SafeArray<TreeViewItem> Children;
					TreeViewItem * Parent;
					TreeView * View;
					bool Expanded;
					SafePointer<Shape> ViewNormal;
					SafePointer<Shape> ViewDisabled;

					int get_height(void) const;
					void reset_cache(void);
					int render(IRenderingDevice * device, int left_offset, int & y, const Box & outer, bool enabled);
					bool is_generalized_children(TreeViewItem * node) const;
					TreeViewItem * hit_test(const Point & p, const Box & outer, int left_offset, int & y, bool & node);
					TreeViewItem * get_topmost(void);
					TreeViewItem * get_bottommost(void);
					TreeViewItem * get_previous(void);
					TreeViewItem * get_next(void);

					TreeViewItem(TreeView * view, TreeViewItem * parent);
				public:
					TreeViewItem(void);
					~TreeViewItem(void);

					void * User;

					bool IsExpanded(void) const;
					bool IsParent(void) const;
					bool IsAccessible(void) const;
					void Expand(bool expand);
					Box GetBounds(void) const;
					TreeView * GetView(void) const;
					TreeViewItem * GetParent(void) const;
					int GetIndexAtParent(void) const;
					int GetChildrenCount(void) const;
					const TreeViewItem * GetChild(int index) const;
					TreeViewItem * GetChild(int index);
					TreeViewItem * AddItem(const string & text, void * user = 0);
					TreeViewItem * AddItem(const string & text, ITexture * image_normal, ITexture * image_grayed, void * user = 0);
					TreeViewItem * AddItem(IArgumentProvider * provider, void * user = 0);
					TreeViewItem * AddItem(Reflection::Reflected & object, void * user = 0);
					TreeViewItem * InsertItem(const string & text, int at, void * user = 0);
					TreeViewItem * InsertItem(const string & text, ITexture * image_normal, ITexture * image_grayed, int at, void * user = 0);
					TreeViewItem * InsertItem(IArgumentProvider * provider, int at, void * user = 0);
					TreeViewItem * InsertItem(Reflection::Reflected & object, int at, void * user = 0);
					void Reset(const string & text);
					void Reset(const string & text, ITexture * image_normal, ITexture * image_grayed);
					void Reset(IArgumentProvider * provider);
					void Reset(Reflection::Reflected & object);
					void SwapItems(int i, int j);
					void Remove(void);	
				};
			private:
				VerticalScrollBar * _scroll = 0;
				ControlGroup * _editor = 0;
				bool _svisible = false;
				TreeViewItem _root;
				TreeViewItem * _current = 0;
				TreeViewItem * _hot = 0;
				bool _is_arrow_hot = false;
				SafePointer<Shape> _view_normal;
				SafePointer<Shape> _view_disabled;
				SafePointer<Shape> _view_focused;
				SafePointer<Shape> _view_element_hot;
				SafePointer<Shape> _view_element_selected;
				SafePointer<Shape> _view_node_collapsed_normal;
				SafePointer<Shape> _view_node_collapsed_disabled;
				SafePointer<Shape> _view_node_collapsed_hot;
				SafePointer<Shape> _view_node_expanded_normal;
				SafePointer<Shape> _view_node_expanded_disabled;
				SafePointer<Shape> _view_node_expanded_hot;
				SafePointer<ILineRenderingInfo> _line_normal;
				SafePointer<ILineRenderingInfo> _line_disabled;

				void reset_scroll_ranges(void);
				void scroll_to_current(void);
				void select(TreeViewItem * item);
				void move_selection(TreeViewItem * to);
			public:
				TreeView(Window * Parent, WindowStation * Station);
				TreeView(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~TreeView(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void ScrollVertically(double delta) override;
				virtual bool KeyDown(int key_code) override;
				virtual Window * HitTest(Point at) override;

				TreeViewItem * GetRootItem(void);
				void ClearItems(void);
				TreeViewItem * GetSelectedItem(void);
				void SetSelectedItem(TreeViewItem * item, bool scroll_to_view = false);
				Window * CreateEmbeddedEditor(Template::ControlTemplate * Template, const Rectangle & Position = Rectangle::Entire());
				Window * GetEmbeddedEditor(void);
				void CloseEmbeddedEditor(void);
			};
			class ListView : public ParentWindow, public Template::Controls::ListView
			{
				friend class Element;
				friend class ListViewColumn;
			private:
				struct Element
				{
					ObjectArray<Shape> ViewNormal;
					ObjectArray<Shape> ViewDisabled;
					void * User;
					bool Selected;

					Element(void);
					Element(ListView * view);
				};
				class ListViewColumn : public Template::Controls::ListViewColumn
				{
				public:
					int _width = 0;
					int _index = 0;
					int _abs_index = 0;
					int _position = 0;
					int _position_limit = 0;
					SafePointer<Shape> _view_normal;
					SafePointer<Shape> _view_hot;
					SafePointer<Shape> _view_pressed;
					SafePointer<Shape> _view_disabled;
					Coordinate _rel_width = Coordinate(0, 0.0, 0.0);

					ListViewColumn(int index);
					ListViewColumn(void);
				};
				VerticalScrollBar * _vscroll = 0;
				HorizontalScrollBar * _hscroll = 0;
				ControlGroup * _editor = 0;
				bool _vsvisible = false, _hsvisible = false;
				SafeArray<ListViewColumn> _columns;
				Array<ListViewColumn *> _col_reorder;
				Array<Element> _elements;
				int _current = -1;
				int _hot = -1;
				int _editor_cell = -1;
				SafePointer<Shape> _view_normal;
				SafePointer<Shape> _view_disabled;
				SafePointer<Shape> _view_focused;
				SafePointer<Shape> _view_element_hot;
				SafePointer<Shape> _view_element_selected;
				SafePointer<Shape> _view_header_normal;
				SafePointer<Shape> _view_header_disabled;
				int _last_cell_id = 0;
				int _state = 0, _cell = 0;
				int _mouse = 0, _mouse_start = 0;
				bool _stretch = false;

				void reset_scroll_ranges(void);
				void scroll_to_current(void);
				void select(int index);
				void move_selection(int to);
			public:
				ListView(Window * Parent, WindowStation * Station);
				ListView(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ListView(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void ArrangeChildren(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsTabStop(void) override;
				virtual void SetID(int ID) override;
				virtual int GetID(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void ScrollVertically(double delta) override;
				virtual void ScrollHorizontally(double delta) override;
				virtual bool KeyDown(int key_code) override;
				virtual void Timer(void) override;
				virtual Window * HitTest(Point at) override;
				virtual void SetCursor(Point at) override;

				void AddColumn(const string & title, int id, int width, int minimal_width, Template::Shape * cell_normal, Template::Shape * cell_disabled);
				void OrderColumn(int ID, int index);
				int GetColumnOrder(int ID);
				void SetColumnTitle(int ID, const string & title);
				string GetColumnTitle(int ID);
				void SetColumnWidth(int ID, int width);
				int GetColumnWidth(int ID);
				void SetColumnMinimalWidth(int ID, int width);
				int GetColumnMinimalWidth(int ID);
				void SetColumnNormalCell(int ID, Template::Shape * shape);
				Template::Shape * GetColumnNormalCell(int ID);
				void SetColumnDisabledCell(int ID, Template::Shape * shape);
				Template::Shape * GetColumnDisabledCell(int ID);
				void SetColumnID(int ID, int NewID);
				Array<int> GetColumns(void);
				void RemoveColumn(int ID);
				void ClearListView(void);

				void AddItem(IArgumentProvider * provider, void * user = 0);
				void AddItem(Reflection::Reflected & object, void * user = 0);
				void InsertItem(IArgumentProvider * provider, int at, void * user = 0);
				void InsertItem(Reflection::Reflected & object, int at, void * user = 0);
				void ResetItem(int index, IArgumentProvider * provider);
				void ResetItem(int index, Reflection::Reflected & object);
				void SwapItems(int i, int j);
				void RemoveItem(int index);
				void ClearItems(void);
				int ItemCount(void);
				void * GetItemUserData(int index);
				void SetItemUserData(int index, void * user);
				int GetSelectedIndex(void);
				void SetSelectedIndex(int index, bool scroll_to_view = false);
				bool IsItemSelected(int index);
				void SelectItem(int index, bool select);
				int GetLastCellID(void);
				Window * CreateEmbeddedEditor(Template::ControlTemplate * Template, int CellID, const Rectangle & Position = Rectangle::Entire());
				Window * GetEmbeddedEditor(void);
				void CloseEmbeddedEditor(void);
			};
		}
	}
}