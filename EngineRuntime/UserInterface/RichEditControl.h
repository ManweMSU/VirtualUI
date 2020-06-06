#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "Menus.h"
#include "ScrollableControls.h"

#include "../Miscellaneous/UndoBuffer.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class RichEdit : public ParentWindow, public Template::Controls::RichEdit
			{
			public:
				enum TextStyle { StyleNone = 0x0, StyleBold = 0x1, StyleItalic = 0x2, StyleUnderline = 0x4, StyleStrikeout = 0x8 };
				enum class TextAlignment { Left, Center, Right, Stretch };
				enum class TextVerticalAlignment { Top, Center, Bottom };
				class IRichEditHook
				{
				public:
					virtual void InitializeContextMenu(Menus::Menu * menu, RichEdit * sender);
					virtual void LinkPressed(const string & resource, RichEdit * sender);
					virtual void CaretPositionChanged(RichEdit * sender);
				};
				class ContentBox : public Object
				{
				public:
					virtual void Render(IRenderingDevice * device, const Box & at) = 0;
					virtual void RenderBackground(IRenderingDevice * device, const Box & at) = 0;
					virtual void ResetCache(void) = 0;
					virtual void AlignContents(IRenderingDevice * device, int box_width) = 0;
					virtual int GetContentsOriginX(void) = 0;
					virtual int GetContentsOriginY(void) = 0;
					virtual int GetContentsWidth(void) = 0;
					virtual int GetContentsCurrentWidth(void) = 0;
					virtual int GetContentsHeight(void) = 0;
					virtual void GetTextPositionFromCursor(int x, int y, int * pos, ContentBox ** box) = 0;
					virtual void SelectWord(int x, int y, int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) = 0;
					virtual void SetAbsoluteOrigin(int x, int y) = 0;
					virtual void ShiftCaretLeft(int * pos, ContentBox ** box, bool enter) = 0;
					virtual void ShiftCaretRight(int * pos, ContentBox ** box, bool enter) = 0;
					virtual void ShiftCaretHome(int * pos, ContentBox ** box, bool enter) = 0;
					virtual void ShiftCaretEnd(int * pos, ContentBox ** box, bool enter) = 0;
					virtual void ClearSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) = 0;
					virtual void RemoveBack(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) = 0;
					virtual void RemoveForward(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) = 0;
					virtual void ChildContentsChanged(ContentBox * child) = 0;
					virtual int GetMaximalPosition(void) = 0;
					virtual SystemCursor RegularCursorForPosition(int x, int y) = 0;
					virtual ContentBox * ActiveBoxForPosition(int x, int y) = 0;
					virtual void MouseMove(int x, int y) = 0;
					virtual void MouseLeft(void) = 0;
					virtual void MouseClick(int x, int y) = 0;
					virtual Box CaretBoxFromPosition(int pos, ContentBox * box) = 0;
					virtual ContentBox * GetParentBox(void) = 0;
					virtual bool HasInternalUnits(void) = 0;
					virtual int GetBoxType(void) = 0; // 1 - general container, 2 - image, 3 - hyperlink, 4 - table
					virtual void InsertText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) = 0;
					virtual void InsertUnattributedText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) = 0;
					virtual void DeserializeFromPlainText(Array<int> & index, const Array<uint32> & text, int & pos, uint64 attr_lo, uint64 attr_hi) = 0;
					virtual void SerializeToPlainText(const Array<int> & index, Array<uint32> & text, uint64 attr_lo, uint64 attr_hi) = 0;
					virtual void SerializeRange(const Array<int> & index, Array<uint32> & text, int from, int to, uint64 attr_lo, uint64 attr_hi) = 0;
					virtual void SerializeUnattributedToPlainText(Array<uint32> & text) = 0;
					virtual void SerializeUnattributedRange(Array<uint32> & text, int from, int to) = 0;
					virtual void EnumerateUsedFonts(Array<int> & index) = 0;
					virtual void EnumerateUsedFonts(Array<int> & index, int from, int to) = 0;
					virtual int GetChildPosition(ContentBox * child) = 0;
					virtual void GetOuterAttributes(ContentBox * child, uint64 & attr_lo, uint64 & attr_hi) = 0;
					virtual void SetAttributesForRange(int from, int to, uint64 attr_lo, uint64 attr_hi, uint64 attr_lo_mask, uint64 attr_hi_mask) = 0;
					virtual ContentBox * GetChildBox(int position) = 0;
					static void UnifyBoxForSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2);
					static ContentBox * GetParentBoxOfType(ContentBox * box_for, int type);
					static ContentBox * GetParentBoxOfType(ContentBox * box_for, int type, ContentBox ** first_child);
					static void MakeFontIndex(RichEdit * edit, Array<int> & index, ContentBox * root, int from, int to);
				};
				struct FontEntry
				{
					SafePointer<IFont> font;
					int face, height, attr;
				};
				struct UndoBufferCopy
				{
					Array<int> caret_box_path;
					int caret_pos, selection_pos;
					Array<uint32> text;

					UndoBufferCopy(void);
					bool friend operator == (const UndoBufferCopy & a, const UndoBufferCopy & b);
					bool friend operator != (const UndoBufferCopy & a, const UndoBufferCopy & b);
				};
			private:
				class UndoBufferInterface : public IUndoProperty<UndoBufferCopy>
				{
					RichEdit * _edit;
				public:
					UndoBufferInterface(RichEdit * edit);
					virtual ~UndoBufferInterface(void) override;
					virtual UndoBufferCopy GetCurrentValue(void) override;
					virtual void SetCurrentValue(const UndoBufferCopy & value) override;
				};
				friend class UndoBufferInterface;
				friend class ContentBox;

				VerticalScrollBar * _vscroll = 0;
				SafePointer<Menus::Menu> _menu;
				SafePointer<IBarRenderingInfo> _background;
				SafePointer<IBarRenderingInfo> _selection;
				SafePointer<Object> _inversion;
				SafePointer<ContentBox> _root;
				ContentBox * _caret_box = 0;
				ContentBox * _selection_box = 0;
				int _caret_pos = 0;
				int _selection_pos = 0;
				ContentBox * _hot_box = 0;
				int _state = 0;
				UndoBufferInterface _undo_interface;
				LimitedUndoBuffer<UndoBufferCopy> _undo;
				bool _save = true;
				bool _init = true;
				bool _deferred_scroll = false;
				bool _use_color_caret = false;
				int _caret_width = -1;
				Array<string> _font_faces;
				Array<FontEntry> _fonts;
				IRichEditHook * _hook = 0;

				void _align_contents(const Box & box);
				void _scroll_to_caret(void);
				bool _selection_empty(void);
				void _make_font_index(Array<int> & index, ContentBox * root, int from, int to);
			public:
				RichEdit(Window * Parent, WindowStation * Station);
				RichEdit(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~RichEdit(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
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
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void FocusChanged(bool got_focus) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void RightButtonDown(Point at) override;
				virtual void RightButtonUp(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void ScrollVertically(double delta) override;
				virtual bool KeyDown(int key_code) override;
				virtual void CharDown(uint32 ucs_code) override;
				virtual void SetCursor(Point at) override;
				virtual RefreshPeriod FocusedRefreshPeriod(void) override;
				virtual string GetControlClass(void) override;

				void Undo(void);
				void Redo(void);
				void Cut(void);
				void Copy(void);
				void Paste(void);
				void Delete(void);
				bool IsReadOnly(void);
				void SetReadOnly(bool read_only);
				void SetAttributedText(const string & text);
				string GetAttributedText(void);
				string SerializeSelection(void);
				string SerializeSelectionAttributed(void);
				void ScrollToCaret(void);
				void SetContextMenu(Menus::Menu * menu);
				Menus::Menu * GetContextMenu(void);
				void SetHook(IRichEditHook * hook);
				IRichEditHook * GetHook(void);
				void ClearFontCollection(void);
				int RegisterFontFace(const string & face);
				int GetFontFaceCount(void);
				const string & GetFontFaceWithIndex(int index);
				int RegisterFont(int face, int height, int attributes);
				void RecreateCachedFont(int index);
				IFont * GetCachedFont(int index);
				void RealignContents(void);

				bool IsSelectionEmpty(void);
				void Print(const string & text);
				void PrintAttributed(const string & text);
				void SetSelectedTextStyle(int attributes_new, int attributes_mask);
				int GetSelectedTextStyle(void);
				void SetSelectedTextAlignment(TextAlignment alignment);
				TextAlignment GetSelectedTextAlignment(void);
				void SetSelectedTextFontFamily(const string & family);
				string GetSelectedTextFontFamily(void);
				void SetSelectedTextHeight(int height);
				int GetSelectedTextHeight(void);
				void SetSelectedTextColor(Color color);
				Color GetSelectedTextColor(void);

				void InsertImage(Codec::Image * image);
				bool IsImageSelected(void);
				void SetSelectedImage(Codec::Image * image);
				Codec::Image * GetSelectedImage(void);
				void SetSelectedImageSize(int width, int height);
				int GetSelectedImageWidth(void);
				int GetSelectedImageHeight(void);

				bool CanCreateLink(void);
				void TransformSelectionIntoLink(const string & resource);
				bool IsLinkSelected(void);
				void DetransformSelectedLink(void);
				void SetSelectedLinkResource(const string & resource);
				string GetSelectedLinkResource(void);

				bool CanInsertTable(void);
				void InsertTable(Point size);
				bool IsTableSelected(void);
				Point GetSelectedTableCell(void);
				Point GetSelectedTableSize(void);
				void InsertSelectedTableRow(int at);
				void InsertSelectedTableColumn(int at);
				void RemoveSelectedTableRow(int index);
				void RemoveSelectedTableColumn(int index);
				void SetSelectedTableColumnWidth(int width);
				void AdjustSelectedTableColumnWidth(void);
				int GetSelectedTableColumnWidth(int index);
				void SetSelectedTableBorderColor(Color color);
				Color GetSelectedTableBorderColor(void);
				void SetSelectedTableCellBackground(Color color);
				Color GetSelectedTableCellBackground(Point cell);
				void SetSelectedTableCellVerticalAlignment(TextVerticalAlignment align);
				TextVerticalAlignment GetSelectedTableCellVerticalAlignment(Point cell);
				void SetSelectedTableBorderWidth(int width);
				int GetSelectedTableBorderWidth(void);
				void SetSelectedTableVerticalBorderWidth(int width);
				int GetSelectedTableVerticalBorderWidth(void);
				void SetSelectedTableHorizontalBorderWidth(int width);
				int GetSelectedTableHorizontalBorderWidth(void);
			};
		}
	}
}