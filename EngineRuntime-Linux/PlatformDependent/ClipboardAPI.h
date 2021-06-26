#include "../Interfaces/Clipboard.h"
#include "../Interfaces/SystemWindows.h"
#include "CoreX11.h"

namespace Engine
{
	namespace X11
	{
		void ClipboardClear(void);
		void ClipboardProcessRequestEvent(Display * display, XSelectionRequestEvent * req_event);
	}
}