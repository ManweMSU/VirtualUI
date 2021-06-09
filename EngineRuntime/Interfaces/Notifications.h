#pragma once

#include "../UserInterface/OverlappedWindows.h"
#include "../UserInterface/Menus.h"

// ICON SIZES VARIANTS FOR STATUS ICON
// WINDOWS : 16x16, 24x24, 32x32
// MACOSX  : 20x20, 40x40
//
// ICON SIZES VARIANTS FOR NOTIFICATION ICON
// WINDOWS : 32x32, 48x48, 64x64
// MACOSX  : CUSTOMIZATION IS NOT SUPPORTED

namespace Engine
{
	namespace UI
	{
		namespace Windows
		{
			enum class StatusBarIconColorUsage { Colourfull, Monochromic };
			class StatusBarIcon : public Object
			{
			public:
				virtual void SetCallback(IWindowEventCallback * callback) = 0;
				virtual IWindowEventCallback * GetCallback(void) = 0;
				virtual void SetIcon(Codec::Image * image) = 0;
				virtual Codec::Image * GetIcon(void) = 0;
				virtual void SetIconColorUsage(StatusBarIconColorUsage color_usage) = 0;
				virtual StatusBarIconColorUsage GetIconColorUsage(void) = 0;
				virtual void SetTooltip(const string & text) = 0;
				virtual string GetTooltip(void) = 0;
				virtual void SetEventID(int ID) = 0;
				virtual int GetEventID(void) = 0;
				virtual void SetMenu(Menus::Menu * menu) = 0;
				virtual Menus::Menu * GetMenu(void) = 0;
				virtual void PresentIcon(bool present) = 0;
				virtual bool IsVisible(void) = 0;
			};
			StatusBarIcon * CreateStatusBarIcon(void);
			void PushUserNotification(const string & title, const string & text, Codec::Image * icon = 0);
		}
	}
}