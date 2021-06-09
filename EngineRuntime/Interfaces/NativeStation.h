#pragma once

#include "../UserInterface/ControlBase.h"
#include "../UserInterface/Templates.h"
#include "../UserInterface/OverlappedWindows.h"
#include "../UserInterface/Menus.h"

namespace Engine
{
	namespace NativeWindows
	{
		void InitializeWindowSystem(void);
		void InitializeCodecCollection(void);
		Graphics::IDeviceFactory * CreateDeviceFactory(void);
		Graphics::IDevice * GetCommonDevice(void);
		UI::IObjectFactory * CreateObjectFactory(void);
		UI::WindowStation * CreateOverlappedWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation, bool NoDevice = false);
		UI::WindowStation * CreatePopupWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation);
		void ShowWindow(UI::WindowStation * Station, bool Show);
		void EnableWindow(UI::WindowStation * Station, bool Enable);
		void SetWindowTitle(UI::WindowStation * Station, const string & Title);
		void SetWindowPosition(UI::WindowStation * Station, const UI::Box & position);
		void ActivateWindow(UI::WindowStation * Station);
		void MaximizeWindow(UI::WindowStation * Station);
		void MinimizeWindow(UI::WindowStation * Station);
		void RestoreWindow(UI::WindowStation * Station);
		void RequestForAttention(UI::WindowStation * Station);
		bool IsWindowVisible(UI::WindowStation * Station);
		bool IsWindowEnabled(UI::WindowStation * Station);
		string GetWindowTitle(UI::WindowStation * Station);
		UI::Box GetWindowPosition(UI::WindowStation * Station);
		bool IsWindowActive(UI::WindowStation * Station);
		bool IsWindowMinimized(UI::WindowStation * Station);
		bool IsWindowMaximized(UI::WindowStation * Station);
		int RunMenuPopup(UI::Menus::Menu * menu, UI::Window * owner, UI::Point at);
		UI::Box GetScreenDimensions(void);
		double GetScreenScale(void);
		void RunMainMessageLoop(void);
		void ExitMainLoop(void);
		Array<string> * GetFontFamilies(void);
		void SetApplicationIcon(Codec::Image * icon);
		Codec::Frame * CaptureScreenState(void);
	}
}