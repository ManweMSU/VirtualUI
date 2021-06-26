#include "CoreX11.h"
#include "../Interfaces/KeyCodes.h"

#include <linux/input-event-codes.h>
#include <poll.h>
#include <signal.h>
#include <errno.h>

namespace Engine
{
	namespace X11
	{
		XServerConnection * XServerConnection::_instance = 0;

		XServerConnection::XServerConnection(void)
		{
			XInitThreads();
			_display = XOpenDisplay(0);
			if (!_display) _display = XOpenDisplay(":0");
			if (!_display) throw Exception();
		}
		XServerConnection::~XServerConnection(void) { XCloseDisplay(_display); _instance = 0; }
		Display * XServerConnection::GetXDisplay(void) const noexcept { return _display; }
		XServerConnection * XServerConnection::Query(void) noexcept
		{
			if (_instance) {
				_instance->Retain();
				return _instance;
			} else {
				try {
					_instance = new XServerConnection;
					return _instance;
				} catch (...) { return 0; }
			}
		}

		void XEventLoop::_tree_destroy(_window_tree_node * node) noexcept
		{
			if (!node) return;
			_tree_destroy(node->left);
			_tree_destroy(node->right);
			delete node;
		}
		bool XEventLoop::_tree_add(_window_tree_node ** root, Window window, IXWindowEventHandler * handler) noexcept
		{
			if (*root) {
				if ((*root)->handler) {
					_window_tree_node * reloc = new (std::nothrow) _window_tree_node;
					if (!reloc) return false;
					_window_tree_node * node = new (std::nothrow) _window_tree_node;
					if (!node) { delete reloc; return false; }
					reloc->depth = 0;
					reloc->left = reloc->right = 0;
					node->depth = 0;
					node->left = node->right = 0;
					reloc->window = (*root)->window;
					reloc->handler = (*root)->handler;
					node->window = window;
					node->handler = handler;
					(*root)->handler = 0;
					if (reloc->window < node->window) {
						(*root)->left = reloc;
						(*root)->right = node;
					} else {
						(*root)->right = reloc;
						(*root)->left = node;
					}
					(*root)->window = (*root)->right->window;
					(*root)->depth = 1;
				} else {
					if (window < (*root)->window) {
						if (!_tree_add(&(*root)->left, window, handler)) return false;
					} else {
						if (!_tree_add(&(*root)->right, window, handler)) return false;
					}
					(*root)->depth = max((*root)->right->depth, (*root)->left->depth);
					_tree_reballance(root);
				}
			} else {
				_window_tree_node * node = new (std::nothrow) _window_tree_node;
				if (!node) return false;
				node->depth = 0;
				node->left = node->right = 0;
				node->window = window;
				node->handler = handler;
				*root = node;
			}
			return true;
		}
		void XEventLoop::_tree_remove(_window_tree_node ** root, Window window) noexcept
		{
			if (!root) return;
			if ((*root)->handler) {
				if ((*root)->window == window) {
					delete *root;
					*root = 0;
				}
			} else {
				_window_tree_node ** affected;
				_window_tree_node * alter;
				if (window < (*root)->window) {
					affected = &(*root)->left;
					alter = (*root)->right;
				} else {
					affected = &(*root)->right;
					alter = (*root)->left;
				}
				_tree_remove(affected, window);
				if ((*root)->left && (*root)->right) {
					(*root)->depth = max((*root)->right->depth, (*root)->left->depth);
					_tree_reballance(root);
				} else {
					(*root)->depth = 0;
					(*root)->window = alter->window;
					(*root)->handler = alter->handler;
					(*root)->left = (*root)->right = 0;
					delete alter;
				}
			}
		}
		void XEventLoop::_tree_reballance(_window_tree_node ** root) noexcept
		{
			// TODO: IMPLEMENT

			// if (!(*root)) return;
			// _window_tree_node * old_root = *root;
			// _window_tree_node * new_root = 0;
			// _window_tree_node * rep1, * rep2, * rep3, * rep4, * rep5; // level 0
			// _window_tree_node * reu1, * reu2, * reu3;
			// if (old_root->right->depth - old_root->left->depth > 1) {
			// 	new_root = old_root->right;
			// 	rep1 = old_root->left;
			// 	rep2 = old_root->right->left->left;
			// 	rep3 = old_root->right->left->right;
			// 	rep4 = old_root->right->right->left;
			// 	rep5 = old_root->right->right->right;
			// 	reu1 = old_root;
			// 	reu2 = old_root->right->left;
			// 	reu3 = old_root->right->right;
			// } else if (old_root->left->depth - old_root->right->depth > 1) {
			// 	new_root = old_root->left;
			// 	rep1 = old_root->left->left->left;
			// 	rep2 = old_root->left->left->right;
			// 	rep3 = old_root->left->right->left;
			// 	rep4 = old_root->left->right->right;
			// 	rep5 = old_root->right;
			// 	reu1 = old_root;
			// 	reu2 = old_root->left->left;
			// 	reu3 = old_root->left->right;
			// }
			// if (!new_root) return;

			// // TODO: IMPLEMENT

			// *root = new_root;
		}
		XEventLoop::_window_tree_node * XEventLoop::_tree_find_endpoint(_window_tree_node * root, Window window) noexcept
		{
			if (root->handler) {
				if (root->window == window) return root; else return 0;
			} else {
				if (window < root->window) return _tree_find_endpoint(root->left, window);
				else return _tree_find_endpoint(root->right, window);
			}
		}
		Bool XEventLoop::_peek_predicate(Display * display, XEvent * event, XPointer arg) { return event->type == int(intptr(arg)); }
		void XEventLoop::_set_sources_updated(void) noexcept
		{
			if (_current_context) _current_context->sources_updated_flag = true;
			for (auto & c : _context_stack) c->sources_updated_flag = true;
		}
		XEventLoop::XEventLoop(XServerConnection * con) : _context_stack(4), _file_inputs(0x20), _timers(0x40)
		{
			_con.SetRetain(con);
			_current_context = 0;
			_window_inputs = 0;
		}
		XEventLoop::~XEventLoop(void)
		{
			delete _current_context;
			for (auto & c : _context_stack) delete c;
			_tree_destroy(_window_inputs);
		}
		bool XEventLoop::RegisterWindowHandler(Window window, IXWindowEventHandler * handler) noexcept
		{
			auto result = _tree_add(&_window_inputs, window, handler);
			if (result) _set_sources_updated();
			return result;
		}
		void XEventLoop::UnregisterWindowHandler(Window window) noexcept
		{
			_tree_remove(&_window_inputs, window);
			for (int i = _timers.Length() - 1; i >= 0; i--) if (_timers[i].window == window) _timers.Remove(i);
			_set_sources_updated();
		}
		bool XEventLoop::RegisterFileHandler(int file, IXFileEventHandler * handler) noexcept
		{
			try {
				_file_input in;
				in.file = file;
				in.handler = handler;
				_file_inputs << in;
				_set_sources_updated();
				return true;
			} catch (...) { return false; }
		}
		void XEventLoop::UnregisterFileHandler(int file) noexcept
		{
			for (int i = 0; i < _file_inputs.Length(); i++) if (_file_inputs[i].file == file) {
				_file_inputs.Remove(i);
				_set_sources_updated();
				break;
			}
		}
		bool XEventLoop::CreateTimer(Window window, int timer, uint period) noexcept
		{
			try {
				_timer_input t;
				t.window = window;
				t.timer_id = timer;
				t.period = period;
				t.last_fired = GetTimerValue();
				_timers << t;
				_set_sources_updated();
				return true;
			} catch (...) { return false; }
		}
		void XEventLoop::DestroyTimer(Window window, int timer) noexcept
		{
			for (int i = 0; i < _timers.Length(); i++) if (_timers[i].window == window && _timers[i].timer_id == timer) {
				_timers.Remove(i);
				_set_sources_updated();
				break;
			}
		}
		bool XEventLoop::Run(void) noexcept
		{
			try {
				if (_current_context) _context_stack << _current_context;
				_current_context = new (std::nothrow) _loop_context;
				if (!_current_context) {
					if (_context_stack.Length()) {
						_current_context = _context_stack.LastElement();
						_context_stack.RemoveLast();
					}
					throw OutOfMemoryException();
				}
				_current_context->sources_updated_flag = true;
				_current_context->exit_flag = false;
			} catch (...) { return false; }
			try {
				bool update_timer = false;
				Array<pollfd> poll_array(0x80);
				int nearest_timer = -1;
				int timeout, poll_result;
				uint date_fire;
				XEvent event;
				while (!_current_context->exit_flag) {
					uint date_now = GetTimerValue();
					if (_current_context->sources_updated_flag) {
						_current_context->sources_updated_flag = false;
						update_timer = true;
						poll_array.SetLength(1 + _file_inputs.Length());
						poll_array[0].fd = XConnectionNumber(_con->GetXDisplay());
						poll_array[0].events = POLLIN;
						for (int i = 0; i < _file_inputs.Length(); i++) {
							poll_array[i + 1].fd = _file_inputs[i].file;
							poll_array[i + 1].events = POLLIN;
						}
					}
					if (update_timer) {
						update_timer = false;
						timeout = nearest_timer = -1;
						for (int i = 0; i < _timers.Length(); i++) {
							int cto = _timers[i].last_fired + _timers[i].period - date_now;
							if (cto < 0) cto = 0;
							if (timeout == -1 || timeout < cto) {
								timeout = cto;
								nearest_timer = i;
								date_fire = _timers[i].last_fired + _timers[i].period;
							}
						}
					}
					if (nearest_timer >= 0) {
						timeout = date_fire - date_now;
						if (timeout < 0) timeout = 0;
					} else timeout = -1;
					if (XPending(_con->GetXDisplay())) {
						poll_result = 1;
						poll_array[0].revents = POLLIN;
						for (int i = 1; i < poll_array.Length(); i++) poll_array[i].revents = 0;
					} else {
						poll_result = poll(poll_array.GetBuffer(), poll_array.Length(), timeout);
					}
					if (poll_result > 0) {
						if (poll_array[0].revents) {
							XNextEvent(_con->GetXDisplay(), &event);
							if (event.type == MappingNotify) XRefreshKeyboardMapping(&event.xmapping); else {
								auto responder = _tree_find_endpoint(_window_inputs, event.xany.window);
								if (responder) responder->handler->HandleEvent(responder->window, &event);
							}
						}
						for (int i = 1; i < poll_array.Length(); i++) if (poll_array[i].revents) {
							_file_inputs[i - 1].handler->HandleFile(_file_inputs[i - 1].file);
							if (_current_context->sources_updated_flag) break;
						}
					} else if (poll_result == 0) {
						if (nearest_timer >= 0) {
							update_timer = true;
							_timers[nearest_timer].last_fired += _timers[nearest_timer].period;
							auto responder = _tree_find_endpoint(_window_inputs, _timers[nearest_timer].window);
							if (responder) responder->handler->HandleTimer(_timers[nearest_timer].window, _timers[nearest_timer].timer_id);
						}
					} else if (poll_result == -1) {
						if (errno == EINTR) continue; else throw Exception();
					}
				}
			} catch (...) {
				delete _current_context;
				if (_context_stack.Length()) {
					_current_context = _context_stack.LastElement();
					_context_stack.RemoveLast();
				} else _current_context = 0;
				return false;
			}
			delete _current_context;
			if (_context_stack.Length()) {
				_current_context = _context_stack.LastElement();
				_context_stack.RemoveLast();
			} else _current_context = 0;
			return true;
		}
		void XEventLoop::Break(void) noexcept { if (_current_context) _current_context->exit_flag = true; }
		void XEventLoop::DrainEvent(int type) noexcept
		{
			XFlush(_con->GetXDisplay());
			XLockDisplay(_con->GetXDisplay());
			XEvent event;
			while (XCheckIfEvent(_con->GetXDisplay(), &event, _peek_predicate, XPointer(intptr(type))));
			XUnlockDisplay(_con->GetXDisplay());
		}
		bool XEventLoop::WaitForEvent(XEvent * event, int type, uint timeout) noexcept
		{
			XFlush(_con->GetXDisplay());
			XLockDisplay(_con->GetXDisplay());
			if (XCheckIfEvent(_con->GetXDisplay(), event, _peek_predicate, XPointer(intptr(type)))) {
				XUnlockDisplay(_con->GetXDisplay());
				return true;
			}
			pollfd poll_struct;
			poll_struct.fd = XConnectionNumber(_con->GetXDisplay());
			poll_struct.events = POLLIN;
			while (true) {
				auto poll_result = poll(&poll_struct, 1, timeout);
				if (poll_result == -1) {
					if (errno == EINTR) continue; else {
						XUnlockDisplay(_con->GetXDisplay());
						return false;
					}
				} else if (poll_result == 0) {
					XUnlockDisplay(_con->GetXDisplay());
					return false;
				} else {
					XIfEvent(_con->GetXDisplay(), event, _peek_predicate, XPointer(intptr(type)));
					XUnlockDisplay(_con->GetXDisplay());
					return true;
				}
			}
		}
		
