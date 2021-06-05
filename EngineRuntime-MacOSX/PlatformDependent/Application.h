#pragma once

#include "../UserInterface/ControlBase.h"
#include "../UserInterface/OverlappedWindows.h"

namespace Engine
{
	namespace Application
	{
		enum class MessageBoxButtonSet { Ok, OkCancel, YesNo, YesNoCancel };
		enum class MessageBoxStyle { Error, Warning, Information };
		enum class MessageBoxResult { Ok, Cancel, Yes, No };
		enum class ApplicationAbility { CreateFiles, OpenFiles, ShowHelp, ShowProperties };
		struct FileFormat
		{
			string Description;
			Array<string> Extensions;

			FileFormat(void);
		};
		struct OpenFileInfo
		{
			bool MultiChoose = false;
			Array<FileFormat> Formats;
			int DefaultFormat = -1;
			string Title;
			Array<string> Files;

			OpenFileInfo(void);
		};
		struct SaveFileInfo
		{
			Array<FileFormat> Formats;
			int Format = -1;
			string Title;
			string File;
			bool AppendExtension = true;

			SaveFileInfo(void);
		};
		struct ChooseDirectoryInfo
		{
			string Title;
			string Directory;

			ChooseDirectoryInfo(void);
		};
		class IApplicationCallback
		{
		public:
			virtual bool GetAbility(ApplicationAbility ability);
			virtual void CreateNewFile(void);
			virtual void OpenSomeFile(void);
			virtual bool OpenExactFile(const string & path);
			virtual void InvokeHelp(void);
			virtual void ShowProperties(void);
		};
		class ApplicationController : public Object
		{
		public:
			virtual UI::Controls::OverlappedWindow * CreateWindow(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, const UI::Rectangle & Position) = 0;
			virtual UI::Controls::OverlappedWindow * CreateWindow(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, const UI::Rectangle & Position, UI::Window * Parent) = 0;
			virtual UI::Controls::OverlappedWindow * CreateModalWindow(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, const UI::Rectangle & Position, UI::Window * Parent) = 0;
			virtual void SystemOpenFileDialog(OpenFileInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) = 0;
			virtual void SystemSaveFileDialog(SaveFileInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) = 0;
			virtual void SystemChooseDirectoryDialog(ChooseDirectoryInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) = 0;
			virtual void SystemMessageBox(MessageBoxResult * Result, const string & Text, const string & Title, UI::Window * Parent, MessageBoxButtonSet Buttons, MessageBoxStyle Style, IDispatchTask * OnExit) = 0;
			virtual void SetCallback(IApplicationCallback * callback) = 0;
			virtual IApplicationCallback * GetCallback(void) = 0;
			virtual void RunApplication(void) = 0;
			virtual void ExitApplication(void) = 0;
			virtual void RegisterMainWindow(UI::Window * window) = 0;
			virtual void UnregisterMainWindow(UI::Window * window) = 0;
			virtual void ExitModalSession(UI::Window * modal) = 0;
		};
		ApplicationController * CreateController(IApplicationCallback * callback);
		ApplicationController * CreateController(IApplicationCallback * callback, const Array<string> & files_open);
		ApplicationController * GetController(void);
	}
}