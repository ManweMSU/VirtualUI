#pragma once

#include "../UserInterface/ControlBase.h"

#include <Windows.h>

#undef LoadCursor

namespace Engine
{
	namespace UI
	{
		class HandleWindowStation : public WindowStation
		{
		private:
			HWND _window;
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
			virtual ICursor * LoadCursor(Streaming::Stream * Source) override;
			virtual ICursor * LoadCursor(Codec::Image * Source) override;
			virtual ICursor * LoadCursor(Codec::Frame * Source) override;
			virtual ICursor * GetSystemCursor(SystemCursor cursor) override;
			virtual void SetSystemCursor(SystemCursor entity, ICursor * cursor) override;
			virtual void SetCursor(ICursor * cursor) override;

			eint ProcessWindowEvents(uint32 Msg, eint WParam, eint LParam);
		};
	}
}