		uint XEngineKeyCode(uint code)
		{
			code -= 8;
			if (code == KEY_ESC) return KeyCodes::Escape;
			else if (code == KEY_0) return KeyCodes::D0;
			else if (code == KEY_1) return KeyCodes::D1;
			else if (code == KEY_2) return KeyCodes::D2;
			else if (code == KEY_3) return KeyCodes::D3;
			else if (code == KEY_4) return KeyCodes::D4;
			else if (code == KEY_5) return KeyCodes::D5;
			else if (code == KEY_6) return KeyCodes::D6;
			else if (code == KEY_7) return KeyCodes::D7;
			else if (code == KEY_8) return KeyCodes::D8;
			else if (code == KEY_9) return KeyCodes::D9;
			else if (code == KEY_MINUS) return KeyCodes::OemMinus;
			else if (code == KEY_EQUAL) return KeyCodes::OemPlus;
			else if (code == KEY_BACKSPACE) return KeyCodes::Back;
			else if (code == KEY_TAB) return KeyCodes::Tab;
			else if (code == KEY_Q) return KeyCodes::Q;
			else if (code == KEY_W) return KeyCodes::W;
			else if (code == KEY_E) return KeyCodes::E;
			else if (code == KEY_R) return KeyCodes::R;
			else if (code == KEY_T) return KeyCodes::T;
			else if (code == KEY_Y) return KeyCodes::Y;
			else if (code == KEY_U) return KeyCodes::U;
			else if (code == KEY_I) return KeyCodes::I;
			else if (code == KEY_O) return KeyCodes::O;
			else if (code == KEY_P) return KeyCodes::P;
			else if (code == KEY_LEFTBRACE) return KeyCodes::Oem4;
			else if (code == KEY_RIGHTBRACE) return KeyCodes::Oem6;
			else if (code == KEY_ENTER) return KeyCodes::Return;
			else if (code == KEY_LEFTCTRL) return KeyCodes::LeftControl;
			else if (code == KEY_A) return KeyCodes::A;
			else if (code == KEY_S) return KeyCodes::S;
			else if (code == KEY_D) return KeyCodes::D;
			else if (code == KEY_F) return KeyCodes::F;
			else if (code == KEY_G) return KeyCodes::G;
			else if (code == KEY_H) return KeyCodes::H;
			else if (code == KEY_J) return KeyCodes::J;
			else if (code == KEY_K) return KeyCodes::K;
			else if (code == KEY_L) return KeyCodes::L;
			else if (code == KEY_SEMICOLON) return KeyCodes::Oem1;
			else if (code == KEY_APOSTROPHE) return KeyCodes::Oem7;
			else if (code == KEY_GRAVE) return KeyCodes::Oem3;
			else if (code == KEY_LEFTSHIFT) return KeyCodes::LeftShift;
			else if (code == KEY_BACKSLASH) return KeyCodes::Oem5;
			else if (code == KEY_Z) return KeyCodes::Z;
			else if (code == KEY_X) return KeyCodes::X;
			else if (code == KEY_C) return KeyCodes::C;
			else if (code == KEY_V) return KeyCodes::V;
			else if (code == KEY_B) return KeyCodes::B;
			else if (code == KEY_N) return KeyCodes::N;
			else if (code == KEY_M) return KeyCodes::M;
			else if (code == KEY_COMMA) return KeyCodes::OemComma;
			else if (code == KEY_DOT) return KeyCodes::OemPeriod;
			else if (code == KEY_SLASH) return KeyCodes::Oem2;
			else if (code == KEY_RIGHTSHIFT) return KeyCodes::RightShift;
			else if (code == KEY_KPASTERISK) return KeyCodes::Multiply;
			else if (code == KEY_LEFTALT) return KeyCodes::LeftAlternative;
			else if (code == KEY_SPACE) return KeyCodes::Space;
			else if (code == KEY_CAPSLOCK) return KeyCodes::CapsLock;
			else if (code == KEY_F1) return KeyCodes::F1;
			else if (code == KEY_F2) return KeyCodes::F2;
			else if (code == KEY_F3) return KeyCodes::F3;
			else if (code == KEY_F4) return KeyCodes::F4;
			else if (code == KEY_F5) return KeyCodes::F5;
			else if (code == KEY_F6) return KeyCodes::F6;
			else if (code == KEY_F7) return KeyCodes::F7;
			else if (code == KEY_F8) return KeyCodes::F8;
			else if (code == KEY_F9) return KeyCodes::F9;
			else if (code == KEY_F10) return KeyCodes::F10;
			else if (code == KEY_NUMLOCK) return KeyCodes::NumLock;
			else if (code == KEY_SCROLLLOCK) return KeyCodes::ScrollLock;
			else if (code == KEY_KP7) return KeyCodes::Num7;
			else if (code == KEY_KP8) return KeyCodes::Num8;
			else if (code == KEY_KP9) return KeyCodes::Num9;
			else if (code == KEY_KPMINUS) return KeyCodes::Subtract;
			else if (code == KEY_KP4) return KeyCodes::Num4;
			else if (code == KEY_KP5) return KeyCodes::Num5;
			else if (code == KEY_KP6) return KeyCodes::Num6;
			else if (code == KEY_KPPLUS) return KeyCodes::Add;
			else if (code == KEY_KP1) return KeyCodes::Num1;
			else if (code == KEY_KP2) return KeyCodes::Num2;
			else if (code == KEY_KP3) return KeyCodes::Num3;
			else if (code == KEY_KP0) return KeyCodes::Num0;
			else if (code == KEY_KPDOT) return KeyCodes::Decimal;
			else if (code == KEY_102ND) return KeyCodes::Oem8;
			else if (code == KEY_F11) return KeyCodes::F11;
			else if (code == KEY_F12) return KeyCodes::F12;
			else if (code == KEY_KPENTER) return KeyCodes::Return;
			else if (code == KEY_RIGHTCTRL) return KeyCodes::RightControl;
			else if (code == KEY_KPSLASH) return KeyCodes::Divide;
			else if (code == KEY_RIGHTALT) return KeyCodes::RightAlternative;
			else if (code == KEY_HOME) return KeyCodes::Home;
			else if (code == KEY_UP) return KeyCodes::Up;
			else if (code == KEY_PAGEUP) return KeyCodes::PageUp;
			else if (code == KEY_LEFT) return KeyCodes::Left;
			else if (code == KEY_RIGHT) return KeyCodes::Right;
			else if (code == KEY_END) return KeyCodes::End;
			else if (code == KEY_DOWN) return KeyCodes::Down;
			else if (code == KEY_PAGEDOWN) return KeyCodes::PageDown;
			else if (code == KEY_INSERT) return KeyCodes::Insert;
			else if (code == KEY_DELETE) return KeyCodes::Delete;
			else if (code == KEY_MUTE) return KeyCodes::VolumeMute;
			else if (code == KEY_VOLUMEDOWN) return KeyCodes::VolumeDown;
			else if (code == KEY_VOLUMEUP) return KeyCodes::VolumeUp;
			else if (code == KEY_PAUSE) return KeyCodes::Pause;
			else if (code == KEY_LEFTMETA) return KeyCodes::LeftSystem;
			else if (code == KEY_RIGHTMETA) return KeyCodes::RightSystem;
			else if (code == KEY_HELP) return KeyCodes::Help;
			else if (code == KEY_SLEEP) return KeyCodes::Sleep;
			else if (code == KEY_PRINT) return KeyCodes::Print;
			else if (code == KEY_SELECT) return KeyCodes::Select;
			else if (code == KEY_CLEAR) return KeyCodes::OemClear;
			else if (code == KEY_F13) return KeyCodes::F13;
			else if (code == KEY_F14) return KeyCodes::F14;
			else if (code == KEY_F15) return KeyCodes::F15;
			else if (code == KEY_F16) return KeyCodes::F16;
			else if (code == KEY_F17) return KeyCodes::F17;
			else if (code == KEY_F18) return KeyCodes::F18;
			else if (code == KEY_F19) return KeyCodes::F19;
			else if (code == KEY_F20) return KeyCodes::F20;
			else if (code == KEY_F21) return KeyCodes::F21;
			else if (code == KEY_F22) return KeyCodes::F22;
			else if (code == KEY_F23) return KeyCodes::F23;
			else if (code == KEY_F24) return KeyCodes::F24;
			else return 0;
		}
		uint XLocalKeyCode(uint code)
		{
			uint ret = 0;
			if (code == KeyCodes::Escape) ret = KEY_ESC;
			else if (code == KeyCodes::D0) ret = KEY_0;
			else if (code == KeyCodes::D1) ret = KEY_1;
			else if (code == KeyCodes::D2) ret = KEY_2;
			else if (code == KeyCodes::D3) ret = KEY_3;
			else if (code == KeyCodes::D4) ret = KEY_4;
			else if (code == KeyCodes::D5) ret = KEY_5;
			else if (code == KeyCodes::D6) ret = KEY_6;
			else if (code == KeyCodes::D7) ret = KEY_7;
			else if (code == KeyCodes::D8) ret = KEY_8;
			else if (code == KeyCodes::D9) ret = KEY_9;
			else if (code == KeyCodes::OemMinus) ret = KEY_MINUS;
			else if (code == KeyCodes::OemPlus) ret = KEY_EQUAL;
			else if (code == KeyCodes::Back) ret = KEY_BACKSPACE;
			else if (code == KeyCodes::Tab) ret = KEY_TAB;
			else if (code == KeyCodes::Q) ret = KEY_Q;
			else if (code == KeyCodes::W) ret = KEY_W;
			else if (code == KeyCodes::E) ret = KEY_E;
			else if (code == KeyCodes::R) ret = KEY_R;
			else if (code == KeyCodes::T) ret = KEY_T;
			else if (code == KeyCodes::Y) ret = KEY_Y;
			else if (code == KeyCodes::U) ret = KEY_U;
			else if (code == KeyCodes::I) ret = KEY_I;
			else if (code == KeyCodes::O) ret = KEY_O;
			else if (code == KeyCodes::P) ret = KEY_P;
			else if (code == KeyCodes::Oem4) ret = KEY_LEFTBRACE;
			else if (code == KeyCodes::Oem6) ret = KEY_RIGHTBRACE;
			else if (code == KeyCodes::Return) ret = KEY_ENTER;
			else if (code == KeyCodes::LeftControl) ret = KEY_LEFTCTRL;
			else if (code == KeyCodes::A) ret = KEY_A;
			else if (code == KeyCodes::S) ret = KEY_S;
			else if (code == KeyCodes::D) ret = KEY_D;
			else if (code == KeyCodes::F) ret = KEY_F;
			else if (code == KeyCodes::G) ret = KEY_G;
			else if (code == KeyCodes::H) ret = KEY_H;
			else if (code == KeyCodes::J) ret = KEY_J;
			else if (code == KeyCodes::K) ret = KEY_K;
			else if (code == KeyCodes::L) ret = KEY_L;
			else if (code == KeyCodes::Oem1) ret = KEY_SEMICOLON;
			else if (code == KeyCodes::Oem7) ret = KEY_APOSTROPHE;
			else if (code == KeyCodes::Oem3) ret = KEY_GRAVE;
			else if (code == KeyCodes::LeftShift) ret = KEY_LEFTSHIFT;
			else if (code == KeyCodes::Oem5) ret = KEY_BACKSLASH;
			else if (code == KeyCodes::Z) ret = KEY_Z;
			else if (code == KeyCodes::X) ret = KEY_X;
			else if (code == KeyCodes::C) ret = KEY_C;
			else if (code == KeyCodes::V) ret = KEY_V;
			else if (code == KeyCodes::B) ret = KEY_B;
			else if (code == KeyCodes::N) ret = KEY_N;
			else if (code == KeyCodes::M) ret = KEY_M;
			else if (code == KeyCodes::OemComma) ret = KEY_COMMA;
			else if (code == KeyCodes::OemPeriod) ret = KEY_DOT;
			else if (code == KeyCodes::Oem2) ret = KEY_SLASH;
			else if (code == KeyCodes::RightShift) ret = KEY_RIGHTSHIFT;
			else if (code == KeyCodes::Multiply) ret = KEY_KPASTERISK;
			else if (code == KeyCodes::LeftAlternative) ret = KEY_LEFTALT;
			else if (code == KeyCodes::Space) ret = KEY_SPACE;
			else if (code == KeyCodes::CapsLock) ret = KEY_CAPSLOCK;
			else if (code == KeyCodes::F1) ret = KEY_F1;
			else if (code == KeyCodes::F2) ret = KEY_F2;
			else if (code == KeyCodes::F3) ret = KEY_F3;
			else if (code == KeyCodes::F4) ret = KEY_F4;
			else if (code == KeyCodes::F5) ret = KEY_F5;
			else if (code == KeyCodes::F6) ret = KEY_F6;
			else if (code == KeyCodes::F7) ret = KEY_F7;
			else if (code == KeyCodes::F8) ret = KEY_F8;
			else if (code == KeyCodes::F9) ret = KEY_F9;
			else if (code == KeyCodes::F10) ret = KEY_F10;
			else if (code == KeyCodes::NumLock) ret = KEY_NUMLOCK;
			else if (code == KeyCodes::ScrollLock) ret = KEY_SCROLLLOCK;
			else if (code == KeyCodes::Num7) ret = KEY_KP7;
			else if (code == KeyCodes::Num8) ret = KEY_KP8;
			else if (code == KeyCodes::Num9) ret = KEY_KP9;
			else if (code == KeyCodes::Subtract) ret = KEY_KPMINUS;
			else if (code == KeyCodes::Num4) ret = KEY_KP4;
			else if (code == KeyCodes::Num5) ret = KEY_KP5;
			else if (code == KeyCodes::Num6) ret = KEY_KP6;
			else if (code == KeyCodes::Add) ret = KEY_KPPLUS;
			else if (code == KeyCodes::Num1) ret = KEY_KP1;
			else if (code == KeyCodes::Num2) ret = KEY_KP2;
			else if (code == KeyCodes::Num3) ret = KEY_KP3;
			else if (code == KeyCodes::Num0) ret = KEY_KP0;
			else if (code == KeyCodes::Decimal) ret = KEY_KPDOT;
			else if (code == KeyCodes::Oem8) ret = KEY_102ND;
			else if (code == KeyCodes::F11) ret = KEY_F11;
			else if (code == KeyCodes::F12) ret = KEY_F12;
			else if (code == KeyCodes::RightControl) ret = KEY_RIGHTCTRL;
			else if (code == KeyCodes::Divide) ret = KEY_KPSLASH;
			else if (code == KeyCodes::RightAlternative) ret = KEY_RIGHTALT;
			else if (code == KeyCodes::Home) ret = KEY_HOME;
			else if (code == KeyCodes::Up) ret = KEY_UP;
			else if (code == KeyCodes::PageUp) ret = KEY_PAGEUP;
			else if (code == KeyCodes::Left) ret = KEY_LEFT;
			else if (code == KeyCodes::Right) ret = KEY_RIGHT;
			else if (code == KeyCodes::End) ret = KEY_END;
			else if (code == KeyCodes::Down) ret = KEY_DOWN;
			else if (code == KeyCodes::PageDown) ret = KEY_PAGEDOWN;
			else if (code == KeyCodes::Insert) ret = KEY_INSERT;
			else if (code == KeyCodes::Delete) ret = KEY_DELETE;
			else if (code == KeyCodes::VolumeMute) ret = KEY_MUTE;
			else if (code == KeyCodes::VolumeDown) ret = KEY_VOLUMEDOWN;
			else if (code == KeyCodes::VolumeUp) ret = KEY_VOLUMEUP;
			else if (code == KeyCodes::Pause) ret = KEY_PAUSE;
			else if (code == KeyCodes::LeftSystem) ret = KEY_LEFTMETA;
			else if (code == KeyCodes::RightSystem) ret = KEY_RIGHTMETA;
			else if (code == KeyCodes::Help) ret = KEY_HELP;
			else if (code == KeyCodes::Sleep) ret = KEY_SLEEP;
			else if (code == KeyCodes::Print) ret = KEY_PRINT;
			else if (code == KeyCodes::Select) ret = KEY_SELECT;
			else if (code == KeyCodes::OemClear) ret = KEY_CLEAR;
			else if (code == KeyCodes::F13) ret = KEY_F13;
			else if (code == KeyCodes::F14) ret = KEY_F14;
			else if (code == KeyCodes::F15) ret = KEY_F15;
			else if (code == KeyCodes::F16) ret = KEY_F16;
			else if (code == KeyCodes::F17) ret = KEY_F17;
			else if (code == KeyCodes::F18) ret = KEY_F18;
			else if (code == KeyCodes::F19) ret = KEY_F19;
			else if (code == KeyCodes::F20) ret = KEY_F20;
			else if (code == KeyCodes::F21) ret = KEY_F21;
			else if (code == KeyCodes::F22) ret = KEY_F22;
			else if (code == KeyCodes::F23) ret = KEY_F23;
			else if (code == KeyCodes::F24) ret = KEY_F24;
			return ret + 8;
		}
	}
}