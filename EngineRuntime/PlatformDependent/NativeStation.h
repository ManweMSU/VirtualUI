#pragma once

#include "../UserInterface/ControlBase.h"
#include "../UserInterface/Templates.h"
#include "../UserInterface/OverlappedWindows.h"
#include "../UserInterface/Menues.h"
#include "../UserInterface/Canvas.h"

namespace Engine
{
	namespace NativeWindows
	{
		void InitializeWindowSystem(void);
		void InitializeCodecCollection(void);
		UI::IResourceLoader * CreateCompatibleResourceLoader(void);
		Drawing::ITextureRenderingDevice * CreateCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color);
		UI::WindowStation * CreateOverlappedWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation);
		UI::WindowStation * CreatePopupWindow(UI::Template::ControlTemplate * Template, const UI::Rectangle & Position, UI::WindowStation * ParentStation);
		void ShowWindow(UI::WindowStation * Station, bool Show);
		void EnableWindow(UI::WindowStation * Station, bool Enable);
		void SetWindowTitle(UI::WindowStation * Station, const string & Title);
		void SetWindowPosition(UI::WindowStation * Station, const UI::Box & position);
		bool IsWindowVisible(UI::WindowStation * Station);
		bool IsWindowEnabled(UI::WindowStation * Station);
		string GetWindowTitle(UI::WindowStation * Station);
		UI::Box GetWindowPosition(UI::WindowStation * Station);
		int RunMenuPopup(UI::Menues::Menu * menu, UI::Window * owner, UI::Point at);
		UI::Box GetScreenDimensions(void);
		double GetScreenScale(void);
		void RunMainMessageLoop(void);
		void ExitMainLoop(void);
		Array<string> * GetFontFamilies(void);
	}
}