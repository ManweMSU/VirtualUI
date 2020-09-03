#include "RichEditControl.h"

#include "../PlatformDependent/KeyCodes.h"
#include "../PlatformDependent/Clipboard.h"
#include "../Storage/JSON.h"
#include "../Miscellaneous/DynamicString.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			namespace RichEditContents
			{
				constexpr uint64 MaskUcsCode			= 0x00000000FFFFFFFF;
				constexpr uint64 MaskStyleAttributes	= 0x0000000F00000000;
				constexpr uint64 StyleBold				= 0x0000000100000000;
				constexpr uint64 StyleItalic			= 0x0000000200000000;
				constexpr uint64 StyleUnderline			= 0x0000000400000000;
				constexpr uint64 StyleStrikeout			= 0x0000000800000000;
				constexpr uint64 FlagLargeObject		= 0x0000001000000000;
				constexpr uint64 MaskAlignment			= 0x0000006000000000;
				constexpr uint64 AlignmentLeft			= 0x0000000000000000;
				constexpr uint64 AlignmentCenter		= 0x0000002000000000;
				constexpr uint64 AlignmentRight			= 0x0000004000000000;
				constexpr uint64 AlignmentStretch		= 0x0000006000000000;
				constexpr uint64 MaskFontFamily			= 0x0000FF0000000000;
				constexpr uint64 MaskFontHeight			= 0xFFFF000000000000;
				constexpr uint64 MaskColor				= 0x00000000FFFFFFFF;
				constexpr uint64 MaskObjectCacheIndex	= 0xFFFFFFFF00000000;
				struct TaggedChar { uint64 lo, hi; };
				class TextContents;
				class LinkContents : public RichEdit::ContentBox
				{
					RichEdit * _edit;
					RichEdit::ContentBox * _parent;
					string resource;
					SafePointer<RichEdit::ContentBox> box;
					SafePointer<RichEdit::ContentBox> hot_box;
					int _width, _height;
					int _org_x, _org_y;
					int _state;
				public:
					LinkContents(RichEdit * edit, RichEdit::ContentBox * parent);
					~LinkContents(void) override {}
					virtual void Render(IRenderingDevice * device, const Box & at) override
					{
						if (_state) hot_box->Render(device, at);
						else box->Render(device, at);
					}
					virtual void RenderBackground(IRenderingDevice * device, const Box & at) override
					{
						if (_state) hot_box->RenderBackground(device, at);
						else box->RenderBackground(device, at);
					}
					virtual void ResetCache(void) override { box->ResetCache(); hot_box->ResetCache(); }
					virtual void AlignContents(IRenderingDevice * device, int box_width) override
					{
						box->AlignContents(device, 0x7FFFFFFF);
						_width = box->GetContentsWidth();
						_height = box->GetContentsHeight();
						box->SetAbsoluteOrigin(_org_x, _org_y);
						hot_box->SetAbsoluteOrigin(_org_x, _org_y);
						box->AlignContents(device, _width);
						hot_box->AlignContents(device, _width);
					}
					virtual int GetContentsOriginX(void) override { return _org_x; }
					virtual int GetContentsOriginY(void) override { return _org_y; }
					virtual int GetContentsWidth(void) override { return _width; }
					virtual int GetContentsCurrentWidth(void) override { return _width; }
					virtual int GetContentsHeight(void) override { return _height; }
					virtual void GetTextPositionFromCursor(int x, int y, int * pos, ContentBox ** pbox) override { box->GetTextPositionFromCursor(x, y, pos, pbox); }
					virtual void SelectWord(int x, int y, int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override { box->SelectWord(x, y, pos1, box1, pos2, box2); }
					virtual void SetAbsoluteOrigin(int x, int y) override { _org_x = x; _org_y = y; }
					virtual void ShiftCaretLeft(int * pos, ContentBox ** pbox, bool enter) override
					{
						if (*pos == 0) {
							*pos = _parent->GetChildPosition(*pbox);
							*pbox = _parent;
							_parent->ShiftCaretLeft(pos, pbox, enter);
						} else {
							*pbox = box;
							*pos = box->GetMaximalPosition();
							box->ShiftCaretLeft(pos, pbox, enter);
						}
					}
					virtual void ShiftCaretRight(int * pos, ContentBox ** pbox, bool enter) override
					{
						if (*pos == 0) {
							*pbox = box;
							*pos = 0;
							box->ShiftCaretRight(pos, pbox, enter);
						} else {
							*pos = _parent->GetChildPosition(*pbox) + 1;
							*pbox = _parent;
							_parent->ShiftCaretRight(pos, pbox, enter);
						}
					}
					virtual void ShiftCaretHome(int * pos, ContentBox ** pbox, bool enter) override { *pos = 0; }
					virtual void ShiftCaretEnd(int * pos, ContentBox ** pbox, bool enter) override { *pos = 1; }
					virtual void ClearSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						int mn = min(*pos1, *pos2);
						int mx = max(*pos1, *pos2);
						if (mn == 0 && mx == 1) {
							*box1 = *box2 = box;
							*pos1 = 0; *pos2 = box->GetMaximalPosition();
							box->ClearSelection(pos1, box1, pos2, box2);
						}
					}
					virtual void RemoveBack(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						if (*pos1 == 0) {
							*box1 = *box2 = _parent;
							*pos1 = *pos2 = _parent->GetChildPosition(this);
							_parent->RemoveBack(pos1, box1, pos2, box2);
						} else {
							*box1 = *box2 = box;
							*pos1 = *pos2 = box->GetMaximalPosition();
							box->RemoveBack(pos1, box1, pos2, box2);
						}
					}
					virtual void RemoveForward(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						if (*pos1 == 0) {
							*box1 = *box2 = box;
							*pos1 = *pos2 = 0;
							box->RemoveForward(pos1, box1, pos2, box2);
						} else {
							*box1 = *box2 = _parent;
							*pos1 = *pos2 = _parent->GetChildPosition(this) + 1;
							_parent->RemoveForward(pos1, box1, pos2, box2);
						}
					}
					virtual void ChildContentsChanged(ContentBox * child) override
					{
						Array<uint32> ucs(0x100);
						{
							Array<int> font_index(0x10);
							MakeFontIndex(_edit, font_index, box, -1, -1);
							box->SerializeToPlainText(font_index, ucs, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
						}
						string s = string(ucs.GetBuffer(), ucs.Length(), Encoding::UTF32);
						int pos = 0;
						Array<int> index(0x10);
						hot_box->DeserializeFromPlainText(index, ucs, pos, StyleUnderline, _edit->HyperlinkHotColor);
						_parent->ChildContentsChanged(this);
					}
					virtual int GetMaximalPosition(void) override { return 1; }
					virtual SystemCursor RegularCursorForPosition(int x, int y) override { return SystemCursor::Link; }
					virtual ContentBox * ActiveBoxForPosition(int x, int y) override { return this; }
					virtual void MouseMove(int x, int y) override { _state = 1; }
					virtual void MouseLeft(void) override { _state = 0; }
					virtual void MouseClick(int x, int y) override { if (_state && _edit->GetHook()) _edit->GetHook()->LinkPressed(resource, _edit); }
					virtual Box CaretBoxFromPosition(int pos, ContentBox * box) override
					{
						if (pos == 0) return Box(_org_x, _org_y, _org_x, _org_y + _height);
						else return Box(_org_x + _width, _org_y, _org_x + _width, _org_y + _height);
					}
					virtual ContentBox * GetParentBox(void) override { return _parent; }
					virtual bool HasInternalUnits(void) override { return true; }
					virtual int GetBoxType(void) override { return 3; }
					virtual void InsertText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override
					{
						if (at == 0) box->InsertText(text, 0, box_end, pos_end);
						else box->InsertText(text, box->GetMaximalPosition(), box_end, pos_end);
					}
					virtual void InsertUnattributedText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override
					{
						if (at == 0) box->InsertUnattributedText(text, 0, box_end, pos_end);
						else box->InsertUnattributedText(text, box->GetMaximalPosition(), box_end, pos_end);
					}
					virtual void DeserializeFromPlainText(Array<int> & index, const Array<uint32> & text, int & pos, uint64 attr_lo, uint64 attr_hi) override
					{
						int rsp = pos;
						while (pos < text.Length() && text[pos] != 27) pos++;
						resource = string(text.GetBuffer() + rsp, pos - rsp, Encoding::UTF32);
						pos++;
						attr_hi = _edit->HyperlinkColor;
						attr_lo |= StyleUnderline;
						if ((attr_lo & MaskAlignment) == AlignmentStretch) attr_lo &= ~MaskAlignment;
						int sp = pos;
						box->DeserializeFromPlainText(index, text, pos, attr_lo, attr_hi);
						attr_hi = _edit->HyperlinkHotColor;
						pos = sp;
						hot_box->DeserializeFromPlainText(index, text, pos, attr_lo, attr_hi);
					}
					virtual void SerializeToPlainText(const Array<int> & index, Array<uint32> & text, uint64 attr_lo, uint64 attr_hi) override
					{
						text << L'\33'; text << L'l';
						Array<uint32> ucs(0x10);
						ucs.SetLength(resource.GetEncodedLength(Encoding::UTF32));
						resource.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
						text << ucs; text << L'\33';
						attr_hi = _edit->HyperlinkColor;
						attr_lo |= StyleUnderline;
						if ((attr_lo & MaskAlignment) == AlignmentStretch) attr_lo &= ~MaskAlignment;
						box->SerializeToPlainText(index, text, attr_lo, attr_hi);
						text << L'\33'; text << L'e';
					}
					virtual void SerializeRange(const Array<int> & index, Array<uint32> & text, int from, int to, uint64 attr_lo, uint64 attr_hi) override { if (from <= 0 && to >= 1) box->SerializeToPlainText(index, text, attr_lo, attr_hi); }
					virtual void SerializeUnattributedToPlainText(Array<uint32> & text) override { box->SerializeUnattributedToPlainText(text); }
					virtual void SerializeUnattributedRange(Array<uint32> & text, int from, int to) override { if (from <= 0 && to >= 1) box->SerializeUnattributedToPlainText(text); }
					virtual void EnumerateUsedFonts(Array<int> & index) override { box->EnumerateUsedFonts(index); }
					virtual void EnumerateUsedFonts(Array<int> & index, int from, int to) override { if (from <= 0 && to >= 1) box->EnumerateUsedFonts(index); }
					virtual int GetChildPosition(ContentBox * child) override { return 0; }
					virtual void GetOuterAttributes(ContentBox * child, uint64 & attr_lo, uint64 & attr_hi) override
					{
						_parent->GetOuterAttributes(this, attr_lo, attr_hi);
						attr_lo |= StyleUnderline;
						attr_lo &= ~MaskAlignment;
						attr_hi = _edit->HyperlinkColor;
					}
					virtual void SetAttributesForRange(int from, int to, uint64 attr_lo, uint64 attr_hi, uint64 attr_lo_mask, uint64 attr_hi_mask) override
					{
						if (from == 0 && to == 1) {
							box->SetAttributesForRange(0, box->GetMaximalPosition(), attr_lo, attr_hi, attr_lo_mask, attr_hi_mask);
							hot_box->SetAttributesForRange(0, box->GetMaximalPosition(), attr_lo, attr_hi, attr_lo_mask, attr_hi_mask);
						}
					}
					virtual ContentBox * GetChildBox(int position) override { if (position) return 0; return box; }
					string & GetResource(void) { return resource; }
				};
				class PictureContents : public RichEdit::ContentBox
				{
					RichEdit * _edit;
					RichEdit::ContentBox * _parent;
					SafePointer<Codec::Image> _image;
					SafePointer<ITexture> _texture;
					SafePointer<ITextureRenderingInfo> _info;
					int _box_width, _box_height;
					int _width, _height;
					int _org_x, _org_y;
				public:
					PictureContents(RichEdit * edit, RichEdit::ContentBox * parent) : _edit(edit), _parent(parent),
						_box_width(0), _box_height(0), _width(0), _height(0), _org_x(0), _org_y(0) {}
					~PictureContents(void) override {}
					virtual void Render(IRenderingDevice * device, const Box & at) override
					{
						if (!_texture && _image) _texture = device->LoadTexture(_image->GetFrameBestDpiFit(Zoom));
						if (!_info && _texture) _info = device->CreateTextureRenderingInfo(_texture, Box(0, 0, _texture->GetWidth(), _texture->GetHeight()), false);
						if (_info) device->RenderTexture(_info, Box(at.Left, at.Top, at.Left + _box_width, at.Top + _box_height));
					}
					virtual void RenderBackground(IRenderingDevice * device, const Box & at) override {}
					virtual void ResetCache(void) override { _texture.SetReference(0); _info.SetReference(0); }
					virtual void AlignContents(IRenderingDevice * device, int box_width) override {}
					virtual int GetContentsOriginX(void) override { return _org_x; }
					virtual int GetContentsOriginY(void) override { return _org_y; }
					virtual int GetContentsWidth(void) override { return _box_width; }
					virtual int GetContentsCurrentWidth(void) override { return _box_width; }
					virtual int GetContentsHeight(void) override { return _box_height; }
					virtual void GetTextPositionFromCursor(int x, int y, int * pos, ContentBox ** box) override {}
					virtual void SelectWord(int x, int y, int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override {}
					virtual void SetAbsoluteOrigin(int x, int y) override { _org_x = x; _org_y = y; }
					virtual void ShiftCaretLeft(int * pos, ContentBox ** box, bool enter) override {}
					virtual void ShiftCaretRight(int * pos, ContentBox ** box, bool enter) override {}
					virtual void ShiftCaretHome(int * pos, ContentBox ** box, bool enter) override {}
					virtual void ShiftCaretEnd(int * pos, ContentBox ** box, bool enter) override {}
					virtual void ClearSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override {}
					virtual void RemoveBack(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override {}
					virtual void RemoveForward(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override {}
					virtual void ChildContentsChanged(ContentBox * child) override {}
					virtual int GetMaximalPosition(void) override { return 0; }
					virtual SystemCursor RegularCursorForPosition(int x, int y) override { return SystemCursor::Beam; }
					virtual ContentBox * ActiveBoxForPosition(int x, int y) override { return 0; }
					virtual void MouseMove(int x, int y) override {}
					virtual void MouseLeft(void) override {}
					virtual void MouseClick(int x, int y) override {}
					virtual Box CaretBoxFromPosition(int pos, ContentBox * box) override { return Box(0, 0, 0, 0); }
					virtual ContentBox * GetParentBox(void) override { return _parent; }
					virtual bool HasInternalUnits(void) override { return false; }
					virtual int GetBoxType(void) override { return 2; }
					virtual void InsertText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override {}
					virtual void InsertUnattributedText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override {}
					virtual void DeserializeFromPlainText(Array<int> & index, const Array<uint32> & text, int & pos, uint64 attr_lo, uint64 attr_hi) override
					{
						if (pos + 8 <= text.Length()) {
							uint width = string(text.GetBuffer() + pos, 4, Encoding::UTF32).ToUInt32(HexadecimalBase);
							uint height = string(text.GetBuffer() + pos + 4, 4, Encoding::UTF32).ToUInt32(HexadecimalBase);
							pos += 8;
							int sp = pos;
							while (pos < text.Length() && text[pos] != 27) pos++;
							int ep = pos;
							if (pos < text.Length()) pos += 2;
							SafePointer< Array<uint8> > data = ConvertFromBase64(string(text.GetBuffer() + sp, ep - sp, Encoding::UTF32));
							Streaming::MemoryStream stream(0x10000);
							stream.WriteArray(data);
							stream.Seek(0, Streaming::Begin);
							data.SetReference(0);
							_image = Codec::DecodeImage(&stream);
							ResetCache();
							_width = width; _height = height;
							_box_width = int(_width * Zoom);
							_box_height = int(_height * Zoom);
						} else pos = text.Length();
					}
					virtual void SerializeToPlainText(const Array<int> & index, Array<uint32> & text, uint64 attr_lo, uint64 attr_hi) override
					{
						text << L'\33'; text << L'p';
						string dims = string(uint(_width), HexadecimalBase, 4) + string(uint(_height), HexadecimalBase, 4);
						text << dims[0]; text << dims[1]; text << dims[2]; text << dims[3];
						text << dims[4]; text << dims[5]; text << dims[6]; text << dims[7];
						Streaming::MemoryStream stream(0x10000);
						Codec::EncodeImage(&stream, _image, L"EIWV");
						string base64 = ConvertToBase64(stream.GetBuffer(), int(stream.Length()));
						Array<uint32> ucs(0x10000);
						ucs.SetLength(base64.GetEncodedLength(Encoding::UTF32));
						base64.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
						text << ucs; text << L'\33'; text << L'e';
					}
					virtual void SerializeRange(const Array<int> & index, Array<uint32> & text, int from, int to, uint64 attr_lo, uint64 attr_hi) override { SerializeToPlainText(index, text, attr_lo, attr_hi); }
					virtual void SerializeUnattributedToPlainText(Array<uint32> & text) override {}
					virtual void SerializeUnattributedRange(Array<uint32> & text, int from, int to) override {}
					virtual void EnumerateUsedFonts(Array<int> & index) override {}
					virtual void EnumerateUsedFonts(Array<int> & index, int from, int to) override {}
					virtual int GetChildPosition(ContentBox * child) override { return 0; }
					virtual void GetOuterAttributes(ContentBox * child, uint64 & attr_lo, uint64 & attr_hi) override { attr_lo = attr_hi = 0; }
					virtual void SetAttributesForRange(int from, int to, uint64 attr_lo, uint64 attr_hi, uint64 attr_lo_mask, uint64 attr_hi_mask) override {}
					virtual ContentBox * GetChildBox(int position) override { return 0; }
					void SetImage(Codec::Image * image) { _image.SetRetain(image); ResetCache(); _parent->ChildContentsChanged(this); }
					Codec::Image * GetImage(void) { return _image; }
					void SetSize(int width, int height)
					{
						_width = width; _height = height;
						_box_width = int(_width * Zoom);
						_box_height = int(_height * Zoom);
						_parent->ChildContentsChanged(this);
					}
					int GetWidth(void) { return _width; }
					int GetHeight(void) { return _height; }
				};
				class TableContents : public RichEdit::ContentBox
				{
					friend class ::Engine::UI::Controls::RichEdit;
					RichEdit * _edit;
					RichEdit::ContentBox * _parent;
					ObjectArray<RichEdit::ContentBox> _cells;
					Array<Box> _aabb;
					int _size_x, _size_y;
					int _border_w, _border_v, _border_h;
					Color _border_c;
					Array<int> _col_widths;
					Array<Color> _cell_colors;
					Array<int> _cell_aligns;
					int _width, _height;
					int _org_x, _org_y;
					SafePointer<IBarRenderingInfo> _border_info;
					ObjectArray<IBarRenderingInfo> _cells_info;
				public:
					TableContents(RichEdit * edit, RichEdit::ContentBox * parent) : _edit(edit), _parent(parent),
						_cells(0x10), _aabb(0x10), _size_x(0), _size_y(0), _border_w(0), _border_v(0), _border_h(0), _border_c(0),
						_col_widths(0x10), _cell_colors(0x10), _cell_aligns(0x10),
						_width(0), _height(0), _org_x(0), _org_y(0) {}
					~TableContents(void) override {}
					virtual void Render(IRenderingDevice * device, const Box & at) override
					{
						for (int i = 0; i < _cells.Length(); i++) {
							int dy = 0;
							if (_cell_aligns[i]) {
								int align = _cell_aligns[i];
								int bw = _aabb[i].Bottom - _aabb[i].Top;
								int bx = _aabb[i].Right - _aabb[i].Left;
								int tw = _cells[i].GetContentsHeight();
								if (align == 1) dy = (bw - tw) / 2;
								else if (align == 2) dy = bw - tw;
							}
							auto & c = _cells[i];
							Box cb = Box(at.Left + _aabb[i].Left, at.Top + _aabb[i].Top + dy, at.Left + _aabb[i].Right, at.Top + _aabb[i].Bottom);
							device->PushClip(cb);
							c.Render(device, cb);
							device->PopClip();
						}
						if (_border_c) {
							if (!_border_info) _border_info = device->CreateBarRenderingInfo(_border_c);
							int w = int(_border_w * Zoom);
							if (_border_w) {
								device->RenderBar(_border_info, Box(at.Left, at.Top, at.Left + _width, at.Top + w));
								device->RenderBar(_border_info, Box(at.Left, at.Top + w, at.Left + w, at.Top + _height));
								device->RenderBar(_border_info, Box(at.Left + _width - w, at.Top + w, at.Left + _width, at.Top + _height));
								device->RenderBar(_border_info, Box(at.Left + w, at.Top + _height - w, at.Left + _width - w, at.Top + _height));
							}
							if (_border_v) {
								for (int i = 0; i < _size_x - 1; i++) {
									device->RenderBar(_border_info, Box(at.Left + _aabb[i].Right, at.Top + w, at.Left + _aabb[i + 1].Left, at.Top + _height - w));
								}
							}
							if (_border_h) {
								for (int i = 0; i < _size_y - 1; i++) {
									auto & aabb_1 = _aabb[i * _size_x];
									auto & aabb_2 = _aabb[(i + 1) * _size_x];
									device->RenderBar(_border_info, Box(at.Left + w, at.Top + aabb_1.Bottom, at.Left + _width - w, at.Top + aabb_2.Top));
								}
							}
						}
					}
					virtual void RenderBackground(IRenderingDevice * device, const Box & at) override
					{
						if (!_cells_info.Length()) {
							for (int i = 0; i < _cell_colors.Length(); i++) {
								if (_cell_colors[i]) {
									SafePointer<IBarRenderingInfo> info = device->CreateBarRenderingInfo(_cell_colors[i]);
									_cells_info.Append(info);
								} else _cells_info.Append(0);
							}
						}
						for (int i = 0; i < _cells_info.Length(); i++) {
							auto info = _cells_info.ElementAt(i);
							if (info) device->RenderBar(info, Box(_aabb[i].Left + at.Left, _aabb[i].Top + at.Top, _aabb[i].Right + at.Left, _aabb[i].Bottom + at.Top));
						}
					}
					virtual void ResetCache(void) override
					{
						_border_info.SetReference(0);
						_cells_info.Clear();
						for (int i = 0; i < _cells.Length(); i++) _cells[i].ResetCache();
					}
					virtual void AlignContents(IRenderingDevice * device, int box_width) override
					{
						_aabb.SetLength(_cells.Length());
						int _act_bw = int(_border_w * Zoom);
						int _act_bv = int(_border_v * Zoom);
						int _act_bh = int(_border_h * Zoom);
						_width = 2 * _act_bw + (_size_x - 1) * _act_bv;
						for (int x = 0; x < _size_x; x++) _width += int(_col_widths[x] * Zoom);
						for (int y = 0; y < _size_y; y++) {
							int mh = 0;
							for (int x = 0; x < _size_x; x++) {
								int lx = x ? (_aabb[x + _size_x * y - 1].Right + _act_bv) : _act_bw;
								int ly = y ? (_aabb[x + _size_x * y - _size_x].Bottom + _act_bh) : _act_bw;
								int lw = int(_col_widths[x] * Zoom);
								_cells[x + _size_x * y].SetAbsoluteOrigin(_org_x + lx, _org_y + ly);
								_cells[x + _size_x * y].AlignContents(device, lw);
								int lh = _cells[x + _size_x * y].GetContentsHeight();
								if (lh > mh) mh = lh;
								_aabb[x + _size_x * y] = Box(lx, ly, lx + lw, ly);
							}
							for (int x = 0; x < _size_x; x++) _aabb[x + _size_x * y].Bottom = _aabb[x + _size_x * y].Top + mh;
							for (int x = 0; x < _size_x; x++) {
								if (_cell_aligns[x + _size_x * y]) {
									int index = x + _size_x * y;
									int align = _cell_aligns[index];
									int bw = _aabb[index].Bottom - _aabb[index].Top;
									int bx = _aabb[index].Right - _aabb[index].Left;
									int tw = _cells[index].GetContentsHeight();
									int dy = 0;
									if (align == 1) dy = (bw - tw) / 2;
									else if (align == 2) dy = bw - tw;
									_cells[index].SetAbsoluteOrigin(_org_x + _aabb[index].Left, _org_y + _aabb[index].Top + dy);
									_cells[index].AlignContents(device, bx);
								}
							}
						}
						_height = _aabb.Length() ? (_aabb.LastElement().Bottom + _act_bw) : 2 * _act_bw;
					}
					virtual int GetContentsOriginX(void) override { return _org_x; }
					virtual int GetContentsOriginY(void) override { return _org_y; }
					virtual int GetContentsWidth(void) override { return _width; }
					virtual int GetContentsCurrentWidth(void) override { return _width; }
					virtual int GetContentsHeight(void) override { return _height; }
					virtual void GetTextPositionFromCursor(int x, int y, int * pos, ContentBox ** box) override
					{
						for (int j = 0; j < _size_y; j++) {
							if (y < _aabb[j * _size_x].Top) {
								*box = this;
								*pos = j * _size_x;
								return;
							}
							for (int i = 0; i < _size_x; i++) {
								if (x < _aabb[i + j * _size_x].Left && y < _aabb[i + j * _size_x].Bottom) {
									*box = this;
									*pos = i + j * _size_x;
									return;
								} else if (_aabb[i + j * _size_x].IsInside(Point(x, y))) {
									int dy = 0;
									int index = i + j * _size_x;
									if (_cell_aligns[index]) {
										int align = _cell_aligns[index];
										int bw = _aabb[index].Bottom - _aabb[index].Top;
										int bx = _aabb[index].Right - _aabb[index].Left;
										int tw = _cells[index].GetContentsHeight();
										if (align == 1) dy = (bw - tw) / 2;
										else if (align == 2) dy = bw - tw;
									}
									_cells[i + j * _size_x].GetTextPositionFromCursor(x - _aabb[i + j * _size_x].Left, y - _aabb[i + j * _size_x].Top - dy, pos, box);
									return;
								}
							}
							if (x >= _aabb[j * _size_x + _size_x - 1].Right) {
								*box = this;
								*pos = j * _size_x + _size_x;
								return;
							}
						}
						*box = this;
						*pos = _size_x * _size_y;
					}
					virtual void SelectWord(int x, int y, int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						for (int i = 0; i < _cells.Length(); i++) if (_aabb[i].IsInside(Point(x, y))) {
							int dy = 0;
							if (_cell_aligns[i]) {
								int align = _cell_aligns[i];
								int bw = _aabb[i].Bottom - _aabb[i].Top;
								int bx = _aabb[i].Right - _aabb[i].Left;
								int tw = _cells[i].GetContentsHeight();
								if (align == 1) dy = (bw - tw) / 2;
								else if (align == 2) dy = bw - tw;
							}
							_cells[i].SelectWord(x - _aabb[i].Left, y - _aabb[i].Top - dy, pos1, box1, pos2, box2);
						}
					}
					virtual void SetAbsoluteOrigin(int x, int y) override { _org_x = x; _org_y = y; }
					virtual void ShiftCaretLeft(int * pos, ContentBox ** box, bool enter) override
					{
						if (*pos > 0) {
							if (enter) {
								auto & cell = _cells[*pos - 1];
								*pos = cell.GetMaximalPosition();
								*box = &cell;
							} else (*pos)--;
						} else {
							*pos = _parent->GetChildPosition(this);
							*box = _parent;
						}
					}
					virtual void ShiftCaretRight(int * pos, ContentBox ** box, bool enter) override
					{
						if (*pos < _size_x * _size_y) {
							if (enter) {
								auto & cell = _cells[*pos];
								*pos = 0;
								*box = &cell;
							} else (*pos)++;
						} else {
							*pos = _parent->GetChildPosition(this) + 1;
							*box = _parent;
						}
					}
					virtual void ShiftCaretHome(int * pos, ContentBox ** box, bool enter) override { *pos = 0; }
					virtual void ShiftCaretEnd(int * pos, ContentBox ** box, bool enter) override { *pos = _size_x * _size_y; }
					virtual void ClearSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override { *pos2 = *pos1; *box2 = *box1; }
					virtual void RemoveBack(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override {}
					virtual void RemoveForward(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override {}
					virtual void ChildContentsChanged(ContentBox * child) override { _parent->ChildContentsChanged(this); }
					virtual int GetMaximalPosition(void) override { return _cells.Length(); }
					virtual SystemCursor RegularCursorForPosition(int x, int y) override
					{
						for (int i = 0; i < _cells.Length(); i++) if (_aabb[i].IsInside(Point(x, y))) {
							int dy = 0;
							if (_cell_aligns[i]) {
								int align = _cell_aligns[i];
								int bw = _aabb[i].Bottom - _aabb[i].Top;
								int bx = _aabb[i].Right - _aabb[i].Left;
								int tw = _cells[i].GetContentsHeight();
								if (align == 1) dy = (bw - tw) / 2;
								else if (align == 2) dy = bw - tw;
							}
							return _cells[i].RegularCursorForPosition(x - _aabb[i].Left, y - _aabb[i].Top - dy);
						}
						return SystemCursor::Beam;
					}
					virtual ContentBox * ActiveBoxForPosition(int x, int y) override
					{
						for (int i = 0; i < _cells.Length(); i++) if (_aabb[i].IsInside(Point(x, y))) {
							int dy = 0;
							if (_cell_aligns[i]) {
								int align = _cell_aligns[i];
								int bw = _aabb[i].Bottom - _aabb[i].Top;
								int bx = _aabb[i].Right - _aabb[i].Left;
								int tw = _cells[i].GetContentsHeight();
								if (align == 1) dy = (bw - tw) / 2;
								else if (align == 2) dy = bw - tw;
							}
							return _cells[i].ActiveBoxForPosition(x - _aabb[i].Left, y - _aabb[i].Top - dy);
						}
						return 0;
					}
					virtual void MouseMove(int x, int y) override {}
					virtual void MouseLeft(void) override {}
					virtual void MouseClick(int x, int y) override {}
					virtual Box CaretBoxFromPosition(int pos, ContentBox * box) override
					{
						if (pos <= 0) {
							if (_aabb.Length()) return Box(_aabb[0].Left + _org_x, _aabb[0].Top + _org_y, _aabb[0].Left + _org_x, _aabb[0].Bottom + _org_y);
							else return Box(_org_x, _org_y, _org_x, _org_y + _edit->DefaultFontHeight);
						} else if (pos <= _aabb.Length()) {
							return Box(_aabb[pos - 1].Right + _org_x, _aabb[pos - 1].Top + _org_y, _aabb[pos - 1].Right + _org_x, _aabb[pos - 1].Bottom + _org_y);
						} else return Box(_org_x, _org_y, _org_x, _org_y + _edit->DefaultFontHeight);
					}
					virtual ContentBox * GetParentBox(void) override { return _parent; }
					virtual bool HasInternalUnits(void) override { return true; }
					virtual int GetBoxType(void) override { return 4; }
					virtual void InsertText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override {}
					virtual void InsertUnattributedText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override {}
					virtual void DeserializeFromPlainText(Array<int> & index, const Array<uint32> & text, int & pos, uint64 attr_lo, uint64 attr_hi) override;
					virtual void SerializeToPlainText(const Array<int> & index, Array<uint32> & text, uint64 attr_lo, uint64 attr_hi) override
					{
						text << 27; text << L't';
						DynamicString hdr;
						hdr << string(uint(_size_x), HexadecimalBase, 4);
						hdr << string(uint(_size_y), HexadecimalBase, 4);
						hdr << string(uint(_border_w), HexadecimalBase, 2);
						hdr << string(uint(_border_v), HexadecimalBase, 2);
						hdr << string(uint(_border_h), HexadecimalBase, 2);
						hdr << string(uint(_border_c), HexadecimalBase, 8);
						for (int i = 0; i < _col_widths.Length(); i++) hdr << string(uint(_col_widths[i]), HexadecimalBase, 4);
						for (int i = 0; i < hdr.Length(); i++) text << hdr[i];
						for (int i = 0; i < _cells.Length(); i++) {
							hdr.Clear();
							hdr << string(uint(_cell_colors[i]), HexadecimalBase, 8);
							hdr << string(uint(_cell_aligns[i] + 1), HexadecimalBase, 1);
							for (int i = 0; i < hdr.Length(); i++) text << hdr[i];
							_cells[i].SerializeToPlainText(index, text, attr_lo, attr_hi);
							text << 27; text << L'e';
						}
						text << 27; text << L'e';
					}
					virtual void SerializeRange(const Array<int> & index, Array<uint32> & text, int from, int to, uint64 attr_lo, uint64 attr_hi) override { if (from != to) SerializeToPlainText(index, text, attr_lo, attr_hi); }
					virtual void SerializeUnattributedToPlainText(Array<uint32> & text) override { SerializeUnattributedRange(text, 0, _size_x * _size_y); }
					virtual void SerializeUnattributedRange(Array<uint32> & text, int from, int to) override
					{
						for (int i = from; i < to; i++) {
							if (i > from) text << L'\n';
							_cells[i].SerializeUnattributedToPlainText(text);
						}
					}
					virtual void EnumerateUsedFonts(Array<int> & index) override { for (int i = 0; i < _cells.Length(); i++) _cells[i].EnumerateUsedFonts(index); }
					virtual void EnumerateUsedFonts(Array<int> & index, int from, int to) override { for (int i = from; i < to; i++) _cells[i].EnumerateUsedFonts(index); }
					virtual int GetChildPosition(ContentBox * child) override { for (int i = 0; i < _cells.Length(); i++) if (_cells.ElementAt(i) == child) return i; return 0; }
					virtual void GetOuterAttributes(ContentBox * child, uint64 & attr_lo, uint64 & attr_hi) override { _parent->GetOuterAttributes(this, attr_lo, attr_hi); }
					virtual void SetAttributesForRange(int from, int to, uint64 attr_lo, uint64 attr_hi, uint64 attr_lo_mask, uint64 attr_hi_mask) override
					{ for (int i = from; i < to; i++) _cells[i].SetAttributesForRange(0, _cells[i].GetMaximalPosition(), attr_lo, attr_hi, attr_lo_mask, attr_hi_mask); }
					virtual ContentBox * GetChildBox(int position) override { if (position <= 0 || position > _cells.Length()) return 0; return _cells.ElementAt(position); }
				};
				class TextContents : public RichEdit::ContentBox
				{
					struct Word {
						Box align_rect;
						int range_left;
						int range_right;
						int embedded_object_index; // >= 0 - index, -1 - word, -2 - linebreak, -3 - tab
						int width, height, align;
						ObjectArray<ITextRenderingInfo> text;
						Array<int> advances;

						Word(void) : text(2), advances(8) {}
					};
					struct Embedded {
						SafePointer<RichEdit::ContentBox> inner;
						int pos;
						Box align_rect;
					};
					RichEdit * _edit;
					RichEdit::ContentBox * _parent;
					Array<TaggedChar> _text;
					Array<Embedded> _objs;
					Array<Word> _words;
					Array<Box> _aabb;
					int _width, _height, _act_width;
					int _org_x, _org_y;

					static bool _is_ident_char(const TaggedChar & chr) { return !(chr.lo & FlagLargeObject) && (IsAlphabetical(chr.lo & MaskUcsCode) || ((chr.lo & MaskUcsCode) >= L'0' && (chr.lo & MaskUcsCode) <= L'9') || ((chr.lo & MaskUcsCode) == L'_')); }
				public:
					TextContents(RichEdit * edit, RichEdit::ContentBox * parent) : _edit(edit), _parent(parent),
						_text(0x400), _objs(0x10), _words(0x100), _aabb(0x400), _width(0), _height(0), _act_width(0), _org_x(0), _org_y(0) {}
					~TextContents(void) override {}
					virtual void Render(IRenderingDevice * device, const Box & at) override
					{
						for (int i = 0; i < _words.Length(); i++) {
							auto & w = _words[i];
							Box wb = Box(at.Left + w.align_rect.Left, at.Top + w.align_rect.Top,
								at.Left + w.align_rect.Right, at.Top + w.align_rect.Bottom);
							if (w.embedded_object_index >= 0) {
								_objs[w.embedded_object_index].inner->Render(device, wb);
							} else if (w.embedded_object_index == -1) {
								int ofs = 0;
								for (int j = 0; j < w.text.Length(); j++) {
									int sw, sh;
									w.text[j].GetExtent(sw, sh);
									device->RenderText(w.text.ElementAt(j), Box(wb.Left + ofs, wb.Top, wb.Left + ofs + sw, wb.Bottom), false);
									ofs += sw;
								}
							}
						}
					}
					virtual void RenderBackground(IRenderingDevice * device, const Box & at) override
					{
						for (int i = 0; i < _objs.Length(); i++) {
							auto & w = _objs[i];
							Box wb = Box(at.Left + w.align_rect.Left, at.Top + w.align_rect.Top,
								at.Left + w.align_rect.Right, at.Top + w.align_rect.Bottom);
							w.inner->RenderBackground(device, wb);
						}
					}
					virtual void ResetCache(void) override { _words.Clear(); for (int i = 0; i < _objs.Length(); i++) _objs[i].inner->ResetCache(); }
					virtual void AlignContents(IRenderingDevice * device, int box_width) override
					{
						if (!_words.Length()) {
							int pos = 0;
							while (pos < _text.Length()) {
								while (pos < _text.Length() && (_text[pos].lo & MaskUcsCode) == 32 &&
									!(_text[pos].lo & StyleUnderline) && !(_text[pos].lo & StyleStrikeout) &&
									!(_text[pos].lo & FlagLargeObject)) pos++;
								if (pos >= _text.Length()) break;
								if ((_text[pos].lo & MaskUcsCode) == L'\n' && !(_text[pos].lo & FlagLargeObject)) {
									_words << Word();
									_words.LastElement().embedded_object_index = -2;
									_words.LastElement().range_left = pos;
									_words.LastElement().range_right = pos + 1;
									_words.LastElement().width = _words.LastElement().height = 0;
									_words.LastElement().align = int((_text[pos].lo & MaskAlignment) >> 37);
									pos++;
								} else if ((_text[pos].lo & MaskUcsCode) == L'\t' && !(_text[pos].lo & FlagLargeObject)) {
									_words << Word();
									_words.LastElement().embedded_object_index = -3;
									_words.LastElement().range_left = pos;
									_words.LastElement().range_right = pos + 1;
									_words.LastElement().width = _words.LastElement().height = 0;
									_words.LastElement().align = int((_text[pos].lo & MaskAlignment) >> 37);
									pos++;
								} else if (_text[pos].lo & FlagLargeObject) {
									_words << Word();
									_words.LastElement().embedded_object_index = _text[pos].lo & MaskUcsCode;
									_words.LastElement().range_left = pos;
									_words.LastElement().range_right = pos + 1;
									_objs[_words.LastElement().embedded_object_index].inner->AlignContents(device, box_width);
									_words.LastElement().width = max(_objs[_words.LastElement().embedded_object_index].inner->GetContentsWidth(), 1);
									_words.LastElement().height = _objs[_words.LastElement().embedded_object_index].inner->GetContentsHeight();
									_words.LastElement().align = int((_text[pos].lo & MaskAlignment) >> 37);
									pos++;
								} else {
									int sp = pos;
									while (pos < _text.Length() && !(_text[pos].lo & FlagLargeObject) &&
										((_text[pos].lo & MaskUcsCode) != 32 || (_text[pos].lo & StyleUnderline) || (_text[pos].lo & StyleStrikeout)) &&
										(_text[pos].lo & MaskUcsCode) != L'\n' && (_text[pos].lo & MaskUcsCode) != L'\t') pos++;
									_words << Word();
									_words.LastElement().range_left = sp;
									_words.LastElement().range_right = pos;
									_words.LastElement().embedded_object_index = -1;
									_words.LastElement().height = 0;
									_words.LastElement().width = 0;
									_words.LastElement().align = int((_text[sp].lo & MaskAlignment) >> 37);
									Array<uint32> ss(0x100);
									uint64 attr;
									int lp = sp;
									int adv = 0;
									while (lp < pos) {
										int lsp = lp;
										attr = _text[lp].hi;
										while (lp < pos && _text[lp].hi == attr) lp++;
										ss.Clear();
										for (int i = lsp; i < lp; i++) ss << (_text[i].lo & MaskUcsCode);
										IFont * fnt = _edit->GetCachedFont(int((attr & MaskObjectCacheIndex) >> 32));
										Color clr = uint32(attr & MaskColor);
										SafePointer<ITextRenderingInfo> obj = device->CreateTextRenderingInfo(fnt, ss, 0, 1, clr);
										for (int i = 0; i < ss.Length(); i++) _words.LastElement().advances << adv + obj->EndOfChar(i);
										int w, h;
										obj->GetExtent(w, h);
										adv += w;
										_words.LastElement().width += w;
										if (h > _words.LastElement().height) _words.LastElement().height = h;
										_words.LastElement().text.Append(obj);
									}
									if (_words.LastElement().width == 0) _words.LastElement().width = 1;
								}
							}
						}
						if (_aabb.Length() != _text.Length()) _aabb.SetLength(_text.Length());
						int wpos = 0;
						int tpos = 0;
						int caret_y = 0;
						_width = _height = _act_width = 0;
						int space, tab, align, font_height;
						int last_line_top = 0;
						int last_line_end = 0;
						IFont * fnt;
						if (tpos < _text.Length()) {
							int f = int(_text[tpos].hi >> 32);
							fnt = _edit->GetCachedFont(f);
							align = _words[wpos].align;
						} else {
							int ff = _edit->RegisterFontFace(_edit->DefaultFontFace);
							int f = _edit->RegisterFont(ff, _edit->DefaultFontHeight, 0);
							fnt = _edit->GetCachedFont(f);
							align = 0;
						}
						space = fnt->GetWidth();
						font_height = fnt->GetHeight();
						tab = font_height * 3;
						while (wpos < _words.Length()) {
							last_line_top = caret_y;
							int caret_x = 0;
							if (tpos < _text.Length()) {
								int f = int(_text[tpos].hi >> 32);
								fnt = _edit->GetCachedFont(f);
								align = _words[wpos].align;
							} else {
								int ff = _edit->RegisterFontFace(_edit->DefaultFontFace);
								int f = _edit->RegisterFont(ff, _edit->DefaultFontHeight, 0);
								fnt = _edit->GetCachedFont(f);
								align = 0;
							}
							space = fnt->GetWidth();
							font_height = fnt->GetHeight();
							tab = font_height * 3;
							int line_height = font_height;
							int swpos = wpos;
							int stpos = tpos;
							int spaces_count = 0;
							Array<int> advances(0x80);
							while (wpos < _words.Length()) {
								int new_caret_x = caret_x;
								int space_shift = space * (_words[wpos].range_left - tpos);
								new_caret_x += space_shift;
								if (_words[wpos].embedded_object_index == -3) {
									int old_caret_x = new_caret_x;
									new_caret_x = ((new_caret_x / tab) + 1) * tab;
									_words[wpos].width = new_caret_x - old_caret_x;
								}
								else new_caret_x += _words[wpos].width;
								bool force_linebreak = _words[wpos].embedded_object_index == -2;
								if (wpos == swpos || new_caret_x <= box_width) {
									spaces_count += _words[wpos].range_left - tpos;
									advances << caret_x + space_shift;
									caret_x = new_caret_x;
									if (_words[wpos].height > line_height) line_height = _words[wpos].height;
									tpos = _words[wpos].range_right;
									wpos++;
								} else {
									if (space_shift) tpos++;
									break;
								}
								if (force_linebreak) { if (align == 3) align = 0; break; }
							}
							if (caret_x > _width) _width = caret_x;
							int line_start = 0;
							if (align == 0) {
								for (int w = swpos; w < wpos; w++) {
									_words[w].align_rect.Left = advances[w - swpos];
									_words[w].align_rect.Right = _words[w].align_rect.Left + _words[w].width;
									_words[w].align_rect.Top = caret_y;
									_words[w].align_rect.Bottom = _words[w].align_rect.Top + line_height;
								}
							} else if (align == 1) {
								int dx = line_start = (box_width - caret_x) / 2;
								for (int w = swpos; w < wpos; w++) {
									_words[w].align_rect.Left = advances[w - swpos] + dx;
									_words[w].align_rect.Right = _words[w].align_rect.Left + _words[w].width;
									_words[w].align_rect.Top = caret_y;
									_words[w].align_rect.Bottom = _words[w].align_rect.Top + line_height;
								}
							} else if (align == 2) {
								int dx = line_start = box_width - caret_x;
								for (int w = swpos; w < wpos; w++) {
									_words[w].align_rect.Left = advances[w - swpos] + dx;
									_words[w].align_rect.Right = _words[w].align_rect.Left + _words[w].width;
									_words[w].align_rect.Top = caret_y;
									_words[w].align_rect.Bottom = _words[w].align_rect.Top + line_height;
								}
							} else if (align == 3) {
								int uncovered = box_width - caret_x;
								int spaces = spaces_count;
								int ltpos = stpos;
								int dx = line_start = 0;
								for (int w = swpos; w < wpos; w++) {
									int local_spaces = _words[w].range_left - ltpos;
									int space_width = uncovered * local_spaces / spaces;
									dx += space_width;
									uncovered -= space_width;
									spaces -= local_spaces;
									ltpos = _words[w].range_right;
									_words[w].align_rect.Left = advances[w - swpos] + dx;
									_words[w].align_rect.Right = _words[w].align_rect.Left + _words[w].width;
									_words[w].align_rect.Top = caret_y;
									_words[w].align_rect.Bottom = _words[w].align_rect.Top + line_height;
								}
							}
							for (int w = swpos; w < wpos; w++) if (_words[w].embedded_object_index >= 0) {
								_objs[_words[w].embedded_object_index].align_rect = _words[w].align_rect;
								_objs[_words[w].embedded_object_index].inner->SetAbsoluteOrigin(_org_x + _words[w].align_rect.Left, _org_y + _words[w].align_rect.Top);
								_objs[_words[w].embedded_object_index].inner->AlignContents(device, _words[w].width);
							}
							int ltpos = stpos;
							int ladv = line_start;
							for (int w = swpos; w < wpos; w++) {
								if (_words[w].range_left > ltpos) {
									int space_width = _words[w].align_rect.Left - ladv;
									int space_num = _words[w].range_left - ltpos;
									for (int c = ltpos; c < _words[w].range_left; c++) {
										int i = c - ltpos;
										_aabb[c].Top = caret_y;
										_aabb[c].Bottom = caret_y + line_height;
										_aabb[c].Left = ladv + i * space_width / space_num;
										_aabb[c].Right = ladv + (i + 1) * space_width / space_num;
									}
								}
								ladv = _words[w].align_rect.Left;
								if (_words[w].embedded_object_index == -1) {
									int prev_adv = 0;
									for (int i = 0; i < _words[w].advances.Length(); i++) {
										_aabb[_words[w].range_left + i] = Box(ladv + prev_adv, caret_y,
											ladv + _words[w].advances[i] + ((_words[w].advances[i] == prev_adv) ? 1 : 0), caret_y + line_height);
										prev_adv = _words[w].advances[i];
									}
								} else {
									int width = _words[w].align_rect.Right - _words[w].align_rect.Left;
									for (int i = _words[w].range_left; i < _words[w].range_right; i++) {
										int prev_ofs = (i - _words[w].range_left) * width / (_words[w].range_right - _words[w].range_left);
										int ofs = (i + 1 - _words[w].range_left) * width / (_words[w].range_right - _words[w].range_left);
										_aabb[i] = Box(ladv + prev_ofs, caret_y, ladv + ofs, caret_y + line_height);
									}
								}
								ltpos = _words[w].range_right;
								ladv = _words[w].align_rect.Right;
							}
							if (ltpos < tpos) {
								for (int i = ltpos; i < tpos - 1; i++) {
									_aabb[i] = Box(ladv, caret_y, ladv + space, caret_y + line_height);
									ladv += space;
								}
								_aabb[tpos - 1] = Box(ladv, caret_y, ladv, caret_y + line_height);
							}
							caret_y += line_height;
							_height = caret_y;
							last_line_end = _aabb[tpos - 1].Right;
						}
						if (!_words.Length() || _words.LastElement().embedded_object_index == -2) {
							last_line_top = _height; last_line_end = 0;
							_height += font_height;
						}
						for (int i = tpos; i < _aabb.Length(); i++) {
							_aabb[i].Top = last_line_top;
							_aabb[i].Bottom = _height;
							_aabb[i].Left = last_line_end;
							_aabb[i].Right = last_line_end + space;
							last_line_end += space;
						}
						for (int i = 0; i < _aabb.Length(); i++) if (_aabb[i].Right > _act_width) _act_width = _aabb[i].Right;
					}
					virtual int GetContentsOriginX(void) override { return _org_x; }
					virtual int GetContentsOriginY(void) override { return _org_y; }
					virtual int GetContentsWidth(void) override { return _width; }
					virtual int GetContentsCurrentWidth(void) override { return _act_width; }
					virtual int GetContentsHeight(void) override { return _height; }
					virtual void GetTextPositionFromCursor(int x, int y, int * pos, ContentBox ** box) override
					{
						for (int i = 0; i < _objs.Length(); i++) if (_objs[i].align_rect.IsInside(Point(x, y)) && _objs[i].inner->HasInternalUnits()) {
							_objs[i].inner->GetTextPositionFromCursor(x - _objs[i].align_rect.Left, y - _objs[i].align_rect.Top, pos, box);
							return;
						}
						*box = this; *pos = _aabb.Length();
						if (y < 0) *pos = 0;
						else if (y >= _height) *pos = _aabb.Length();
						else for (int i = 0; i < _aabb.Length(); i++) {
							if (y >= _aabb[i].Top && y < _aabb[i].Bottom) {
								if (x < _aabb[i].Right) {
									if (x < (_aabb[i].Left + _aabb[i].Right) / 2) { *pos = i; return; } else { *pos = i + 1; return; }
								} else if (i == _aabb.Length() - 1 || y < _aabb[i + 1].Top) {
									if (_aabb[i].Right == _aabb[i].Left) { *pos = i; return; } else { *pos = i + 1; return; }
								}
							}
						}
					}
					virtual void SelectWord(int x, int y, int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						for (int i = 0; i < _objs.Length(); i++) if (_objs[i].align_rect.IsInside(Point(x, y)) && _objs[i].inner->HasInternalUnits()) {
							_objs[i].inner->SelectWord(x - _objs[i].align_rect.Left, y - _objs[i].align_rect.Top, pos1, box1, pos2, box2);
							return;
						}
						*box1 = *box2 = this;
						int pos = _aabb.Length();
						int tag_pos = -1;
						if (y < 0) pos = 0;
						else if (y >= _height) pos = _aabb.Length();
						else for (int i = 0; i < _aabb.Length(); i++) {
							if (y >= _aabb[i].Top && y < _aabb[i].Bottom) {
								if (x < _aabb[i].Right) {
									if (x >= _aabb[i].Left) tag_pos = i;
									if (x < (_aabb[i].Left + _aabb[i].Right) / 2) { pos = i; break; } else { pos = i + 1; break; }
								} else if (i == _aabb.Length() - 1 || y < _aabb[i + 1].Top) {
									if (_aabb[i].Right == _aabb[i].Left) { pos = i; break; } else { pos = i + 1; break; }
								}
							}
						}
						int sel_pos = pos;
						int len = _text.Length();
						if (_text[tag_pos].lo & FlagLargeObject) {
							sel_pos = tag_pos; pos = tag_pos + 1;
						} else {
							while (sel_pos > 0 && _is_ident_char(_text[sel_pos - 1])) sel_pos--;
							while (pos < len && _is_ident_char(_text[pos])) pos++;
							if (pos == sel_pos && tag_pos != -1) { sel_pos = tag_pos; pos = tag_pos + 1; }
						}
						*pos1 = pos;
						*pos2 = sel_pos;
					}
					virtual void SetAbsoluteOrigin(int x, int y) override { _org_x = x; _org_y = y; }
					virtual void ShiftCaretLeft(int * pos, ContentBox ** box, bool enter) override
					{
						if (*pos == 0) {
							if (!_parent) return;
							*pos = _parent->GetChildPosition(*box);
							*box = _parent;
							_parent->ShiftCaretLeft(pos, box, enter);
						} else {
							(*pos)--;
							if (enter && (_text[*pos].lo & FlagLargeObject) && _objs[_text[*pos].lo & MaskUcsCode].inner->HasInternalUnits()) {
								*box = _objs[_text[*pos].lo & MaskUcsCode].inner;
								*pos = (*box)->GetMaximalPosition();
								(*box)->ShiftCaretLeft(pos, box, true);
							}
						}
					}
					virtual void ShiftCaretRight(int * pos, ContentBox ** box, bool enter) override
					{
						if (*pos == _text.Length()) {
							if (!_parent) return;
							*pos = _parent->GetChildPosition(*box) + 1;
							*box = _parent;
							_parent->ShiftCaretRight(pos, box, enter);
						} else {
							if (enter && (_text[*pos].lo & FlagLargeObject) && _objs[_text[*pos].lo & MaskUcsCode].inner->HasInternalUnits()) {
								*box = _objs[_text[*pos].lo & MaskUcsCode].inner;
								*pos = 0;
								(*box)->ShiftCaretRight(pos, box, true);
							} else (*pos)++;
						}
					}
					virtual void ShiftCaretHome(int * pos, ContentBox ** box, bool enter) override { while (*pos > 0) { if (_aabb[*pos - 1].Top == _aabb[*pos].Top) (*pos)--; else break; } }
					virtual void ShiftCaretEnd(int * pos, ContentBox ** box, bool enter) override
					{
						while (*pos < _aabb.Length()) {
							if (CaretBoxFromPosition(*pos, *box).Top == CaretBoxFromPosition(*pos + 1, *box).Top) (*pos)++;
							else break;
						}
					}
					virtual void ClearSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						int mn = min(*pos1, *pos2);
						int mx = max(*pos1, *pos2);
						Array<int> obj_reind(0x10);
						obj_reind.SetLength(_objs.Length());
						ZeroMemory(obj_reind.GetBuffer(), obj_reind.Length() * sizeof(int));
						for (int i = mn; i < mx; i++) {
							if (_text[i].lo & FlagLargeObject) {
								_objs[_text[i].lo & MaskUcsCode].inner.SetReference(0);
								_text[i].lo &= ~(FlagLargeObject | MaskUcsCode);
							}
						}
						int ci = 0;
						for (int i = 0; i < _objs.Length(); i++) {
							if (_objs[i].inner) { obj_reind[i] = ci; ci++; } else obj_reind[i] = -1;
						}
						for (int i = _objs.Length() - 1; i >= 0; i--) if (!_objs[i].inner) _objs.Remove(i);
						for (int i = 0; i < _text.Length(); i++) {
							if (_text[i].lo & FlagLargeObject) {
								int index = _text[i].lo & MaskUcsCode;
								_text[i].lo &= ~MaskUcsCode;
								_text[i].lo |= obj_reind[index];
							}
						}
						int d = mx - mn;
						for (int i = mx; i < _text.Length(); i++) _text[i - d] = _text[i];
						_text.SetLength(_text.Length() - d);
						for (int i = mn; i < _text.Length(); i++) if (_text[i].lo & FlagLargeObject) _objs[_text[i].lo & MaskUcsCode].pos = i;
						int pos = 0;
						while (pos < _text.Length()) {
							auto align_attr = _text[pos].lo & MaskAlignment;
							int sp = pos;
							while (pos < _text.Length() && (_text[pos].lo & FlagLargeObject || (_text[pos].lo & MaskUcsCode) != L'\n')) pos++;
							if (pos < _text.Length() && !(_text[pos].lo & FlagLargeObject)) pos++;
							for (int i = sp; i < pos; i++) _text[i].lo = (_text[i].lo & ~MaskAlignment) | align_attr;
						}
						if (_parent && _parent->GetBoxType() == 3 && _text.Length() == 0) {
							auto box_exit_to = _parent->GetParentBox();
							*pos1 = box_exit_to->GetChildPosition(_parent);
							*pos2 = *pos1 + 1;
							*box1 = *box2 = box_exit_to;
							box_exit_to->ClearSelection(pos1, box1, pos2, box2);
						} else {
							*pos1 = *pos2 = mn;
							ChildContentsChanged(this);
						}
					}
					virtual void RemoveBack(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						if (*pos1 > 0) {
							if ((_text[(*pos1) - 1].lo & FlagLargeObject) && _objs[_text[(*pos1) - 1].lo & MaskUcsCode].inner->HasInternalUnits()) {
								*box1 = *box2 = _objs[_text[(*pos1) - 1].lo & MaskUcsCode].inner;
								*pos1 = *pos2 = (*box1)->GetMaximalPosition();
								(*box1)->RemoveBack(pos1, box1, pos2, box2);
							} else {
								(*pos1)--;
								ClearSelection(pos1, box1, pos2, box2);
							}
						} else {
							if (_parent) {
								*box1 = *box2 = _parent;
								*pos1 = *pos2 = _parent->GetChildPosition(this);
								(*box1)->RemoveBack(pos1, box1, pos2, box2);
							}
						}
					}
					virtual void RemoveForward(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2) override
					{
						if (*pos1 < _text.Length()) {
							if ((_text[*pos1].lo & FlagLargeObject) && _objs[_text[*pos1].lo & MaskUcsCode].inner->HasInternalUnits()) {
								*box1 = *box2 = _objs[_text[*pos1].lo & MaskUcsCode].inner;
								*pos1 = *pos2 = 0;
								(*box1)->RemoveForward(pos1, box1, pos2, box2);
							} else {
								(*pos1)++;
								ClearSelection(pos1, box1, pos2, box2);
							}
						} else {
							if (_parent) {
								*box1 = *box2 = _parent;
								*pos1 = *pos2 = _parent->GetChildPosition(this) + 1;
								(*box1)->RemoveForward(pos1, box1, pos2, box2);
							}
						}
					}
					virtual void ChildContentsChanged(ContentBox * child) override
					{
						_words.Clear();
						if (_parent) _parent->ChildContentsChanged(this);
						else _edit->RealignContents();
					}
					virtual int GetMaximalPosition(void) override { return _text.Length(); }
					virtual SystemCursor RegularCursorForPosition(int x, int y) override
					{
						for (int i = 0; i < _objs.Length(); i++) if (_objs[i].align_rect.IsInside(Point(x, y)))
							return _objs[i].inner->RegularCursorForPosition(x - _objs[i].align_rect.Left, y - _objs[i].align_rect.Top);
						return SystemCursor::Beam;
					}
					virtual ContentBox * ActiveBoxForPosition(int x, int y) override
					{
						for (int i = 0; i < _objs.Length(); i++) if (_objs[i].align_rect.IsInside(Point(x, y)))
							return _objs[i].inner->ActiveBoxForPosition(x - _objs[i].align_rect.Left, y - _objs[i].align_rect.Top);
						return 0;
					}
					virtual void MouseMove(int x, int y) override {}
					virtual void MouseLeft(void) override {}
					virtual void MouseClick(int x, int y) override {}
					virtual Box CaretBoxFromPosition(int pos, ContentBox * box) override
					{
						if (pos == 0) {
							int dh = _aabb.Length() ? (_aabb.FirstElement().Bottom - _aabb.FirstElement().Top) : int(_edit->DefaultFontHeight * Zoom);
							return Box(_org_x, _org_y, _org_x, _org_y + dh);
						} else {
							auto & aabb = _aabb[pos - 1];
							if (aabb.Right == aabb.Left) {
								int nx = (pos < _aabb.Length()) ? _aabb[pos].Left : 0;
								int h = (pos < _aabb.Length()) ? (_aabb[pos].Bottom - _aabb[pos].Top) : (aabb.Bottom - aabb.Top);
								return Box(_org_x + nx, _org_y + aabb.Bottom, _org_x + nx, _org_y + aabb.Bottom + h);
							} else {
								return Box(_org_x + aabb.Right, _org_y + aabb.Top, _org_x + aabb.Right, _org_y + aabb.Bottom);
							}
						}
					}
					virtual ContentBox * GetParentBox(void) override { return _parent; }
					virtual bool HasInternalUnits(void) override { return true; }
					virtual int GetBoxType(void) override { return 1; }
					void DeserializeFromPlainTextRecurrent(Array<int> & index, const Array<uint32> & text, int & pos, int & at, uint64 attr_lo, uint64 attr_hi)
					{
						bool is_link = _parent && _parent->GetBoxType() == 3;
						if (is_link) attr_lo |= StyleUnderline;
						uint64 attr = (attr_lo & MaskStyleAttributes) >> 32;
						uint64 family = (attr_lo & MaskFontFamily) >> 40;
						uint64 size = (attr_lo & MaskFontHeight) >> 48;
						uint64 font_idx = size ? _edit->RegisterFont(int(family), int(size), int(attr)) : 0;
						attr_hi = (attr_hi & MaskColor) | (font_idx << 32);
						while (pos < text.Length()) {
							if (text[pos] == 27) {
								pos++;
								if (text[pos] == L'e') { pos++; break; }
								else if (text[pos] == L'b') {
									pos++;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo | StyleBold, attr_hi);
								} else if (text[pos] == L'i') {
									pos++;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo | StyleItalic, attr_hi);
								} else if (text[pos] == L'u') {
									pos++;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo | StyleUnderline, attr_hi);
								} else if (text[pos] == L's') {
									pos++;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo | StyleStrikeout, attr_hi);
								} else if (text[pos] == L'f') {
									pos++;
									uint32 dg[3];
									dg[0] = text[pos];
									dg[1] = dg[0] ? text[pos + 1] : 0;
									dg[2] = 0;
									uint64 face_idx = index[string(&dg, -1, Encoding::UTF32).ToUInt32(HexadecimalBase)];
									pos += 2;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, (attr_lo & ~MaskFontFamily) | (face_idx << 40), attr_hi);
								} else if (text[pos] == L'c') {
									pos++;
									uint32 dg[9];
									dg[0] = text[pos];
									for (int j = 1; j < 8; j++) dg[j] = dg[j - 1] ? text[pos + j] : 0;
									dg[8] = 0;
									string clr_code = string(&dg, -1, Encoding::UTF32);
									uint64 clr = (clr_code == L"********") ? uint32(_edit->DefaultTextColor) : clr_code.ToUInt32(HexadecimalBase);
									pos += 8;
									if (is_link) DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo, attr_hi);
									else DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo, clr);
								} else if (text[pos] == L'h') {
									pos++;
									uint32 dg[5];
									dg[0] = text[pos];
									for (int j = 1; j < 4; j++) dg[j] = dg[j - 1] ? text[pos + j] : 0;
									dg[4] = 0;
									string height_code = string(&dg, -1, Encoding::UTF32);
									uint64 height = (height_code == L"****") ? _edit->DefaultFontHeight : height_code.ToUInt32(HexadecimalBase);
									pos += 4;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, (attr_lo & ~MaskFontHeight) | (height << 48), attr_hi);
								} else if (text[pos] == L'a') {
									pos++;
									uint32 dg[2];
									dg[0] = text[pos];
									dg[1] = 0;
									uint64 align = string(&dg, -1, Encoding::UTF32).ToUInt32(HexadecimalBase) - 1;
									pos++;
									if (is_link && align == 3) align = 0;
									DeserializeFromPlainTextRecurrent(index, text, pos, at, (attr_lo & ~MaskAlignment) | (align << 37), attr_hi);
								} else if (text[pos] == L'n') {
									pos++;
									int sp = pos;
									while (pos < text.Length() && text[pos] != 27) pos++;
									string fn = string(text.GetBuffer() + sp, pos - sp, Encoding::UTF32);
									if (fn == L"*") fn = _edit->DefaultFontFace;
									pos += 2;
									index << _edit->RegisterFontFace(fn);
								} else if (text[pos] == L'l') {
									pos++;
									_text.Insert(TaggedChar{ _objs.Length() | attr_lo | FlagLargeObject, attr_hi }, at);
									at++;
									_objs << Embedded();
									_objs.LastElement().pos = at - 1;
									_objs.LastElement().inner = new LinkContents(_edit, this);
									_objs.LastElement().inner->DeserializeFromPlainText(index, text, pos, attr_lo, attr_hi);
									if (is_link) { at--; _objs.RemoveLast(); _text.Remove(at); }
								} else if (text[pos] == L'p') {
									pos++;
									_text.Insert(TaggedChar{ _objs.Length() | attr_lo | FlagLargeObject, attr_hi }, at);
									at++;
									_objs << Embedded();
									_objs.LastElement().pos = at - 1;
									_objs.LastElement().inner = new PictureContents(_edit, this);
									_objs.LastElement().inner->DeserializeFromPlainText(index, text, pos, attr_lo, attr_hi);
								} else if (text[pos] == L't') {
									pos++;
									_text.Insert(TaggedChar{ _objs.Length() | attr_lo | FlagLargeObject, attr_hi }, at);
									at++;
									_objs << Embedded();
									_objs.LastElement().pos = at - 1;
									_objs.LastElement().inner = new TableContents(_edit, this);
									_objs.LastElement().inner->DeserializeFromPlainText(index, text, pos, attr_lo, attr_hi);
								} else break;
							} else if (text[pos] >= 32 || text[pos] == L'\n' || text[pos] == L'\t') {
								_text.Insert(TaggedChar{ uint64(text[pos]) | attr_lo, attr_hi }, at);
								at++;
								pos++;
							} else pos++;
						}
					}
					void SerializeToPlainTextRecurrent(const Array<int> & index, Array<uint32> & text, int & pos, int to, uint64 attr_lo, uint64 attr_hi, uint64 prev_attr_lo, uint64 prev_attr_hi)
					{
						while (pos < to) {
							if ((_text[pos].lo & 0xFFFFFFEF00000000) == attr_lo && (_text[pos].hi & 0xFFFFFFFF) == attr_hi) {
								if (_text[pos].lo & FlagLargeObject) {
									_objs[_text[pos].lo & MaskUcsCode].inner->SerializeToPlainText(index, text, attr_lo, attr_hi);
								} else {
									text << uint32(_text[pos].lo & MaskUcsCode);
								}
								pos++;
							} else {
								if ((_text[pos].lo & 0xFFFFFFEF00000000) == prev_attr_lo && (_text[pos].hi & 0xFFFFFFFF) == prev_attr_hi) {
									break;
								} else {
									uint64 new_attr_lo = (_text[pos].lo & 0xFFFFFFEF00000000);
									uint64 new_attr_hi = (_text[pos].hi & 0xFFFFFFFF);
									if (((new_attr_lo | attr_lo) & MaskStyleAttributes) != (new_attr_lo & MaskStyleAttributes)) {
										break;
									} else if ((new_attr_lo & MaskFontFamily) != (attr_lo & MaskFontFamily)) {
										int ff = index[int((new_attr_lo & MaskFontFamily) >> 40)];
										string code(uint(ff), HexadecimalBase, 2);
										text << L'\33'; text << L'f'; text << code[0]; text << code[1];
										SerializeToPlainTextRecurrent(index, text, pos, to, (attr_lo & ~MaskFontFamily) | (new_attr_lo & MaskFontFamily), attr_hi, attr_lo, attr_hi);
										text << L'\33'; text << L'e';
									} else if ((new_attr_lo & MaskFontHeight) != (attr_lo & MaskFontHeight)) {
										int fh = int((new_attr_lo & MaskFontHeight) >> 48);
										string code(uint(fh), HexadecimalBase, 4);
										text << L'\33'; text << L'h'; text << code[0]; text << code[1]; text << code[2]; text << code[3];
										SerializeToPlainTextRecurrent(index, text, pos, to, (attr_lo & ~MaskFontHeight) | (new_attr_lo & MaskFontHeight), attr_hi, attr_lo, attr_hi);
										text << L'\33'; text << L'e';
									} else if ((new_attr_hi & MaskColor) != (attr_hi & MaskColor)) {
										uint32 fc = uint32(new_attr_hi & MaskColor);
										string code(fc, HexadecimalBase, 8);
										text << L'\33'; text << L'c';
										text << code[0]; text << code[1]; text << code[2]; text << code[3];
										text << code[4]; text << code[5]; text << code[6]; text << code[7];
										SerializeToPlainTextRecurrent(index, text, pos, to, attr_lo, fc, attr_lo, attr_hi);
										text << L'\33'; text << L'e';
									} else if ((new_attr_lo & MaskAlignment) != (attr_lo & MaskAlignment)) {
										int fa = int((new_attr_lo & MaskAlignment) >> 37);
										text << L'\33'; text << L'a'; text << (L'1' + fa);
										SerializeToPlainTextRecurrent(index, text, pos, to, (attr_lo & ~MaskAlignment) | (new_attr_lo & MaskAlignment), attr_hi, attr_lo, attr_hi);
										text << L'\33'; text << L'e';
									} else {
										uint64 style = (attr_lo & MaskStyleAttributes);
										uint64 style_new = (new_attr_lo & MaskStyleAttributes);
										if ((style_new & StyleBold) != (style & StyleBold)) {
											text << L'\33'; text << L'b';
											SerializeToPlainTextRecurrent(index, text, pos, to, attr_lo | StyleBold, attr_hi, attr_lo, attr_hi);
											text << L'\33'; text << L'e';
										} else if ((style_new & StyleItalic) != (style & StyleItalic)) {
											text << L'\33'; text << L'i';
											SerializeToPlainTextRecurrent(index, text, pos, to, attr_lo | StyleItalic, attr_hi, attr_lo, attr_hi);
											text << L'\33'; text << L'e';
										} else if ((style_new & StyleUnderline) != (style & StyleUnderline)) {
											text << L'\33'; text << L'u';
											SerializeToPlainTextRecurrent(index, text, pos, to, attr_lo | StyleUnderline, attr_hi, attr_lo, attr_hi);
											text << L'\33'; text << L'e';
										} else if ((style_new & StyleStrikeout) != (style & StyleStrikeout)) {
											text << L'\33'; text << L's';
											SerializeToPlainTextRecurrent(index, text, pos, to, attr_lo | StyleStrikeout, attr_hi, attr_lo, attr_hi);
											text << L'\33'; text << L'e';
										}
									}
								}
							}
						}
					}
					virtual void InsertText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override
					{
						Array<int> index(2);
						int pos = 0, local_at = at;
						bool is_link = _parent && _parent->GetBoxType() == 3;
						DeserializeFromPlainTextRecurrent(index, text, pos, local_at, 0, is_link ? uint32(_edit->HyperlinkColor) : 0);
						for (int i = local_at; i < _text.Length(); i++) if (_text[i].lo & FlagLargeObject) _objs[_text[i].lo & MaskUcsCode].pos = i;
						*box_end = this;
						*pos_end = local_at;
						ChildContentsChanged(this);
					}
					virtual void InsertUnattributedText(const Array<uint32> & text, int at, ContentBox ** box_end, int * pos_end) override
					{
						uint64 attr_lo, attr_hi;
						if (at < _text.Length()) {
							attr_lo = _text[at].lo & ~(MaskUcsCode | FlagLargeObject);
							attr_hi = _text[at].hi & MaskColor;
						} else if (_text.Length()) {
							attr_lo = _text.LastElement().lo & ~(MaskUcsCode | FlagLargeObject);
							attr_hi = _text.LastElement().hi & MaskColor;
						} else if (_parent) {
							_parent->GetOuterAttributes(this, attr_lo, attr_hi);
						} else {
							attr_lo = (uint64(_edit->DefaultFontHeight) << 48);
							attr_hi = _edit->DefaultTextColor;
						}
						int font_face = int((attr_lo & MaskFontFamily) >> 40);
						attr_lo &= ~MaskFontFamily;
						Array<int> index(2);
						index << font_face;
						int pos = 0, local_at = at;
						DeserializeFromPlainTextRecurrent(index, text, pos, local_at, attr_lo, attr_hi);
						for (int i = local_at; i < _text.Length(); i++) if (_text[i].lo & FlagLargeObject) _objs[_text[i].lo & MaskUcsCode].pos = i;
						*box_end = this;
						*pos_end = local_at;
						ChildContentsChanged(this);
					}
					virtual void DeserializeFromPlainText(Array<int> & index, const Array<uint32> & text, int & pos, uint64 attr_lo, uint64 attr_hi) override
					{
						_text.Clear(); _words.Clear(); _objs.Clear(); _aabb.Clear();
						int at = 0;
						DeserializeFromPlainTextRecurrent(index, text, pos, at, attr_lo, attr_hi);
					}
					virtual void SerializeToPlainText(const Array<int> & index, Array<uint32> & text, uint64 attr_lo, uint64 attr_hi) override { SerializeRange(index, text, 0, _text.Length(), attr_lo, attr_hi); }
					virtual void SerializeRange(const Array<int> & index, Array<uint32> & text, int from, int to, uint64 attr_lo, uint64 attr_hi) override
					{
						if (attr_hi == 0xFFFFFFFFFFFFFFFF) {
							Array<uint32> ucs(0x10);
							for (int i = 0; i < index.Length(); i++) if (index[i] >= 0) {
								string family = _edit->GetFontFaceWithIndex(index[i]);
								ucs.SetLength(family.GetEncodedLength(Encoding::UTF32));
								family.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
								text << L'\33'; text << L'n'; text << ucs; text << L'\33'; text << L'e';
							}
							int attr_src = -1;
							for (int i = 0; i < _text.Length(); i++) if (!(_text[i].lo & FlagLargeObject)) { attr_src = i; break; }
							int family, height;
							uint32 color;
							if (attr_src >= 0) {
								family = int((_text[attr_src].lo & MaskFontFamily) >> 40);
								height = int((_text[attr_src].lo & MaskFontHeight) >> 48);
								color = uint32(_text[attr_src].hi & MaskColor);
							} else {
								family = 0; for (int i = 0; i < index.Length(); i++) if (index[i] >= 0) { family = i; break; }
								height = _edit->DefaultFontHeight;
								color = _edit->DefaultTextColor;
							}
							attr_lo = (uint64(family) << 40) | (uint64(height) << 48);
							attr_hi = color;
							ucs.SetLength(2); string(uint(index[family]), HexadecimalBase, 2).Encode(ucs.GetBuffer(), Encoding::UTF32, false);
							text << L'\33'; text << L'f'; text << ucs;
							ucs.SetLength(4); string(uint(height), HexadecimalBase, 4).Encode(ucs.GetBuffer(), Encoding::UTF32, false);
							text << L'\33'; text << L'h'; text << ucs;
							ucs.SetLength(8); string(uint(color), HexadecimalBase, 8).Encode(ucs.GetBuffer(), Encoding::UTF32, false);
							text << L'\33'; text << L'c'; text << ucs;
							SerializeRange(index, text, from, to, attr_lo, attr_hi);
							text << L'\33'; text << L'e'; text << L'\33'; text << L'e'; text << L'\33'; text << L'e';
						} else {
							int pos = from;
							SerializeToPlainTextRecurrent(index, text, pos, to, attr_lo, attr_hi, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
						}
					}
					virtual void SerializeUnattributedToPlainText(Array<uint32> & text) override { SerializeUnattributedRange(text, 0, _text.Length()); }
					virtual void SerializeUnattributedRange(Array<uint32> & text, int from, int to) override
					{
						for (int i = from; i < to; i++) {
							if (_text[i].lo & FlagLargeObject) _objs[_text[i].lo & MaskUcsCode].inner->SerializeUnattributedToPlainText(text);
							else text << (_text[i].lo & MaskUcsCode);
						}
					}
					virtual void EnumerateUsedFonts(Array<int> & index) override { EnumerateUsedFonts(index, 0, _text.Length()); }
					virtual void EnumerateUsedFonts(Array<int> & index, int from, int to) override
					{
						for (int i = from; i < to; i++) {
							if (_text[i].lo & FlagLargeObject) _objs[_text[i].lo & MaskUcsCode].inner->EnumerateUsedFonts(index);
							else index[int((_text[i].lo & MaskFontFamily) >> 40)] = 1;
						}
					}
					virtual int GetChildPosition(ContentBox * child) override
					{
						for (int i = 0; i < _objs.Length(); i++) if (_objs[i].inner.Inner() == child) return _objs[i].pos;
						return 0;
					}
					virtual void GetOuterAttributes(ContentBox * child, uint64 & attr_lo, uint64 & attr_hi) override
					{
						int index = GetChildPosition(child);
						attr_lo = _text[index].lo & ~(MaskUcsCode | FlagLargeObject);
						attr_hi = _text[index].hi & MaskColor;
					}
					virtual void SetAttributesForRange(int from, int to, uint64 attr_lo, uint64 attr_hi, uint64 attr_lo_mask, uint64 attr_hi_mask) override
					{
						attr_lo_mask &= ~(MaskUcsCode | FlagLargeObject);
						attr_hi_mask &= MaskColor;
						bool is_link = _parent && _parent->GetBoxType() == 3;
						if (is_link) {
							attr_lo |= StyleUnderline;
							attr_lo &= ~MaskAlignment;
							attr_hi = _edit->HyperlinkColor;
						}
						attr_lo &= attr_lo_mask;
						attr_hi &= attr_hi_mask;
						attr_hi_mask |= MaskObjectCacheIndex;
						for (int i = from; i < to; i++) {
							_text[i].lo &= ~attr_lo_mask;
							_text[i].hi &= ~attr_hi_mask;
							_text[i].lo |= attr_lo;
							_text[i].hi |= attr_hi;
							uint64 attr = (_text[i].lo & MaskStyleAttributes) >> 32;
							uint64 family = (_text[i].lo & MaskFontFamily) >> 40;
							uint64 size = (_text[i].lo & MaskFontHeight) >> 48;
							uint64 font_idx = size ? _edit->RegisterFont(int(family), int(size), int(attr)) : 0;
							_text[i].hi |= font_idx << 32;
							if (_text[i].lo & FlagLargeObject) {
								auto & obj = _objs[_text[i].lo & MaskUcsCode];
								obj.inner->SetAttributesForRange(0, obj.inner->GetMaximalPosition(), attr_lo, attr_hi, attr_lo_mask, attr_hi_mask);
							}
						}
						int pos = from;
						while (pos < to) {
							auto align_attr = _text[pos].lo & MaskAlignment;
							int sp = pos;
							int ep = pos;
							while (sp >= 0 && (_text[sp].lo & FlagLargeObject || (_text[sp].lo & MaskUcsCode) != L'\n')) sp--;
							sp++;
							while (ep < _text.Length() && (_text[ep].lo & FlagLargeObject || (_text[ep].lo & MaskUcsCode) != L'\n')) ep++;
							if (ep < _text.Length() && (_text[ep].lo & MaskUcsCode) == L'\n') ep++;
							for (int i = sp; i < ep; i++) _text[i].lo = (_text[i].lo & ~MaskAlignment) | align_attr;
							pos = ep;
						}
						ChildContentsChanged(this);
					}
					virtual ContentBox * GetChildBox(int position) override
					{
						if (position < 0 || position >= _text.Length()) return 0;
						if (_text[position].lo & FlagLargeObject) return _objs[_text[position].lo & MaskUcsCode].inner;
						else return 0;
					}
					void GetAttributesForPosition(int at, uint64 & attr_lo, uint64 & attr_hi)
					{
						if (at < _text.Length()) {
							attr_lo = _text[at].lo & ~(MaskUcsCode | FlagLargeObject);
							attr_hi = _text[at].hi & MaskColor;
						} else if (_text.Length()) {
							attr_lo = _text.LastElement().lo & ~(MaskUcsCode | FlagLargeObject);
							attr_hi = _text.LastElement().hi & MaskColor;
						} else if (_parent) {
							_parent->GetOuterAttributes(this, attr_lo, attr_hi);
						} else {
							attr_lo = (uint64(_edit->DefaultFontHeight) << 48);
							attr_hi = _edit->DefaultTextColor;
						}
					}
					bool CanCreateLinkFromRange(int from, int to)
					{
						for (int i = from; i < to; i++) if (_text[i].lo & FlagLargeObject) {
							if (_objs[_text[i].lo & MaskUcsCode].inner->HasInternalUnits()) return false;
						}
						return true;
					}
				};
				LinkContents::LinkContents(RichEdit * edit, RichEdit::ContentBox * parent) : _edit(edit), _parent(parent),
					_width(0), _height(0), _org_x(0), _org_y(0), _state(0) { box = new TextContents(edit, this); hot_box = new TextContents(edit, this); }
				void TableContents::DeserializeFromPlainText(Array<int> & index, const Array<uint32> & text, int & pos, uint64 attr_lo, uint64 attr_hi)
				{
					ResetCache();
					_cells.Clear();
					_col_widths.Clear();
					_cell_aligns.Clear();
					_cell_colors.Clear();
					_aabb.Clear();
					_size_x = _size_y = _border_w = _border_v = _border_h = _border_c = 0;
					if (pos + 22 <= text.Length()) {
						int size_x = string(text.GetBuffer() + pos, 4, Encoding::UTF32).ToUInt32(HexadecimalBase);
						int size_y = string(text.GetBuffer() + pos + 4, 4, Encoding::UTF32).ToUInt32(HexadecimalBase);
						int border_w = string(text.GetBuffer() + pos + 8, 2, Encoding::UTF32).ToUInt32(HexadecimalBase);
						int border_v = string(text.GetBuffer() + pos + 10, 2, Encoding::UTF32).ToUInt32(HexadecimalBase);
						int border_h = string(text.GetBuffer() + pos + 12, 2, Encoding::UTF32).ToUInt32(HexadecimalBase);
						uint border_c = string(text.GetBuffer() + pos + 14, 8, Encoding::UTF32).ToUInt32(HexadecimalBase);
						pos += 22;
						if (pos + 4 * size_x <= text.Length()) {
							_col_widths.SetLength(size_x);
							for (int x = 0; x < size_x; x++) {
								uint col_w = string(text.GetBuffer() + pos, 4, Encoding::UTF32).ToUInt32(HexadecimalBase);
								_col_widths[x] = col_w; pos += 4;
							}
							for (int y = 0; y < size_y; y++) for (int x = 0; x < size_x; x++) {
								if (pos + 9 <= text.Length()) {
									uint back = string(text.GetBuffer() + pos, 8, Encoding::UTF32).ToUInt32(HexadecimalBase);
									uint align = string(text.GetBuffer() + pos + 8, 1, Encoding::UTF32).ToUInt32(HexadecimalBase) - 1;
									pos += 9;
									_cell_colors << back;
									_cell_aligns << align;
								} else {
									pos = text.Length();
									_cell_colors << 0;
									_cell_aligns << 0;
								}
								SafePointer<RichEdit::ContentBox> cell = new TextContents(_edit, this);
								_cells.Append(cell);
								cell->DeserializeFromPlainText(index, text, pos, attr_lo, attr_hi);
							}
							_size_x = size_x; _size_y = size_y;
							_border_w = border_w; _border_v = border_v; _border_h = border_h; _border_c = border_c;
						} else pos = text.Length();
					} else pos = text.Length();
				}
			}
			void RichEdit::_align_contents(const Box & box)
			{
				if (_init) return;
				if (_caret_width < 0) _caret_width = CaretWidth ? CaretWidth : GetStation()->GetVisualStyles().CaretWidth;
				auto device = GetStation()->GetRenderingDevice();
				_root->AlignContents(device, box.Right - box.Left - (ReadOnly ? 0 : _caret_width) - 2 * Border);
				if (_root->GetContentsHeight() > box.Bottom - box.Top - 2 * Border) {
					_root->AlignContents(device, box.Right - box.Left - ScrollSize - (ReadOnly ? 0 : _caret_width) - 2 * Border);
					_vscroll->Show(true);
					_vscroll->SetRangeSilent(0, _root->GetContentsHeight());
					_vscroll->SetPageSilent(box.Bottom - box.Top - 2 * Border);
				} else {
					_vscroll->Show(false);
					_vscroll->SetScrollerPositionSilent(0);
				}
			}
			void RichEdit::_scroll_to_caret(void)
			{
				if (_caret_box) {
					auto caret = _caret_box->CaretBoxFromPosition(_caret_pos, _caret_box);
					if (_vscroll->IsVisible()) {
						if (caret.Top < _vscroll->Position) _vscroll->SetScrollerPositionSilent(caret.Top);
						else if (caret.Bottom >= _vscroll->Position + _vscroll->Page) _vscroll->SetScrollerPositionSilent(caret.Bottom - _vscroll->Page);
					} else _vscroll->SetScrollerPositionSilent(0);
				}
			}
			bool RichEdit::_selection_empty(void) { return _caret_box == _selection_box && _caret_pos == _selection_pos; }
			void RichEdit::_make_font_index(Array<int> & index, ContentBox * root, int from, int to)
			{
				if (!_font_faces.Length()) RegisterFontFace(DefaultFontFace);
				index.SetLength(_font_faces.Length());
				ZeroMemory(index.GetBuffer(), index.Length() * sizeof(int));
				if (from >= 0) root->EnumerateUsedFonts(index, from, to);
				else root->EnumerateUsedFonts(index);
				int reint = 0;
				for (int i = 0; i < index.Length(); i++) { if (index[i]) { index[i] = reint; reint++; } else index[i] = -1; }
				if (!reint) index[0] = 0;
			}
			RichEdit::RichEdit(Window * Parent, WindowStation * Station) : ParentWindow(Parent, Station), _undo_interface(this), _undo(&_undo_interface, 10), _font_faces(0x10), _fonts(0x20)
			{
				ControlPosition = Rectangle::Invalid();
				Reflection::PropertyZeroInitializer Initializer;
				EnumerateProperties(Initializer);
				_root = new RichEditContents::TextContents(this, 0);
				_root->SetAbsoluteOrigin(0, 0);
				ResetCache();
			}
			RichEdit::RichEdit(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template) : ParentWindow(Parent, Station), _undo_interface(this), _undo(&_undo_interface, 10), _font_faces(0x10), _fonts(0x20)
			{
				if (Template->Properties->GetTemplateClass() != L"RichEdit") throw InvalidArgumentException();
				static_cast<Template::Controls::RichEdit &>(*this) = static_cast<Template::Controls::RichEdit &>(*Template->Properties);
				_menu.SetReference(ContextMenu ? new Menus::Menu(ContextMenu) : 0);
				_root = new RichEditContents::TextContents(this, 0);
				_root->SetAbsoluteOrigin(0, 0);
				ResetCache();
				SetText(Text); Text = L"";
			}
			RichEdit::~RichEdit(void) {}
			void RichEdit::Render(const Box & at)
			{
				auto device = GetStation()->GetRenderingDevice();
				if (BackgroundColor) {
					if (!_background) _background = device->CreateBarRenderingInfo(BackgroundColor);
					device->RenderBar(_background, at);
				}
				if (_init) {
					for (int i = 0; i < _fonts.Length(); i++) RecreateCachedFont(i);
					_init = false;
					_align_contents(at);
				}
				if (_deferred_scroll) { _scroll_to_caret(); _deferred_scroll = false; }
				int dx = at.Left + Border;
				int dy = at.Top + Border - _vscroll->Position;
				Box rect = Box(at.Left + Border, at.Top + Border - _vscroll->Position, at.Right, at.Bottom);
				_root->RenderBackground(device, rect);
				if (!_selection) _selection.SetReference(device->CreateBarRenderingInfo(SelectionColor));
				if (_selection_pos != _caret_pos && _selection_box) {
					int min_sel = min(_selection_pos, _caret_pos);
					int max_sel = max(_selection_pos, _caret_pos);
					auto frame_begin = _caret_box->CaretBoxFromPosition(min_sel, _caret_box);
					auto frame_end = _caret_box->CaretBoxFromPosition(max_sel, _caret_box);
					if (frame_begin.Top == frame_end.Top) {
						device->RenderBar(_selection, Box(dx + frame_begin.Left, dy + frame_begin.Top, dx + frame_end.Right, dy + frame_end.Bottom));
					} else {
						int left_bound = _caret_box->GetContentsOriginX();
						int right_bound = left_bound + _caret_box->GetContentsCurrentWidth();
						device->RenderBar(_selection, Box(dx + frame_begin.Left, dy + frame_begin.Top, dx + right_bound, dy + frame_begin.Bottom));
						device->RenderBar(_selection, Box(dx + left_bound, dy + frame_end.Top, dx + frame_end.Right, dy + frame_end.Bottom));
						if (frame_begin.Bottom != frame_end.Top) {
							device->RenderBar(_selection, Box(dx + left_bound, dy + frame_begin.Bottom, dx + right_bound, dy + frame_end.Top));
						}
					}
				}
				_root->Render(device, rect);
				if (_caret_box && !ReadOnly && GetFocus() == this) {
					auto caret = _caret_box->CaretBoxFromPosition(_caret_pos, _caret_box);
					if (!_inversion) {
						if (CaretColor.a) {
							_inversion.SetReference(device->CreateBarRenderingInfo(CaretColor));
							_use_color_caret = true;
						} else {
							_inversion.SetReference(device->CreateInversionEffectRenderingInfo());
							_use_color_caret = false;
						}
					}
					caret.Left += dx; caret.Right += dx + _caret_width; caret.Top += dy; caret.Bottom += dy;
					if (_use_color_caret) { if (device->CaretShouldBeVisible()) device->RenderBar(static_cast<IBarRenderingInfo *>(_inversion.Inner()), caret); }
					else device->ApplyInversion(static_cast<IInversionEffectRenderingInfo *>(_inversion.Inner()), caret, true);
				}
				ParentWindow::Render(at);
			}
			void RichEdit::ResetCache(void)
			{
				_caret_width = -1;
				for (int i = 0; i < _fonts.Length(); i++) RecreateCachedFont(i);
				bool scroll_visible = _vscroll ? _vscroll->IsVisible() : false;
				if (_vscroll) _vscroll->Destroy();
				_vscroll = GetStation()->CreateWindow<VerticalScrollBar>(this, this);
				_vscroll->SetRectangle(Rectangle(
					Coordinate::Right() - ScrollSize, 0,
					Coordinate::Right(), Coordinate::Bottom()));
				_vscroll->Disabled = Disabled;
				_vscroll->Line = int(Zoom * 20.0);
				_vscroll->Show(scroll_visible);
				_inversion.SetReference(0);
				_background.SetReference(0);
				ParentWindow::ResetCache();
				_root->ResetCache();
				Box box(0, 0, WindowPosition.Right - WindowPosition.Left, WindowPosition.Bottom - WindowPosition.Top);
				_align_contents(box);
			}
			void RichEdit::Enable(bool enable) { Disabled = !enable; _state = 0; }
			bool RichEdit::IsEnabled(void) { return !Disabled; }
			void RichEdit::Show(bool visible) { Invisible = !visible; _state = 0; }
			bool RichEdit::IsVisible(void) { return !Invisible; }
			bool RichEdit::IsTabStop(void) { return true; }
			void RichEdit::SetID(int _ID) { ID = _ID; }
			int RichEdit::GetID(void) { return ID; }
			void RichEdit::SetRectangle(const Rectangle & rect) { ControlPosition = rect; GetParent()->ArrangeChildren(); }
			Rectangle RichEdit::GetRectangle(void) { return ControlPosition; }
			void RichEdit::SetPosition(const Box & box) { ParentWindow::SetPosition(box); _align_contents(box); }
			void RichEdit::SetText(const string & text)
			{
				if (text[0] == L'\33' && text[1] == L'X') SetAttributedText(text.Fragment(2, -1));
				else SetAttributedText(L"\33n*\33e\33f00\33h****\33c********" + text + L"\33e\33e\33e");
			}
			string RichEdit::GetText(void)
			{
				Array<uint32> ucs(0x1000);
				_root->SerializeUnattributedToPlainText(ucs);
				return string(ucs.GetBuffer(), ucs.Length(), Encoding::UTF32);
			}
			void RichEdit::RaiseEvent(int ID, Event event, Window * sender)
			{
				if (event == Event::MenuCommand) {
					if (ID == 1001) Undo();
					else if (ID == 1000) Redo();
					else if (ID == 1002) Cut();
					else if (ID == 1003) Copy();
					else if (ID == 1004) Paste();
					else if (ID == 1005) Delete();
					else GetParent()->RaiseEvent(ID, Event::Command, this);
				} else if (ID) GetParent()->RaiseEvent(ID, event, sender);
			}
			void RichEdit::FocusChanged(bool got_focus) { if (!got_focus) { _save = true; } }
			void RichEdit::CaptureChanged(bool got_capture) { if (!got_capture) { _state = 0; if (_hot_box) { _hot_box->MouseLeft(); _hot_box = 0; } } }
			void RichEdit::LeftButtonDown(Point at)
			{
				if (!_hot_box || !ReadOnly) {
					SetFocus();
					SetCapture();
					_hot_box = 0;
					_state = 1;
					_save = true;
					auto prev_caret_box = _caret_box;
					auto prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos;
					auto prev_selection_pos = _selection_pos;
					_root->GetTextPositionFromCursor(at.x - Border, at.y - Border + _vscroll->Position, &_caret_pos, &_caret_box);
					if (Keyboard::IsKeyPressed(KeyCodes::Shift)) {
						_root->UnifyBoxForSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					} else {
						_selection_box = _caret_box;
						_selection_pos = _caret_pos;
					}
					_scroll_to_caret();
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
						prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
				}
			}
			void RichEdit::LeftButtonUp(Point at)
			{
				if (_state) {
					ReleaseCapture();
				} else if (ReadOnly && _hot_box) {
					_hot_box->MouseClick(at.x - Border, at.y - Border + _vscroll->Position);
				}
			}
			void RichEdit::LeftButtonDoubleClick(Point at)
			{
				if (!_hot_box || !ReadOnly) {
					auto prev_caret_box = _caret_box;
					auto prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos;
					auto prev_selection_pos = _selection_pos;
					_root->SelectWord(at.x - Border, at.y - Border + _vscroll->Position, &_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					_scroll_to_caret();
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
						prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
				}
			}
			void RichEdit::RightButtonDown(Point at)
			{
				if (!_hot_box || !ReadOnly) {
					SetFocus();
					_hot_box = 0;
					_save = true;
					auto prev_caret_box = _caret_box;
					auto prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos;
					auto prev_selection_pos = _selection_pos;
					ContentBox * click_box;
					int click_pos;
					_root->GetTextPositionFromCursor(at.x - Border, at.y - Border + _vscroll->Position, &click_pos, &click_box);
					ContentBox * check_box = click_box;
					int check_pos = click_pos;
					while (check_box && check_box != _caret_box) {
						auto parent = check_box->GetParentBox();
						if (parent) check_pos = parent->GetChildPosition(check_box); else check_pos = -1;
						check_box = parent;
					}
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (!_caret_box || check_pos < mn || check_pos > mx) {
						_selection_box = _caret_box = click_box;
						_selection_pos = _caret_pos = click_pos;
						_scroll_to_caret();
						if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
							prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
					}
				}
			}
			void RichEdit::RightButtonUp(Point at)
			{
				if (_menu) {
					auto pos = GetStation()->GetCursorPos();
					auto undo = _menu->FindChild(1001);
					auto redo = _menu->FindChild(1000);
					auto cut = _menu->FindChild(1002);
					auto copy = _menu->FindChild(1003);
					auto paste = _menu->FindChild(1004);
					auto remove = _menu->FindChild(1005);
					if (undo) undo->Disabled = ReadOnly || !_undo.CanUndo();
					if (redo) redo->Disabled = ReadOnly || !_undo.CanRedo();
					if (cut) cut->Disabled = ReadOnly || (_caret_pos == _selection_pos && _caret_box == _selection_box);
					if (copy) copy->Disabled = _caret_pos == _selection_pos && _caret_box == _selection_box;
					if (paste) paste->Disabled = ReadOnly || (!Clipboard::IsFormatAvailable(Clipboard::Format::Text) &&
						!Clipboard::IsFormatAvailable(Clipboard::Format::Image) && !Clipboard::IsFormatAvailable(Clipboard::Format::RichText));
					if (remove) remove->Disabled = ReadOnly || (_caret_pos == _selection_pos && _caret_box == _selection_box);
					if (_hook) _hook->InitializeContextMenu(_menu, this);
					_menu->RunPopup(this, pos);
				}
			}
			void RichEdit::MouseMove(Point at)
			{
				if (_state) {
					auto prev_caret_box = _caret_box;
					auto prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos;
					auto prev_selection_pos = _selection_pos;
					_root->GetTextPositionFromCursor(at.x - Border, at.y - Border + _vscroll->Position, &_caret_pos, &_caret_box);
					_root->UnifyBoxForSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);					
					_scroll_to_caret();
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
						prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
				} else if (ReadOnly) {
					bool inside;
					if (at.x >= 0 && at.y >= 0 &&
						at.x < WindowPosition.Right - WindowPosition.Left - (_vscroll->IsVisible() ? ScrollSize : 0) &&
						at.y < WindowPosition.Bottom - WindowPosition.Top) inside = true;
					else inside = false;
					if (inside) {
						SetCapture();
						auto box = _root->ActiveBoxForPosition(at.x - Border, at.y - Border + _vscroll->Position);
						if (box != _hot_box) {
							if (_hot_box) _hot_box->MouseLeft();
							_hot_box = box;
						}
						if (_hot_box) _hot_box->MouseMove(at.x - Border, at.y - Border + _vscroll->Position);
					} else if (GetCapture() == this) ReleaseCapture();
				}
			}
			void RichEdit::ScrollVertically(double delta)
			{
				if (_vscroll->IsVisible()) _vscroll->SetScrollerPositionSilent(_vscroll->Position + int(delta * double(_vscroll->Line)));
				else ParentWindow::ScrollVertically(delta);
			}
			bool RichEdit::KeyDown(int key_code)
			{
				if (key_code == KeyCodes::Back && !ReadOnly) {
					if (!_caret_box) { _caret_box = _selection_box = _root; _caret_pos = _selection_pos = 0; }
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					if (_caret_pos == _selection_pos) {
						if (_caret_box) _caret_box->RemoveBack(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					} else {
						if (_caret_box) _caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					}
					_deferred_scroll = true;
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					return true;
				} else if (key_code == KeyCodes::Delete && !ReadOnly) {
					if (!_caret_box) { _caret_box = _selection_box = _root; _caret_pos = _selection_pos = 0; }
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					if (_caret_pos == _selection_pos) {
						if (_caret_box) _caret_box->RemoveForward(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					} else {
						if (_caret_box) _caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					}
					_deferred_scroll = true;
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					return true;
				} else if (key_code == KeyCodes::Left || key_code == KeyCodes::Right) {
					_save = true;
					auto prev_caret_box = _caret_box, prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos, prev_selection_pos = _selection_pos;
					if (_caret_box) {
						if (Keyboard::IsKeyPressed(KeyCodes::Shift)) {
							if (key_code == KeyCodes::Left) _caret_box->ShiftCaretLeft(&_caret_pos, &_caret_box, false);
							else _caret_box->ShiftCaretRight(&_caret_pos, &_caret_box, false);
							_root->UnifyBoxForSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
						} else {
							if (key_code == KeyCodes::Left) _caret_box->ShiftCaretLeft(&_caret_pos, &_caret_box, true);
							else _caret_box->ShiftCaretRight(&_caret_pos, &_caret_box, true);
							_selection_box = _caret_box;
							_selection_pos = _caret_pos;
						}
					} else {
						_caret_box = _selection_box = _root;
						_caret_pos = _selection_pos = 0;
					}
					_scroll_to_caret();
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
						prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
					return true;
				} else if (key_code == KeyCodes::Escape && _caret_box) {
					_save = true;
					auto prev_selection_box = _selection_box;
					auto prev_selection_pos = _selection_pos;
					_selection_box = _caret_box;
					_selection_pos = _caret_pos;
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos) if (_hook) _hook->CaretPositionChanged(this);
					return true;
				} else if (key_code == KeyCodes::Home || key_code == KeyCodes::End) {
					_save = true;
					auto prev_caret_box = _caret_box, prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos, prev_selection_pos = _selection_pos;
					if (_caret_box) {
						if (key_code == KeyCodes::Home) _caret_box->ShiftCaretHome(&_caret_pos, &_caret_box, false);
						else _caret_box->ShiftCaretEnd(&_caret_pos, &_caret_box, false);
						if (!Keyboard::IsKeyPressed(KeyCodes::Shift)) {
							_selection_box = _caret_box;
							_selection_pos = _caret_pos;
						}
					} else {
						_caret_box = _selection_box = _root;
						_caret_pos = _selection_pos = 0;
					}
					_scroll_to_caret();
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
						prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
					return true;
				} else if (key_code == KeyCodes::Up || key_code == KeyCodes::Down || key_code == KeyCodes::PageUp || key_code == KeyCodes::PageDown) {
					_save = true;
					auto prev_caret_box = _caret_box, prev_selection_box = _selection_box;
					auto prev_caret_pos = _caret_pos, prev_selection_pos = _selection_pos;
					if (_caret_box) {
						auto caret = _caret_box->CaretBoxFromPosition(_caret_pos, _caret_box);
						int hx = caret.Left;
						int hy_mn = caret.Top;
						int hy_mx = caret.Bottom;
						int hy = 0;
						if (key_code == KeyCodes::Up) hy = hy_mn - 1;
						else if (key_code == KeyCodes::Down) hy = hy_mx;
						else if (key_code == KeyCodes::PageUp) hy = hy_mn - _vscroll->Page;
						else if (key_code == KeyCodes::PageDown) hy = hy_mx + _vscroll->Page;
						_root->GetTextPositionFromCursor(hx, hy, &_caret_pos, &_caret_box);
						if (Keyboard::IsKeyPressed(KeyCodes::Shift)) {
							_root->UnifyBoxForSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
						} else {
							_selection_box = _caret_box;
							_selection_pos = _caret_pos;
						}
					} else {
						_caret_box = _selection_box = _root;
						_caret_pos = _selection_pos = 0;
					}
					_scroll_to_caret();
					if (prev_selection_box != _selection_box || prev_selection_pos != _selection_pos ||
						prev_caret_box != _caret_box || prev_caret_pos != _caret_pos) if (_hook) _hook->CaretPositionChanged(this);
					return true;
				} else if (key_code == KeyCodes::Return && !ReadOnly) {
					_save = true;
					CharDown(L'\n');
					return true;
				} else if (!Keyboard::IsKeyPressed(KeyCodes::Shift) && Keyboard::IsKeyPressed(KeyCodes::Control) &&
					!Keyboard::IsKeyPressed(KeyCodes::Alternative) && !Keyboard::IsKeyPressed(KeyCodes::System)) {
					if (key_code == KeyCodes::Z) { Undo(); return true; }
					else if (key_code == KeyCodes::X) { Cut(); return true; }
					else if (key_code == KeyCodes::C) { Copy(); return true; }
					else if (key_code == KeyCodes::V) { Paste(); return true; }
					else if (key_code == KeyCodes::Y) { Redo(); return true; }
				}
				return false;
			}
			void RichEdit::CharDown(uint32 ucs_code)
			{
				if (!ReadOnly) {
					if (_save) {
						_undo.PushCurrentVersion();
						_save = false;
					}
					Print(string(&ucs_code, 1, Encoding::UTF32));
					_deferred_scroll = true;
				}
			}
			void RichEdit::SetCursor(Point at)
			{
				if (ReadOnly) GetStation()->SetCursor(GetStation()->GetSystemCursor(
					_root->RegularCursorForPosition(at.x - Border, at.y - Border + _vscroll->Position)));
				else GetStation()->SetCursor(GetStation()->GetSystemCursor(SystemCursor::Beam));
			}
			Window::RefreshPeriod RichEdit::FocusedRefreshPeriod(void) { return ReadOnly ? Window::RefreshPeriod::None : Window::RefreshPeriod::CaretBlink; }
			string RichEdit::GetControlClass(void) { return L"RichEdit"; }
			void RichEdit::Undo(void)
			{
				if (!ReadOnly && _undo.CanUndo()) {
					_undo.Undo();
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void RichEdit::Redo(void)
			{
				if (!ReadOnly && _undo.CanRedo()) {
					_undo.Redo();
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void RichEdit::Cut(void)
			{
				if (!ReadOnly && _caret_box && _caret_pos != _selection_pos) {
					_undo.PushCurrentVersion();
					_save = true;
					Copy();
					_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					_deferred_scroll = true;
				}
			}
			void RichEdit::Copy(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					_save = true;
					Array<uint32> ucs(0x1000);
					Array<uint32> ucs_noattr(0x1000);
					Array<int> font_index(0x10);
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (DontCopyAttributes) {
						_caret_box->SerializeUnattributedRange(ucs_noattr, mn, mx);
						Clipboard::SetData(string(ucs_noattr.GetBuffer(), ucs_noattr.Length(), Encoding::UTF32));
					} else {
						_make_font_index(font_index, _caret_box, mn, mx);
						_caret_box->SerializeRange(font_index, ucs, mn, mx, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
						_caret_box->SerializeUnattributedRange(ucs_noattr, mn, mx);
						Clipboard::SetData(string(ucs_noattr.GetBuffer(), ucs_noattr.Length(), Encoding::UTF32),
							string(ucs.GetBuffer(), ucs.Length(), Encoding::UTF32));
					}
				}
			}
			void RichEdit::Paste(void)
			{
				if (!ReadOnly) {
					string text;
					if (Clipboard::GetData(text, true)) {
						_undo.PushCurrentVersion();
						_save = true;
						PrintAttributed(text);
						_deferred_scroll = true;
					} else if (Clipboard::GetData(text)) {
						_undo.PushCurrentVersion();
						_save = true;
						Print(text);
						_deferred_scroll = true;
					} else {
						SafePointer<Codec::Frame> frame;
						if (Clipboard::GetData(frame.InnerRef())) {
							SafePointer<Codec::Image> image = new Codec::Image;
							image->Frames.Append(frame);
							_undo.PushCurrentVersion();
							_save = true;
							InsertImage(image);
							_deferred_scroll = true;
						}
					}
				}
			}
			void RichEdit::Delete(void)
			{
				if (!ReadOnly && _caret_box && _caret_pos != _selection_pos) {
					_undo.PushCurrentVersion();
					_save = true;
					_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					_deferred_scroll = true;
				}
			}
			bool RichEdit::IsReadOnly(void) { return ReadOnly; }
			void RichEdit::SetReadOnly(bool read_only)
			{
				if (read_only) {
					_undo.RemoveAllVersions();
					string contents = GetAttributedText();
					SetAttributedText(contents);
				}
				ReadOnly = read_only;
			}
			void RichEdit::SetAttributedText(const string & text)
			{
				_caret_box = _selection_box = 0;
				_caret_pos = _selection_pos = 0;
				_hot_box = 0;
				_state = 0;
				_vscroll->Position = 0;
				_font_faces.Clear();
				_fonts.Clear();
				_save = true;
				int pos = 0;
				Array<uint32> ucs(0x100);
				Array<int> index(0x10);
				ucs.SetLength(text.GetEncodedLength(Encoding::UTF32) + 1);
				text.Encode(ucs.GetBuffer(), Encoding::UTF32, true);
				_root->DeserializeFromPlainText(index, ucs, pos, 0, 0);
				_vscroll->SetScrollerPositionSilent(0);
				_align_contents(WindowPosition);
				_undo.RemoveAllVersions();
			}
			string RichEdit::GetAttributedText(void)
			{
				Array<uint32> ucs(0x1000);
				Array<int> font_index(0x10);
				_make_font_index(font_index, _root, -1, -1);
				_root->SerializeToPlainText(font_index, ucs, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
				return string(ucs.GetBuffer(), ucs.Length(), Encoding::UTF32);
			}
			string RichEdit::SerializeSelection(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					Array<uint32> ucs_noattr(0x1000);
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					_caret_box->SerializeUnattributedRange(ucs_noattr, mn, mx);
					return string(ucs_noattr.GetBuffer(), ucs_noattr.Length(), Encoding::UTF32);
				} else return L"";
			}
			string RichEdit::SerializeSelectionAttributed(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					Array<uint32> ucs(0x1000);
					Array<int> font_index(0x10);
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					_make_font_index(font_index, _caret_box, mn, mx);
					_caret_box->SerializeRange(font_index, ucs, mn, mx, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
					return string(ucs.GetBuffer(), ucs.Length(), Encoding::UTF32);
				} else return L"";
			}
			void RichEdit::ScrollToCaret(void) { _deferred_scroll = true; }
			void RichEdit::SetContextMenu(Menus::Menu * menu) { _menu.SetRetain(menu); }
			Menus::Menu * RichEdit::GetContextMenu(void) { return _menu; }
			void RichEdit::SetHook(IRichEditHook * hook) { _hook = hook; }
			RichEdit::IRichEditHook * RichEdit::GetHook(void) { return _hook; }
			void RichEdit::ClearFontCollection(void) { _font_faces.Clear(); _fonts.Clear(); }
			int RichEdit::RegisterFontFace(const string & face)
			{
				for (int i = 0; i < _font_faces.Length(); i++) if (_font_faces[i] == face) return i;
				_font_faces << face; return _font_faces.Length() - 1;
			}
			int RichEdit::GetFontFaceCount(void) { return _font_faces.Length(); }
			const string & RichEdit::GetFontFaceWithIndex(int index) { return _font_faces[index]; }
			int RichEdit::RegisterFont(int face, int height, int attributes)
			{
				for (int i = 0; i < _fonts.Length(); i++) {
					if (_fonts[i].face == face && _fonts[i].height == height && _fonts[i].attr == attributes) return i;
				}
				_fonts << FontEntry{};
				_fonts.LastElement().face = face;
				_fonts.LastElement().height = height;
				_fonts.LastElement().attr = attributes;
				try { RecreateCachedFont(_fonts.Length() - 1); } catch (...) { _fonts.RemoveLast(); return -1; }
				return _fonts.Length() - 1;
			}
			void RichEdit::RecreateCachedFont(int index)
			{
				auto device = GetStation()->GetRenderingDevice();
				if (!device) return;
				int weight = (_fonts[index].attr & 1) ? 700 : 400;
				bool italic = (_fonts[index].attr & 2);
				bool underline = (_fonts[index].attr & 4);
				bool strikeout = (_fonts[index].attr & 8);
				_fonts[index].font = device->LoadFont(_font_faces[_fonts[index].face],
					int(_fonts[index].height * Zoom), weight, italic, underline, strikeout);
			}
			IFont * RichEdit::GetCachedFont(int index) { if (index < 0 || index >= _fonts.Length()) return 0; return _fonts[index].font; }
			void RichEdit::RealignContents(void) { _align_contents(WindowPosition); }
			bool RichEdit::IsSelectionEmpty(void) { return !_caret_box || _caret_pos == _selection_pos; }
			void RichEdit::Print(const string & text)
			{
				if (!IsSelectionEmpty() && _caret_box) _caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
				if (!_caret_box) { _caret_box = _selection_box = _root; _caret_pos = _selection_pos = 0; }
				Array<uint32> ucs(0x100);
				ucs.SetLength(text.GetEncodedLength(Encoding::UTF32));
				text.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
				_caret_box->InsertUnattributedText(ucs, _caret_pos, &_caret_box, &_caret_pos);
				_selection_box = _caret_box;
				_selection_pos = _caret_pos;
				if (_hook) _hook->CaretPositionChanged(this);
				GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			void RichEdit::PrintAttributed(const string & text)
			{
				if (!IsSelectionEmpty() && _caret_box) _caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
				if (!_caret_box) { _caret_box = _selection_box = _root; _caret_pos = _selection_pos = 0; }
				Array<uint32> ucs(0x100);
				ucs.SetLength(text.GetEncodedLength(Encoding::UTF32));
				text.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
				_caret_box->InsertText(ucs, _caret_pos, &_caret_box, &_caret_pos);
				_selection_box = _caret_box;
				_selection_pos = _caret_pos;
				if (_hook) _hook->CaretPositionChanged(this);
				GetParent()->RaiseEvent(ID, Event::ValueChange, this);
			}
			void RichEdit::SetSelectedTextStyle(int attributes_new, int attributes_mask)
			{
				if (_caret_box) {
					_undo.PushCurrentVersion(); _save = true;
					if (_caret_pos != _selection_pos) {
						_caret_box->SetAttributesForRange(min(_caret_pos, _selection_pos), max(_caret_pos, _selection_pos),
							uint64(attributes_new) << 32, 0, (uint64(attributes_mask) << 32) & RichEditContents::MaskStyleAttributes, 0);
					} else if (_caret_pos < _caret_box->GetMaximalPosition()) {
						_caret_box->SetAttributesForRange(_caret_pos, _caret_pos + 1,
							uint64(attributes_new) << 32, 0, (uint64(attributes_mask) << 32) & RichEditContents::MaskStyleAttributes, 0);
					}
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			int RichEdit::GetSelectedTextStyle(void)
			{
				if (_caret_box) {
					uint64 attr_lo, attr_hi;
					if (_caret_box->GetBoxType() == 1) {
						static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(min(_caret_pos, _selection_pos), attr_lo, attr_hi);
					} else {
						_caret_box->GetParentBox()->GetOuterAttributes(_caret_box, attr_lo, attr_hi);
					}
					return int((attr_lo & RichEditContents::MaskStyleAttributes) >> 32);
				} else return 0;
			}
			void RichEdit::SetSelectedTextAlignment(TextAlignment alignment)
			{
				if (_caret_box) {
					_undo.PushCurrentVersion(); _save = true;
					uint64 align = 0;
					if (alignment == TextAlignment::Left) align = RichEditContents::AlignmentLeft;
					else if (alignment == TextAlignment::Right) align = RichEditContents::AlignmentRight;
					else if (alignment == TextAlignment::Center) align = RichEditContents::AlignmentCenter;
					else if (alignment == TextAlignment::Stretch) align = RichEditContents::AlignmentStretch;
					if (_caret_pos != _selection_pos) {
						_caret_box->SetAttributesForRange(min(_caret_pos, _selection_pos), max(_caret_pos, _selection_pos),
							align, 0, RichEditContents::MaskAlignment, 0);
					} else if (_caret_pos < _caret_box->GetMaximalPosition()) {
						_caret_box->SetAttributesForRange(_caret_pos, _caret_pos + 1, align, 0, RichEditContents::MaskAlignment, 0);
					}
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			RichEdit::TextAlignment RichEdit::GetSelectedTextAlignment(void)
			{
				if (_caret_box) {
					uint64 attr_lo, attr_hi;
					if (_caret_box->GetBoxType() == 1) {
						static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(min(_caret_pos, _selection_pos), attr_lo, attr_hi);
					} else {
						_caret_box->GetParentBox()->GetOuterAttributes(_caret_box, attr_lo, attr_hi);
					}
					attr_lo &= RichEditContents::MaskAlignment;
					if (attr_lo == RichEditContents::AlignmentLeft) return RichEdit::TextAlignment::Left;
					else if (attr_lo == RichEditContents::AlignmentRight) return RichEdit::TextAlignment::Right;
					else if (attr_lo == RichEditContents::AlignmentCenter) return RichEdit::TextAlignment::Center;
					else return RichEdit::TextAlignment::Stretch;
				} else return RichEdit::TextAlignment::Left;
			}
			void RichEdit::SetSelectedTextFontFamily(const string & family)
			{
				if (_caret_box) {
					_undo.PushCurrentVersion(); _save = true;
					uint64 ff = uint64(RegisterFontFace(family)) << 40;
					if (_caret_pos != _selection_pos) {
						_caret_box->SetAttributesForRange(min(_caret_pos, _selection_pos), max(_caret_pos, _selection_pos),
							ff, 0, RichEditContents::MaskFontFamily, 0);
					} else if (_caret_pos < _caret_box->GetMaximalPosition()) {
						_caret_box->SetAttributesForRange(_caret_pos, _caret_pos + 1, ff, 0, RichEditContents::MaskFontFamily, 0);
					}
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			string RichEdit::GetSelectedTextFontFamily(void)
			{
				if (_caret_box) {
					uint64 attr_lo, attr_hi;
					if (_caret_box->GetBoxType() == 1) {
						static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(min(_caret_pos, _selection_pos), attr_lo, attr_hi);
					} else {
						_caret_box->GetParentBox()->GetOuterAttributes(_caret_box, attr_lo, attr_hi);
					}
					attr_lo &= RichEditContents::MaskFontFamily;
					attr_lo >>= 40;
					return _font_faces[int(attr_lo)];
				} else return DefaultFontFace;
			}
			void RichEdit::SetSelectedTextHeight(int height)
			{
				if (_caret_box) {
					_undo.PushCurrentVersion(); _save = true;
					uint64 fh = uint64(height) << 48;
					if (_caret_pos != _selection_pos) {
						_caret_box->SetAttributesForRange(min(_caret_pos, _selection_pos), max(_caret_pos, _selection_pos),
							fh, 0, RichEditContents::MaskFontHeight, 0);
					} else if (_caret_pos < _caret_box->GetMaximalPosition()) {
						_caret_box->SetAttributesForRange(_caret_pos, _caret_pos + 1, fh, 0, RichEditContents::MaskFontHeight, 0);
					}
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			int RichEdit::GetSelectedTextHeight(void)
			{
				if (_caret_box) {
					uint64 attr_lo, attr_hi;
					if (_caret_box->GetBoxType() == 1) {
						static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(min(_caret_pos, _selection_pos), attr_lo, attr_hi);
					} else {
						_caret_box->GetParentBox()->GetOuterAttributes(_caret_box, attr_lo, attr_hi);
					}
					attr_lo &= RichEditContents::MaskFontHeight;
					attr_lo >>= 48;
					return int(attr_lo);
				} else return DefaultFontHeight;
			}
			void RichEdit::SetSelectedTextColor(Color color)
			{
				if (_caret_box) {
					_undo.PushCurrentVersion(); _save = true;
					if (_caret_pos != _selection_pos) {
						_caret_box->SetAttributesForRange(min(_caret_pos, _selection_pos), max(_caret_pos, _selection_pos),
							0, color.Value, 0, RichEditContents::MaskColor);
					} else if (_caret_pos < _caret_box->GetMaximalPosition()) {
						_caret_box->SetAttributesForRange(_caret_pos, _caret_pos + 1, 0, color.Value, 0, RichEditContents::MaskColor);
					}
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			Color RichEdit::GetSelectedTextColor(void)
			{
				if (_caret_box) {
					uint64 attr_lo, attr_hi;
					if (_caret_box->GetBoxType() == 1) {
						static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(min(_caret_pos, _selection_pos), attr_lo, attr_hi);
					} else {
						_caret_box->GetParentBox()->GetOuterAttributes(_caret_box, attr_lo, attr_hi);
					}
					attr_hi &= RichEditContents::MaskColor;
					return Color(uint32(attr_hi));
				} else return DefaultTextColor;
			}
			void RichEdit::InsertImage(Codec::Image * image)
			{
				auto best_frame = image->GetFrameBestDpiFit(Zoom);
				uint width = uint(best_frame->GetWidth() / Zoom);
				uint height = uint(best_frame->GetHeight() / Zoom);
				Streaming::MemoryStream stream(0x10000);
				Codec::EncodeImage(&stream, image, L"EIWV");
				string code = L"\33p" + string(width, HexadecimalBase, 4) + string(height, HexadecimalBase, 4) +
					ConvertToBase64(stream.GetBuffer(), int(stream.Length())) + L"\33e";
				_undo.PushCurrentVersion();
				_save = true;
				if (!IsSelectionEmpty() && _caret_box) _caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
				if (!_caret_box) { _caret_box = _selection_box = _root; _caret_pos = _selection_pos = 0; }
				Array<uint32> ucs(0x100);
				ucs.SetLength(code.GetEncodedLength(Encoding::UTF32));
				code.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
				_caret_box->InsertText(ucs, _caret_pos, &_caret_box, &_caret_pos);
				_selection_box = _caret_box;
				_selection_pos = _caret_pos;
				if (_hook) _hook->CaretPositionChanged(this);
				GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				_deferred_scroll = true;
			}
			bool RichEdit::IsImageSelected(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (mx == mn + 1) {
						auto box = _caret_box->GetChildBox(mn);
						if (box && box->GetBoxType() == 2) return true;
						else return false;
					} else return false;
				} else return false;
			}
			void RichEdit::SetSelectedImage(Codec::Image * image)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (mx == mn + 1) {
						auto box = _caret_box->GetChildBox(mn);
						if (box && box->GetBoxType() == 2) {
							_undo.PushCurrentVersion();
							_save = true;
							static_cast<RichEditContents::PictureContents *>(box)->SetImage(image);
							GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						}
					}
				}
			}
			Codec::Image * RichEdit::GetSelectedImage(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (mx == mn + 1) {
						auto box = _caret_box->GetChildBox(mn);
						if (box && box->GetBoxType() == 2) {
							return static_cast<RichEditContents::PictureContents *>(box)->GetImage();
						}
					}
				}
				return 0;
			}
			void RichEdit::SetSelectedImageSize(int width, int height)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (mx == mn + 1) {
						auto box = _caret_box->GetChildBox(mn);
						if (box && box->GetBoxType() == 2) {
							_undo.PushCurrentVersion();
							_save = true;
							static_cast<RichEditContents::PictureContents *>(box)->SetSize(width, height);
							GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						}
					}
				}
			}
			int RichEdit::GetSelectedImageWidth(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (mx == mn + 1) {
						auto box = _caret_box->GetChildBox(mn);
						if (box && box->GetBoxType() == 2) {
							return static_cast<RichEditContents::PictureContents *>(box)->GetWidth();
						}
					}
				}
				return 0;
			}
			int RichEdit::GetSelectedImageHeight(void)
			{
				if (_caret_box && _caret_pos != _selection_pos) {
					int mn = min(_caret_pos, _selection_pos);
					int mx = max(_caret_pos, _selection_pos);
					if (mx == mn + 1) {
						auto box = _caret_box->GetChildBox(mn);
						if (box && box->GetBoxType() == 2) {
							return static_cast<RichEditContents::PictureContents *>(box)->GetHeight();
						}
					}
				}
				return 0;
			}
			bool RichEdit::CanCreateLink(void)
			{
				if (!_caret_box) return false;
				if (_caret_box->GetBoxType() != 1) return false;
				if (ContentBox::GetParentBoxOfType(_caret_box, 3)) return false;
				if (_caret_pos == _selection_pos) return false;
				auto box = static_cast<RichEditContents::TextContents *>(_caret_box);
				auto mn = min(_caret_pos, _selection_pos);
				auto mx = max(_caret_pos, _selection_pos);
				return box->CanCreateLinkFromRange(mn, mx);
			}
			void RichEdit::TransformSelectionIntoLink(const string & resource)
			{
				if (CanCreateLink()) {
					auto selection = SerializeSelectionAttributed();
					_undo.PushCurrentVersion();
					_save = true;
					uint64 attr_lo, attr_hi;
					auto mn = min(_caret_pos, _selection_pos);
					auto mx = max(_caret_pos, _selection_pos);
					static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(mn, attr_lo, attr_hi);
					_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					auto link_code = L"\33l" + resource + L"\33" + selection + L"\33e";
					Array<uint32> ucs(0x100);
					ucs.SetLength(link_code.GetEncodedLength(Encoding::UTF32));
					link_code.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
					_caret_box->InsertText(ucs, _caret_pos, &_caret_box, &_caret_pos);
					_caret_box->SetAttributesForRange(mn, mn + 1, attr_lo, attr_hi, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					_deferred_scroll = true;
				}
			}
			bool RichEdit::IsLinkSelected(void) { return _caret_box && ContentBox::GetParentBoxOfType(_caret_box, 3); }
			void RichEdit::DetransformSelectedLink(void)
			{
				ContentBox * box = 0;
				if (_caret_box && (box = ContentBox::GetParentBoxOfType(_caret_box, 3))) {
					_undo.PushCurrentVersion();
					_save = true;
					auto lnk = static_cast<RichEditContents::LinkContents *>(box);
					auto super = lnk->GetParentBox();
					int ext_pos = super->GetChildPosition(lnk);
					uint64 attr_lo, attr_hi;
					static_cast<RichEditContents::TextContents *>(lnk->GetParentBox())->GetAttributesForPosition(ext_pos, attr_lo, attr_hi);
					Array<uint32> ucs(0x1000);
					Array<int> font_index(0x10);
					_make_font_index(font_index, lnk, 0, 1);
					lnk->SerializeRange(font_index, ucs, 0, 1, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
					_caret_box = _selection_box = super;
					_caret_pos = ext_pos + 1; _selection_pos = ext_pos;
					_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					_caret_box->InsertText(ucs, ext_pos, &_caret_box, &_caret_pos);
					_caret_box->SetAttributesForRange(_selection_pos, _caret_pos, attr_lo, attr_hi, RichEditContents::StyleUnderline, RichEditContents::MaskColor);
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					_deferred_scroll = true;
				}
			}
			void RichEdit::SetSelectedLinkResource(const string & resource)
			{
				ContentBox * box = 0;
				if (_caret_box && (box = ContentBox::GetParentBoxOfType(_caret_box, 3))) {
					_undo.PushCurrentVersion();
					_save = true;
					static_cast<RichEditContents::LinkContents *>(box)->GetResource() = resource;
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			string RichEdit::GetSelectedLinkResource(void)
			{
				ContentBox * box = 0;
				if (_caret_box && (box = ContentBox::GetParentBoxOfType(_caret_box, 3))) {
					return static_cast<RichEditContents::LinkContents *>(box)->GetResource();
				}
				return L"";
			}
			bool RichEdit::CanInsertTable(void)
			{
				if (!_caret_box) return false;
				if (_caret_box->GetBoxType() != 1) return false;
				if (ContentBox::GetParentBoxOfType(_caret_box, 3)) return false;
				return true;
			}
			void RichEdit::InsertTable(Point size)
			{
				if (CanInsertTable() && size.x > 0 && size.y > 0) {
					_undo.PushCurrentVersion();
					_save = true;
					uint64 attr_lo, attr_hi;
					auto mn = min(_caret_pos, _selection_pos);
					auto mx = max(_caret_pos, _selection_pos);
					static_cast<RichEditContents::TextContents *>(_caret_box)->GetAttributesForPosition(mn, attr_lo, attr_hi);
					_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
					DynamicString dynamic;
					dynamic << L"\33t" << string(uint(size.x), HexadecimalBase, 4) << string(uint(size.y), HexadecimalBase, 4);
					dynamic << L"010101" << string(uint(DefaultTextColor), HexadecimalBase, 8);
					for (int i = 0; i < size.x; i++) dynamic << L"0064";
					for (int i = 0; i < size.x * size.y; i++) dynamic << L"000000001\33e";
					dynamic << L"\33e";
					auto code = dynamic.ToString();
					Array<uint32> ucs(0x100);
					ucs.SetLength(code.GetEncodedLength(Encoding::UTF32));
					code.Encode(ucs.GetBuffer(), Encoding::UTF32, false);
					_caret_box->InsertText(ucs, _caret_pos, &_caret_box, &_caret_pos);
					_selection_box = _caret_box; _selection_pos = _caret_pos;
					_caret_box->SetAttributesForRange(mn, mn + 1, attr_lo, attr_hi, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
					if (_hook) _hook->CaretPositionChanged(this);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
					_deferred_scroll = true;
				}
			}
			bool RichEdit::IsTableSelected(void) { return _caret_box && ContentBox::GetParentBoxOfType(_caret_box, 4); }
			Point RichEdit::GetSelectedTableCell(void)
			{
				ContentBox * table, * cell;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4, &cell))) {
					int size = static_cast<RichEditContents::TableContents *>(table)->_size_x;
					int pos = cell ? table->GetChildPosition(cell) : max(_caret_pos, table->GetMaximalPosition() - 1);
					return Point(pos % size, pos / size);
				}
				return Point(-1, -1);
			}
			Point RichEdit::GetSelectedTableSize(void)
			{
				ContentBox * table, * cell;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4, &cell))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return Point(tab->_size_x, tab->_size_y);
				}
				return Point(-1, -1);
			}
			void RichEdit::InsertSelectedTableRow(int at)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					if (at >= 0 && at <= tab->_size_y) {
						_undo.PushCurrentVersion();
						_save = true;
						int index_at = at * tab->_size_x;
						for (int i = 0; i < tab->_size_x; i++) {
							SafePointer<ContentBox> cell = new RichEditContents::TextContents(this, tab);
							tab->_cells.Insert(cell, i + at * tab->_size_x);
							tab->_cell_colors.Insert(0, i + at * tab->_size_x);
							tab->_cell_aligns.Insert(0, i + at * tab->_size_x);
						}
						tab->_size_y++;
						tab->ResetCache();
						tab->ChildContentsChanged(tab);
						_caret_box = _selection_box = tab->_cells.ElementAt(index_at);
						_caret_pos = _selection_pos = 0;
						if (_hook) _hook->CaretPositionChanged(this);
						GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						_deferred_scroll = true;
					}
				}
			}
			void RichEdit::InsertSelectedTableColumn(int at)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					if (at >= 0 && at <= tab->_size_x) {
						_undo.PushCurrentVersion();
						_save = true;
						int index_at = at;
						tab->_col_widths.Insert(100, at);
						for (int j = 0; j < tab->_size_y; j++) {
							SafePointer<ContentBox> cell = new RichEditContents::TextContents(this, tab);
							tab->_cells.Insert(cell, at + j * tab->_size_x + j);
							tab->_cell_colors.Insert(0, at + j * tab->_size_x + j);
							tab->_cell_aligns.Insert(0, at + j * tab->_size_x + j);
						}
						tab->_size_x++;
						tab->ResetCache();
						tab->ChildContentsChanged(tab);
						_caret_box = _selection_box = tab->_cells.ElementAt(index_at);
						_caret_pos = _selection_pos = 0;
						if (_hook) _hook->CaretPositionChanged(this);
						GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						_deferred_scroll = true;
					}
				}
			}
			void RichEdit::RemoveSelectedTableRow(int index)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					if (index >= 0 && index < tab->_size_y) {
						_undo.PushCurrentVersion();
						_save = true;
						if (tab->_size_y == 1) {
							_caret_pos = tab->_parent->GetChildPosition(tab);
							_selection_pos = _caret_pos + 1;
							_caret_box = _selection_box = tab->_parent;
							_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
						} else {
							for (int i = tab->_size_x - 1; i >= 0; i--) {
								tab->_cells.Remove(i + index * tab->_size_x);
								tab->_cell_colors.Remove(i + index * tab->_size_x);
								tab->_cell_aligns.Remove(i + index * tab->_size_x);
							}
							tab->_size_y--;
							tab->ResetCache();
							tab->ChildContentsChanged(tab);
							_caret_box = _selection_box = tab;
							_caret_pos = _selection_pos = 0;
						}
						if (_hook) _hook->CaretPositionChanged(this);
						GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						_deferred_scroll = true;
					}
				}
			}
			void RichEdit::RemoveSelectedTableColumn(int index)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					if (index >= 0 && index < tab->_size_x) {
						_undo.PushCurrentVersion();
						_save = true;
						if (tab->_size_x == 1) {
							_caret_pos = tab->_parent->GetChildPosition(tab);
							_selection_pos = _caret_pos + 1;
							_caret_box = _selection_box = tab->_parent;
							_caret_box->ClearSelection(&_caret_pos, &_caret_box, &_selection_pos, &_selection_box);
						} else {
							tab->_col_widths.Remove(index);
							for (int j = tab->_size_y - 1; j >= 0; j--) {
								tab->_cells.Remove(index + j * tab->_size_x);
								tab->_cell_colors.Remove(index + j * tab->_size_x);
								tab->_cell_aligns.Remove(index + j * tab->_size_x);
							}
							tab->_size_x--;
							tab->ResetCache();
							tab->ChildContentsChanged(tab);
							_caret_box = _selection_box = tab;
							_caret_pos = _selection_pos = 0;
						}
						if (_hook) _hook->CaretPositionChanged(this);
						GetParent()->RaiseEvent(ID, Event::ValueChange, this);
						_deferred_scroll = true;
					}
				}
			}
			void RichEdit::SetSelectedTableColumnWidth(int width)
			{
				ContentBox * table, * cell;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4, &cell))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					int from, to;
					if (cell && cell->GetBoxType() != 4) {
						from = table->GetChildPosition(cell);
						to = from + 1;
					} else {
						from = min(_caret_pos, _selection_pos);
						to = max(_caret_pos, _selection_pos);
					}
					for (int i = from; i < to; i++) tab->_col_widths[i % tab->_size_x] = width;
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			void RichEdit::AdjustSelectedTableColumnWidth(void)
			{
				ContentBox * table, * cell;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4, &cell))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					int from, to;
					if (cell && cell->GetBoxType() != 4) {
						from = table->GetChildPosition(cell);
						to = from + 1;
					} else {
						from = min(_caret_pos, _selection_pos);
						to = max(_caret_pos, _selection_pos);
					}
					for (int i = from; i < to; i++) {
						int col = i % tab->_size_x;
						int width = 0;
						for (int j = 0; j < tab->_size_y; j++) {
							int lw = tab->_cells[col + j * tab->_size_x].GetContentsWidth();
							if (lw > width) width = lw;
						}
						tab->_col_widths[col] = int(width / Zoom) + 1;
					}
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			int RichEdit::GetSelectedTableColumnWidth(int index)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return tab->_col_widths[index % tab->_size_x];
				}
				return 0;
			}
			void RichEdit::SetSelectedTableBorderColor(Color color)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					tab->_border_c = color;
					tab->ResetCache();
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			Color RichEdit::GetSelectedTableBorderColor(void)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return tab->_border_c;
				}
				return 0;
			}
			void RichEdit::SetSelectedTableCellBackground(Color color)
			{
				ContentBox * table, * cell;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4, &cell))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					int from, to;
					if (cell && cell->GetBoxType() != 4) {
						from = table->GetChildPosition(cell);
						to = from + 1;
					} else {
						from = min(_caret_pos, _selection_pos);
						to = max(_caret_pos, _selection_pos);
					}
					for (int i = from; i < to; i++) tab->_cell_colors[i] = color;
					tab->ResetCache();
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			Color RichEdit::GetSelectedTableCellBackground(Point cell)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return tab->_cell_colors[cell.x + cell.y * tab->_size_x];
				}
				return 0;
			}
			void RichEdit::SetSelectedTableCellVerticalAlignment(TextVerticalAlignment align)
			{
				ContentBox * table, * cell;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4, &cell))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					int from, to;
					if (cell && cell->GetBoxType() != 4) {
						from = table->GetChildPosition(cell);
						to = from + 1;
					} else {
						from = min(_caret_pos, _selection_pos);
						to = max(_caret_pos, _selection_pos);
					}
					int alg = 0;
					if (align == RichEdit::TextVerticalAlignment::Top) alg = 0;
					else if (align == RichEdit::TextVerticalAlignment::Center) alg = 1;
					else if (align == RichEdit::TextVerticalAlignment::Bottom) alg = 2;
					for (int i = from; i < to; i++) tab->_cell_aligns[i] = alg;
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			RichEdit::TextVerticalAlignment RichEdit::GetSelectedTableCellVerticalAlignment(Point cell)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					auto align = tab->_cell_aligns[cell.x + cell.y * tab->_size_x];
					if (align == 1) return RichEdit::TextVerticalAlignment::Center;
					else if (align == 2) return RichEdit::TextVerticalAlignment::Bottom;
					else return RichEdit::TextVerticalAlignment::Top;
				}
				return RichEdit::TextVerticalAlignment::Top;
			}
			void RichEdit::SetSelectedTableBorderWidth(int width)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					tab->_border_w = width;
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			int RichEdit::GetSelectedTableBorderWidth(void)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return tab->_border_w;
				}
				return 0;
			}
			void RichEdit::SetSelectedTableVerticalBorderWidth(int width)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					tab->_border_v = width;
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			int RichEdit::GetSelectedTableVerticalBorderWidth(void)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return tab->_border_v;
				}
				return 0;
			}
			void RichEdit::SetSelectedTableHorizontalBorderWidth(int width)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					tab->_border_h = width;
					tab->ChildContentsChanged(tab);
					GetParent()->RaiseEvent(ID, Event::ValueChange, this);
				}
			}
			int RichEdit::GetSelectedTableHorizontalBorderWidth(void)
			{
				ContentBox * table;
				if (_caret_box && (table = ContentBox::GetParentBoxOfType(_caret_box, 4))) {
					auto tab = static_cast<RichEditContents::TableContents *>(table);
					return tab->_border_h;
				}
				return 0;
			}
			void RichEdit::IRichEditHook::InitializeContextMenu(Menus::Menu * menu, RichEdit * sender) {}
			void RichEdit::IRichEditHook::LinkPressed(const string & resource, RichEdit * sender) {}
			void RichEdit::IRichEditHook::CaretPositionChanged(RichEdit * sender) {}
			void RichEdit::ContentBox::UnifyBoxForSelection(int * pos1, ContentBox ** box1, int * pos2, ContentBox ** box2)
			{
				if (*box1 == *box2) return;
				ContentBox * box;
				Array<ContentBox *> first_box_chain(0x10);
				Array<ContentBox *> second_box_chain(0x10);
				box = *box1; while (box) { first_box_chain << box; box = box->GetParentBox(); }
				box = *box2; while (box) { second_box_chain << box; box = box->GetParentBox(); }
				int level = 1;
				int first_depth = first_box_chain.Length(), second_depth = second_box_chain.Length();
				int min_depth = min(first_depth, second_depth);
				while (level < min_depth && first_box_chain[first_depth - level - 1] == second_box_chain[second_depth - level - 1]) level++;
				ContentBox * common = first_box_chain[first_depth - level];
				int p1_mn, p1_mx, p2_mn, p2_mx;
				if (*box1 != common) {
					p1_mn = common->GetChildPosition(first_box_chain[first_depth - level - 1]);
					p1_mx = p1_mn + 1;
				} else { p1_mn = p1_mx = *pos1; }
				if (*box2 != common) {
					p2_mn = common->GetChildPosition(second_box_chain[second_depth - level - 1]);
					p2_mx = p2_mn + 1;
				} else { p2_mn = p2_mx = *pos2; }
				*box1 = *box2 = common;
				if (p1_mn < p2_mn || p1_mx < p2_mx) { *pos1 = p1_mn; *pos2 = p2_mx; } else { *pos1 = p1_mx; *pos2 = p2_mn; }
			}
			RichEdit::ContentBox * RichEdit::ContentBox::GetParentBoxOfType(ContentBox * box_for, int type)
			{
				ContentBox * current = box_for;
				while (current) {
					if (current->GetBoxType() == type) return current;
					current = current->GetParentBox();
				}
				return 0;
			}
			RichEdit::ContentBox * RichEdit::ContentBox::GetParentBoxOfType(ContentBox * box_for, int type, ContentBox ** first_child)
			{
				ContentBox * current = box_for;
				ContentBox * last = 0;
				while (current) {
					if (current->GetBoxType() == type) {
						*first_child = last;
						return current;
					}
					last = current;
					current = current->GetParentBox();
				}
				*first_child = 0;
				return 0;
			}
			void RichEdit::ContentBox::MakeFontIndex(RichEdit * edit, Array<int> & index, ContentBox * root, int from, int to) { edit->_make_font_index(index, root, from, to); }
			RichEdit::UndoBufferCopy::UndoBufferCopy(void) : caret_box_path(0x10), caret_pos(0), selection_pos(0), text(0x1000) {}
			bool operator==(const RichEdit::UndoBufferCopy & a, const RichEdit::UndoBufferCopy & b)
			{
				return a.text.Length() == b.text.Length() &&
					MemoryCompare(a.text.GetBuffer(), b.text.GetBuffer(), a.text.Length() * sizeof(uint32)) == 0;
			}
			bool operator!=(const RichEdit::UndoBufferCopy & a, const RichEdit::UndoBufferCopy & b)
			{
				return a.text.Length() != b.text.Length() ||
					MemoryCompare(a.text.GetBuffer(), b.text.GetBuffer(), a.text.Length() * sizeof(uint32)) != 0;
			}
			RichEdit::UndoBufferInterface::UndoBufferInterface(RichEdit * edit) : _edit(edit) {}
			RichEdit::UndoBufferInterface::~UndoBufferInterface(void) {}
			RichEdit::UndoBufferCopy RichEdit::UndoBufferInterface::GetCurrentValue(void)
			{
				UndoBufferCopy result;
				result.caret_pos = _edit->_caret_pos;
				result.selection_pos = _edit->_selection_pos;
				ContentBox * box = _edit->_caret_box;
				while (box && box->GetParentBox()) {
					auto parent = box->GetParentBox();
					result.caret_box_path << parent->GetChildPosition(box);
					box = parent;
				}
				Array<int> font_index(0x10);
				_edit->_make_font_index(font_index, _edit->_root, -1, -1);
				_edit->_root->SerializeToPlainText(font_index, result.text, 0xFFFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF);
				return result;
			}
			void RichEdit::UndoBufferInterface::SetCurrentValue(const UndoBufferCopy & value)
			{
				Array<int> font_index(0x10);
				int pos = 0;
				_edit->_font_faces.Clear();
				_edit->_fonts.Clear();
				_edit->_root->DeserializeFromPlainText(font_index, value.text, pos, 0, 0);
				_edit->_caret_box = _edit->_root;
				for (int i = value.caret_box_path.Length() - 1; i >= 0; i--) {
					auto next = _edit->_caret_box->GetChildBox(value.caret_box_path[i]);
					if (!next) break;
					_edit->_caret_box = next;
				}
				_edit->_selection_box = _edit->_caret_box;
				_edit->_caret_pos = value.caret_pos;
				_edit->_selection_pos = value.selection_pos;
				_edit->_hot_box = 0;
				_edit->_state = 0;
				_edit->_save = true;
				_edit->_align_contents(_edit->WindowPosition);
				_edit->ScrollToCaret();
			}
		}
	}
}