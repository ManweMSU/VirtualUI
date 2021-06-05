#pragma once

#include "../UserInterface/ControlBase.h"

#include <Windows.h>
#include <dwmapi.h>

#undef LoadCursor

namespace Engine
{
	namespace UI
	{
		HICON CreateWinIcon(Codec::Frame * Source);
		class HandleWindowStation : public WindowStation
		{
		protected:
			HWND _window;
			bool _clear_background;
			bool _use_custom_device;

			HandleWindowStation(HWND window, IDesktopWindowFactory * Factory);
		private:
			SafePointer<ICursor> _null;
			SafePointer<ICursor> _arrow;
			SafePointer<ICursor> _beam;
			SafePointer<ICursor> _link;
			SafePointer<ICursor> _size_left_right;
			SafePointer<ICursor> _size_up_down;
			SafePointer<ICursor> _size_left_up_right_down;
			SafePointer<ICursor> _size_left_down_right_up;
			SafePointer<ICursor> _size_all;
			uint32 _surrogate = 0;
			Array<Window *> _timers;
			bool _fx_blur_behind;
			Color _fx_clear_background;
			MARGINS _fx_margins;
			
			void _reset_dwm(void);
		public:
			HandleWindowStation(HWND window);
			~HandleWindowStation(void) override;

			virtual void SetFocus(Window * window) override;
			virtual Window * GetFocus(void) override;
			virtual void SetCapture(Window * window) override;
			virtual Window * GetCapture(void) override;
			virtual void ReleaseCapture(void) override;
			virtual void SetExclusiveWindow(Window * window) override;
			virtual Window * GetExclusiveWindow(void) override;
			virtual Point GetCursorPos(void) override;
			virtual void SetCursorPos(Point pos) override;
			virtual bool NativeHitTest(const Point & at) override;
			virtual ICursor * LoadCursor(Streaming::Stream * Source) override;
			virtual ICursor * LoadCursor(Codec::Image * Source) override;
			virtual ICursor * LoadCursor(Codec::Frame * Source) override;
			virtual ICursor * GetSystemCursor(SystemCursor cursor) override;
			virtual void SetSystemCursor(SystemCursor entity, ICursor * cursor) override;
			virtual void SetCursor(ICursor * cursor) override;
			virtual void SetTimer(Window * window, uint32 period) override;
			virtual void DeferredDestroy(Window * window) override;
			virtual void DeferredRaiseEvent(Window * window, int ID) override;
			virtual void AppendTask(IDispatchTask * task) override;
			virtual handle GetOSHandle(void) override;

			HWND Handle(void);
			bool & ClearBackgroundFlag(void);
			Color GetClearBackgroundColor(void);
			void SetFrameMargins(int left, int top, int right, int bottom);
			void SetBlurBehind(bool enable);

			virtual void UseCustomRendering(bool use);

			eint ProcessWindowEvents(uint32 Msg, eint WParam, eint LParam);
		};
	}
}