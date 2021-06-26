#pragma once

#include "../Miscellaneous/ThreadPool.h"

namespace Engine
{
	namespace Windows
	{
		class ICoreWindow;
		class IPresentationEngine : public Object
		{
		public:
			// Adding and removing engine to window
			virtual void Attach(ICoreWindow * window) = 0;
			virtual void Detach(void) = 0;
			// Presentation control
			virtual void Invalidate(void) = 0;
			virtual void Resize(int width, int height) = 0;
		};
		class ICoreWindow : public IDispatchQueue
		{
		public:
			virtual void SetPresentationEngine(IPresentationEngine * engine) = 0;
			virtual IPresentationEngine * GetPresentationEngine(void) = 0;
			virtual void InvalidateContents(void) = 0;
			virtual handle GetOSHandle(void) = 0;
		};
	}
}