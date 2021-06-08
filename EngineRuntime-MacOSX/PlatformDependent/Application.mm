#include "../Interfaces/Application.h"

#include "../Miscellaneous/DynamicString.h"
#include "NativeStation.h"
#include "NativeStationBackdoors.h"
#include "CocoaInterop.h"
#include "../Interfaces/Assembly.h"

@interface EngineRuntimeOpenSaveDelegate : NSObject<NSOpenSavePanelDelegate>
{
@public
	NSSavePanel * panel;
	Engine::Array<Engine::Application::FileFormat> * formats;
	int * selected;
	bool save;
}
- (void) format_changed: (id) sender;
@end
@implementation EngineRuntimeOpenSaveDelegate
- (void) format_changed: (id) sender
{
	if (save) {
		int index = [(NSPopUpButton *) [panel accessoryView] indexOfSelectedItem];
		NSMutableArray<NSString *> * exts = [[NSMutableArray<NSString *> alloc] init];
		for (int j = 0; j < formats->ElementAt(index).Extensions.Length(); j++) {
			NSString * ext = Engine::Cocoa::CocoaString(formats->ElementAt(index).Extensions[j]);
			[exts addObject: ext];
			[ext release];
		}
		[panel setAllowedFileTypes: exts];
		[exts release];
	} else {
		int index = [(NSPopUpButton *) [panel accessoryView] indexOfSelectedItem];
		if (index > formats->Length()) {
			[panel setAllowedFileTypes: nil];
		} else if (index <= 0) {
			NSMutableArray<NSString *> * exts = [[NSMutableArray<NSString *> alloc] init];
			for (int i = 0; i < formats->Length(); i++) for (int j = 0; j < formats->ElementAt(i).Extensions.Length(); j++) {
				NSString * ext = Engine::Cocoa::CocoaString(formats->ElementAt(i).Extensions[j]);
				[exts addObject: ext];
				[ext release];
			}
			[panel setAllowedFileTypes: exts];
			[exts release];
		} else {
			NSMutableArray<NSString *> * exts = [[NSMutableArray<NSString *> alloc] init];
			for (int j = 0; j < formats->ElementAt(index - 1).Extensions.Length(); j++) {
				NSString * ext = Engine::Cocoa::CocoaString(formats->ElementAt(index - 1).Extensions[j]);
				[exts addObject: ext];
				[ext release];
			}
			[panel setAllowedFileTypes: exts];
			[exts release];
		}
	}
}
@end

namespace Engine
{
	namespace Application
	{
		bool IApplicationCallback::GetAbility(ApplicationAbility ability) { return false; }
		void IApplicationCallback::CreateNewFile(void) {}
		void IApplicationCallback::OpenSomeFile(void) {}
		bool IApplicationCallback::OpenExactFile(const string & path) { return false; }
		void IApplicationCallback::InvokeHelp(void) {}
		void IApplicationCallback::ShowProperties(void) {}
		FileFormat::FileFormat(void) : Extensions(0x4) {}
		OpenFileInfo::OpenFileInfo(void) : Formats(0x8), Files(0x8) {}
		SaveFileInfo::SaveFileInfo(void) : Formats(0x8) {}
		ChooseDirectoryInfo::ChooseDirectoryInfo(void) {}
		ApplicationController * _Controller = 0;
		class MacOSXApplicationController : public ApplicationController
		{
			friend void ApplicationLaunched(void);
		public:
			IApplicationCallback * _callback;
			Array<UI::Window *> _main_windows;
			Array<string> _deferred_open;
		public:
			MacOSXApplicationController(IApplicationCallback * callback, const Array<string>& files_open) : _callback(callback), _main_windows(0x10), _deferred_open(0x10)
			{
				_Controller = this;
				NativeWindows::InitializeWindowSystem();
				_deferred_open << files_open;
				if (callback) {
					NSMenu * menu = [NSApp mainMenu];
					if (callback->GetAbility(ApplicationAbility::CreateFiles) || callback->GetAbility(ApplicationAbility::OpenFiles)) {
						NSString * file_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(313, L"File"));
						NSString * new_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(314, L"New file"));
						NSString * open_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(315, L"Open file"));
						NSString * close_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(316, L"Close window"));

