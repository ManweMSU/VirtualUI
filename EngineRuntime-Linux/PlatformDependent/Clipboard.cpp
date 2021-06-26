#include "ClipboardAPI.h"

namespace Engine
{
	namespace X11
	{
		bool _clipboard_data_set = false;
		Clipboard::Format _clipboard_format;
		string _clipboard_string, _clipboard_string_attr, _clipboard_custom_subtype;
		SafePointer<Codec::Frame> _clipboard_frame;
		SafePointer<DataBlock> _clipboard_custom_block;
		Atom _clipboard_atom_main = 0;
		Atom _clipboard_atom_incremental = 0;
		Atom _clipboard_atom_atom = 0;
		Atom _clipboard_atom_targets = 0;
		Atom _clipboard_atom_text = 0;
		Atom _clipboard_atom_text_utf8 = 0;
		Atom _clipboard_atom_pixmap = 0;
		Atom _clipboard_atom_rich_text = 0;
		Atom _clipboard_atom_engine_data = 0;
		Atom _clipboard_atom_engine_data_subtype = 0;
		int _clipboard_error = 0;
	}
	namespace X11
	{
		int ClipboardErrorHanlder(Display * display, XErrorEvent * error)
		{
			_clipboard_error = error->error_code;
			return 0;
		}
		void ClipboardInitAtoms(Display * display)
		{
			if (!X11::_clipboard_atom_main) {
				X11::_clipboard_atom_main = XInternAtom(display, "CLIPBOARD", false);
				Codec::InitializeDefaultCodecs();
			}
			if (!X11::_clipboard_atom_incremental) {
				X11::_clipboard_atom_incremental = XInternAtom(display, "INCR", false);
			}
			if (!X11::_clipboard_atom_atom) {
				X11::_clipboard_atom_atom = XInternAtom(display, "ATOM", false);
			}
			if (!X11::_clipboard_atom_targets) {
				X11::_clipboard_atom_targets = XInternAtom(display, "TARGETS", false);
			}
			if (!X11::_clipboard_atom_text) {
				X11::_clipboard_atom_text = XInternAtom(display, "STRING", false);
			}
			if (!X11::_clipboard_atom_text_utf8) {
				X11::_clipboard_atom_text_utf8 = XInternAtom(display, "UTF8_STRING", false);
			}
			if (!X11::_clipboard_atom_pixmap) {
				X11::_clipboard_atom_pixmap = XInternAtom(display, "image/png", false);
			}
			if (!X11::_clipboard_atom_rich_text) {
				X11::_clipboard_atom_rich_text = XInternAtom(display, "EngineRuntime.RichText", false);
			}
			if (!X11::_clipboard_atom_engine_data) {
				X11::_clipboard_atom_engine_data = XInternAtom(display, "EngineRuntime.CustomData", false);
			}
			if (!X11::_clipboard_atom_engine_data_subtype) {
				X11::_clipboard_atom_engine_data_subtype = XInternAtom(display, "EngineRuntime.CustomData.Subtype", false);
			}
		}
		void ClipboardClear(void)
		{
			_clipboard_data_set = false;
			_clipboard_string = _clipboard_string_attr = _clipboard_custom_subtype = L"";
			_clipboard_frame.SetReference(0);
			_clipboard_custom_block.SetReference(0);
		}
		void ClipboardRespondNotAvailable(Display * display, XSelectionRequestEvent * req_event)
		{
			XEvent resp;
			XSelectionEvent & sel = resp.xselection;
			ZeroMemory(&resp, sizeof(resp));
			sel.type = SelectionNotify;
			sel.display = display;
			sel.requestor = req_event->requestor;
			sel.selection = req_event->selection;
			sel.target = req_event->target;
			sel.property = 0;
			sel.time = req_event->time;
			XSendEvent(display, req_event->requestor, false, 0, &resp);
			XFlush(display);
		}
		void ClipboardRespondWithAtoms(Display * display, XSelectionRequestEvent * req_event, const Array<Atom> & atoms)
		{
			if (!req_event->property) req_event->property = _clipboard_atom_main;
			XChangeProperty(display, req_event->requestor, req_event->property, _clipboard_atom_atom, 32, PropModeReplace,
				reinterpret_cast<const uint8 *>(atoms.GetBuffer()), atoms.Length());
			XEvent resp;
			XSelectionEvent & sel = resp.xselection;
			ZeroMemory(&resp, sizeof(resp));
			sel.type = SelectionNotify;
			sel.display = display;
			sel.requestor = req_event->requestor;
			sel.selection = req_event->selection;
			sel.target = req_event->target;
			sel.property = req_event->property;
			sel.time = req_event->time;
			XSendEvent(display, req_event->requestor, false, 0, &resp);
			XFlush(display);
		}
		void ClipboardRespondWithData(Display * display, XSelectionRequestEvent * req_event, const DataBlock & data)
		{
			if (!req_event->property) req_event->property = _clipboard_atom_main;
			_clipboard_error = 0;
			XSetErrorHandler(ClipboardErrorHanlder);
			XChangeProperty(display, req_event->requestor, req_event->property, req_event->target, 8, PropModeReplace, data.GetBuffer(), data.Length());
			XSync(display, false);
			XSetErrorHandler(0);
			if (_clipboard_error == BadAlloc) {
				auto ws = static_cast<IXWindowSystem *>(Windows::GetWindowSystem());
				int block_size = data.Length();
				XChangeProperty(display, req_event->requestor, req_event->property, _clipboard_atom_incremental, 8, PropModeReplace, reinterpret_cast<uint8 *>(&block_size), 4);
				XSync(display, false);
				ws->GetEventLoop()->DrainEvent(PropertyNotify);
				XSetWindowAttributes attr;
				attr.event_mask = PropertyChangeMask;
				XChangeWindowAttributes(display, req_event->requestor, CWEventMask, &attr);
				XEvent event;
				XSelectionEvent & sel = event.xselection;
				ZeroMemory(&event, sizeof(event));
				sel.type = SelectionNotify;
				sel.display = display;
				sel.requestor = req_event->requestor;
				sel.selection = req_event->selection;
				sel.target = req_event->target;
				sel.property = req_event->property;
				sel.time = req_event->time;
				XSendEvent(display, req_event->requestor, false, 0, &event);
				int blt_size = 0x100000;
				int current = 0;
				while (current < block_size) {
					while (true) {
						if (ws->GetEventLoop()->WaitForEvent(&event, PropertyNotify, 1000)) {
							if (event.xproperty.window == req_event->requestor && event.xproperty.state == PropertyDelete && event.xproperty.atom == req_event->property) break;
						} else return;
					}
					while (true) {
						int cblt = min(blt_size, data.Length() - current);
						_clipboard_error = 0;
						XSetErrorHandler(ClipboardErrorHanlder);
						XChangeProperty(display, req_event->requestor, req_event->property, req_event->target, 8, PropModeReplace, data.GetBuffer() + current, cblt);
						XSync(display, false);
						XSetErrorHandler(0);
						if (_clipboard_error == BadAlloc) {
							blt_size /= 2;
							if (!blt_size) return;
							continue;
						} else if (!_clipboard_error) {
							current += cblt;
							break;
						} else return;
					}
				}
				while (true) {
					if (ws->GetEventLoop()->WaitForEvent(&event, PropertyNotify, 1000)) {
						if (event.xproperty.window == req_event->requestor && event.xproperty.state == PropertyDelete && event.xproperty.atom == req_event->property) break;
					} else return;
				}
				attr.event_mask = NoEventMask;
				XChangeWindowAttributes(display, req_event->requestor, CWEventMask, &attr);
				XChangeProperty(display, req_event->requestor, req_event->property, req_event->target, 8, PropModeReplace, 0, 0);
				XFlush(display);
			} else if (_clipboard_error) {
				ClipboardRespondNotAvailable(display, req_event);
			} else {
				XEvent resp;
				XSelectionEvent & sel = resp.xselection;
				ZeroMemory(&resp, sizeof(resp));
				sel.type = SelectionNotify;
				sel.display = display;
				sel.requestor = req_event->requestor;
				sel.selection = req_event->selection;
				sel.target = req_event->target;
				sel.property = req_event->property;
				sel.time = req_event->time;
				XSendEvent(display, req_event->requestor, false, 0, &resp);
				XFlush(display);
			}
		}
		void ClipboardProcessRequestEvent(Display * display, XSelectionRequestEvent * req_event)
		{
			if (_clipboard_data_set && req_event->selection == _clipboard_atom_main) {
				try {
					if (req_event->target == _clipboard_atom_targets) {
						Array<Atom> atoms(0x10);
						atoms << _clipboard_atom_targets;
						if (_clipboard_format == Clipboard::Format::Text) {
							atoms << _clipboard_atom_text_utf8;
							atoms << _clipboard_atom_text;
						} else if (_clipboard_format == Clipboard::Format::Image) atoms << _clipboard_atom_pixmap;
						else if (_clipboard_format == Clipboard::Format::RichText) {
							atoms << _clipboard_atom_text_utf8;
							atoms << _clipboard_atom_text;
							atoms << _clipboard_atom_rich_text;
						} else if (_clipboard_format == Clipboard::Format::Custom) {
							atoms << _clipboard_atom_engine_data;
							atoms << _clipboard_atom_engine_data_subtype;
						}
						ClipboardRespondWithAtoms(display, req_event, atoms);
					} else if (req_event->target == _clipboard_atom_text || req_event->target == _clipboard_atom_text_utf8) {
						if (_clipboard_format == Clipboard::Format::Text || _clipboard_format == Clipboard::Format::RichText) {
							SafePointer<DataBlock> result = _clipboard_string.EncodeSequence(Encoding::UTF8, false);
							ClipboardRespondWithData(display, req_event, *result);
						} else ClipboardRespondNotAvailable(display, req_event);
					} else if (req_event->target == _clipboard_atom_pixmap) {
						if (_clipboard_format == Clipboard::Format::Image) {
							Streaming::MemoryStream stream(0x10000);
							Codec::EncodeFrame(&stream, _clipboard_frame, Codec::ImageFormatPNG);
							SafePointer<DataBlock> result = new DataBlock(1);
							result->Append(reinterpret_cast<uint8 *>(stream.GetBuffer()), int(stream.Length()));
							ClipboardRespondWithData(display, req_event, *result);
						} else ClipboardRespondNotAvailable(display, req_event);
					} else if (req_event->target == _clipboard_atom_rich_text) {
						if (_clipboard_format == Clipboard::Format::RichText) {
							SafePointer<DataBlock> result = _clipboard_string_attr.EncodeSequence(Encoding::UTF8, false);
							ClipboardRespondWithData(display, req_event, *result);
						} else ClipboardRespondNotAvailable(display, req_event);
					} else if (req_event->target == _clipboard_atom_engine_data) {
						if (_clipboard_format == Clipboard::Format::Custom) {
							ClipboardRespondWithData(display, req_event, *_clipboard_custom_block);
						} else ClipboardRespondNotAvailable(display, req_event);
					} else if (req_event->target == _clipboard_atom_engine_data_subtype) {
						if (_clipboard_format == Clipboard::Format::Custom) {
							SafePointer<DataBlock> result = _clipboard_custom_subtype.EncodeSequence(Encoding::UTF8, false);
							ClipboardRespondWithData(display, req_event, *result);
						} else ClipboardRespondNotAvailable(display, req_event);
					} else ClipboardRespondNotAvailable(display, req_event);
				} catch (...) { ClipboardRespondNotAvailable(display, req_event); }
			} else ClipboardRespondNotAvailable(display, req_event);
		}
		Atom ClipboardWaitRequest(IXWindowSystem * ws, Atom prop_name)
		{
			while (true) {
				XEvent event;
				if (ws->GetEventLoop()->WaitForEvent(&event, PropertyNotify, 1000)) {
					if (event.xproperty.atom != prop_name || event.xproperty.state != PropertyNewValue) continue;
					return event.xproperty.atom;
				} else return 0;
			}
		}
		Atom ClipboardSendRequest(IXWindowSystem * ws, Atom prop_type)
		{
			XEvent event;
			XConvertSelection(ws->GetXDisplay(), X11::_clipboard_atom_main, prop_type, X11::_clipboard_atom_main, ws->GetSystemUsageWindow(), CurrentTime);
			if (ws->GetEventLoop()->WaitForEvent(&event, SelectionNotify, 1000)) return event.xselection.property; else return 0;
		}
		DataBlock * ClipboardQueryProperty(IXWindowSystem * ws, Atom prop_type, Window owner, bool bulk = false)
		{
			auto prop = bulk ? ClipboardWaitRequest(ws, _clipboard_atom_main) : ClipboardSendRequest(ws, prop_type);
			if (prop) {
				SafePointer<DataBlock> data;
				uint8 * pdata = 0;
				Atom act_type;
				int act_format;
				unsigned long read, size;
				if (!bulk) ws->GetEventLoop()->DrainEvent(PropertyNotify);
				XGetWindowProperty(ws->GetXDisplay(), ws->GetSystemUsageWindow(), prop, 0, __LONG_MAX__, true, AnyPropertyType, &act_type, &act_format, &read, &size, &pdata);
				if (act_type == _clipboard_atom_atom) {
					try {
						data = new DataBlock(1);
						data->SetLength(read * sizeof(Atom));
						MemoryCopy(data->GetBuffer(), pdata, read * sizeof(Atom));
					} catch (...) {}
					XFree(pdata);
					if (data) data->Retain();
					return data;
				} else if (act_type == prop_type) {
					try {
						data = new DataBlock(1);
						data->SetLength(read);
						MemoryCopy(data->GetBuffer(), pdata, read);
					} catch (...) {}
					XFree(pdata);
					if (data) data->Retain();
					return data;
				} else if (!bulk && act_type == _clipboard_atom_incremental) {
					XFree(pdata);
					try { data = new DataBlock(1); } catch (...) { return 0; }
					while (true) {
						SafePointer<DataBlock> block = ClipboardQueryProperty(ws, prop_type, owner, true);
						if (!block) return 0;
						if (!block->Length()) break;
						try {
							int base = data->Length();
							data->SetLength(base + block->Length());
							MemoryCopy(data->GetBuffer() + base, block->GetBuffer(), block->Length());
						} catch (...) { return 0; }
					}
					data->Retain();
					return data;
				} else {
					XFree(pdata);
					return 0;
				}
			} else return 0;
		}
	}
	namespace Clipboard
	{
		bool IsFormatAvailable(Format format)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			auto owner = XGetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main);
			if (!owner) return false;
			if (owner == ws->GetSystemUsageWindow()) {
				if (X11::_clipboard_data_set && X11::_clipboard_format == format) return true;
				if (X11::_clipboard_data_set && X11::_clipboard_format == Format::RichText && format == Format::Text) return true;
				return false;
			} else {
				SafePointer<DataBlock> targets = X11::ClipboardQueryProperty(ws, X11::_clipboard_atom_targets, owner);
				Atom * atoms = reinterpret_cast<Atom *>(targets->GetBuffer());
				int num_atoms = targets->Length() / sizeof(Atom);
				for (int i = 0; i < num_atoms; i++) {
					auto a = atoms[i];
					if (a == X11::_clipboard_atom_text && format == Format::Text) return true;
					else if (a == X11::_clipboard_atom_pixmap && format == Format::Image) return true;
					else if (a == X11::_clipboard_atom_rich_text && format == Format::RichText) return true;
					else if (a == X11::_clipboard_atom_engine_data && format == Format::Custom) return true;
				}
				return false;
			}
		}
		bool GetData(string & value)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			auto owner = XGetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main);
			if (!owner) return false;
			if (owner == ws->GetSystemUsageWindow()) {
				if (X11::_clipboard_data_set && X11::_clipboard_format == Format::Text) {
					value = X11::_clipboard_string;
					return true;
				}
				if (X11::_clipboard_data_set && X11::_clipboard_format == Format::RichText) {
					value = X11::_clipboard_string;
					return true;
				}
				return false;
			} else {
				SafePointer<DataBlock> data = X11::ClipboardQueryProperty(ws, X11::_clipboard_atom_text, owner);
				if (data) {
					value = string(data->GetBuffer(), data->Length(), Encoding::UTF8);
					return true;
				} else return false;
			}
		}
		bool GetData(Codec::Frame ** value)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			auto owner = XGetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main);
			if (!owner) return false;
			if (owner == ws->GetSystemUsageWindow()) {
				if (X11::_clipboard_data_set && X11::_clipboard_format == Format::Image) {
					*value = new Codec::Frame(X11::_clipboard_frame);
					return true;
				}
				return false;
			} else {
				SafePointer<DataBlock> data = X11::ClipboardQueryProperty(ws, X11::_clipboard_atom_pixmap, owner);
				if (data) {
					SafePointer<Codec::Frame> frame;
					try {
						Streaming::MemoryStream stream(data->GetBuffer(), data->Length());
						frame = Codec::DecodeFrame(&stream);
						if (!frame) return false;
					} catch (...) { return false; }
					frame->Retain();
					*value = frame.Inner();
					return true;
				} else return false;
			}
		}
		bool GetData(string & value, bool attributed)
		{
			if (!attributed) return GetData(value);
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			auto owner = XGetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main);
			if (!owner) return false;
			if (owner == ws->GetSystemUsageWindow()) {
				if (X11::_clipboard_data_set && X11::_clipboard_format == Format::RichText) {
					value = X11::_clipboard_string_attr;
					return true;
				}
				return false;
			} else {
				SafePointer<DataBlock> data = X11::ClipboardQueryProperty(ws, X11::_clipboard_atom_rich_text, owner);
				if (data) {
					value = string(data->GetBuffer(), data->Length(), Encoding::UTF8);
					return true;
				} else return false;
			}
		}
		bool GetData(const string & subclass, Array<uint8> ** value)
		{
			if (GetCustomSubclass() == subclass) {
				auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
				if (!ws) return false;
				X11::ClipboardInitAtoms(ws->GetXDisplay());
				auto owner = XGetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main);
				if (!owner) return false;
				if (owner == ws->GetSystemUsageWindow()) {
					if (X11::_clipboard_data_set && X11::_clipboard_format == Format::Custom && X11::_clipboard_custom_subtype == subclass) {
						*value = new DataBlock(*X11::_clipboard_custom_block);
						return true;
					}
					return false;
				} else {
					SafePointer<DataBlock> data = X11::ClipboardQueryProperty(ws, X11::_clipboard_atom_engine_data, owner);
					if (data) {
						data->Retain();
						*value = data.Inner();
						return true;
					} else return false;
				}
			} else return false;
		}
		bool SetData(const string & value)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			bool prev_set = X11::_clipboard_data_set;
			try {
				X11::_clipboard_data_set = true;
				X11::_clipboard_format = Format::Text;
				X11::_clipboard_string = value;
			} catch (...) { X11::_clipboard_data_set = false; return false; }
			if (!prev_set) XSetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main, ws->GetSystemUsageWindow(), CurrentTime);
			return true;
		}
		bool SetData(Codec::Frame * value)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			bool prev_set = X11::_clipboard_data_set;
			try {
				X11::_clipboard_data_set = true;
				X11::_clipboard_format = Format::Image;
				X11::_clipboard_frame = new Codec::Frame(value);
			} catch (...) { X11::_clipboard_data_set = false; return false; }
			if (!prev_set) XSetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main, ws->GetSystemUsageWindow(), CurrentTime);
			return true;
		}
		bool SetData(const string & plain, const string & attributed)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			bool prev_set = X11::_clipboard_data_set;
			try {
				X11::_clipboard_data_set = true;
				X11::_clipboard_format = Format::RichText;
				X11::_clipboard_string = plain;
				X11::_clipboard_string_attr = attributed;
			} catch (...) { X11::_clipboard_data_set = false; return false; }
			if (!prev_set) XSetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main, ws->GetSystemUsageWindow(), CurrentTime);
			return true;
		}
		bool SetData(const string & subclass, const void * data, int size)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return false;
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			bool prev_set = X11::_clipboard_data_set;
			try {
				X11::_clipboard_data_set = true;
				X11::_clipboard_format = Format::Custom;
				X11::_clipboard_custom_subtype = subclass;
				X11::_clipboard_custom_block = new DataBlock(1);
				X11::_clipboard_custom_block->Append(reinterpret_cast<const uint8 *>(data), size);
			} catch (...) { X11::_clipboard_data_set = false; return false; }
			if (!prev_set) XSetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main, ws->GetSystemUsageWindow(), CurrentTime);
			return true;
		}
		string GetCustomSubclass(void)
		{
			auto ws = static_cast<X11::IXWindowSystem *>(Windows::GetWindowSystem());
			if (!ws) return L"";
			X11::ClipboardInitAtoms(ws->GetXDisplay());
			auto owner = XGetSelectionOwner(ws->GetXDisplay(), X11::_clipboard_atom_main);
			if (!owner) return L"";
			if (owner == ws->GetSystemUsageWindow()) {
				if (X11::_clipboard_data_set && X11::_clipboard_format == Format::Custom) return X11::_clipboard_custom_subtype; else return L"";
			} else {
				SafePointer<DataBlock> data = X11::ClipboardQueryProperty(ws, X11::_clipboard_atom_engine_data_subtype, owner);
				if (data) return string(data->GetBuffer(), data->Length(), Encoding::UTF8); else return L"";
			}
		}
	}
}