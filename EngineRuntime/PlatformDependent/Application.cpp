#include "Application.h"

#include "WindowStation.h"
#include "NativeStation.h"
#include "../Miscellaneous/DynamicString.h"
#include "Assembly.h"

#include <ObjIdl.h>
#include <ShlObj.h>

#undef CreateWindow

#define BEGIN_GLOBAL_MODAL_SESSION(hwnd_for) \
::Engine::Array<HWND> enable(0x10); \
HWND prev = 0; \
DWORD pid = GetCurrentProcessId(); \
do { \
	prev = FindWindowExW(0, prev, 0, 0); \
	if (prev) { \
		DWORD wpid; \
		GetWindowThreadProcessId(prev, &wpid); \
		if (wpid == pid && prev != hwnd_for && ::IsWindowEnabled(prev)) { \
			enable << prev; \
			::EnableWindow(prev, FALSE); \
		} \
	} \
} while (prev);
#define END_GLOBAL_MODAL_SESSION \
for (int i = 0; i < enable.Length(); i++) ::EnableWindow(enable[i], TRUE);

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
		class WindowsApplicationController : public ApplicationController
		{
		private:
			IApplicationCallback * _callback;
			Array<UI::Window *> _main_windows;
			struct OpenFileStruct
			{
				OpenFileInfo * Info;
				HWND Owner;
				UI::WindowStation * Context;
				IDispatchTask * Task;
			};
			struct SaveFileStruct
			{
				SaveFileInfo * Info;
				HWND Owner;
				UI::WindowStation * Context;
				IDispatchTask * Task;
			};
			struct ChooseDirectoryStruct
			{
				ChooseDirectoryInfo * Info;
				HWND Owner;
				UI::WindowStation * Context;
				IDispatchTask * Task;
			};
			struct MessageBoxStruct
			{
				MessageBoxResult * Result;
				string Text;
				string Title;
				MessageBoxButtonSet Buttons;
				MessageBoxStyle Style;
				HWND Owner;
				UI::WindowStation * Context;
				IDispatchTask * Task;
			};
			static int OpenFileProc(void * argument)
			{
				OpenFileStruct * data = reinterpret_cast<OpenFileStruct *>(argument);
				OPENFILENAMEW ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
				if (data->Info->MultiChoose) ofn.Flags |= OFN_ALLOWMULTISELECT;
				Array<widechar> filter(0x100);
				DynamicString result;
				result.ReserveLength(0x10000);
				if (data->Info->Formats.Length()) {
					auto & formats = data->Info->Formats;
					if (formats.Length() > 1) {
						Array<widechar> exts(0x100);
						Array<string> was(0x10);
						for (int i = 0; i < formats.Length(); i++) for (int j = 0; j < formats[i].Extensions.Length(); j++) {
							auto & ext = formats[i].Extensions[j];
							bool f_was = false;
							for (int k = 0; k < was.Length(); k++) if (string::CompareIgnoreCase(was[k], ext) == 0) { f_was = true; break; }
							if (f_was) continue;
							was << ext;
							if (i || j) exts.Append(L";", 1);
							exts.Append(L"*.", 2);
							exts.Append(ext, ext.Length());
						}
						string all_supp = string(Assembly::GetLocalizedCommonString(201, L"All supported")) + L" (";
						filter.Append(all_supp, all_supp.Length());
						filter << exts;
						filter.Append(L")\0", 2);
						filter << exts;
						filter.Append(L"\0", 1);
					}
					for (int i = 0; i < formats.Length(); i++) {
						Array<widechar> exts(0x20);
						for (int j = 0; j < formats[i].Extensions.Length(); j++) {
							auto & ext = formats[i].Extensions[j];
							if (j) exts.Append(L";", 1);
							exts.Append(L"*.", 2);
							exts.Append(ext, ext.Length());
						}
						filter.Append(formats[i].Description, formats[i].Description.Length());
						filter.Append(L" (", 2);
						filter << exts;
						filter.Append(L")\0", 2);
						filter << exts;
						filter.Append(L"\0", 1);
					}
					string all_files = string(Assembly::GetLocalizedCommonString(202, L"All files")) + L" (*.*)";
					filter.Append(all_files, all_files.Length() + 1);
					filter.Append(L"*.*\0\0", 5);
				} else {
					string all_files = string(Assembly::GetLocalizedCommonString(202, L"All files")) + L" (*.*)";
					filter.Append(all_files, all_files.Length() + 1);
					filter.Append(L"*.*\0\0", 5);
				}
				ofn.hwndOwner = data->Owner;
				ofn.lpstrFile = result;
				ofn.nMaxFile = result.ReservedLength();
				ofn.nFilterIndex = data->Info->Formats.Length() ? (1 + data->Info->DefaultFormat) : 1;
				ofn.lpstrFilter = filter;
				if (data->Info->Title.Length()) ofn.lpstrTitle = data->Info->Title;
				if (GetOpenFileNameW(&ofn)) {
					data->Info->Files.Clear();
					if (data->Info->MultiChoose) {
						Array<string> extract(0x10);
						int sp = 0, ep = 0;
						while (true) {
							while (result[ep]) ep++;
							if (ep == sp) break;
							extract << string(result.GetBuffer() + sp);
							sp = ep + 1;
							ep = sp;
						}
						if (extract.Length() == 1) {
							data->Info->Files << extract.FirstElement();
						} else if (extract.Length() > 1) {
							for (int i = 1; i < extract.Length(); i++) data->Info->Files << extract.FirstElement() + string(IO::PathChar) + extract[i];
						}
					} else data->Info->Files << result;
				} else {
					data->Info->Files.Clear();
				}
				if (data->Task) {
					if (data->Context) data->Context->AppendTask(data->Task);
					else data->Task->DoTask(0);
					data->Task->Release();
				}
				delete data;
				return 0;
			}
			static int SaveFileProc(void * argument)
			{
				SaveFileStruct * data = reinterpret_cast<SaveFileStruct *>(argument);
				OPENFILENAMEW ofn;
				ZeroMemory(&ofn, sizeof(ofn));
				ofn.lStructSize = sizeof(ofn);
				ofn.Flags = OFN_OVERWRITEPROMPT | OFN_EXPLORER;
				Array<widechar> filter(0x100);
				DynamicString result;
				result.ReserveLength(0x10000);
				if (data->Info->Formats.Length()) {
					auto & formats = data->Info->Formats;
					for (int i = 0; i < formats.Length(); i++) {
						Array<widechar> exts(0x20);
						for (int j = 0; j < formats[i].Extensions.Length(); j++) {
							auto & ext = formats[i].Extensions[j];
							if (j) exts.Append(L";", 1);
							exts.Append(L"*.", 2);
							exts.Append(ext, ext.Length());
						}
						filter.Append(formats[i].Description, formats[i].Description.Length());
						filter.Append(L" (", 2);
						filter << exts;
						filter.Append(L")\0", 2);
						filter << exts;
						filter.Append(L"\0", 1);
					}
					filter.Append(L"\0", 1);
				} else {
					string all_files = string(Assembly::GetLocalizedCommonString(202, L"All files")) + L" (*.*)";
					filter.Append(all_files, all_files.Length() + 1);
					filter.Append(L"*.*\0\0", 5);
				}
				ofn.hwndOwner = data->Owner;
				ofn.lpstrFile = result;
				ofn.nMaxFile = result.ReservedLength();
				ofn.nFilterIndex = data->Info->Formats.Length() ? (1 + data->Info->Format) : 1;
				ofn.lpstrFilter = filter;
				if (data->Info->Title.Length()) ofn.lpstrTitle = data->Info->Title;
				if (GetSaveFileNameW(&ofn)) {
					data->Info->File = result.ToString();
					data->Info->Format = ofn.nFilterIndex - 1;
					if (data->Info->AppendExtension && data->Info->Formats.Length()) {
						bool has_ext = false;
						for (int i = 0; i < data->Info->Formats[data->Info->Format].Extensions.Length(); i++) {
							if (string::CompareIgnoreCase(IO::Path::GetExtension(data->Info->File), data->Info->Formats[data->Info->Format].Extensions[i]) == 0) {
								has_ext = true;
								break;
							}
						}
						if (!has_ext) data->Info->File += L"." + data->Info->Formats[data->Info->Format].Extensions.FirstElement();
					}
				} else {
					data->Info->File = L"";
				}
				if (data->Task) {
					if (data->Context) data->Context->AppendTask(data->Task);
					else data->Task->DoTask(0);
					data->Task->Release();
				}
				delete data;
				return 0;
			}
			static int ChooseDirectoryProc(void * argument)
			{
				ChooseDirectoryStruct * data = reinterpret_cast<ChooseDirectoryStruct *>(argument);
				{
					data->Info->Directory = L"";
					SafePointer<IFileOpenDialog> dialog = 0;
					if (CoCreateInstance(CLSID_FileOpenDialog, 0, CLSCTX_INPROC_SERVER, __uuidof(IFileOpenDialog), reinterpret_cast<void **>(dialog.InnerRef())) == S_OK) {
						dialog->SetOptions(FOS_PICKFOLDERS | FOS_DONTADDTORECENT);
						if (data->Info->Title.Length()) dialog->SetTitle(data->Info->Title);
						if (dialog->Show(data->Owner) == S_OK) {
							IShellItem * item;
							if (dialog->GetResult(&item) == S_OK) {
								LPWSTR Path;
								item->GetDisplayName(SIGDN_FILESYSPATH, &Path);
								data->Info->Directory = Path;
								CoTaskMemFree(Path);
								item->Release();
							}
						}
					}
				}
				if (data->Task) {
					if (data->Context) data->Context->AppendTask(data->Task);
					else data->Task->DoTask(0);
					data->Task->Release();
				}
				delete data;
				return 0;
			}
			static int MessageBoxProc(void * argument)
			{
				MessageBoxStruct * data = reinterpret_cast<MessageBoxStruct *>(argument);
				if (data->Result) *data->Result = MessageBoxResult::Cancel;
				UINT type = 0;
				if (data->Buttons == MessageBoxButtonSet::Ok) type = MB_OK;
				else if (data->Buttons == MessageBoxButtonSet::OkCancel) type = MB_OKCANCEL;
				else if (data->Buttons == MessageBoxButtonSet::YesNo) type = MB_YESNO;
				else if (data->Buttons == MessageBoxButtonSet::YesNoCancel) type = MB_YESNOCANCEL;
				if (data->Style == MessageBoxStyle::Error) type |= MB_ICONSTOP;
				else if (data->Style == MessageBoxStyle::Warning) type |= MB_ICONEXCLAMATION;
				else if (data->Style == MessageBoxStyle::Information) type |= MB_ICONINFORMATION;
				int result = MessageBoxW(data->Owner, data->Text, data->Title, type);
				if (data->Result) {
					if (result == IDYES) *data->Result = MessageBoxResult::Yes;
					else if (result == IDNO) *data->Result = MessageBoxResult::No;
					else if (result == IDCANCEL) *data->Result = MessageBoxResult::Cancel;
					else if (result == IDOK) *data->Result = MessageBoxResult::Ok;
				}
				if (data->Task) {
					if (data->Context) data->Context->AppendTask(data->Task);
					else data->Task->DoTask(0);
					data->Task->Release();
				}
				delete data;
				return 0;
			}
		public:
			WindowsApplicationController(IApplicationCallback * callback, const Array<string>& files_open) : _callback(callback), _main_windows(0x10)
			{
				_Controller = this;
				NativeWindows::InitializeWindowSystem();
				if (_callback) {
					bool opened = false;
					for (int i = 0; i < files_open.Length(); i++) {
						if (_callback->OpenExactFile(files_open[i])) opened = true;
					}
					if (!opened) _callback->CreateNewFile();
				}
			}
			~WindowsApplicationController(void) override {}

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
					SetWindowLongPtrW(reinterpret_cast<HWND>(dialog->GetStation()->GetOSHandle()),
						GWLP_HWNDPARENT, eint(reinterpret_cast<HWND>(Parent->GetStation()->GetOSHandle())));
					dialog->Show(true);
					SetForegroundWindow(reinterpret_cast<HWND>(dialog->GetStation()->GetOSHandle()));
					SetActiveWindow(reinterpret_cast<HWND>(dialog->GetStation()->GetOSHandle()));
					Parent->Enable(false);
					return dialog;
				} else {
					dialog->Show(true);
					BEGIN_GLOBAL_MODAL_SESSION(reinterpret_cast<HWND>(dialog->GetStation()->GetOSHandle()))
					UI::Windows::RunMessageLoop();
					END_GLOBAL_MODAL_SESSION
					dialog->Destroy();
					return 0;
				}
			}
			virtual void SystemOpenFileDialog(OpenFileInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				OpenFileStruct * data = new OpenFileStruct;
				data->Info = Info;
				data->Owner = Parent ? reinterpret_cast<HWND>(Parent->GetStation()->GetOSHandle()) : 0;
				data->Task = OnExit;
				data->Context = Parent ? Parent->GetStation() : 0;
				if (Parent) {
					SafePointer<Thread> isolated = CreateThread(OpenFileProc, data);
				} else {
					BEGIN_GLOBAL_MODAL_SESSION(0)
					OpenFileProc(data);
					END_GLOBAL_MODAL_SESSION
				}
			}
			virtual void SystemSaveFileDialog(SaveFileInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				SaveFileStruct * data = new SaveFileStruct;
				data->Info = Info;
				data->Owner = Parent ? reinterpret_cast<HWND>(Parent->GetStation()->GetOSHandle()) : 0;
				data->Task = OnExit;
				data->Context = Parent ? Parent->GetStation() : 0;
				if (Parent) {
					SafePointer<Thread> isolated = CreateThread(SaveFileProc, data);
				} else {
					BEGIN_GLOBAL_MODAL_SESSION(0)
					SaveFileProc(data);
					END_GLOBAL_MODAL_SESSION
				}
			}
			virtual void SystemChooseDirectoryDialog(ChooseDirectoryInfo * Info, UI::Window * Parent, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				ChooseDirectoryStruct * data = new ChooseDirectoryStruct;
				data->Info = Info;
				data->Owner = Parent ? reinterpret_cast<HWND>(Parent->GetStation()->GetOSHandle()) : 0;
				data->Task = OnExit;
				data->Context = Parent ? Parent->GetStation() : 0;
				if (Parent) {
					SafePointer<Thread> isolated = CreateThread(ChooseDirectoryProc, data);
				} else {
					BEGIN_GLOBAL_MODAL_SESSION(0)
					ChooseDirectoryProc(data);
					END_GLOBAL_MODAL_SESSION
				}
			}
			virtual void SystemMessageBox(MessageBoxResult * Result, const string & Text, const string & Title, UI::Window * Parent, MessageBoxButtonSet Buttons, MessageBoxStyle Style, IDispatchTask * OnExit) override
			{
				if (OnExit) OnExit->Retain();
				MessageBoxStruct * data = new MessageBoxStruct;
				data->Result = Result;
				data->Text = Text;
				data->Title = Title;
				data->Buttons = Buttons;
				data->Style = Style;
				data->Owner = Parent ? reinterpret_cast<HWND>(Parent->GetStation()->GetOSHandle()) : 0;
				data->Task = OnExit;
				data->Context = Parent ? Parent->GetStation() : 0;
				if (Parent) {
					SafePointer<Thread> isolated = CreateThread(MessageBoxProc, data, 0x40000);
				} else {
					BEGIN_GLOBAL_MODAL_SESSION(0)
					MessageBoxProc(data);
					END_GLOBAL_MODAL_SESSION
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
				HWND parent = GetWindow(reinterpret_cast<HWND>(modal->GetStation()->GetOSHandle()), GW_OWNER);
				if (parent) {
					NativeWindows::EnableWindow(reinterpret_cast<UI::WindowStation *>(GetWindowLongPtrW(parent, 0)), true);
					modal->Destroy();
				} else {
					UI::Windows::ExitMessageLoop();
				}
			}
		};
		ApplicationController * CreateController(IApplicationCallback * callback)
		{
			if (_Controller) return _Controller;
			Array<string> files(1);
			new WindowsApplicationController(callback, files);
			return _Controller;
		}
		ApplicationController * CreateController(IApplicationCallback * callback, const Array<string>& files_open)
		{
			if (_Controller) return _Controller;
			new WindowsApplicationController(callback, files_open);
			return _Controller;
		}
		ApplicationController * GetController(void) { return _Controller; }
	}
}