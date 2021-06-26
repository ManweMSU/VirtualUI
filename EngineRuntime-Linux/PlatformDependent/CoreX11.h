#pragma once

#include "../Interfaces/SystemWindows.h"

#include <X11/Xlib.h>

namespace Engine
{
	namespace X11
	{
		class XServerConnection : public Object
		{
			static XServerConnection * _instance;

			Display * _display;

			XServerConnection(void);
		public:
			virtual ~XServerConnection(void) override;

			Display * GetXDisplay(void) const noexcept;

			static XServerConnection * Query(void) noexcept;
		};

		class IXWindowEventHandler
		{
		public:
			virtual void HandleEvent(Window window, XEvent * event) noexcept = 0;
			virtual void HandleTimer(Window window, int timer) noexcept = 0;
		};
		class IXFileEventHandler
		{
		public:
			virtual void HandleFile(int file) noexcept = 0;
		};
		class IXRenderNotify
		{
		public:
			virtual void RenderNotify(Display * display, Window window, UI::Point size, uint32 background_flags) noexcept = 0;
		};

		class XEventLoop : public Object
		{
			struct _loop_context
			{
				bool exit_flag;
				bool sources_updated_flag;
			};
			struct _window_tree_node
			{
				int depth;
				_window_tree_node * left, * right;
				Window window;
				IXWindowEventHandler * handler;
			};
			struct _file_input
			{
				int file;
				IXFileEventHandler * handler;
			};
			struct _timer_input
			{
				Window window;
				int timer_id;
				uint last_fired;
				uint period;
			};
			
			_loop_context * _current_context;
			_window_tree_node * _window_inputs;
			Array<_loop_context *> _context_stack;
			Array<_file_input> _file_inputs;
			Array<_timer_input> _timers;
			SafePointer<XServerConnection> _con;

			static void _tree_destroy(_window_tree_node * node) noexcept;
			static bool _tree_add(_window_tree_node ** root, Window window, IXWindowEventHandler * handler) noexcept;
			static void _tree_remove(_window_tree_node ** root, Window window) noexcept;
			static void _tree_reballance(_window_tree_node ** root) noexcept;
			static _window_tree_node * _tree_find_endpoint(_window_tree_node * root, Window window) noexcept;
			static Bool _peek_predicate(Display * display, XEvent * event, XPointer arg);
			void _set_sources_updated(void) noexcept;
		public:
			XEventLoop(XServerConnection * con);
			virtual ~XEventLoop(void) override;

			bool RegisterWindowHandler(Window window, IXWindowEventHandler * handler) noexcept;
			void UnregisterWindowHandler(Window window) noexcept;
			bool RegisterFileHandler(int file, IXFileEventHandler * handler) noexcept;
			void UnregisterFileHandler(int file) noexcept;

			bool CreateTimer(Window window, int timer, uint period) noexcept;
			void DestroyTimer(Window window, int timer) noexcept;

			bool Run(void) noexcept;
			void Break(void) noexcept;

			void DrainEvent(int type) noexcept;
			bool WaitForEvent(XEvent * event, int type, uint timeout) noexcept;
		};

		class IXWindow : public Windows::IWindow
		{
		public:
			virtual void SetRenderNotify(IXRenderNotify * notify) noexcept = 0;
			virtual void SetFullscreen(bool set) noexcept = 0;
			virtual void LockWindow(bool lock) noexcept = 0;
			virtual bool IsFullscreen(void) noexcept = 0;
			virtual Window GetWindow(void) noexcept = 0;
			virtual Visual * GetVisual(void) noexcept = 0;
		};
		class IXWindowSystem : public Windows::IWindowSystem
		{
		public:
			virtual XEventLoop * GetEventLoop(void) noexcept = 0;
			virtual Window GetSystemUsageWindow(void) noexcept = 0;
			virtual XServerConnection * GetConnection(void) noexcept = 0;
			virtual Display * GetXDisplay(void) noexcept = 0;
			virtual void Beep(void) noexcept = 0;
			virtual bool IsKeyPressed(uint code) noexcept = 0;
			virtual bool IsKeyToggled(uint code) noexcept = 0;
			virtual bool GetAutoRepeatTimes(uint & primary, uint & period) noexcept = 0;
			virtual Codec::Image * GetApplicationIcon(void) noexcept = 0;
			virtual uint32 GetModalLevel(void) noexcept = 0;
			virtual Windows::IWindow * GetModalWindow(void) noexcept = 0;
			virtual void PopModalWindow(void) noexcept = 0;
			virtual void PushModalWindow(Windows::IWindow * window) noexcept = 0;
			virtual Windows::ICursor * GetCurrentCursor(void) noexcept = 0;
		};

		uint XEngineKeyCode(uint code);
		uint XLocalKeyCode(uint code);
	}
}