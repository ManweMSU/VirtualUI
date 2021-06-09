#include "../Interfaces/SystemColors.h"

@import Foundation;
@import AppKit;

namespace Engine
{
	namespace UI
	{
		Color GetSystemColor(SystemColor color)
		{
			NSColor * clr = 0;
			if (color == SystemColor::Theme) clr = [NSColor controlAccentColor];
			else if (color == SystemColor::WindowBackgroup) clr = [NSColor windowBackgroundColor];
			else if (color == SystemColor::WindowText) clr = [NSColor textColor];
			else if (color == SystemColor::SelectedBackground) clr = [NSColor selectedTextBackgroundColor];
			else if (color == SystemColor::SelectedText) clr = [NSColor selectedTextColor];
			else if (color == SystemColor::MenuBackground) clr = [NSColor clearColor];
			else if (color == SystemColor::MenuText) clr = [NSColor textColor];
			else if (color == SystemColor::MenuHotBackground) clr = [NSColor selectedContentBackgroundColor];
			else if (color == SystemColor::MenuHotText) clr = [NSColor selectedMenuItemTextColor];
			else if (color == SystemColor::GrayedText) clr = [NSColor disabledControlTextColor];
			else if (color == SystemColor::Hyperlink) clr = [NSColor linkColor];
			else clr = [NSColor clearColor];
			NSColor * conv = [clr colorUsingColorSpace: [NSColorSpace deviceRGBColorSpace]];
			double r, g, b, a;
			[conv getRed: &r green: &g blue: &b alpha: &a];
			[conv release];
			return Color(uint8(r * 255.0), uint8(g * 255.0), uint8(b * 255.0), uint8(a * 255.0));
		}
	}
}