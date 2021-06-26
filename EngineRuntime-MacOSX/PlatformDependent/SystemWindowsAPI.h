#pragma once

#include "../Interfaces/SystemWindows.h"

@import Foundation;
@import AppKit;
@import CoreGraphics;

namespace Engine
{
	namespace Cocoa
	{
		typedef void (* RenderLayerCallback) (Windows::IPresentationEngine * engine, Windows::IWindow * window);

		void SetWindowTouchBar(Windows::IWindow * window, Object * object);
		Object * GetWindowTouchBar(Windows::IWindow * window);
		NSWindow * GetWindowObject(Windows::ICoreWindow * window);
		NSView * GetWindowCoreView(Windows::ICoreWindow * window);
		Windows::IWindowCallback * GetWindowCallback(Windows::ICoreWindow * window);
		void SetWindowRenderCallback(Windows::ICoreWindow * window, RenderLayerCallback callback = 0);
		void UnsetWindowRenderCallback(Windows::ICoreWindow * window);
		bool SetWindowFullscreenMode(Windows::ICoreWindow * window, bool switch_on);
		bool IsWindowInFullscreenMode(Windows::ICoreWindow * window);
		bool WindowNeedsAlphaBackbuffer(Windows::ICoreWindow * window);
		CGDirectDisplayID GetDirectDisplayID(Windows::IScreen * screen);
	}
}