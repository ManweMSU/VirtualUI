#include "../Interfaces/SystemWindows.h"
#include "../Interfaces/KeyCodes.h"

#include "CoreX11.h"
#include "DeviceX11.h"
#include "ClipboardAPI.h"

#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include <X11/Xcursor/Xcursor.h>
#include <X11/XKBlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define XATOM(atom) XInternAtom(display, atom, false)

namespace Engine
{
	namespace Windows
	{
		IScreen * GetScreenForBestCoverage(const UI::Box & rect);
	}
	namespace X11
	{
		XServerConnection * _com_conn = 0;
		SafePointer<Windows::IWindowSystem> _com_ws;
		SafePointer<Windows::ITheme> _com_theme;
		double _system_def_scale = 0.0;

		double GetScaleVariable(const char * var)
		{
			try {
				int ei = 0;
				int len = strlen(var);
				while (environ[ei]) {
					if (memcmp(environ[ei], var, len) == 0) {
						string e(environ[ei] + len, -1, Encoding::UTF8);
						double val = e.ToDouble();
						return val;
					}
					ei++;
				}
				return 0.0;
			} catch (...) { return 0.0; }
		}
		void InitCommonConnection(void)
		{
			if (!_com_conn) {
				_com_conn = XServerConnection::Query();
				double gtk_scaling = GetScaleVariable("GDK_SCALE=");
				double qt_scaling = GetScaleVariable("QT_SCALE_FACTOR=");
				_system_def_scale = max(max(gtk_scaling, qt_scaling), 1.0);
			}
		}
		bool CheckForXRANDR(void)
		{
			int base_event, base_error, ver_major, ver_minor;
			return XRRQueryExtension(_com_conn->GetXDisplay(), &base_event, &base_error) && XRRQueryVersion(_com_conn->GetXDisplay(), &ver_major, &ver_minor);
		}

		class XScreen : public Windows::IScreen
		{
			string _name;
			UI::Box _rect;
			UI::Point _size;
		public:
			XScreen(const string & name, const UI::Box & rect) : _name(name), _rect(rect) { _size.x = _rect.Right - _rect.Left; _size.y = _rect.Bottom - _rect.Top; }
			virtual ~XScreen(void) override {}
			virtual string GetName(void) override { return _name; }
			virtual UI::Box GetScreenRectangle(void) noexcept override { return _rect; }
			virtual UI::Box GetUserRectangle(void) noexcept override { return GetScreenRectangle(); }
			virtual UI::Point GetResolution(void) noexcept override { return _size; }
			virtual double GetDpiScale(void) noexcept override { return _system_def_scale; }
			virtual Codec::Frame * Capture(void) noexcept override
			{
				auto root = XRootWindow(_com_conn->GetXDisplay(), XDefaultScreen(_com_conn->GetXDisplay()));
				return QueryPixmapSurface(_com_conn, root, _size.x, _size.y, _rect.Left, _rect.Top);
			}
			virtual ImmutableString ToString(void) const override { return L"X Screen"; }
		};
		class XTheme : public Windows::ITheme
		{
		public:
			XTheme(void) {}
			virtual ~XTheme(void) override {}
			virtual Windows::ThemeClass GetClass(void) noexcept override { return Windows::ThemeClass::Light; }
			virtual UI::Color GetColor(Windows::ThemeColor color) noexcept override
			{
				if (color == Windows::ThemeColor::Accent) return UI::Color(0x00, 0x00, 0xC0);
				else if (color == Windows::ThemeColor::WindowBackgroup) return UI::Color(0xF0, 0xF0, 0xF0);
				else if (color == Windows::ThemeColor::WindowText) return UI::Color(0x00, 0x00, 0x00);
				else if (color == Windows::ThemeColor::SelectedBackground) return UI::Color(0x40, 0x40, 0xFF);
				else if (color == Windows::ThemeColor::SelectedText) return UI::Color(0x00, 0x00, 0x00);
				else if (color == Windows::ThemeColor::MenuBackground) return UI::Color(0xF0, 0xF0, 0xF0);
				else if (color == Windows::ThemeColor::MenuText) return UI::Color(0x00, 0x00, 0x00);
				else if (color == Windows::ThemeColor::MenuHotBackground) return UI::Color(0x40, 0x40, 0xFF);
				else if (color == Windows::ThemeColor::MenuHotText) return UI::Color(0x00, 0x00, 0x00);
				else if (color == Windows::ThemeColor::GrayedText) return UI::Color(0x80, 0x80, 0x80);
				else if (color == Windows::ThemeColor::Hyperlink) return UI::Color(0x00, 0x00, 0xFF);
				else return 0;
			}
			virtual ImmutableString ToString(void) const override { return L"Engine Runtime Internal Theme"; }
		};
		class XIPCClient : public Windows::IIPCClient, IXFileEventHandler
		{
			struct _request {
				uint64 serial;
				SafePointer<XIPCClient> retain;
				SafePointer<IDispatchTask> task;
				Windows::IPCStatus * status;
				DataBlock ** result;
			};

			Windows::IPCStatus _status;
			SafePointer<XEventLoop> _loop;
			IXWindowSystem * _system;
			int _socket;
			uint64 _serial;
			Array<_request> _requests;