						NSMenuItem * file_menu_item = [[NSMenuItem alloc] initWithTitle: file_str action: NULL keyEquivalent: @""];
						NSMenu * file_menu = [[NSMenu alloc] initWithTitle: file_str];
						[file_menu_item setSubmenu: file_menu];
						if (callback->GetAbility(ApplicationAbility::CreateFiles)) {
							NSMenuItem * create = [[NSMenuItem alloc] initWithTitle: new_str action: @selector(new_file:) keyEquivalent: @"n"];
							[file_menu addItem: create];
							[create release];
						}
						if (callback->GetAbility(ApplicationAbility::OpenFiles)) {
							NSMenuItem * open = [[NSMenuItem alloc] initWithTitle: open_str action: @selector(open_file:) keyEquivalent: @"o"];
							[file_menu addItem: open];
							[open release];
						}
						NSMenuItem * sep = [NSMenuItem separatorItem];
						[file_menu addItem: sep];
						[sep release];
						NSMenuItem * close = [[NSMenuItem alloc] initWithTitle: close_str action: @selector(performClose:) keyEquivalent: @"w"];
						[file_menu addItem: close];
						[close release];
						[menu insertItem: file_menu_item atIndex: 1];
						[file_menu_item release];
						[file_menu release];

						[file_str release];
						[new_str release];
						[open_str release];
						[close_str release];
					}
					if (callback->GetAbility(ApplicationAbility::ShowProperties)) {
						NSMenu * main = [[menu itemAtIndex: 0] submenu];
						NSString * prop_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(317, L"Preferences"));

