#pragma once

#include "../UserInterface/ControlBase.h"

#include <Windows.h>

namespace Engine
{
	namespace UI
	{
		class HandleWindowStation : public WindowStation
		{
		private:
			HWND _window;
		public:
			HandleWindowStation(HWND window);
			~HandleWindowStation(void) override;

			virtual void SetFocus(Window * window) override;
			virtual Window * GetFocus(void) override;
			virtual void SetCapture(Window * window) override;
			virtual Window * GetCapture(void) override;
			virtual void ReleaseCapture(void) override;
			virtual Point GetCursorPos(void) override;

			eint ProcessWindowEvents(uint32 Msg, eint WParam, eint LParam);
		};
	}
}