			void _cancel_requests(Windows::IPCStatus with_status) noexcept
			{
				_status = with_status;
				for (auto & r : _requests) {
					if (r.status) *r.status = with_status;
					if (r.result) *r.result = 0;
					if (r.task) r.task->DoTask(Windows::GetWindowSystem());
				}
				_requests.Clear();
			}
		public:
			XIPCClient(IXWindowSystem * system, const string & app, const string & auth) : _system(system), _serial(1), _requests(0x10)
			{
				_status = Windows::IPCStatus::Unknown;
				_loop.SetRetain(_system->GetEventLoop());
				auto path = L"/tmp/eipc." + auth + L"." + app;
				Array<char> path_chars(1);
				path_chars.SetLength(path.GetEncodedLength(Encoding::UTF8) + 1);
				path.Encode(path_chars.GetBuffer(), Encoding::UTF8, true);
				_socket = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
				if (_socket < 0) throw Exception();
				struct sockaddr_un addr;
				ZeroMemory(&addr, sizeof(addr));
				addr.sun_family = AF_UNIX;
				if (path_chars.Length() > sizeof(addr.sun_path)) { close(_socket); throw Exception(); }
				strcpy(addr.sun_path, path_chars);
				if (connect(_socket, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) { close(_socket); throw Exception(); }
				if (!_loop->RegisterFileHandler(_socket, this)) { close(_socket); throw Exception(); }
			}
			virtual ~XIPCClient(void) override
			{
				shutdown(_socket, SHUT_RDWR);
				close(_socket);
				_loop->UnregisterFileHandler(_socket);
			}
			virtual void HandleFile(int file) noexcept override
			{
				handle hfile = handle(intptr(file));
				uint64 resp_serial, resp_hdr;
				try {
					Windows::IPCStatus status;
					IO::ReadFile(hfile, &resp_serial, 8);
					IO::ReadFile(hfile, &resp_hdr, 8);
					if (resp_hdr & 0x8000000000000000UL) status = Windows::IPCStatus::Accepted; else status = Windows::IPCStatus::Discarded;
					uint32 data_size = resp_hdr & 0xFFFFFFFF;
					SafePointer<DataBlock> data;
					if (data_size) {
						data = new DataBlock(1);
						data->SetLength(data_size);
						IO::ReadFile(hfile, data->GetBuffer(), data->Length());
					}
					_status = status;
					int idx = -1;
					for (int i = 0; i < _requests.Length(); i++) if (_requests[i].serial == resp_serial) { idx = i; break; }
					if (idx >= 0) {
						auto & req = _requests[idx];
						if (req.status) *req.status = status;
						if (req.result) {
							if (!data && status == Windows::IPCStatus::Accepted) data = new DataBlock(1);
							if (data) data->Retain();
							*req.result = data.Inner();
						}
						if (req.task) req.task->DoTask(Windows::GetWindowSystem());
						_requests.Remove(idx);
					}
				} catch (IO::FileReadEndOfFileException & e) {
					_loop->UnregisterFileHandler(_socket);
					_cancel_requests(e.DataRead ? Windows::IPCStatus::InternalError : Windows::IPCStatus::ServerClosed);
				} catch (...) {
					_loop->UnregisterFileHandler(_socket);
					_cancel_requests(Windows::IPCStatus::InternalError);
				}
			}
			virtual bool SendData(const string & verb, const DataBlock * data, IDispatchTask * on_responce, Windows::IPCStatus * result) noexcept override
			{
				if (_status == Windows::IPCStatus::InternalError || _status == Windows::IPCStatus::ServerClosed) return false;
				try {
					_request req;
					req.serial = _serial;
					req.retain.SetRetain(this);
					req.task.SetRetain(on_responce);
					req.status = result;
					req.result = 0;
					_requests << req;
					_serial++;
				} catch (...) { return false; }
				try {
					SafePointer<DataBlock> verb_block = verb.EncodeSequence(Encoding::UTF8, true);
					if (verb_block->Length() > 0xFF) throw Exception();
					uint32 size = data ? data->Length() : 0;
					uint64 hdr = size | (uint64(verb_block->Length()) << 32);
					handle hfile = handle(intptr(_socket));
					IO::WriteFile(hfile, &_requests.LastElement().serial, 8);
					IO::WriteFile(hfile, &hdr, 8);
					IO::WriteFile(hfile, verb_block->GetBuffer(), verb_block->Length());
					if (data && data->Length()) IO::WriteFile(hfile, data->GetBuffer(), data->Length());
				} catch (...) { _requests.RemoveLast(); return false; }
				return true;
			}
			virtual bool RequestData(const string & verb, IDispatchTask * on_responce, Windows::IPCStatus * result, DataBlock ** data) noexcept override
			{
				if (_status == Windows::IPCStatus::InternalError || _status == Windows::IPCStatus::ServerClosed) return false;
				try {
					_request req;
					req.serial = _serial;
					req.retain.SetRetain(this);
					req.task.SetRetain(on_responce);
					req.status = result;
					req.result = data;
					_requests << req;
					_serial++;
				} catch (...) { return false; }
				try {
					SafePointer<DataBlock> verb_block = verb.EncodeSequence(Encoding::UTF8, true);
					if (verb_block->Length() > 0xFF) throw Exception();
					uint64 hdr = 0x8000000000000000UL | (uint64(verb_block->Length()) << 32);
					handle hfile = handle(intptr(_socket));
					IO::WriteFile(hfile, &_requests.LastElement().serial, 8);
					IO::WriteFile(hfile, &hdr, 8);
					IO::WriteFile(hfile, verb_block->GetBuffer(), verb_block->Length());
				} catch (...) { _requests.RemoveLast(); return false; }
				return true;
			}
			virtual Windows::IPCStatus GetStatus(void) noexcept override { return _status; }
			virtual ImmutableString ToString(void) const override { return L"X IPC Client"; }
		};
		class XIPCSession : public Object, IXFileEventHandler
		{
			IXWindowSystem * _system;
			int _socket;
		public:
			XIPCSession(IXWindowSystem * system, int file) : _system(system), _socket(file) { if (!_system->GetEventLoop()->RegisterFileHandler(_socket, this)) throw Exception(); }
			virtual ~XIPCSession(void) override
			{
				_system->GetEventLoop()->UnregisterFileHandler(_socket);
				auto callback = _system->GetCallback();
				if (callback) callback->DataExchangeDisconnect(handle(intptr(_socket)));
				close(_socket);
			}
			virtual void HandleFile(int file) noexcept override
			{
				handle hfile = handle(intptr(file));
				try {
					uint64 serial, hdr;
					IO::ReadFile(hfile, &serial, 8);
					IO::ReadFile(hfile, &hdr, 8);
					uint32 verb_size = (hdr >> 32) & 0xFF;
					uint32 main_size = hdr & 0xFFFFFFFF;
					SafePointer<DataBlock> verb, main;
					verb = new DataBlock(1); main = new DataBlock(1);
					verb->SetLength(verb_size); main->SetLength(main_size);
					if (verb_size) IO::ReadFile(hfile, verb->GetBuffer(), verb->Length());
					if (main_size) IO::ReadFile(hfile, main->GetBuffer(), main->Length());
					auto verb_str = string(verb->GetBuffer(), verb->Length(), Encoding::UTF8);
					auto callback = _system->GetCallback();
					if (hdr & 0x8000000000000000UL) {
						SafePointer<DataBlock> resp;
						if (callback) resp = callback->DataExchangeRespond(hfile, verb_str);
						hdr = resp ? 0x8000000000000000UL : 0;
						if (resp) hdr |= resp->Length();
						IO::WriteFile(hfile, &serial, 8);
						IO::WriteFile(hfile, &hdr, 8);
						if (resp) IO::WriteFile(hfile, resp->GetBuffer(), resp->Length());
					} else {
						bool resp = false;
						if (callback) resp = callback->DataExchangeReceive(hfile, verb_str, main);
						hdr = resp ? 0x8000000000000000UL : 0;
						IO::WriteFile(hfile, &serial, 8);
						IO::WriteFile(hfile, &hdr, 8);
					}
				} catch (...) { shutdown(_socket, SHUT_RDWR); Release(); }
			}
			virtual ImmutableString ToString(void) const override { return L"X IPC Session"; }
		};
		class XCursor : public Windows::ICursor
		{
			SafePointer<XServerConnection> _con;
			Cursor _cursor;
		public:
			XCursor(XServerConnection * con, uint shape) { _con.SetRetain(con); _cursor = XCreateFontCursor(_con->GetXDisplay(), shape); }
			XCursor(XServerConnection * con, const char * name) { _con.SetRetain(con); _cursor = XcursorLibraryLoadCursor(_con->GetXDisplay(), name); }
			XCursor(XServerConnection * con, Codec::Frame * frame) { _con.SetRetain(con); _cursor = LoadCursor(_con, frame); if (!_cursor) throw Exception(); }
			virtual ~XCursor(void) override { if (_cursor) XFreeCursor(_con->GetXDisplay(), _cursor); }
			virtual handle GetOSHandle(void) noexcept override { return handle(intptr(_cursor)); }
			virtual ImmutableString ToString(void) const override { return L"X Cursor"; }
			Cursor GetCursor(void) noexcept { return _cursor; }
		};
		class XPresentationEngine : public Windows::I2DPresentationEngine, IXRenderNotify
		{
			SafePointer<XServerConnection> _con;
			SafePointer<IXRenderingDevice> _device;
			SafePointer<Windows::ITheme> _theme;
			IXWindow * _host;
			struct {
				bool managed;
				SafePointer<Codec::Frame> image;
				Windows::ImageRenderMode mode;
				UI::Color filling;
				SafePointer<UI::ITexture> _texture;
				SafePointer<UI::ITextureRenderingInfo> _info;
			} _managed;
			UI::Point _size;
			Codec::PixelFormat _engine_format;
			XRenderPictFormat * _format;
			Pixmap _backbuffer;
			GC _gc;
		public:
			XPresentationEngine(XServerConnection * con) : _host(0), _size(0, 0), _format(0), _backbuffer(0), _gc(0)
			{
				_con.SetRetain(con);
				_managed.managed = false;
				_device = CreateXDevice(_con);
				_theme = Windows::GetCurrentTheme();
				if (!_device) throw Exception();
			}
			XPresentationEngine(XServerConnection * con, Codec::Frame * image, Windows::ImageRenderMode mode, UI::Color filling) : XPresentationEngine(con)
			{
				_managed.managed = true;
				_managed.image.SetRetain(image);
				_managed.mode = mode;
				_managed.filling = filling;
			}
			virtual ~XPresentationEngine(void) override
			{
				_device->UnsetRenderTarget();
				if (_backbuffer) XFreePixmap(_con->GetXDisplay(), _backbuffer);
				if (_gc) XFreeGC(_con->GetXDisplay(), _gc);
			}
			// Core API
			virtual void Attach(Windows::ICoreWindow * window) override
			{
				_host = static_cast<IXWindow *>(window);
				try {
					auto visual = _host->GetVisual();
					if (!visual) visual = XDefaultVisual(_con->GetXDisplay(), XDefaultScreen(_con->GetXDisplay()));
					_format = XRenderFindVisualFormat(_con->GetXDisplay(), visual);
					if (_format->depth == 24) {
						if (_format->direct.red == 16 && _format->direct.blue == 0) _engine_format = Codec::PixelFormat::B8G8R8;
						else _engine_format = Codec::PixelFormat::R8G8B8;
					} else {
						if (_format->direct.red == 16 && _format->direct.blue == 0) _engine_format = Codec::PixelFormat::B8G8R8A8;
						else _engine_format = Codec::PixelFormat::R8G8B8A8;
					}
				} catch (...) { _host = 0; throw; }
				_host->SetRenderNotify(this);
			}
			virtual void Detach(void) override
			{
				_host = 0;
				_device->UnsetRenderTarget();
				if (_backbuffer) {
					XFreePixmap(_con->GetXDisplay(), _backbuffer);
					_backbuffer = 0;
				}
				if (_gc) {
					XFreeGC(_con->GetXDisplay(), _gc);
					_gc = 0;
				}
				_size = UI::Point(0, 0);
				_format = 0;
				_managed._texture.SetReference(0);
				_managed._info.SetReference(0);
			}
			virtual void Invalidate(void) override
			{
				if (_managed.managed) {
					_managed._texture.SetReference(0);
					_managed._info.SetReference(0);
				}
				XClearArea(_con->GetXDisplay(), _host->GetWindow(), 0, 0, 1, 1, true);
			}
			virtual void Resize(int width, int height) override
			{
				auto new_size = UI::Point(max(width, 1), max(height, 1));
				if (new_size != _size && _format) {
					_device->UnsetRenderTarget();
					auto display = _con->GetXDisplay();
					_size = new_size;
					if (_backbuffer) {
						XFreePixmap(display, _backbuffer);
						_backbuffer = 0;
					}
					if (_gc) {
						XFreeGC(_con->GetXDisplay(), _gc);
						_gc = 0;
					}
					_backbuffer = XCreatePixmap(display, XRootWindow(display, XDefaultScreen(display)), new_size.x, new_size.y, _format->depth);
					if (_backbuffer) _gc = XCreateGC(display, _backbuffer, 0, 0);
					_device->SetRenderTarget(_backbuffer, _size.x, _size.y, _format);
					XClearArea(display, _host->GetWindow(), 0, 0, 1, 1, true);
				}
			}
			// 2D API extension
			virtual UI::IRenderingDevice * GetRenderingDevice(void) noexcept override { if (_managed.managed) return 0; else return _device; }
			virtual bool BeginRenderingPass(void) noexcept override { if (_managed.managed) return false; else return _device->XBeginDraw(); }
			virtual bool EndRenderingPass(void) noexcept override { if (_managed.managed) return false; else return _device->XEndDraw(); }
			// Rendering callback
			virtual void RenderNotify(Display * display, Window window, UI::Point size, uint32 background_flags) noexcept override
			{
				if ((background_flags & 3) || _managed.managed) {
					UI::Color back;
					if (_managed.managed) back = _managed.filling;
					else if ((background_flags & 5) == 5) back = 0;
					else back = _theme->GetColor(Windows::ThemeColor::WindowBackgroup);
					if (!(background_flags & 4)) back.a = 0xFF;
					auto val = Codec::ConvertPixel(back, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Straight, _engine_format, Codec::AlphaMode::Premultiplied);
					XSetForeground(display, _gc, val);
					XFillRectangle(display, _backbuffer, _gc, 0, 0, size.x, size.y);
					XFlushGC(display, _gc);
				}
				if (_managed.managed) {
					if (_device->XBeginDraw()) {
						UI::Box entire(0, 0, size.x, size.y);
						if (!_managed._texture && _managed.image) {
							_managed._texture = _device->LoadTexture(_managed.image);
						}
						if (!_managed._info && _managed._texture) {
							_managed._info = _device->CreateTextureRenderingInfo(_managed._texture, UI::Box(0, 0, _managed._texture->GetWidth(), _managed._texture->GetHeight()), false);
						}
						if (_managed._info) {
							if (_managed.mode == Windows::ImageRenderMode::Stretch) {
								_device->RenderTexture(_managed._info, entire);
							} else if (_managed.mode == Windows::ImageRenderMode::Blit) {
								int xc = size.x / 2;
								int yc = size.y / 2;
								int w = _managed._texture->GetWidth();
								int h = _managed._texture->GetHeight();
								int xorg = xc - w / 2;
								int yorg = yc - h / 2;
								_device->RenderTexture(_managed._info, UI::Box(xorg, yorg, xorg + w, yorg + h));
							} else {
								UI::Box at;
								int xc = size.x / 2;
								int yc = size.y / 2;
								int w = _managed._texture->GetWidth();
								int h = _managed._texture->GetHeight();
								double iaspect = double(w) / double(h);
								double saspect = double(_size.x) / double(_size.y);
								bool size_horz = true;
								if (_managed.mode == Windows::ImageRenderMode::CoverKeepAspectRatio) size_horz = iaspect < saspect;
								else if (_managed.mode == Windows::ImageRenderMode::FitKeepAspectRatio) size_horz = iaspect > saspect;
								if (size_horz) {
									double sf = double(size.x) / double(w);
									int sh = max(int(h * sf), 1);
									at.Left = 0;
									at.Right = size.x;
									at.Top = yc - sh / 2;
									at.Bottom = at.Top + sh;
								} else {
									double sf = double(size.y) / double(h);
									int sw = max(int(w * sf), 1);
									at.Top = 0;
									at.Bottom = size.y;
									at.Left = xc - sw / 2;
									at.Right = at.Left + sw;
								}
								_device->RenderTexture(_managed._info, at);
							}
						}
						_device->XEndDraw();
					}
				} else {
					auto callback = _host->GetCallback();
					if (callback) callback->RenderWindow(_host);
					if (_device->GetState()) _device->XEndDraw();
				}
				XCopyArea(display, _backbuffer, window, _gc, 0, 0, size.x, size.y, 0, 0);
			}
			// Object API
			virtual ImmutableString ToString(void) const override { return L"X Presentation Engine"; }
		};
		class XWindow : public IXWindow, IXWindowEventHandler
		{
			SafePointer<IXWindowSystem> _system;
			SafePointer<Windows::ICursor> _last_cursor;
			SafePointer<Windows::IPresentationEngine> _engine;
			IXRenderNotify * _render_notify;
			XWindow * _parent;
			Array<XWindow *> _children;
			Windows::IWindowCallback * _callback;
			Window _window;
			Visual * _visual;
			Colormap _colormap;
			XIC _ic;
			XIM _im;
			bool _modal, _sizeble, _visible, _mapped, _allow_close;
			bool _margins_unknown, _captured, _locked;
			UI::Point _origin, _size;
			UI::Point _min, _max, _last_click_pos;
			UI::Box _margins;
			uint32 _modal_level;
			uint32 _state_mask; // 1 - maximized horz, 2 - maximized vert, 4 - minimized, 8 - fullscreen
			uint32 _background_mask; // 1 - zero background, 2 - set background, 4 - allow alpha
			uint32 _background_flags;
			uint32 _last_click_mask; // 1 - left, 2 - right
			uint32 _last_click_time;

