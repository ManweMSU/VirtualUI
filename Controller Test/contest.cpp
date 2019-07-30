#include <EngineRuntime.h>

using namespace Engine;

UI::InterfaceTemplate * templ = 0;

class SlaveDelegate : public UI::Windows::IWindowEventCallback
{
public:
	virtual void OnInitialized(UI::Window * window) override
	{
	}
	virtual void OnControlEvent(UI::Window * window, int ID, UI::Window::Event event, UI::Window * sender) override
	{
	}
	virtual void OnFrameEvent(UI::Window * window, UI::Windows::FrameEvent event) override
	{
		if (event == UI::Windows::FrameEvent::Close) {
			UI::Window * desktop = window->GetStation()->GetDesktop();
			desktop->Destroy();
			delete this;
		}
	}
};
class ModalDelegate : public UI::Windows::IWindowEventCallback
{
public:
	virtual void OnInitialized(UI::Window * window) override
	{
		window->As<UI::Controls::OverlappedWindow>()->AddDialogStandardAccelerators();
	}
	virtual void OnControlEvent(UI::Window * window, int ID, UI::Window::Event event, UI::Window * sender) override
	{
		if (ID == 1 || ID == 2) {
			OnFrameEvent(window, UI::Windows::FrameEvent::Close);
		}
	}
	virtual void OnFrameEvent(UI::Window * window, UI::Windows::FrameEvent event) override
	{
		if (event == UI::Windows::FrameEvent::Close) {
			UI::Window * desktop = window->GetStation()->GetDesktop();
			Application::GetController()->ExitModalSession(desktop);
			delete this;
		}
	}
};
class OpenFileJob : public Tasks::ThreadJob
{
public:
	UI::Window * output;
	Application::OpenFileInfo * info;
	virtual void DoJob(Tasks::ThreadPool * pool) override
	{
		DynamicString str;
		if (info->Files.Length()) {
			str << L"open:" << IO::NewLineChar;
			for (int i = 0; i < info->Files.Length(); i++) str << info->Files[i] << IO::NewLineChar;
		} else str << L"operation cancelled";
		if (output) {
			output->SetText(str);
			output->RequireRedraw();
		}
		delete info;
	}
};
class SaveFileJob : public Tasks::ThreadJob
{
public:
	UI::Window * output;
	Application::SaveFileInfo * info;
	virtual void DoJob(Tasks::ThreadPool * pool) override
	{
		DynamicString str;
		if (info->File.Length()) {
			str << L"save:" << IO::NewLineChar;
			str << info->File << IO::NewLineChar;
			str << L"format #" << string(info->Format);
		} else str << L"operation cancelled";
		if (output) {
			output->SetText(str);
			output->RequireRedraw();
		}
		delete info;
	}
};
class ChooseDirectoryJob : public Tasks::ThreadJob
{
public:
	UI::Window * output;
	Application::ChooseDirectoryInfo * info;
	virtual void DoJob(Tasks::ThreadPool * pool) override
	{
		DynamicString str;
		if (info->Directory.Length()) {
			str << L"choosen:" << IO::NewLineChar;
			str << info->Directory;
		} else str << L"operation cancelled";
		if (output) {
			output->SetText(str);
			output->RequireRedraw();
		}
		delete info;
	}
};
class MessageBoxJob : public Tasks::ThreadJob
{
public:
	UI::Window * output;
	Application::MessageBoxResult result;
	virtual void DoJob(Tasks::ThreadPool * pool) override
	{
		DynamicString str;
		if (result == Application::MessageBoxResult::Cancel) str += L"CANCEL";
		else if (result == Application::MessageBoxResult::Ok) str += L"OK";
		else if (result == Application::MessageBoxResult::Yes) str += L"YES";
		else if (result == Application::MessageBoxResult::No) str += L"NO";
		if (output) {
			output->SetText(str);
			output->RequireRedraw();
		}
	}
};
class MainDelegate : public UI::Windows::IWindowEventCallback
{
public:
	virtual void OnInitialized(UI::Window * window) override {}
	virtual void OnControlEvent(UI::Window * window, int ID, UI::Window::Event event, UI::Window * sender) override
	{
		if (ID == 101) {
			Application::GetController()->GetCallback()->CreateNewFile();
		} else if (ID == 102) {
			SlaveDelegate * delegate = new SlaveDelegate;
			auto w = Application::GetController()->CreateWindow(templ->Dialog[L"Slave"], delegate, UI::Rectangle::Invalid(), window);
			w->AddDialogStandardAccelerators();
			w->Show(true);
		} else if (ID == 103) {
			ModalDelegate * delegate = new ModalDelegate;
			Application::GetController()->CreateModalWindow(templ->Dialog[L"Modal"], delegate, UI::Rectangle::Invalid(), window);
		} else if (ID == 104) {
			ModalDelegate * delegate = new ModalDelegate;
			Application::GetController()->CreateModalWindow(templ->Dialog[L"Modal"], delegate, UI::Rectangle::Invalid(), 0);
		} else if (ID == 201) {
			Application::OpenFileInfo * Info = new Application::OpenFileInfo;
			Info->Title = L"pidor";
			Info->MultiChoose = true;
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Engine Storage";
			Info->Formats.LastElement().Extensions << L"ecs";
			Info->Formats.LastElement().Extensions << L"ecsr";
			Info->Formats.LastElement().Extensions << L"ecsa";
			Info->Formats.LastElement().Extensions << L"ecst";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Windows Bitmap";
			Info->Formats.LastElement().Extensions << L"bmp";
			Info->Formats.LastElement().Extensions << L"dib";
			Info->Formats.LastElement().Extensions << L"rle";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Tagged Image File Format";
			Info->Formats.LastElement().Extensions << L"tif";
			Info->Formats.LastElement().Extensions << L"tiff";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Windows Shell Script";
			Info->Formats.LastElement().Extensions << L"bat";
			OpenFileJob * job = new OpenFileJob;
			job->info = Info;
			job->output = window->FindChild(301);
			Application::GetController()->SystemOpenFileDialog(Info, window->FindChild(205)->As<UI::Controls::CheckBox>()->Checked ? 0 : window, job);
			job->Release();
		} else if (ID == 202) {
			Application::ChooseDirectoryInfo * Info = new Application::ChooseDirectoryInfo;
			Info->Title = L"choose pidor";
			ChooseDirectoryJob * job = new ChooseDirectoryJob;
			job->info = Info;
			job->output = window->FindChild(301);
			Application::GetController()->SystemChooseDirectoryDialog(Info, window->FindChild(205)->As<UI::Controls::CheckBox>()->Checked ? 0 : window, job);
			job->Release();
		} else if (ID == 203) {
			Application::SaveFileInfo * Info = new Application::SaveFileInfo;
			Info->Title = L"save pidor";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Engine Storage";
			Info->Formats.LastElement().Extensions << L"ecs";
			Info->Formats.LastElement().Extensions << L"ecsr";
			Info->Formats.LastElement().Extensions << L"ecsa";
			Info->Formats.LastElement().Extensions << L"ecst";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Windows Bitmap";
			Info->Formats.LastElement().Extensions << L"bmp";
			Info->Formats.LastElement().Extensions << L"dib";
			Info->Formats.LastElement().Extensions << L"rle";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Tagged Image File Format";
			Info->Formats.LastElement().Extensions << L"tif";
			Info->Formats.LastElement().Extensions << L"tiff";
			Info->Formats << Application::FileFormat();
			Info->Formats.LastElement().Description = L"Windows Shell Script";
			Info->Formats.LastElement().Extensions << L"bat";
			SaveFileJob * job = new SaveFileJob;
			job->info = Info;
			job->output = window->FindChild(301);
			Application::GetController()->SystemSaveFileDialog(Info, window->FindChild(205)->As<UI::Controls::CheckBox>()->Checked ? 0 : window, job);
			job->Release();
		} else if (ID == 204) {
			MessageBoxJob * job = new MessageBoxJob;
			job->output = window->FindChild(301);
			Application::GetController()->SystemMessageBox(&job->result, L"kornevgen", L"pidor?", window->FindChild(205)->As<UI::Controls::CheckBox>()->Checked ? 0 : window,
				Application::MessageBoxButtonSet::YesNoCancel, Application::MessageBoxStyle::Warning, job);
			job->Release();
		}
	}
	virtual void OnFrameEvent(UI::Window * window, UI::Windows::FrameEvent event) override
	{
		if (event == UI::Windows::FrameEvent::Close) {
			UI::Window * desktop = window->GetStation()->GetDesktop();
			Application::GetController()->UnregisterMainWindow(desktop);
			desktop->Destroy();
			delete this;
		}
	}
};
class AppDelegate : public Application::IApplicationCallback
{
public:
	virtual bool GetAbility(Application::ApplicationAbility ability) override
	{
		return true;
	}
	virtual void CreateNewFile(void) override
	{
		MainDelegate * main_delegate = new MainDelegate;
		auto w = Application::GetController()->CreateWindow(templ->Dialog[L"Main"], main_delegate, UI::Rectangle::Invalid());
		w->AddDialogStandardAccelerators();
		w->Show(true);
		Application::GetController()->RegisterMainWindow(w);
	}
	virtual void OpenSomeFile(void) override
	{
		Application::OpenFileInfo Info;
		Application::GetController()->SystemOpenFileDialog(&Info, 0, 0);
		if (Info.Files.Length()) OpenExactFile(Info.Files.FirstElement());
	}
	virtual bool OpenExactFile(const string & path) override
	{
		MainDelegate * main_delegate = new MainDelegate;
		auto w = Application::GetController()->CreateWindow(templ->Dialog[L"Main"], main_delegate, UI::Rectangle::Invalid());
		w->AddDialogStandardAccelerators();
		w->FindChild(301)->SetText(L"open file:\n" + path);
		w->Show(true);
		Application::GetController()->RegisterMainWindow(w);
		return true;
	}
	virtual void InvokeHelp(void) override
	{
		MainDelegate * main_delegate = new MainDelegate;
		auto w = Application::GetController()->CreateWindow(templ->Dialog[L"Main"], main_delegate, UI::Rectangle::Invalid());
		w->AddDialogStandardAccelerators();
		w->FindChild(301)->SetText(L"show help");
		w->Show(true);
		Application::GetController()->RegisterMainWindow(w);
	}
	virtual void ShowProperties(void) override
	{
		MainDelegate * main_delegate = new MainDelegate;
		auto w = Application::GetController()->CreateWindow(templ->Dialog[L"Main"], main_delegate, UI::Rectangle::Invalid());
		w->AddDialogStandardAccelerators();
		w->FindChild(301)->SetText(L"show properties");
		w->Show(true);
		Application::GetController()->RegisterMainWindow(w);
	}
};

int Main(void)
{
	UI::Zoom = UI::Windows::GetScreenScale();
	UI::Windows::InitializeCodecCollection();
	{
		SafePointer<Streaming::Stream> resource = Assembly::QueryResource(L"GUI");
		if (resource) {
			templ = new UI::InterfaceTemplate;
			auto loader = UI::Windows::CreateNativeCompatibleResourceLoader();
			UI::Loader::LoadUserInterfaceFromBinary(*templ, resource, loader, 0);
			loader->Release();
		}
	}
	AppDelegate * app_callback = new AppDelegate;
	SafePointer< Array<string> > args = GetCommandLine();
	args->RemoveFirst();
	Application::CreateController(app_callback, *args);
	Application::GetController()->RunApplication();
	return 0;
}