						NSMenuItem * sep = [NSMenuItem separatorItem];
						[main insertItem: sep atIndex: 1];
						[sep release];
						NSMenuItem * props = [[NSMenuItem alloc] initWithTitle: prop_str action: @selector(show_props:) keyEquivalent: @","];
						[main insertItem: props atIndex: 2];
						[props release];
						[prop_str release];
					}
					if (callback->GetAbility(ApplicationAbility::ShowHelp)) {
						NSMenu * help = [NSApp helpMenu];
						NSString * docs_str = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(318, L"Show Help"));
						NSMenuItem * item = [[NSMenuItem alloc] initWithTitle: docs_str action: @selector(show_help:) keyEquivalent: @""];
						[help addItem: item];
						[item release];
						[docs_str release];
					}
				}
			}
			~MacOSXApplicationController(void) override {}

			virtual UI::Controls::OverlappedWindow * CreateWindow(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, const UI::Rectangle & Position) override
			{
				return UI::Windows::CreateFramedDialog(Template, Callback, Position, 0);
			}
			virtual UI::Controls::OverlappedWindow * CreateWindow(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, const UI::Rectangle & Position, UI::Window * Parent) override
			{
				return UI::Windows::CreateFramedDialog(Template, Callback, Position, Parent->GetStation());
			}
			virtual UI::Controls::OverlappedWindow * CreateModalWindow(UI::Template::ControlTemplate * Template, UI::Windows::IWindowEventCallback * Callback, const UI::Rectangle & Position, UI::Window * Parent) override
			{
				UI::Box superbox = Parent ? Parent->GetPosition() : UI::Windows::GetScreenDimensions();
				if (Position.IsValid()) superbox = UI::Box(Position, superbox);
				UI::Rectangle superrect(superbox.Left, superbox.Top, superbox.Right, superbox.Bottom);
				UI::Controls::OverlappedWindow * dialog = UI::Windows::CreateFramedDialog(Template, Callback, superrect, 0);
				if (Parent) {
					NSWindow * parent = NativeWindows::GetWindowObject(Parent->GetStation());
					NSWindow * sheet = NativeWindows::GetWindowObject(dialog->GetStation());
					[parent beginSheet: sheet completionHandler: nil];
					return dialog;
				} else {
					dialog->Show(true);
					[NSApp runModalForWindow: NativeWindows::GetWindowObject(dialog->GetStation())];
					dialog->Destroy();
					return 0;
				}
			}
			virtual void SystemOpenFileDialog(OpenFileInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				NSOpenPanel * panel = [NSOpenPanel openPanel];
				EngineRuntimeOpenSaveDelegate * delegate = [[EngineRuntimeOpenSaveDelegate alloc] init];
				delegate->panel = panel;
				delegate->formats = &Info->Formats;
				delegate->selected = &Info->DefaultFormat;
				delegate->save = false;
				[panel setCanChooseFiles: YES];
				[panel setCanChooseDirectories: NO];
				[panel setAllowsMultipleSelection: Info->MultiChoose ? YES : NO];
				[panel setCanCreateDirectories: YES];
				[panel setAllowsOtherFileTypes: YES];
				[panel setDelegate: delegate];
				if (Info->Formats.Length()) {
					NSPopUpButton * view = [[NSPopUpButton alloc] initWithFrame: NSMakeRect(10.0, 0.0, 400.0, 26.0) pullsDown: NO];
					auto & formats = Info->Formats;
					if (formats.Length() > 1) {
						DynamicString exts;
						Array<string> was(0x10);
						for (int i = 0; i < formats.Length(); i++) for (int j = 0; j < formats[i].Extensions.Length(); j++) {
							auto & ext = formats[i].Extensions[j];
							bool f_was = false;
							for (int k = 0; k < was.Length(); k++) if (string::CompareIgnoreCase(was[k], ext) == 0) { f_was = true; break; }
							if (f_was) continue;
							was << ext;
							if (i || j) exts += L";";
							exts += L"*.";
							exts += ext;
						}
						auto str = string(Assembly::GetLocalizedCommonString(201, L"All supported")) + L" (" + exts.ToString() + L")";
						NSString * ns = Engine::Cocoa::CocoaString(str);
						[view addItemWithTitle: ns];
						[ns release];
					}
					for (int i = 0; i < formats.Length(); i++) {
						DynamicString exts;
					 	for (int j = 0; j < formats[i].Extensions.Length(); j++) {
					 		auto & ext = formats[i].Extensions[j];
					 		if (j) exts += L";";
					 		exts += L"*.";
					 		exts += ext;
					 	}
						auto str = formats[i].Description + L" (" + exts.ToString() + L")";
						NSString * ns = Engine::Cocoa::CocoaString(str);
						[view addItemWithTitle: ns];
						[ns release];
					}
					NSString * all = Cocoa::CocoaString(string(Assembly::GetLocalizedCommonString(202, L"All files")) + L" (*.*)");
					[view addItemWithTitle: all];
					[all release];
					[view selectItemAtIndex: 1 + Info->DefaultFormat];
					[view setAction: @selector(format_changed:)];
					[view setTarget: delegate];
					NSMutableArray<NSString *> * exts = [[NSMutableArray<NSString *> alloc] init];
					if (Info->DefaultFormat == -1) {
						for (int i = 0; i < formats.Length(); i++) for (int j = 0; j < formats[i].Extensions.Length(); j++) {
							NSString * ext = Engine::Cocoa::CocoaString(formats[i].Extensions[j]);
							[exts addObject: ext];
							[ext release];
						}
					} else {
						for (int j = 0; j < formats[Info->DefaultFormat].Extensions.Length(); j++) {
							NSString * ext = Engine::Cocoa::CocoaString(formats[Info->DefaultFormat].Extensions[j]);
							[exts addObject: ext];
							[ext release];
						}
					}
					[panel setAllowedFileTypes: exts];
					[exts release];
					[panel setAccessoryView: view];
					[view release];
				}
				if (Info->Title.Length()) {
					NSString * title = Engine::Cocoa::CocoaString(Info->Title);
					[panel setTitle: title];
					[title release];
				}
				void (^handler) (NSModalResponse) = ^(NSModalResponse responce)
				{
					if (responce == NSModalResponseOK) {
						Info->Files.Clear();
						NSArray<NSURL *> * urls = [panel URLs];
						for (int i = 0; i < [urls count]; i++) {
							NSURL * url = [urls objectAtIndex: i];
							Info->Files << Engine::Cocoa::EngineString([url path]);
						}
					} else {
						Info->Files.Clear();
					}
					[panel orderOut: nil];
					[delegate release];
					if (OnExit) {
						if (Parent) {
							Parent->GetStation()->AppendTask(OnExit);
						} else {
							OnExit->DoTask(0);
						}
						OnExit->Release();
					}
				};
				if (Parent) {
					[panel beginSheetModalForWindow: NativeWindows::GetWindowObject(Parent->GetStation()) completionHandler: handler];
				} else {
					handler([panel runModal]);
				}
			}
			virtual void SystemSaveFileDialog(SaveFileInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				NSSavePanel * panel = [NSSavePanel savePanel];
				EngineRuntimeOpenSaveDelegate * delegate = [[EngineRuntimeOpenSaveDelegate alloc] init];
				delegate->panel = panel;
				delegate->formats = &Info->Formats;
				delegate->selected = &Info->Format;
				delegate->save = true;
				[panel setCanCreateDirectories: YES];
				[panel setAllowsOtherFileTypes: Info->AppendExtension ? NO : YES];
				[panel setDelegate: delegate];
				NSPopUpButton * view = 0;
				if (Info->Formats.Length()) {
					view = [[NSPopUpButton alloc] initWithFrame: NSMakeRect(10.0, 0.0, 300.0, 26.0) pullsDown: NO];
					auto & formats = Info->Formats;
					for (int i = 0; i < formats.Length(); i++) {
						DynamicString exts;
					 	for (int j = 0; j < formats[i].Extensions.Length(); j++) {
					 		auto & ext = formats[i].Extensions[j];
					 		if (j) exts += L";";
					 		exts += L"*.";
					 		exts += ext;
					 	}
						auto str = formats[i].Description + L" (" + exts.ToString() + L")";
						NSString * ns = Engine::Cocoa::CocoaString(str);
						[view addItemWithTitle: ns];
						[ns release];
					}
					int index = min(max(Info->Format, 0), formats.Length() - 1);
					[view selectItemAtIndex: index];
					[view setAction: @selector(format_changed:)];
					[view setTarget: delegate];
					NSMutableArray<NSString *> * exts = [[NSMutableArray<NSString *> alloc] init];
					for (int j = 0; j < formats[index].Extensions.Length(); j++) {
						NSString * ext = Engine::Cocoa::CocoaString(formats[index].Extensions[j]);
						[exts addObject: ext];
						[ext release];
					}
					[panel setAllowedFileTypes: exts];
					[exts release];
					[panel setAccessoryView: view];
					[view release];
				}
				if (Info->Title.Length()) {
					NSString * title = Engine::Cocoa::CocoaString(Info->Title);
					[panel setTitle: title];
					[title release];
				}
				void (^handler) (NSModalResponse) = ^(NSModalResponse responce)
				{
					if (responce == NSModalResponseOK) {
						Info->File = Engine::Cocoa::EngineString([[panel URL] path]);
						Info->Format = view ? [view indexOfSelectedItem] : 0;
					} else {
						Info->File = L"";
					}
					[panel orderOut: nil];
					[delegate release];
					if (OnExit) {
						if (Parent) {
							Parent->GetStation()->AppendTask(OnExit);
						} else {
							OnExit->DoTask(0);
						}
						OnExit->Release();
					}
				};
				if (Parent) {
					[panel beginSheetModalForWindow: NativeWindows::GetWindowObject(Parent->GetStation()) completionHandler: handler];
				} else {
					handler([panel runModal]);
				}
			}
			virtual void SystemChooseDirectoryDialog(ChooseDirectoryInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				NSOpenPanel * panel = [NSOpenPanel openPanel];
				[panel setCanChooseFiles: NO];
				[panel setCanChooseDirectories: YES];
				[panel setAllowsMultipleSelection: NO];
				[panel setCanCreateDirectories: YES];
				if (Info->Title.Length()) {
					NSString * title = Engine::Cocoa::CocoaString(Info->Title);
					[panel setTitle: title];
					[title release];
				}
				void (^handler) (NSModalResponse) = ^(NSModalResponse responce)
				{
					if (responce == NSModalResponseOK) {
						Info->Directory = Engine::Cocoa::EngineString([[panel URL] path]);
					} else {
						Info->Directory = L"";
					}
					[panel orderOut: nil];
					if (OnExit) {
						if (Parent) {
							Parent->GetStation()->AppendTask(OnExit);
						} else {
							OnExit->DoTask(0);
						}
						OnExit->Release();
					}
				};
				if (Parent) {
					[panel beginSheetModalForWindow: NativeWindows::GetWindowObject(Parent->GetStation()) completionHandler: handler];
				} else {
					handler([panel runModal]);
				}
			}
			virtual void SystemMessageBox(MessageBoxResult * Result, const string & Text, const string & Title, UI::Window * Parent, MessageBoxButtonSet Buttons, MessageBoxStyle Style, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				NSAlert * alert = [[NSAlert alloc] init];
				NSString * text = Engine::Cocoa::CocoaString(Text);
				NSString * title = Engine::Cocoa::CocoaString(Title);
				[alert setMessageText: title];
				[alert setInformativeText: text];
				[text release];
				[title release];
				if (Buttons == MessageBoxButtonSet::Ok) {
					NSString * text_1 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(101, L"OK"));
					[alert addButtonWithTitle: text_1];
					[text_1 release];
				} else if (Buttons == MessageBoxButtonSet::OkCancel) {
					NSString * text_1 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(101, L"OK"));
					NSString * text_2 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(102, L"Cancel"));
					[alert addButtonWithTitle: text_2];
					[alert addButtonWithTitle: text_1];
					[text_1 release];
					[text_2 release];
				} else if (Buttons == MessageBoxButtonSet::YesNo) {
					NSString * text_1 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(103, L"Yes"));
					NSString * text_2 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(104, L"No"));
					[alert addButtonWithTitle: text_2];
					[alert addButtonWithTitle: text_1];
					[text_1 release];
					[text_2 release];
				} else if (Buttons == MessageBoxButtonSet::YesNoCancel) {
					NSString * text_0 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(102, L"Cancel"));
					NSString * text_1 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(103, L"Yes"));
					NSString * text_2 = Cocoa::CocoaString(Assembly::GetLocalizedCommonString(104, L"No"));
					[alert addButtonWithTitle: text_0];
					[alert addButtonWithTitle: text_2];
					[alert addButtonWithTitle: text_1];
					[text_0 release];
					[text_1 release];
					[text_2 release];
				}
				if (Style == MessageBoxStyle::Error) [alert setAlertStyle: NSAlertStyleCritical];
				else if (Style == MessageBoxStyle::Warning) [alert setAlertStyle: NSAlertStyleWarning];
				else if (Style == MessageBoxStyle::Information) [alert setAlertStyle: NSAlertStyleInformational];
				void (^handler) (NSModalResponse) = ^(NSModalResponse responce)
				{
					if (Result) {
						if (Buttons == MessageBoxButtonSet::Ok) {
							*Result = MessageBoxResult::Ok;
						} else if (Buttons == MessageBoxButtonSet::OkCancel) {
							*Result = MessageBoxResult::Ok;
							if (responce == NSAlertFirstButtonReturn) *Result = MessageBoxResult::Cancel;
						} else if (Buttons == MessageBoxButtonSet::YesNo) {
							*Result = MessageBoxResult::Yes;
							if (responce == NSAlertFirstButtonReturn) *Result = MessageBoxResult::No;
						} else if (Buttons == MessageBoxButtonSet::YesNoCancel) {
							*Result = MessageBoxResult::Yes;
							if (responce == NSAlertSecondButtonReturn) *Result = MessageBoxResult::No;
							else if (responce == NSAlertFirstButtonReturn) *Result = MessageBoxResult::Cancel;
						}
					}
					[alert release];
					if (OnExit) {
						if (Parent) {
							Parent->GetStation()->AppendTask(OnExit);
						} else {
							OnExit->DoTask(0);
						}
						OnExit->Release();
					}
				};
				if (Parent) {
					[alert beginSheetModalForWindow: NativeWindows::GetWindowObject(Parent->GetStation()) completionHandler: handler];
				} else {
					handler([alert runModal]);
				}
			}
			virtual void SetCallback(IApplicationCallback * callback) override { _callback = callback; }
			virtual IApplicationCallback * GetCallback(void) override { return _callback; }
			virtual void RunApplication(void) override { UI::Windows::RunMessageLoop(); }
			virtual void ExitApplication(void) override { UI::Windows::ExitMessageLoop(); }
			virtual void RegisterMainWindow(UI::Window * window) override { _main_windows << window; }
			virtual void UnregisterMainWindow(UI::Window * window) override
			{
				for (int i = 0; i < _main_windows.Length(); i++) if (_main_windows[i] == window) {
					_main_windows.Remove(i);
					break;
				}
				if (!_main_windows.Length()) ExitApplication();
			}
			virtual void ExitModalSession(UI::Window * modal) override
			{
				NSWindow * sheet = NativeWindows::GetWindowObject(modal->GetStation());
				NSWindow * parent = [sheet sheetParent];
				if (parent) {
					[parent endSheet: sheet];
					modal->Destroy();
				} else {
					[NSApp stopModal];
				}
			}
		};
		void ApplicationLaunched(void)
		{
			auto controller = static_cast<MacOSXApplicationController *>(GetController());
			if (controller && controller->_callback) {
				for (int i = 0; i < controller->_deferred_open.Length(); i++) if (controller->_deferred_open[i].Fragment(0, 5) != L"-psn_") {
					controller->_callback->OpenExactFile(controller->_deferred_open[i]);
				}
				controller->_deferred_open.Clear();
			}
		}
		ApplicationController * CreateController(IApplicationCallback * callback)
		{
			if (_Controller) return _Controller;
			Array<string> files(1);
			new MacOSXApplicationController(callback, files);
			return _Controller;
		}
		ApplicationController * CreateController(IApplicationCallback * callback, const Array<string>& files_open)
		{
			if (_Controller) return _Controller;
			new MacOSXApplicationController(callback, files_open);
			return _Controller;
		}
		ApplicationController * GetController(void) { return _Controller; }
	}
}