			bool _is_locked(void)
			{
				if (_locked) return true;
				else if (_modal_level >= _system->GetModalLevel()) return false; 
				else return true;
			}
			void _internal_show_window(void)
			{
				bool must_be_visible = _visible && (!_parent || _parent->_mapped) && (!_parent || !_parent->_locked || _modal);
				if (!_mapped && must_be_visible) {
					XMapRaised(_system->GetXDisplay(), _window);
					_mapped = true;
					for (auto & c : _children) c->_internal_show_window();
				} else if (_mapped && !must_be_visible) {
					_mapped = false;
					for (auto & c : _children) c->_internal_show_window();
					auto display = _system->GetXDisplay();
					if (_state_mask & 4) XWithdrawWindow(display, _window, XDefaultScreen(display));
					else XUnmapWindow(display, _window);
				}
			}
			uint32 _query_current_state(void)
			{
				uint32 state = 0;
				auto display = _system->GetXDisplay();
				Atom * pdata = 0;
				Atom act_type;
				int act_format;
				unsigned long read, size;
				XGetWindowProperty(display, _window, XATOM("_NET_WM_STATE"), 0, __LONG_MAX__, false, AnyPropertyType, &act_type, &act_format, &read, &size, reinterpret_cast<uint8 **>(&pdata));
				for (unsigned long i = 0; i < read; i++) {
					auto atom = pdata[i];
					if (atom == XATOM("_NET_WM_STATE_MAXIMIZED_HORZ")) state |= 1;
					else if (atom == XATOM("_NET_WM_STATE_MAXIMIZED_VERT")) state |= 2;
					else if (atom == XATOM("_NET_WM_STATE_FULLSCREEN")) state |= (3 | 8);
					else if (atom == XATOM("_NET_WM_STATE_HIDDEN")) state |= 4;
					else if (atom == XATOM("_NET_WM_STATE_SHADED")) state |= 4;
				}
				XFree(pdata);
				return state;
			}
			void _update_window_state(void)
			{
				uint32 old_state = _state_mask;
				_state_mask = _query_current_state();
				uint32 omx = old_state & 3, nmx = _state_mask & 3;
				uint32 omn = old_state & 4, nmn = _state_mask & 4;
				if (omx < 3) omx = 0;
				if (nmx < 3) nmx = 0;
				if (_callback && (omx != nmx || omn != nmn)) {
					if (nmn == 4) _callback->WindowMinimize(this);
					else if (nmx == 3) _callback->WindowMaximize(this);
					else _callback->WindowRestore(this);
				}
			}
			void _update_hints(void)
			{
				XSizeHints sz_hints;
				sz_hints.flags = PMinSize | PMaxSize | PPosition;
				sz_hints.x = _origin.x;
				sz_hints.y = _origin.y;
				if (_sizeble) {
					sz_hints.min_width = max(_min.x, 1);
					sz_hints.max_width = _max.x ? _max.x : 0x7FFF;
					sz_hints.min_height = max(_min.y, 1);
					sz_hints.max_height = _max.y ? _max.y : 0x7FFF;
				} else {
					sz_hints.min_width = sz_hints.max_width = _size.x;
					sz_hints.min_height = sz_hints.max_height = _size.y;
				}
				auto display = _system->GetXDisplay();
				XSetSizeHints(display, _window, &sz_hints, XATOM("WM_NORMAL_HINTS"));
			}
			static int _x_null_error_handler(Display * display, XErrorEvent * error) { return 0; }
		public:
			XWindow(IXWindowSystem * system, const Windows::CreateWindowDesc & desc, bool modal) : _modal(modal), _background_mask(0), _children(0x10)
			{
				_background_flags = 0;
				_render_notify = 0;
				_state_mask = 0;
				_last_click_pos = UI::Point(0, 0);
				_last_click_mask = 0;
				_last_click_time = 0;
				_margins = UI::Box(0, 0, 0, 0);
				_modal_level = system->GetModalLevel();
				_visible = _mapped = false;
				_margins_unknown = true;
				_captured = _locked = false;
				_system.SetRetain(system);
				_callback = desc.Callback;
				_parent = static_cast<XWindow *>(desc.ParentWindow);
				UI::Box rect;
				if (desc.Screen) {
					auto screen = desc.Screen->GetScreenRectangle();
					rect.Left = desc.Position.Left + screen.Left;
					rect.Top = desc.Position.Top + screen.Top;
					rect.Right = desc.Position.Right + screen.Left;
					rect.Bottom = desc.Position.Bottom + screen.Top;
				} else rect = desc.Position;
				_origin.x = rect.Left;
				_origin.y = rect.Top;
				int w = max(rect.Right - rect.Left, 1);
				int h = max(rect.Bottom - rect.Top, 1);
				if (desc.Flags & Windows::WindowFlagSizeble) _sizeble = true; else _sizeble = false;
				XSetWindowAttributes attr;
				attr.override_redirect = (desc.Flags & Windows::WindowFlagPopup) ? true : false;
				attr.save_under = (desc.Flags & Windows::WindowFlagPopup) ? true : false;
				attr.event_mask = KeyPressMask | KeyReleaseMask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask |
					FocusChangeMask | PropertyChangeMask | ExposureMask | StructureNotifyMask;
				auto display = _system->GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				_min = desc.MinimalConstraints;
				_max = desc.MaximalConstraints;
				_size = UI::Point(w, h);
				Visual * use_visual = CopyFromParent;
				int use_depth = CopyFromParent;
				unsigned long use_mask = CWEventMask | CWSaveUnder | CWOverrideRedirect;
				_visual = 0;
				if (desc.Flags & Windows::WindowFlagWindowsExtendedFrame) _background_mask = 2;
				if (desc.Flags & Windows::WindowFlagTransparent) _background_flags |= Windows::WindowFlagTransparent;
				if (desc.Flags & Windows::WindowFlagBlurBehind) _background_flags |= Windows::WindowFlagTransparent | Windows::WindowFlagBlurBehind;
				if (!XGetSelectionOwner(display, XATOM("_NET_WM_CM_S0"))) _background_flags = 0;
				if (_background_flags & Windows::WindowFlagTransparent) {
					_background_mask = 1;
					XVisualInfo use_info;
					if (XMatchVisualInfo(display, XDefaultScreen(display), 32, TrueColor, &use_info)) {
						use_depth = use_info.depth;
						use_visual = _visual = use_info.visual;
						attr.colormap = XCreateColormap(display, root, use_visual, AllocNone);
						attr.background_pixel = attr.background_pixel = 0;
						use_mask |= CWColormap | CWBorderPixel | CWBackPixel;
						_background_mask |= 4;
					} else _background_flags = 0;
				}
				Atom blur_behind_atom = 0;
				if (_background_flags & Windows::WindowFlagBlurBehind) {
					uint8 * pdata = 0;
					Atom type;
					int format;
					unsigned long read, size;
					blur_behind_atom = XATOM("_KDE_NET_WM_BLUR_BEHIND_REGION");
					XGetWindowProperty(display, root, blur_behind_atom, 0, 0, true, AnyPropertyType, &type, &format, &read, &size, &pdata);
					XFree(pdata);
					if (!type) _background_flags &= ~Windows::WindowFlagBlurBehind;
				}
				_window = XCreateWindow(display, root, rect.Left, rect.Top, w, h, 0, use_depth, InputOutput, use_visual, use_mask, &attr);
				if (_visual) _colormap = attr.colormap; else _colormap = 0;
				_system->GetEventLoop()->RegisterWindowHandler(_window, this);
				_update_hints();
				SetText(desc.Title);
				Atom wnd_type;
				if (desc.Flags & Windows::WindowFlagPopup) wnd_type = XATOM("_NET_WM_WINDOW_TYPE_COMBO");
				else if (desc.Flags & Windows::WindowFlagToolWindow) wnd_type = XATOM("_NET_WM_WINDOW_TYPE_UTILITY");
				else if (_parent) wnd_type = XATOM("_NET_WM_WINDOW_TYPE_DIALOG");
				else wnd_type = XATOM("_NET_WM_WINDOW_TYPE_NORMAL");
				XChangeProperty(display, _window, XATOM("_NET_WM_WINDOW_TYPE"), XATOM("ATOM"), 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_type), 1);
				if (_modal && _parent) {
					wnd_type = XATOM("_NET_WM_STATE_MODAL");
					XChangeProperty(display, _window, XATOM("_NET_WM_STATE"), XATOM("ATOM"), 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
				}
				if (_parent || (desc.Flags & Windows::WindowFlagPopup)) {
					wnd_type = XATOM("_NET_WM_STATE_SKIP_TASKBAR");
					XChangeProperty(display, _window, XATOM("_NET_WM_STATE"), XATOM("ATOM"), 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
					wnd_type = XATOM("_NET_WM_STATE_SKIP_PAGER");
					XChangeProperty(display, _window, XATOM("_NET_WM_STATE"), XATOM("ATOM"), 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
				}
				wnd_type = XATOM("WM_DELETE_WINDOW");
				XChangeProperty(display, _window, XATOM("WM_PROTOCOLS"), XATOM("ATOM"), 32, PropModeReplace, reinterpret_cast<uint8 *>(&wnd_type), 1);
				if (desc.Flags & Windows::WindowFlagHelpButton) {
					wnd_type = XATOM("_NET_WM_CONTEXT_HELP");
					XChangeProperty(display, _window, XATOM("WM_PROTOCOLS"), XATOM("ATOM"), 32, PropModeAppend, reinterpret_cast<uint8 *>(&wnd_type), 1);
				}
				if (_parent) XChangeProperty(display, _window, XATOM("WM_TRANSIENT_FOR"), XATOM("WINDOW"), 32, PropModeReplace, reinterpret_cast<uint8 *>(&_parent->_window), 1);
				_allow_close = (desc.Flags & Windows::WindowFlagCloseButton) ? true : false;
				Codec::Image * icon;
				if (icon = _system->GetApplicationIcon()) {
					Array<unsigned long> icon_data(0x10000);
					for (auto & f : icon->Frames) {
						SafePointer<Codec::Frame> fc = f.ConvertFormat(Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Straight, Codec::ScanOrigin::TopDown);
						icon_data << fc->GetWidth();
						icon_data << fc->GetHeight();
						for (int y = 0; y < fc->GetHeight(); y++) for (int x = 0; x < fc->GetWidth(); x++) icon_data << fc->GetPixel(x, y);
					}
					XChangeProperty(display, _window, XATOM("_NET_WM_ICON"), XATOM("CARDINAL"), 32, PropModeReplace, reinterpret_cast<uint8 *>(icon_data.GetBuffer()), icon_data.Length());
				}
				if (_background_flags & Windows::WindowFlagBlurBehind) {
					unsigned long value = 0;
					auto type = XATOM("CARDINAL");
					XChangeProperty(display, _window, blur_behind_atom, type, 32, PropModeReplace, reinterpret_cast<uint8 *>(&value), 1);
				}
				if (!(desc.Flags & Windows::WindowFlagPopup)) {
					XWMHints hints;
					hints.flags = InputHint;
					hints.input = true;
					XSetWMHints(display, _window, &hints);
				}
				_im = XOpenIM(_system->GetXDisplay(), 0, 0, 0);
				_ic = XCreateIC(_im, XNInputStyle, XIMPreeditNothing | XIMStatusNothing, XNClientWindow, _window, NULL);
				XEvent event;
				ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = XATOM("_NET_REQUEST_FRAME_EXTENTS");
				event.xclient.format = 32;
				XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				if (_parent) try { _parent->_children.Append(this); } catch (...) {}
				if (desc.Flags & Windows::WindowFlagNonOpaque) SetOpacity(desc.Opacity);
				if (_callback) _callback->Created(this);
			}
			virtual ~XWindow(void) override {}
			// Event handling
			virtual void HandleEvent(Window window, XEvent * event) noexcept override
			{
				if (event->type == Expose) {
					if (!event->xexpose.count && _render_notify) _render_notify->RenderNotify(_system->GetXDisplay(), _window, _size, _background_mask);
				} else if (event->type == MapNotify) {
					if (_callback) _callback->Shown(this, true);
					XResizeWindow(_system->GetXDisplay(), _window, _size.x, _size.y);
					XFlush(_system->GetXDisplay());
				} else if (event->type == UnmapNotify) {
					_update_hints();
					if (_callback) _callback->Shown(this, false);
				} else if (event->type == ClientMessage) {
					if (!_is_locked()) {
						auto display = _system->GetXDisplay();
						if (event->xclient.data.l[0] == XATOM("WM_DELETE_WINDOW") && _allow_close) {
							if (_callback) _callback->WindowClose(this);
						}
						if (event->xclient.data.l[0] == XATOM("_NET_WM_CONTEXT_HELP")) {
							if (_callback) _callback->WindowHelp(this);
						}
					}
				} else if (event->type == ConfigureNotify) {
					int dx = event->xconfigure.x - _origin.x;
					int dy = event->xconfigure.y - _origin.y;
					bool moved = (dx || dy);
					_origin.x = event->xconfigure.x;
					_origin.y = event->xconfigure.y;
					bool sized = (event->xconfigure.width != _size.x || event->xconfigure.height != _size.y);
					if (moved) {
						if (_callback) _callback->WindowMove(this);
					}
					if (sized) {
						_size.x = event->xconfigure.width;
						_size.y = event->xconfigure.height;
						if (_engine) _engine->Resize(_size.x, _size.y);
						if (_callback) _callback->WindowSize(this);
					}
				} else if (event->type == PropertyNotify) {
					auto display = _system->GetXDisplay();
					auto ext_prop = XATOM("_NET_FRAME_EXTENTS");
					if (event->xproperty.atom == ext_prop && event->xproperty.state == PropertyNewValue) {
						unsigned long * pdata = 0;
						Atom act_type;
						int act_format;
						unsigned long read, size;
						XGetWindowProperty(display, _window, ext_prop, 0, __LONG_MAX__, false, AnyPropertyType, &act_type, &act_format,
							&read, &size, reinterpret_cast<uint8 **>(&pdata));
						_margins.Left = (read >= 4) ? pdata[0] : 0;
						_margins.Top = (read >= 4) ? pdata[2] : 0;
						_margins.Right = (read >= 4) ? pdata[1] : 0;
						_margins.Bottom = (read >= 4) ? pdata[3] : 0;
						XFree(pdata);
						if (_margins_unknown) {
							_margins_unknown = false;
							XMoveWindow(display, _window, _origin.x - _margins.Left, _origin.y - _margins.Top);
						}
					} else if (event->xproperty.atom == XATOM("_NET_WM_STATE")) {
						_update_window_state();
					}
				} else if (event->type == FocusIn) {
					if (_callback) {
						_callback->WindowActivate(this);
						_callback->FocusChanged(this, true);
					}
					XSetICFocus(_ic);
					auto display = _system->GetXDisplay();
					auto root = XRootWindow(display, XDefaultScreen(display));
					XEvent event;
					ZeroMemory(&event, sizeof(event));
					event.xclient.type = ClientMessage;
					event.xclient.display = display;
					event.xclient.send_event = true;
					event.xclient.window = _window;
					event.xclient.message_type = XATOM("_NET_WM_STATE");
					event.xclient.format = 32;
					event.xclient.data.l[0] = 0;
					event.xclient.data.l[1] = XATOM("_NET_WM_STATE_DEMANDS_ATTENTION");
					event.xclient.data.l[2] = 0;
					event.xclient.data.l[3] = 1;
					XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				} else if (event->type == FocusOut) {
					ReleaseCapture();
					XUnsetICFocus(_ic);
					if (_callback) {
						_callback->FocusChanged(this, false);
						_callback->WindowDeactivate(this);
					}
				} else if (event->type == KeyPress) {
					if (!_is_locked() && _callback) {
						auto code = XEngineKeyCode(event->xkey.keycode);
						auto status = code ? _callback->KeyDown(this, code) : false;
						if (!status) {
							int len;
							KeySym sym;
							Status lookup_status;
							Array<widechar> ucs(0x100);
							ucs.SetLength(0x10);
							while (true) {
								len = XwcLookupString(_ic, &event->xkey, ucs.GetBuffer(), ucs.Length(), &sym, &lookup_status);
								if (lookup_status == XBufferOverflow) { ucs.SetLength(len + 1); continue; }
								else break;
							}
							if (lookup_status == XLookupChars || lookup_status == XLookupBoth) {
								for (int i = 0; i < len; i++) _callback->CharDown(this, ucs[i]);
							} else if (code == KeyCodes::Tab) {
								_callback->CharDown(this, L'\t');
							}
						}
					}
				} else if (event->type == KeyRelease) {
					if (!_is_locked() && _callback) {
						auto code = XEngineKeyCode(event->xkey.keycode);
						if (code) _callback->KeyUp(this, code);
					}
				} else if (event->type == ButtonPress) {
					if (!_is_locked() && _callback) {
						auto pos = UI::Point(event->xbutton.x, event->xbutton.y);
						uint32 mask = 0;
						if (event->xbutton.button == Button1) mask = 1;
						else if (event->xbutton.button == Button3) mask = 2;
						else {
							if (event->xbutton.button == Button4) _callback->ScrollVertically(this, -1.0);
							else if (event->xbutton.button == Button5) _callback->ScrollVertically(this, 1.0);
							else if (event->xbutton.button == 6) _callback->ScrollHorizontally(this, -1.0);
							else if (event->xbutton.button == 7) _callback->ScrollHorizontally(this, 1.0);
						}
						if (mask) {
							uint32 ts = GetTimerValue();
							if (pos == _last_click_pos && mask == _last_click_mask && ts - _last_click_time <= 750) {
								if (mask == 1) _callback->LeftButtonDoubleClick(this, pos);
								else if (mask == 2) _callback->RightButtonDoubleClick(this, pos);
								mask = 0;
							} else {
								if (mask == 1) _callback->LeftButtonDown(this, pos);
								else if (mask == 2) _callback->RightButtonDown(this, pos);
							}
							_last_click_pos = pos;
							_last_click_mask = mask;
							_last_click_time = ts;
						}
					}
				} else if (event->type == ButtonRelease) {
					if (!_is_locked() && _callback) {
						auto pos = UI::Point(event->xbutton.x, event->xbutton.y);
						if (event->xbutton.button == Button1) _callback->LeftButtonUp(this, pos);
						else if (event->xbutton.button == Button3) _callback->RightButtonUp(this, pos);
					}
				} else if (event->type == MotionNotify) {
					if (!_is_locked() && _callback) {
						auto pos = UI::Point(event->xmotion.x, event->xmotion.y);
						_callback->SetCursor(this, pos);
						_callback->MouseMove(this, pos);
					} else _system->SetCursor(_system->GetSystemCursor(Windows::SystemCursorClass::Arrow));
					auto cursor = _system->GetCurrentCursor();
					if (_last_cursor.Inner() != cursor) {
						_last_cursor.SetRetain(cursor);
						if (_last_cursor) {
							auto hcursor = static_cast<XCursor *>(_last_cursor.Inner())->GetCursor();
							XDefineCursor(_system->GetXDisplay(), _window, hcursor);
						}
					}
				}
			}
			virtual void HandleTimer(Window window, int timer) noexcept override { if (_callback) _callback->Timer(this, timer); }
			// Frame properties control
			virtual void Show(bool show) override { _visible = show; _internal_show_window(); }
			virtual bool IsVisible(void) override { return _visible; }
			virtual void SetText(const string & text) override
			{
				Array<uint8> chars(1);
				chars.SetLength(text.GetEncodedLength(Encoding::UTF8) + 1);
				text.Encode(chars.GetBuffer(), Encoding::UTF8, true);
				auto display = _system->GetXDisplay();
				XChangeProperty(display, _window, XATOM("WM_NAME"), XATOM("UTF8_STRING"), 8, PropModeReplace, chars.GetBuffer(), chars.Length());
				XChangeProperty(display, _window, XATOM("_NET_WM_NAME"), XATOM("UTF8_STRING"), 8, PropModeReplace, chars.GetBuffer(), chars.Length());
				XChangeProperty(display, _window, XATOM("_NET_WM_VISIBLE_NAME"), XATOM("UTF8_STRING"), 8, PropModeReplace, chars.GetBuffer(), chars.Length());
			}
			virtual string GetText(void) override
			{
				string result;
				uint8 * pdata = 0;
				Atom act_type;
				int act_format;
				unsigned long read, size;
				auto display = _system->GetXDisplay();
				XGetWindowProperty(display, _window, XATOM("_NET_WM_NAME"), 0, __LONG_MAX__, false, AnyPropertyType, &act_type, &act_format, &read, &size, &pdata);
				try { result = string(pdata, read, Encoding::UTF8); } catch (...) {}
				XFree(pdata);
				return result;
			}
			virtual void SetPosition(const UI::Box & box) override
			{
				auto old_org = _origin, old_size = _size;
				_origin = UI::Point(box.Left, box.Top);
				_size = UI::Point(box.Right - box.Left, box.Bottom - box.Top);
				if (_size.x < 1) _size.x = 1;
				if (_size.y < 1) _size.y = 1;
				if (old_size != _size && _engine) _engine->Resize(_size.x, _size.y);
				if (_callback) {
					if (old_org != _origin) _callback->WindowMove(this);
					if (old_size != _size) _callback->WindowSize(this);
				}
				_update_hints();
				XMoveWindow(_system->GetXDisplay(), _window, _origin.x - _margins.Left, _origin.y - _margins.Top);
				XResizeWindow(_system->GetXDisplay(), _window, _size.x, _size.y);
				XFlush(_system->GetXDisplay());
			}
			virtual UI::Box GetPosition(void) override { return UI::Box(_origin.x, _origin.y, _origin.x + _size.x, _origin.y + _size.y); }
			virtual UI::Point GetClientSize(void) override { return _size; }
			virtual void SetMinimalConstraints(UI::Point size) override { _min = size; _update_hints(); }
			virtual UI::Point GetMinimalConstraints(void) override { return _min; }
			virtual void SetMaximalConstraints(UI::Point size) override { _max = size; _update_hints(); }
			virtual UI::Point GetMaximalConstraints(void) override { return _max; }
			virtual void Activate(void) override
			{
				XSetErrorHandler(_x_null_error_handler);
				XRaiseWindow(_system->GetXDisplay(), _window);
				XSetInputFocus(_system->GetXDisplay(), _window, RevertToNone, CurrentTime);
				XSync(_system->GetXDisplay(), false);
				XSetErrorHandler(0);
			}
			virtual bool IsActive(void) override { return IsFocused(); }
			virtual void Maximize(void) override
			{
				auto display = _system->GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				XEvent event;
				ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = XATOM("_NET_WM_STATE");
				event.xclient.format = 32;
				event.xclient.data.l[0] = 1;
				event.xclient.data.l[1] = XATOM("_NET_WM_STATE_MAXIMIZED_VERT");
				event.xclient.data.l[2] = XATOM("_NET_WM_STATE_MAXIMIZED_HORZ");
				event.xclient.data.l[3] = 1;
				XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				event.xclient.data.l[0] = 0;
				event.xclient.data.l[1] = XATOM("_NET_WM_STATE_FULLSCREEN");
				event.xclient.data.l[2] = 0;
				XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
			}
			virtual bool IsMaximized(void) override { return (_state_mask & 3) == 3; }
			virtual void Minimize(void) override { if (!(_state_mask & 4)) XIconifyWindow(_system->GetXDisplay(), _window, XDefaultScreen(_system->GetXDisplay())); }
			virtual bool IsMinimized(void) override { return (_state_mask & 4) == 4; }
			virtual void Restore(void) override
			{
				if (_state_mask & 4) XMapWindow(_system->GetXDisplay(), _window);
				auto display = _system->GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				XEvent event;
				ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = XATOM("_NET_WM_STATE");
				event.xclient.format = 32;
				event.xclient.data.l[0] = 0;
				event.xclient.data.l[1] = XATOM("_NET_WM_STATE_MAXIMIZED_VERT");
				event.xclient.data.l[2] = XATOM("_NET_WM_STATE_MAXIMIZED_HORZ");
				event.xclient.data.l[3] = 1;
				XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				event.xclient.data.l[0] = 0;
				event.xclient.data.l[1] = XATOM("_NET_WM_STATE_FULLSCREEN");
				event.xclient.data.l[2] = 0;
				XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
			}
			virtual void RequireAttention(void) override
			{
				auto display = _system->GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				XEvent event;
				ZeroMemory(&event, sizeof(event));
				event.xclient.type = ClientMessage;
				event.xclient.display = display;
				event.xclient.send_event = true;
				event.xclient.window = _window;
				event.xclient.message_type = XATOM("_NET_WM_STATE");
				event.xclient.format = 32;
				event.xclient.data.l[0] = 1;
				event.xclient.data.l[1] = XATOM("_NET_WM_STATE_DEMANDS_ATTENTION");
				event.xclient.data.l[2] = 0;
				event.xclient.data.l[3] = 1;
				XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
			}
			virtual void SetOpacity(double opacity) override
			{
				auto display = _system->GetXDisplay();
				unsigned long value = uint(0xFFFFFFFF * Math::saturate(opacity));
				XChangeProperty(display, _window, XATOM("_NET_WM_WINDOW_OPACITY"), XATOM("CARDINAL"), 32, PropModeReplace, reinterpret_cast<uint8 *>(&value), 1);
			}
			virtual void SetCloseButtonState(Windows::CloseButtonState state) override { _allow_close = state != Windows::CloseButtonState::Disabled; }
			// Children control
			virtual Windows::IWindow * GetParentWindow(void) override { return _parent; }
			virtual Windows::IWindow * GetChildWindow(int index) override { return _children[index]; }
			virtual int GetChildrenCount(void) override { return _children.Length(); }
			// Frame system-dependent properties control
			virtual void SetProgressMode(Windows::ProgressDisplayMode mode) override {}
			virtual void SetProgressValue(double value) override {}
			virtual void SetCocoaEffectMaterial(Windows::CocoaEffectMaterial material) override {}
			// Event handling
			virtual void SetCallback(Windows::IWindowCallback * callback) override { _callback = callback; }
			virtual Windows::IWindowCallback * GetCallback(void) override { return _callback; }
			virtual bool PointHitTest(UI::Point at) override
			{
				auto display = _system->GetXDisplay();
				Window root = XRootWindow(display, XDefaultScreen(display)), child, null;
				int x, y;
				while (true) {
					if (!XTranslateCoordinates(display, root, root, at.x, at.y, &x, &y, &child)) return false;
					if (child) {
						if (child == _window) return true;
						if (!XTranslateCoordinates(display, root, child, x, y, &at.x, &at.y, &null)) return false;
						root = child;
					} else return false;
				}
			}
			virtual UI::Point PointClientToGlobal(UI::Point at) override
			{
				auto display = _system->GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				Window child;
				int x, y;
				if (XTranslateCoordinates(display, _window, root, at.x, at.y, &x, &y, &child)) return UI::Point(x, y);
				else return UI::Point(-1, -1);
			}
			virtual UI::Point PointGlobalToClient(UI::Point at) override
			{
				auto display = _system->GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				Window child;
				int x, y;
				if (XTranslateCoordinates(display, root, _window, at.x, at.y, &x, &y, &child)) return UI::Point(x, y);
				else return UI::Point(-1, -1);
			}
			virtual void SetFocus(void) override {}
			virtual bool IsFocused(void) override
			{
				Window focused;
				int revert;
				XGetInputFocus(_system->GetXDisplay(), &focused, &revert);
				return focused == _window;
			}
			virtual void SetCapture(void) override
			{
				if (!_captured) {
					uint mask = ButtonPressMask | ButtonReleaseMask | PointerMotionMask | ButtonMotionMask;
					if (XGrabPointer(_system->GetXDisplay(), _window, true, mask, GrabModeAsync, GrabModeAsync, 0, 0, CurrentTime) == GrabSuccess) {
						_captured = true;
						if (_callback) _callback->CaptureChanged(this, true);
					}
				}
			}
			virtual void ReleaseCapture(void) override
			{
				if (_captured) {
					XUngrabPointer(_system->GetXDisplay(), CurrentTime);
					_captured = false;
					if (_callback) _callback->CaptureChanged(this, false);
				}
			}
			virtual bool IsCaptured(void) override { return _captured; }
			virtual void SetTimer(uint32 id, uint32 period) override
			{
				_system->GetEventLoop()->DestroyTimer(_window, id);
				if (period) _system->GetEventLoop()->CreateTimer(_window, id, period);
			}
			// Setting special presentation settings
			virtual void SetBackbufferedRenderingDevice(Codec::Frame * image, Windows::ImageRenderMode mode, UI::Color filling) noexcept override
			{
				try {
					SafePointer<XPresentationEngine> engine = new XPresentationEngine(_system->GetConnection(), image, mode, filling);
					SetPresentationEngine(engine);
				} catch (...) {}
			}
			virtual Windows::I2DPresentationEngine * Set2DRenderingDevice(Windows::DeviceClass device_class) noexcept override
			{
				if (device_class == Windows::DeviceClass::Basic || device_class == Windows::DeviceClass::DontCare) {
					SetPresentationEngine(0);
					SafePointer<XPresentationEngine> engine;
					try { engine = new XPresentationEngine(_system->GetConnection()); } catch (...) { return 0; }
					SetPresentationEngine(engine);
					if (_engine) return engine;
				} else if (device_class == Windows::DeviceClass::Hardware) {

					// TODO: IMPLEMENT HARDWARE
					abort();

				}
			}
			// Getting visual appearance
			virtual double GetDpiScale(void) override { return _system_def_scale; }
			virtual Windows::IScreen * GetCurrentScreen(void) override { return Windows::GetScreenForBestCoverage(GetPosition()); }
			virtual Windows::ITheme * GetCurrentTheme(void) override { try { return new XTheme; } catch (...) { return 0; } }
			virtual uint GetBackgroundFlags(void) override { return _background_flags; }
			// Finilizing
			virtual void Destroy(void) override
			{
				SetPresentationEngine(0);
				if (_callback) _callback->Destroyed(this);
				_callback = 0;
				while (_children.Length()) _children.LastElement()->Destroy();
				if (_parent) {
					for (int i = 0; i < _parent->_children.Length(); i++) if (_parent->_children[i] == this) {
						_parent->_children.Remove(i);
						break;
					}
				}
				_system->GetEventLoop()->UnregisterWindowHandler(_window);
				XDestroyIC(_ic);
				XCloseIM(_im);
				XDestroyWindow(_system->GetXDisplay(), _window);
				if (_colormap) XFreeColormap(_system->GetXDisplay(), _colormap);
				Release();
			}
			// X window extension API
			virtual void SetRenderNotify(IXRenderNotify * notify) noexcept override { _render_notify = notify; }
			virtual void SetFullscreen(bool set) noexcept override
			{
				if (set == IsFullscreen()) return;
				if (set) {
					auto display = _system->GetXDisplay();
					auto root = XRootWindow(display, XDefaultScreen(display));
					XEvent event;
					ZeroMemory(&event, sizeof(event));
					event.xclient.type = ClientMessage;
					event.xclient.display = display;
					event.xclient.send_event = true;
					event.xclient.window = _window;
					event.xclient.message_type = XATOM("_NET_WM_STATE");
					event.xclient.format = 32;
					event.xclient.data.l[0] = 0;
					event.xclient.data.l[1] = XATOM("_NET_WM_STATE_MAXIMIZED_VERT");
					event.xclient.data.l[2] = XATOM("_NET_WM_STATE_MAXIMIZED_HORZ");
					event.xclient.data.l[3] = 1;
					XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
					event.xclient.data.l[0] = 1;
					event.xclient.data.l[1] = XATOM("_NET_WM_STATE_FULLSCREEN");
					event.xclient.data.l[2] = 0;
					XSendEvent(display, root, false, SubstructureRedirectMask | SubstructureNotifyMask, &event);
				} else Restore();
			}
			virtual void LockWindow(bool lock) noexcept override
			{
				_locked = lock;
				for (auto & c : _children) c->_internal_show_window();
			}
			virtual bool IsFullscreen(void) noexcept override { return (_state_mask & 8) == 8; }
			virtual Window GetWindow(void) noexcept override { return _window; }
			virtual Visual * GetVisual(void) noexcept override { return _visual; }
			// Core window API
			virtual void SetPresentationEngine(Windows::IPresentationEngine * engine) override
			{
				_render_notify = 0;
				if (_engine) {
					_engine->Detach();
					_engine.SetReference(0);
				}
				_engine.SetRetain(engine);
				if (_engine) {
					try {
						_engine->Attach(this);
						_engine->Resize(_size.x, _size.y);
						_engine->Invalidate();
					} catch (...) { _engine.SetReference(0); _render_notify = 0; }
				}
			}
			virtual Windows::IPresentationEngine * GetPresentationEngine(void) override { return _engine; }
			virtual void InvalidateContents(void) override { if (_engine) _engine->Invalidate(); }
			virtual handle GetOSHandle(void) override { return handle(intptr(_window)); }
			// Dispatch queue API
			virtual void SubmitTask(IDispatchTask * task) override { _system->SubmitTask(task); }
			virtual void BeginSubmit(void) override {}
			virtual void AppendTask(IDispatchTask * task) override { _system->SubmitTask(task); }
			virtual void EndSubmit(void) override {}
			// Object API
			virtual ImmutableString ToString(void) const override { return L"X Window"; }
		};
		class XWindowSystem : public IXWindowSystem, IXWindowEventHandler, IXFileEventHandler
		{
			struct _hotkey_record
			{
				uint base_code;
				uint mod_mask;
				int id;
			};

			SafePointer<XServerConnection> _con;
			SafePointer<XEventLoop> _loop;
			SafePointer<Semaphore> _ibus_sync;
			SafePointer<Codec::Image> _icon;
			SafePointer<Windows::ICursor> _current_cursor;
			Windows::IApplicationCallback * _callback;
			Window _service_window;
			handle _ibus_in, _ibus_out;
			int _ebus;
			struct {
				bool present;
				int ver_minor, ver_major;
				int opcode, event_base, error_base;
				int numlock_mod, scrolllock_mod, alt_mod, system_mod;
			} _xkb;
			struct {
				SafePointer<XCursor> null, arrow, beam, link, size_we, size_ns, size_nw_se, size_sw_ne, size_all;
			} _cursors;
			Array<_hotkey_record> _hotkeys;
			Array<Windows::IWindow *> _main_window_list;
			Array<string> _file_list_open;
			Array<char> _ebus_socket_name;
			Array<Windows::IWindow *> _modal;
			uint32 _modal_level;
			bool _first_time_loop;
			static int _x_last_error;

			static int _x_error_handler(Display * display, XErrorEvent * error) { _x_last_error = error->error_code; return 0; }
		public:
			XWindowSystem(void) : _callback(0), _ebus(-1), _hotkeys(0x10), _main_window_list(0x20), _file_list_open(0x20), _ebus_socket_name(1), _modal(0x10)
			{
				_modal_level = 0;
				_first_time_loop = true;
				_con.SetRetain(_com_conn);
				_loop = new XEventLoop(_con);
				_ibus_sync = CreateSemaphore(1);
				if (!_ibus_sync) throw Exception();
				_xkb.ver_major = XkbMajorVersion; _xkb.ver_minor = XkbMinorVersion;
				_xkb.numlock_mod = _xkb.scrolllock_mod = _xkb.alt_mod = _xkb.system_mod = 0;
				if (XkbLibraryVersion(&_xkb.ver_major, &_xkb.ver_minor)) {
					if (XkbQueryExtension(_con->GetXDisplay(), &_xkb.opcode, &_xkb.event_base, &_xkb.error_base, &_xkb.ver_major, &_xkb.ver_minor)) {
						auto keyboard_desc = XkbGetKeyboard(_con->GetXDisplay(), XkbAllComponentsMask, XkbUseCoreKbd);
						if (keyboard_desc) {
							for (int i = 0; i < 16; i++) {
								if (!keyboard_desc->names->vmods[i]) continue;
								auto mod = keyboard_desc->server->vmods[i];
								auto name = XGetAtomName(_con->GetXDisplay(), keyboard_desc->names->vmods[i]);
								if (name) {
									if (strcmp(name, "NumLock") == 0) _xkb.numlock_mod = mod;
									else if (strcmp(name, "ScrollLock") == 0) _xkb.scrolllock_mod = mod;
									else if (strcmp(name, "Alt") == 0) _xkb.alt_mod = mod;
									else if (strcmp(name, "Meta") == 0) _xkb.system_mod = mod;
									XFree(name);
								}
							}
							XkbFreeKeyboard(keyboard_desc, 0, true);
						}
						_xkb.present = true;
					} else _xkb.present = false;
				} else _xkb.present = false;
				try {
					auto icon_name = IO::GetExecutablePath() + L".ico";
					SafePointer<Streaming::Stream> icon_stream = new Streaming::FileStream(icon_name, Streaming::AccessRead, Streaming::OpenExisting);
					_icon = Codec::DecodeImage(icon_stream);
				} catch (...) {}
				try {
					_cursors.arrow = new XCursor(_con, XC_arrow);
					_cursors.beam = new XCursor(_con, XC_xterm);
					_cursors.link = new XCursor(_con, XC_hand1);
					_cursors.size_we = new XCursor(_con, XC_sb_h_double_arrow);
					_cursors.size_ns = new XCursor(_con, XC_sb_v_double_arrow);
					_cursors.size_nw_se = new XCursor(_con, "nwse-resize");
					_cursors.size_sw_ne = new XCursor(_con, "nesw-resize");
					_cursors.size_all = new XCursor(_con, XC_fleur);
					SafePointer<Codec::Frame> null = new Codec::Frame(1, 1, Codec::PixelFormat::B8G8R8A8);
					null->SetPixel(0, 0, 0);
					_cursors.null = new XCursor(_con, null.Inner());
				} catch (...) {}

				// TODO: IMPLEMENT

				_current_cursor.SetRetain(_cursors.arrow);
				IO::CreatePipe(&_ibus_in, &_ibus_out);
				XSetWindowAttributes attr;
				attr.event_mask = PropertyChangeMask;
				_service_window = XCreateWindow(_con->GetXDisplay(), XRootWindow(_con->GetXDisplay(), XDefaultScreen(_con->GetXDisplay())),
					0, 0, 1, 1, 0, CopyFromParent, InputOnly, CopyFromParent, CWEventMask, &attr);
				if (!_service_window) {
					IO::CloseHandle(_ibus_in);
					IO::CloseHandle(_ibus_out);
					throw Exception();
				}
				if (!_loop->RegisterFileHandler(int(intptr(_ibus_out)), this) || !_loop->RegisterWindowHandler(_service_window, this)) {
					XDestroyWindow(_con->GetXDisplay(), _service_window);
					IO::CloseHandle(_ibus_in);
					IO::CloseHandle(_ibus_out);
					throw OutOfMemoryException();
				}
			}
			virtual ~XWindowSystem(void) override
			{
				// TODO: IMPLEMENT

				XDestroyWindow(_con->GetXDisplay(), _service_window);
				IO::CloseHandle(_ibus_in);
				IO::CloseHandle(_ibus_out);
				if (_ebus >= 0) {
					close(_ebus);
					unlink(_ebus_socket_name);
				}
			}
			// Internal event handling
			virtual void HandleEvent(Window window, XEvent * event) noexcept override
			{
				if (event->type == SelectionClear) {
					ClipboardClear();
				} else if (event->type == SelectionRequest) {
					ClipboardProcessRequestEvent(_con->GetXDisplay(), &event->xselectionrequest);
				} else if (event->type == KeyPress) {
					if (_callback) for (auto & hk : _hotkeys) if (hk.base_code == event->xkey.keycode && hk.mod_mask == event->xkey.state) {
						_callback->HotKeyEvent(hk.id);
					}
				}
			}
			virtual void HandleTimer(Window window, int timer) noexcept override {}
			virtual void HandleFile(int file) noexcept override
			{
				if (file == int(intptr(_ibus_out))) {
					IDispatchTask * task;
					try { IO::ReadFile(_ibus_out, &task, sizeof(task)); } catch (...) { task = 0; }
					if (task) { task->DoTask(this); task->Release(); }
				} else if (_ebus >= 0 && file == _ebus) {
					int client = accept4(_ebus, 0, 0, SOCK_CLOEXEC);
					if (client >= 0) {
						try { auto session = new XIPCSession(this, client); } catch (...) { close(client); }
					} else if (errno != EINTR) {
						_loop->UnregisterFileHandler(_ebus);
						close(_ebus);
						unlink(_ebus_socket_name);
						_ebus = -1;
					}
				}
			}
			// Creating windows
			virtual Windows::IWindow * CreateWindow(const Windows::CreateWindowDesc & desc) noexcept override { try { return new XWindow(this, desc, false); } catch (...) { return 0; } }
			virtual Windows::IWindow * CreateModalWindow(const Windows::CreateWindowDesc & desc) noexcept override
			{
				XWindow * window;
				auto parent = desc.ParentWindow;
				if (!parent) PushModalWindow(0);
				try { window = new XWindow(this, desc, true); } catch (...) { if (!parent) PopModalWindow(); return 0; }
				window->Show(true);
				if (parent) {
					static_cast<XWindow *>(parent)->LockWindow(true);
					return window;
				} else {
					_modal.LastElement() = window;
					_loop->Run();
					PopModalWindow();
					window->Show(false);
					window->Destroy();
					return 0;
				}
			}
			// Measuring windows
			virtual UI::Box ConvertClientToWindow(const UI::Box & box, uint flags) noexcept override { return box; }
			virtual UI::Point ConvertClientToWindow(const UI::Point & size, uint flags) noexcept override { return size; }
			// Event handling
			virtual void SetFilesToOpen(const string * files, int num_files) noexcept override { try { for (int i = 0; i < num_files; i++) _file_list_open.Append(files[i]); } catch (...) {} }
			virtual Windows::IApplicationCallback * GetCallback(void) noexcept override { return _callback; }
			virtual void SetCallback(Windows::IApplicationCallback * callback) noexcept override { _callback = callback; }
			// Application loop control
			virtual void RunMainLoop(void) noexcept override
			{
				if (_first_time_loop || _file_list_open.Length()) {
					bool opened = false;
					if (_callback && _callback->IsHandlerEnabled(Windows::ApplicationHandler::OpenExactFile)) {
						for (auto & file : _file_list_open) try { if (_callback->OpenExactFile(file)) opened = true; } catch (...) {}
					}
					_file_list_open.Clear();
					_first_time_loop = false;
					if (!opened && _callback && _callback->IsHandlerEnabled(Windows::ApplicationHandler::CreateFile)) _callback->CreateNewFile();
				}
				_loop->Run();
			}
			virtual void ExitMainLoop(void) noexcept override { _loop->Break(); }
			virtual void ExitModalSession(Windows::IWindow * window) noexcept override
			{
				auto parent = window->GetParentWindow();
				if (parent) {
					static_cast<XWindow *>(parent)->LockWindow(false);
					window->Show(false);
					window->Destroy();
				} else _loop->Break();
			}
			// Main window registry maintanance
			virtual void RegisterMainWindow(Windows::IWindow * window) noexcept override { try { _main_window_list.Append(window); } catch (...) {} }
			virtual void UnregisterMainWindow(Windows::IWindow * window) noexcept override
			{
				for (int i = 0; i < _main_window_list.Length(); i++) if (_main_window_list[i] == window) { _main_window_list.Remove(i); break; }
				if (!_main_window_list.Length()) ExitMainLoop();
			}
			// Global manupulation with cursor
			virtual UI::Point GetCursorPosition(void) noexcept override
			{
				auto display = GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				Window __root, __child;
				int root_x, root_y, child_x, child_y;
				uint mask;
				if (XQueryPointer(display, root, &__root, &__child, &root_x, &root_y, &child_x, &child_y, &mask)) return UI::Point(root_x, root_y);
				else return UI::Point(-1, -1);
			}
			virtual void SetCursorPosition(UI::Point position) noexcept override
			{
				auto display = GetXDisplay();
				auto root = XRootWindow(display, XDefaultScreen(display));
				XWarpPointer(display, None, root, 0, 0, 0, 0, position.x, position.y);
			}
			virtual Windows::ICursor * LoadCursor(Codec::Frame * source) noexcept override { try { return new XCursor(_con, source); } catch (...) { return 0; } }
			virtual Windows::ICursor * GetSystemCursor(Windows::SystemCursorClass cursor) noexcept override
			{
				if (cursor == Windows::SystemCursorClass::Arrow) return _cursors.arrow;
				else if (cursor == Windows::SystemCursorClass::Null) return _cursors.null;
				else if (cursor == Windows::SystemCursorClass::Beam) return _cursors.beam;
				else if (cursor == Windows::SystemCursorClass::Link) return _cursors.link;
				else if (cursor == Windows::SystemCursorClass::SizeLeftRight) return _cursors.size_we;
				else if (cursor == Windows::SystemCursorClass::SizeUpDown) return _cursors.size_ns;
				else if (cursor == Windows::SystemCursorClass::SizeLeftUpRightDown) return _cursors.size_nw_se;
				else if (cursor == Windows::SystemCursorClass::SizeLeftDownRightUp) return _cursors.size_sw_ne;
				else if (cursor == Windows::SystemCursorClass::SizeAll) return _cursors.size_all;
				else return 0;
			}
			virtual void SetCursor(Windows::ICursor * cursor) noexcept override { if (cursor) _current_cursor.SetRetain(cursor); }
			// Miscellaneous
			virtual Array<UI::Point> * GetApplicationIconSizes(void) noexcept override
			{
				try {
					SafePointer< Array<UI::Point> > result = new Array<UI::Point>(0x10);
					result->Append(UI::Point(128, 128));
					result->Append(UI::Point(64, 64));
					result->Append(UI::Point(48, 48));
					result->Append(UI::Point(32, 32));
					result->Append(UI::Point(24, 24));
					result->Append(UI::Point(16, 16));
					result->Retain();
					return result;
				} catch (...) { return 0; }
			}
			virtual void SetApplicationIcon(Codec::Image * icon) noexcept override { _icon.SetRetain(icon); }
			virtual void SetApplicationBadge(const string & text) noexcept override {}
			virtual void SetApplicationIconVisibility(bool visible) noexcept override {}
			// Starting system dialogs
			virtual bool OpenFileDialog(Windows::OpenFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool SaveFileDialog(Windows::SaveFileInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool ChooseDirectoryDialog(Windows::ChooseDirectoryInfo * info, Windows::IWindow * parent, IDispatchTask * on_exit) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			virtual bool MessageBox(Windows::MessageBoxResult * result, const string & text, const string & title, Windows::IWindow * parent, Windows::MessageBoxButtonSet buttons, Windows::MessageBoxStyle style, IDispatchTask * on_exit) noexcept override
			{
				// TODO: IMPLEMENT
				return false;
			}
			// Menu control
			virtual Windows::IMenu * CreateMenu(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			virtual Windows::IMenuItem * CreateMenuItem(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			// Notification control
			virtual UI::Point GetUserNotificationIconSize(void) noexcept override
			{
				// TODO: IMPLEMENT
				return UI::Point(0, 0);
			}
			virtual void PushUserNotification(const string & title, const string & text, Codec::Image * icon) noexcept override
			{
				// TODO: IMPLEMENT
			}
			// Status icon control
			virtual Windows::IStatusBarIcon * CreateStatusBarIcon(void) noexcept override
			{
				// TODO: IMPLEMENT
				return 0;
			}
			// Global hot key control
			virtual bool CreateHotKey(int event_id, int key_code, uint key_flags) noexcept override
			{
				auto root = XRootWindow(_con->GetXDisplay(), XDefaultScreen(_con->GetXDisplay()));
				try {
					_hotkey_record rec;
					rec.base_code = XLocalKeyCode(key_code);
					rec.mod_mask = 0;
					rec.id = event_id;
					if (key_flags & Windows::HotKeyShift) rec.mod_mask |= ShiftMask;
					if (key_flags & Windows::HotKeyControl) rec.mod_mask |= ControlMask;
					if (key_flags & Windows::HotKeyAlternative) rec.mod_mask |= _xkb.alt_mod;
					if (key_flags & Windows::HotKeySystem) rec.mod_mask |= _xkb.system_mod;
					_hotkeys << rec;
				} catch (...) { return false; }
				if (_hotkeys.Length() == 1) {
					XSetWindowAttributes attr;
					attr.event_mask = KeyPress;
					XChangeWindowAttributes(_con->GetXDisplay(), root, CWEventMask, &attr);
					_loop->RegisterWindowHandler(root, this);
				}
				_x_last_error = 0;
				XSetErrorHandler(_x_error_handler);
				XGrabKey(_con->GetXDisplay(), _hotkeys.LastElement().base_code, _hotkeys.LastElement().mod_mask, root, true, GrabModeAsync, GrabModeAsync);
				XSync(_con->GetXDisplay(), false);
				XSetErrorHandler(0);
				if (_x_last_error) { RemoveHotKey(event_id); return false; }
				return true;
			}
			virtual void RemoveHotKey(int event_id) noexcept override
			{
				if (!_hotkeys.Length()) return;
				auto root = XRootWindow(_con->GetXDisplay(), XDefaultScreen(_con->GetXDisplay()));
				for (int i = 0; i < _hotkeys.Length(); i++) if (_hotkeys[i].id == event_id) {
					_x_last_error = 0;
					XSetErrorHandler(_x_error_handler);
					XUngrabKey(_con->GetXDisplay(), _hotkeys[i].base_code, _hotkeys[i].mod_mask, root);
					XSync(_con->GetXDisplay(), false);
					XSetErrorHandler(0);
					_hotkeys.Remove(i);
					break;
				}
				if (!_hotkeys.Length()) {
					_loop->UnregisterWindowHandler(root);
					XSetWindowAttributes attr;
					attr.event_mask = 0;
					XChangeWindowAttributes(_con->GetXDisplay(), root, CWEventMask, &attr);
				}
			}
			// Interprocess communication / Dynamic data exchange
			virtual bool LaunchIPCServer(const string & app_id, const string & auth_id) noexcept override
			{
				if (_ebus >= 0) return false;
				try {
					auto path = L"/tmp/eipc." + auth_id + L"." + app_id;
					_ebus_socket_name.SetLength(path.GetEncodedLength(Encoding::UTF8) + 1);
					path.Encode(_ebus_socket_name.GetBuffer(), Encoding::UTF8, true);
				} catch (...) { return false; }
				_ebus = socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
				if (_ebus < 0) return false;
				struct sockaddr_un addr;
				ZeroMemory(&addr, sizeof(addr));
				addr.sun_family = AF_UNIX;
				if (_ebus_socket_name.Length() > sizeof(addr.sun_path)) { close(_ebus); _ebus = -1; return false; }
				strcpy(addr.sun_path, _ebus_socket_name);
				if (bind(_ebus, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr)) < 0) { close(_ebus); _ebus = -1; return false; }
				if (listen(_ebus, SOMAXCONN) < 0) { close(_ebus); _ebus = -1; return false; }
				if (!_loop->RegisterFileHandler(_ebus, this)) { close(_ebus); _ebus = -1; return false; }
				return true;
			}
			virtual Windows::IIPCClient * CreateIPCClient(const string & server_app_id, const string & server_auth_id) noexcept override { try { return new XIPCClient(this, server_app_id, server_auth_id); } catch (...) { return 0; } }
			// Extension API
			virtual XEventLoop * GetEventLoop(void) noexcept override { return _loop; }
			virtual Window GetSystemUsageWindow(void) noexcept override { return _service_window; }
			virtual XServerConnection * GetConnection(void) noexcept override { return _con; }
			virtual Display * GetXDisplay(void) noexcept override { return _con->GetXDisplay(); }
			virtual void Beep(void) noexcept override { XBell(_con->GetXDisplay(), 100); XFlush(_con->GetXDisplay()); }
			virtual bool IsKeyPressed(uint code) noexcept override
			{
				if (code == KeyCodes::Control) return IsKeyPressed(KeyCodes::LeftControl) || IsKeyPressed(KeyCodes::RightControl);
				else if (code == KeyCodes::Shift) return IsKeyPressed(KeyCodes::LeftShift) || IsKeyPressed(KeyCodes::RightShift);
				else if (code == KeyCodes::Alternative) return IsKeyPressed(KeyCodes::LeftAlternative) || IsKeyPressed(KeyCodes::RightAlternative);
				else if (code == KeyCodes::System) return IsKeyPressed(KeyCodes::LeftSystem) || IsKeyPressed(KeyCodes::RightSystem); else {
					uint ktr = X11::XLocalKeyCode(code);
					char status[32];
					XQueryKeymap(_con->GetXDisplay(), status);
					return (status[ktr >> 3] >> (ktr & 7)) & 1;
				}
			}
			virtual bool IsKeyToggled(uint code) noexcept override
			{
				if (_xkb.present) {
					XkbStateRec state;
					XkbGetState(_con->GetXDisplay(), XkbUseCoreKbd, &state);
					if (code == KeyCodes::CapsLock) return (state.mods & LockMask) != 0;
					else if (code == KeyCodes::NumLock) return (state.mods & _xkb.numlock_mod) != 0;
					else if (code == KeyCodes::ScrollLock) return (state.mods & _xkb.scrolllock_mod) != 0;
					else return false;
				} else return false;
			}
			virtual bool GetAutoRepeatTimes(uint & primary, uint & period) noexcept override
			{
				if (_xkb.present) {
					XkbGetAutoRepeatRate(_con->GetXDisplay(), XkbUseCoreKbd, &primary, &period);
					return true;
				} else return false;
			}
			virtual Codec::Image * GetApplicationIcon(void) noexcept override { return _icon; }
			virtual uint32 GetModalLevel(void) noexcept override { return _modal_level; }
			virtual Windows::IWindow * GetModalWindow(void) noexcept override { return _modal.Length() ? _modal.LastElement() : 0; }
			virtual void PopModalWindow(void) noexcept override { if (_modal.Length()) { _modal.RemoveLast(); _modal_level--; } }
			virtual void PushModalWindow(Windows::IWindow * window) noexcept override { try { _modal.Append(window); _modal_level++; } catch (...) {} }
			virtual Windows::ICursor * GetCurrentCursor(void) noexcept override { return _current_cursor; }
			// Dispatch API
			virtual void SubmitTask(IDispatchTask * task) override
			{
				if (!task) return;
				task->Retain();
				_ibus_sync->Wait();
				try { IO::WriteFile(_ibus_in, &task, sizeof(task)); }
				catch (...) { _ibus_sync->Open(); task->Release(); throw; }
				_ibus_sync->Open();
			}
			virtual void BeginSubmit(void) override {}
			virtual void AppendTask(IDispatchTask * task) override { SubmitTask(task); }
			virtual void EndSubmit(void) override {}
			// Object API
			virtual ImmutableString ToString(void) const override { return L"X Window System"; }
		};

		int XWindowSystem::_x_last_error = 0;
	}
	namespace Windows
	{
		IScreen * GetFakeScreen(void)
		{
			X11::InitCommonConnection();
			try {
				auto screen = XDefaultScreenOfDisplay(X11::_com_conn->GetXDisplay());
				auto w = XWidthOfScreen(screen);
				auto h = XHeightOfScreen(screen);
				return new X11::XScreen(L"", UI::Box(0, 0, w, h));
			} catch (...) { return 0; }
		}
		ObjectArray<IScreen> * GetActiveScreens(void)
		{
			X11::InitCommonConnection();
			SafePointer< ObjectArray<IScreen> > result;
			try {
				result = new ObjectArray<IScreen>(0x10);
				if (X11::CheckForXRANDR()) {
					int num_monitors;
					auto root = XRootWindow(X11::_com_conn->GetXDisplay(), XDefaultScreen(X11::_com_conn->GetXDisplay()));
					auto monitors = XRRGetMonitors(X11::_com_conn->GetXDisplay(), root, 1, &num_monitors);
					if (monitors) {
						for (int i = 0; i < num_monitors; i++) {
							auto & m = monitors[i];
							auto name = XGetAtomName(X11::_com_conn->GetXDisplay(), m.name);
							if (!name) continue;
							try {
								string wname = string(name, -1, Encoding::UTF8);
								UI::Box rect = UI::Box(m.x, m.y, m.x + m.width, m.y + m.height);
								SafePointer<IScreen> screen = new X11::XScreen(wname, rect);
								result->Append(screen);
							} catch (...) {}
							XFree(name);
						}
						XRRFreeMonitors(monitors);
					}
				} else {
					SafePointer<IScreen> fake = GetFakeScreen();
					if (!fake) return 0;
					result->Append(fake);
				}
			} catch (...) { return 0; }
			result->Retain();
			return result;
		}
		IScreen * GetDefaultScreen(void)
		{
			X11::InitCommonConnection();
			if (!X11::CheckForXRANDR()) return GetFakeScreen();
			SafePointer<IScreen> result;
			int num_monitors;
			auto root = XRootWindow(X11::_com_conn->GetXDisplay(), XDefaultScreen(X11::_com_conn->GetXDisplay()));
			auto monitors = XRRGetMonitors(X11::_com_conn->GetXDisplay(), root, 1, &num_monitors);
			if (monitors) {
				if (!num_monitors) { XRRFreeMonitors(monitors); return 0; }
				int selected = 0;
				for (int i = 0; i < num_monitors; i++) {
					auto & m = monitors[i];
					if (m.primary) { selected = i; break; }
				}
				auto & m = monitors[selected];
				auto name = XGetAtomName(X11::_com_conn->GetXDisplay(), m.name);
				if (!name) { XRRFreeMonitors(monitors); return 0; }
				try {
					string wname = string(name, -1, Encoding::UTF8);
					UI::Box rect = UI::Box(m.x, m.y, m.x + m.width, m.y + m.height);
					result = new X11::XScreen(wname, rect);
				} catch (...) { XFree(name); XRRFreeMonitors(monitors); return 0; }
				XFree(name);
				XRRFreeMonitors(monitors);
			} else return 0;
			result->Retain();
			return result;
		}
		IScreen * GetScreenForBestCoverage(const UI::Box & rect)
		{
			X11::InitCommonConnection();
			if (!X11::CheckForXRANDR()) return GetFakeScreen();
			SafePointer<IScreen> result;
			int num_monitors;
			auto root = XRootWindow(X11::_com_conn->GetXDisplay(), XDefaultScreen(X11::_com_conn->GetXDisplay()));
			auto monitors = XRRGetMonitors(X11::_com_conn->GetXDisplay(), root, 1, &num_monitors);
			if (monitors) {
				if (!num_monitors) { XRRFreeMonitors(monitors); return 0; }
				int selected = 0;
				int max_area = 0;
				for (int i = 0; i < num_monitors; i++) {
					auto & m = monitors[i];
					auto box = UI::Box(m.x, m.y, m.x + m.width, m.y + m.height);
					auto clip = UI::Box::Intersect(box, rect);
					int area = (clip.Right - clip.Left) * (clip.Bottom - clip.Top);
					if (area > max_area) { max_area = area; selected = i; }
				}
				auto & m = monitors[selected];
				auto name = XGetAtomName(X11::_com_conn->GetXDisplay(), m.name);
				if (!name) { XRRFreeMonitors(monitors); return 0; }
				try {
					string wname = string(name, -1, Encoding::UTF8);
					UI::Box rect = UI::Box(m.x, m.y, m.x + m.width, m.y + m.height);
					result = new X11::XScreen(wname, rect);
				} catch (...) { XFree(name); XRRFreeMonitors(monitors); return 0; }
				XFree(name);
				XRRFreeMonitors(monitors);
			} else return 0;
			result->Retain();
			return result;
		}
		ITheme * GetCurrentTheme(void)
		{
			if (!X11::_com_theme) { try { X11::_com_theme = new X11::XTheme; } catch (...) { return 0; } }
			if (X11::_com_theme) { X11::_com_theme->Retain(); return X11::_com_theme; } else return 0;
		}
		IWindowSystem * GetWindowSystem(void)
		{
			if (X11::_com_ws) return X11::_com_ws;
			X11::InitCommonConnection();
			try { X11::_com_ws = new X11::XWindowSystem; } catch (...) { return 0; }
			return X11::_com_ws;
		}
	}
}