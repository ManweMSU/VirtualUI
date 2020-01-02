// Tests.cpp: определяет точку входа для приложения.
//

#include <Miscellaneous/Time.h>
#include <Processes/Process.h>
#include <Processes/Threading.h>
#include <Miscellaneous/DynamicString.h>
#include <UserInterface/ShapeBase.h>
#include <UserInterface/Templates.h>
#include <UserInterface/ControlBase.h>
#include <Streaming.h>
#include <PlatformDependent/Direct2D.h>
#include <PlatformDependent/Direct3D.h>
#include <PlatformDependent/Clipboard.h>
#include <Miscellaneous/Dictionary.h>
#include <UserInterface/ControlClasses.h>
#include <UserInterface/BinaryLoader.h>
#include <UserInterface/StaticControls.h>
#include <UserInterface/ButtonControls.h>
#include <UserInterface/GroupControls.h>
#include <UserInterface/Menues.h>
#include <UserInterface/OverlappedWindows.h>
#include <UserInterface/EditControls.h>
#include <UserInterface/ListControls.h>
#include <UserInterface/CombinedControls.h>
#include <Syntax/Tokenization.h>
#include <Syntax/Grammar.h>
#include <Syntax/MathExpression.h>
#include <Syntax/Regular.h>
#include <PlatformDependent/KeyCodes.h>
#include <PlatformDependent/NativeStation.h>
#include <ImageCodec/IconCodec.h>
#include <Processes/Shell.h>
#include <Network/HTTP.h>
#include <Network/Socket.h>
#include <Network/Punycode.h>
#include <Storage/Registry.h>
#include <Storage/TextRegistry.h>
#include <Storage/TextRegistryGrammar.h>
#include <Storage/Compression.h>
#include <Storage/Chain.h>
#include <Miscellaneous/ThreadPool.h>
#include <Storage/Archive.h>
#include <Storage/ImageVolume.h>
#include <PlatformDependent/Assembly.h>
#include <Storage/StringTable.h>
#include <Math/MathBase.h>
#include <Math/Complex.h>
#include <Math/Vector.h>
#include <Math/Matrix.h>
#include <Math/Color.h>
#include <PlatformSpecific/WindowsTaskbar.h>
#include <PlatformSpecific/WindowsRegistry.h>
#include <PlatformSpecific/WindowsShortcut.h>
#include <PlatformSpecific/WindowsEffects.h>
#include <PlatformDependent/SystemColors.h>
#include <PlatformDependent/Console.h>
#include <Storage/JSON.h>
#include <Storage/Object.h>

#include "stdafx.h"
#include "Tests.h"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include <PlatformDependent/WindowStation.h>

#undef CreateWindow
#undef GetCurrentDirectory
#undef SetCurrentDirectory
#undef GetCommandLine
#undef CreateProcess
#undef GetCurrentTime
#undef CreateFile
#undef CreateSemaphore
#undef CreateSymbolicLink

using namespace Engine;
using namespace Engine::UI;

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна
HWND Window;

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);

Engine::Direct2D::D2DRenderDevice * Device;
Engine::UI::FrameShape * Shape;
Engine::UI::ITexture * Texture;

Engine::UI::HandleWindowStation * station = 0;

ID2D1DeviceContext * Target = 0;
IDXGISwapChain1 * SwapChain = 0;

SafePointer<Engine::Streaming::TextWriter> conout;
SafePointer<Engine::UI::InterfaceTemplate> Template;

UI::IInversionEffectRenderingInfo * Inversion = 0;

SafePointer<Menues::Menu> menu;

ENGINE_PACKED_STRUCTURE(TestPacked)
	uint8 foo;
	uint32 bar;
	uint16 foobar;
ENGINE_END_PACKED_STRUCTURE

class Chronometer
{
	uint32 begin;
public:
	Chronometer(void) : begin(GetTimerValue()) {}
	operator uint32 (void) const { return GetTimerValue() - begin; }
};

void CreateBlangSpelling(Syntax::Spelling & spelling)
{
	spelling.BooleanFalseLiteral = L"false";
	spelling.BooleanTrueLiteral = L"true";
	spelling.CommentEndOfLineWord = L"//";
	spelling.CommentBlockOpeningWord = L"/*";
	spelling.CommentBlockClosingWord = L"*/";
	spelling.IsolatedChars << L'(';
	spelling.IsolatedChars << L')';
	spelling.IsolatedChars << L'[';
	spelling.IsolatedChars << L']';
	spelling.IsolatedChars << L'{';
	spelling.IsolatedChars << L'}';
	spelling.IsolatedChars << L',';
	spelling.IsolatedChars << L';';
	spelling.IsolatedChars << L'^';
	spelling.IsolatedChars << L'.';
	spelling.IsolatedChars << L'~';
	spelling.IsolatedChars << L'@';
}

ENGINE_REFLECTED_CLASS(lv_item, Reflection::Reflected)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text1)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text2)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text3)
ENGINE_END_REFLECTED_CLASS

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	SetProcessDPIAware();

	if (GetStdHandle(STD_OUTPUT_HANDLE) == 0) {
		AllocConsole();
		SetConsoleTitleW(L"ui tests");
	}
	SafePointer<Engine::Streaming::FileStream> constream = new Engine::Streaming::FileStream(Engine::IO::GetStandardOutput());
	conout.SetReference(new Engine::Streaming::TextWriter(constream));

	IO::Console cns;
	cns.SetTextColor(15);
	cns.SetBackgroundColor(8);
	cns.MoveCaret(5, 2);
	cns.ClearScreen();
	cns.WriteLine(L"Привет!");

	(*conout) << IO::GetCurrentDirectory() << IO::NewLineChar;
	(*conout) << L"Full path      : " << IO::GetExecutablePath() << IO::NewLineChar;
	(*conout) << L"Directory      : " << IO::Path::GetDirectory(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"File name      : " << IO::Path::GetFileName(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"Clear file name: " << IO::Path::GetFileNameWithoutExtension(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"Extension      : " << IO::Path::GetExtension(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"Scale          : " << UI::Windows::GetScreenScale() << IO::NewLineChar;
	(*conout) << L"Locale         : " << Assembly::GetCurrentUserLocale() << IO::NewLineChar;
	(*conout) << L"Memory (GB)    : " << GetInstalledMemory() / 0x40000000 << IO::NewLineChar;

	(*conout) << string(Math::Complex(5.0)) << IO::NewLineChar;
	(*conout) << string(ENGINE_I) << IO::NewLineChar;

	auto v = Math::Vector<Math::Complex, 4>(ENGINE_I, -ENGINE_I, 0.0, 0.0);
	(*conout) << Math::length(v) << IO::NewLineChar;
	(*conout) << string(v) << IO::NewLineChar;

	(*conout) << string(Math::TensorProduct(Math::Vector3(1, 2, 3), Math::Vector2(1, 2))) << IO::NewLineChar;
	(*conout) << string(Math::VectorColumnCast(v)) << IO::NewLineChar;
	(*conout) << string(Math::Matrix<Math::Complex, 2, 2>::MatrixCast(v)) << IO::NewLineChar;

	auto m1 = Math::TensorProduct(Math::Vector3(1, 2, 3), Math::Vector2(1, 2));
	(*conout) << string(m1) << IO::NewLineChar;
	(*conout) << string(m1 * transpone(m1)) << IO::NewLineChar;
	(*conout) << string(m1 * Math::Vector2(1, 1)) << IO::NewLineChar;

	Math::Vector2 f(1.0, 1.0);
	Math::Vector2 r(1.0, 1.0);
	Math::Matrix2x2 m;
	m(0, 0) = 0.0;
	m(0, 1) = -2.0;
	m(1, 0) = 7.0;
	m(1, 1) = -3.0;
	Math::Matrix2x2 im = Math::inverse(m);
	auto hi = m * im;

	(*conout) << string(m) << IO::NewLineChar;
	(*conout) << string(Math::VectorColumnCast(f)) << IO::NewLineChar;
	(*conout) << L"det = " << string(det(m)) << IO::NewLineChar;
	(*conout) << L"tr  = " << string(tr(m)) << IO::NewLineChar;

	Math::GaussianEliminationMethod(m, f, r);
	(*conout) << string(r) << IO::NewLineChar << IO::NewLineChar;
	(*conout) << string(im) << IO::NewLineChar;
	(*conout) << string(hi) << IO::NewLineChar;

	IO::SetCurrentDirectory(IO::Path::GetDirectory(IO::GetExecutablePath()));
	UI::Windows::InitializeCodecCollection();
	
	UI::Zoom = 2.0;

	SafePointer<IResourceLoader> resource_loader = Engine::NativeWindows::CreateCompatibleResourceLoader();

    // TODO: разместите код здесь.

    // Инициализация глобальных строк
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_TESTS, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Выполнить инициализацию приложения:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

	//WindowsSpecific::SetRenderingDeviceFeatureClass(WindowsSpecific::RenderingDeviceFeatureClass::D2DDevice11);

	// Starting D3D
	Direct3D::CreateDevices();
	Direct3D::CreateD2DDeviceContextForWindow(::Window, &Target, &SwapChain);
	Device = new Engine::Direct2D::D2DRenderDevice(Target);
	{
		cns.WriteLine(L"====================================");
		SafePointer< Array<string> > fonts = UI::Windows::GetFontFamilies();
		SortArray(*fonts);
		for (int i = 0; i < fonts->Length(); i++) cns.WriteLine(fonts->ElementAt(i));
		cns.WriteLine(L"====================================");
	}

	SafePointer<Codec::Frame> image_frame;
	Clipboard::GetData(image_frame.InnerRef());
	SafePointer<UI::ITexture> image = image_frame ? Device->LoadTexture(image_frame) : 0;
	Codec::Image icon;

	{
		SafePointer<Codec::Frame> frame = new Codec::Frame(512, 512, -1, Codec::PixelFormat::R8G8B8A8, Codec::AlphaFormat::Normal, Codec::LineDirection::TopDown);
		SafePointer<Streaming::Stream> stream = new Streaming::FileStream(L"hsv.png", Streaming::AccessReadWrite, Streaming::CreateAlways);
		for (int y = 0; y < 512; y++) for (int x = 0; x < 512; x++) {
			auto hsv = Math::ColorHSV(x / 512.0 * 2.0 * ENGINE_PI, (512 - y) / 512.0, 1.0, 1.0);
			frame->SetPixel(x, y, UI::Color(hsv).Value);
		}
		Codec::EncodeFrame(stream, frame, L"PNG");
		icon.Frames.Append(frame);
	}
	NativeWindows::SetApplicationIcon(&icon);

	{
		{
			::Template.SetReference(new Engine::UI::InterfaceTemplate());
			{
				//SafePointer<Streaming::Stream> Source = new Streaming::FileStream(L"Test.eui", Streaming::AccessRead, Streaming::OpenExisting);
				SafePointer<Streaming::Stream> Source = Assembly::QueryResource(L"GUI");
				Engine::UI::Loader::LoadUserInterfaceFromBinary(*::Template, Source, resource_loader, 0);
			}
			station = new HandleWindowStation(::Window);
			station->SetRenderingDevice(Device);
			auto Main = station->GetDesktop();
			SendMessageW(::Window, WM_SIZE, 0, 0);
			SafePointer<Template::FrameShape> back = new Template::FrameShape;
			{
				SafePointer<Template::TextureShape> Back = new Template::TextureShape;
				Back->From = Rectangle::Entire();
				Back->RenderMode = TextureShape::TextureRenderMode::Fit;
				Back->Position = Rectangle::Entire();

				Back->Texture = image ? UI::Template::TextureTemplate(image) : ::Template->Texture[L"Wallpaper"];

				SafePointer<Template::BarShape> Fill = new Template::BarShape;
				Fill->Gradient << GradientPoint(0xFF303050);
				back->Children.Append(Back);
				back->Children.Append(Fill);
			}
			Main->As<TopLevelWindow>()->Background.SetRetain(back);

			station->GetVisualStyles().MenuArrow.SetReference(::Template->Application[L"a"]);
			SafePointer<Template::FrameShape> MenuBack = new Template::FrameShape;
			MenuBack->Position = UI::Rectangle::Entire();
			{
				SafePointer<Template::BlurEffectShape> Blur = new Template::BlurEffectShape;
				Blur->Position = UI::Rectangle::Entire();
				Blur->BlurPower = 20.0;
				SafePointer<Template::BarShape> Bk = new Template::BarShape;
				Bk->Position = UI::Rectangle::Entire();
				Bk->Gradient << GradientPoint(Color(64, 64, 64, 128));
				SafePointer<Template::BarShape> Left = new Template::BarShape;
				Left->Position = UI::Rectangle(0, 0, UI::Coordinate(0, 1.0, 0.0), UI::Coordinate::Bottom());
				Left->Gradient << GradientPoint(Color(255, 255, 255, 255));
				SafePointer<Template::BarShape> Top = new Template::BarShape;
				Top->Position = UI::Rectangle(0, 0, UI::Coordinate::Right(), UI::Coordinate(0, 1.0, 0.0));
				Top->Gradient << GradientPoint(Color(255, 255, 255, 255));
				SafePointer<Template::BarShape> Right = new Template::BarShape;
				Right->Position = UI::Rectangle(UI::Coordinate::Right() - UI::Coordinate(0, 1.0, 0.0), 0, UI::Coordinate::Right(), UI::Coordinate::Bottom());
				Right->Gradient << GradientPoint(Color(255, 255, 255, 255));
				SafePointer<Template::BarShape> Bottom = new Template::BarShape;
				Bottom->Position = UI::Rectangle(0, UI::Coordinate::Bottom() - UI::Coordinate(0, 1.0, 0.0), UI::Coordinate::Right(), UI::Coordinate::Bottom());
				Bottom->Gradient << GradientPoint(Color(255, 255, 255, 255));
				MenuBack->Children.Append(Left);
				MenuBack->Children.Append(Top);
				MenuBack->Children.Append(Right);
				MenuBack->Children.Append(Bottom);
				MenuBack->Children.Append(Bk);
				MenuBack->Children.Append(Blur);
			}
			station->GetVisualStyles().MenuBackground.SetRetain(MenuBack);
			station->GetVisualStyles().MenuBorder = int(UI::Zoom * 4.0);
			station->GetVisualStyles().WindowCloseButton.SetRetain(::Template->Dialog[L"Header"]->Children[0].Children.ElementAt(0));
			station->GetVisualStyles().WindowMaximizeButton.SetRetain(::Template->Dialog[L"Header"]->Children[0].Children.ElementAt(1));
			station->GetVisualStyles().WindowMinimizeButton.SetRetain(::Template->Dialog[L"Header"]->Children[0].Children.ElementAt(2));
			station->GetVisualStyles().WindowHelpButton.SetRetain(::Template->Dialog[L"Header"]->Children[0].Children.ElementAt(3));
			station->GetVisualStyles().WindowFixedBorder = 10;
			station->GetVisualStyles().WindowSizableBorder = 15;
			station->GetVisualStyles().WindowCaptionHeight = 60;
			station->GetVisualStyles().WindowSmallCaptionHeight = 40;
			{
				SafePointer<Template::BlurEffectShape> Blur = new Template::BlurEffectShape;
				Blur->Position = UI::Rectangle::Entire();
				Blur->BlurPower = 20.0;
				SafePointer<Template::BarShape> BkActive = new Template::BarShape;
				BkActive->Position = UI::Rectangle::Entire();
				BkActive->Gradient << GradientPoint(Color(96, 96, 96, 128));
				SafePointer<Template::BarShape> BkInactive = new Template::BarShape;
				BkInactive->Position = UI::Rectangle::Entire();
				BkInactive->Gradient << GradientPoint(Color(16, 16, 16, 128));
				SafePointer<Template::FrameShape> Decor = new Template::FrameShape;
				Decor->Position = UI::Rectangle::Entire();
				SafePointer<Template::TextShape> Title = new Template::TextShape;
				Title->Position = Template::Rectangle(
					Template::Coordinate(Template::IntegerTemplate::Undefined(L"Border"), 20.0, 0.0),
					Template::Coordinate(Template::IntegerTemplate::Undefined(L"Border"), 0.0, 0.0),
					Template::Coordinate(Template::IntegerTemplate::Undefined(L"= 0 - Border - ButtonsWidth"), 0.0, 1.0),
					Template::Coordinate(Template::IntegerTemplate::Undefined(L"= Border + Caption"), 0.0, 0.0)
				);
				Title->Font = ::Template->Font[L"Font"];
				Title->Text = Template::StringTemplate::Undefined(L"Text");
				Title->TextColor = UI::Color(0xFFFFFFFF);
				Title->HorizontalAlign = TextShape::TextHorizontalAlign::Left;
				Title->VerticalAlign = TextShape::TextVerticalAlign::Center;
				Decor->Children.Append(Title);

				SafePointer<Template::FrameShape> Active = new Template::FrameShape;
				Active->Position = Rectangle::Entire();
				Active->Children.Append(Decor);
				Active->Children.Append(BkActive);
				Active->Children.Append(Blur);
				SafePointer<Template::FrameShape> Inactive = new Template::FrameShape;
				Inactive->Position = Rectangle::Entire();
				Inactive->Children.Append(Decor);
				Inactive->Children.Append(BkInactive);
				Inactive->Children.Append(Blur);
				station->GetVisualStyles().WindowActiveView.SetRetain(Active);
				station->GetVisualStyles().WindowInactiveView.SetRetain(Inactive);
				station->GetVisualStyles().CaretWidth = 2;
			}
			SafePointer<Template::FrameShape> wb = new Template::FrameShape;
			wb->Children.Append(::Template->Application[L"Waffle"]);
			wb->Opacity = 0.5;
			wb->RenderMode = FrameShape::FrameRenderMode::Layering;
			station->GetVisualStyles().WindowDefaultBackground.SetRetain(wb);

			menu = new Menues::Menu(::Template->Dialog[L"Menu"]);

			class _cb : public Windows::IWindowEventCallback
			{
			public:
				virtual void OnInitialized(UI::Window * window) override
				{
					(*conout) << L"Callback: Initialized, window = " << string(static_cast<handle>(window)) << IO::NewLineChar;
				}
				virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
				{
					(*conout) << L"Callback: Event with ID = " << ID << L", window = " << string(static_cast<handle>(window)) << L", sender = " << string(static_cast<handle>(sender)) << IO::NewLineChar;
					if (event == Window::Event::Command || event == Window::Event::MenuCommand) {
						if (ID == 876) {
							menu->RunPopup(sender, station->GetCursorPos());
						} else if (ID == 2) {
							auto bar = static_cast<Controls::ProgressBar *>(window->FindChild(888));
							bar->SetValue(min(max(bar->GetValue() + 0.05, 0.0), 1.0));
							window->FindChild(1)->Enable(true);
							if (bar->GetValue() == 1.0) sender->Enable(false);
						} else if (ID == 1) {
							auto bar = static_cast<Controls::ProgressBar *>(window->FindChild(888));
							bar->SetValue(min(max(bar->GetValue() - 0.05, 0.0), 1.0));
							window->FindChild(2)->Enable(true);
							if (bar->GetValue() == 0.0) sender->Enable(false);
						} else if (ID == 202) {
							static_cast<Controls::ToolButtonPart *>(window->FindChild(202))->Checked ^= true;
							static_cast<Controls::ToolButtonPart *>(window->FindChild(201))->Disabled ^= true;
						} else if (ID == 201) {
							auto bar = window->FindChild(888);
							if (bar->IsVisible()) {
								bar->HideAnimated(Animation::SlideSide::Top, 500, Animation::AnimationClass::Smooth);
							} else {
								bar->ShowAnimated(Animation::SlideSide::Left, 500, Animation::AnimationClass::Smooth);
							}
						}
					}
				}
				virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
				{
					(*conout) << L"Callback: ";
					if (event == Windows::FrameEvent::Move) (*conout) << L"Move";
					else if (event == Windows::FrameEvent::Close) (*conout) << L"Close";
					else if (event == Windows::FrameEvent::Minimize) (*conout) << L"Minimize";
					else if (event == Windows::FrameEvent::Maximize) (*conout) << L"Maximize";
					else if (event == Windows::FrameEvent::Help) (*conout) << L"Help";
					else if (event == Windows::FrameEvent::PopupMenuCancelled) (*conout) << L"Popup menu cancelled";
					(*conout) << L", window = " << string(static_cast<handle>(window)) << IO::NewLineChar;
				}
			};
			auto Callback = new _cb;
			class _cb2 : public Windows::IWindowEventCallback
			{
			public:
				virtual void OnInitialized(UI::Window * window) override
				{
					auto list = window->FindChild(343434)->As<Controls::ListView>();
					for (int i = 1; i <= 10; i++) {
						lv_item item;
						item.Text1 = L"Item " + string(i) + L".1";
						item.Text2 = L"Item " + string(i) + L".2";
						item.Text3 = L"Item " + string(i) + L".3";
						list->AddItem(item);
					}
					list->OrderColumn(2, 0);
					list->MultiChoose = true;
					auto list2 = window->FindChild(353535)->As<Controls::TreeView>();
					auto rt = list2->GetRootItem();
					auto i1 = rt->AddItem(L"Tree View Item 1");
					i1->AddItem(L"Tree View Item 1.1");
					i1->AddItem(L"Tree View Item 1.2");
					auto i13 = i1->AddItem(L"Tree View Item 1.3");
					i1->AddItem(L"Tree View Item 1.4");
					i13->AddItem(L"Tree View Item 1.3.1");
					i13->AddItem(L"Tree View Item 1.3.2");
					auto i2 = rt->AddItem(L"Tree View Item 2");
					i2->AddItem(L"Tree View Item 2.1");
					i2->AddItem(L"Tree View Item 2.2");
					rt->AddItem(L"Tree View Item 3");
					auto i4 = rt->AddItem(L"Tree View Item 4");
					i4->AddItem(L"§ ыыыы §");
					i1->Expand(true);
					auto combo = window->FindChild(565656)->As<Controls::ComboBox>();
					for (int i = 0; i < 100; i++) combo->AddItem(L"Combo Box Item " + string(i + 1));
					auto box = window->FindChild(575757)->As<Controls::TextComboBox>();
					for (int i = 0; i < 100; i++) box->AddItem(L"Text Combo Box Item " + string(i + 1));
					box->AddItem(L"kornevgen pidor");
				}
				virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
				{
					if (ID == 343434) {
						if (event == Window::Event::DoubleClick) {
							sender->As<Controls::ListView>()->CreateEmbeddedEditor(::Template->Dialog[L"editor"],
								sender->As<Controls::ListView>()->GetLastCellID(),
								Rectangle::Entire())->FindChild(888888)->SetFocus();
						} else if (event == Window::Event::ContextClick) {
							auto p = sender->GetStation()->GetCursorPos();
							auto b = sender->GetStation()->GetAbsoluteDesktopBox(Box(p.x, p.y, p.x, p.y));
							Windows::CreatePopupDialog(::Template->Dialog[L"Test3"], this, UI::Rectangle(b.Left, b.Top, b.Left + 500, b.Top + 400), sender->GetStation())->Show(true);
						}
					} else if (ID == 353535) {
						if (event == Window::Event::DoubleClick) {
							sender->As<Controls::TreeView>()->CreateEmbeddedEditor(::Template->Dialog[L"editor"], Rectangle::Entire())->FindChild(888888)->SetFocus();
						}
					} else if (ID == 1) {
						auto group1 = window->FindChild(101);
						auto group2 = window->FindChild(102);
						if (group1->IsVisible()) {
							group1->HideAnimated(Animation::SlideSide::Left, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
							group2->ShowAnimated(Animation::SlideSide::Right, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
						} else {
							group2->HideAnimated(Animation::SlideSide::Left, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
							group1->ShowAnimated(Animation::SlideSide::Right, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
						}
					} else if (ID == 2) {
						auto group1 = window->FindChild(101);
						auto group2 = window->FindChild(102);
						if (group1->IsVisible()) {
							group1->HideAnimated(Animation::SlideSide::Right, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
							group2->ShowAnimated(Animation::SlideSide::Left, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
						} else {
							group2->HideAnimated(Animation::SlideSide::Right, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
							group1->ShowAnimated(Animation::SlideSide::Left, 500,
								Animation::AnimationClass::Smooth, Animation::AnimationClass::Smooth);
						}
					}
				}
				virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
				{
					if (event == Windows::FrameEvent::Close) window->Destroy();
				}
			};
			class _thl : public Controls::Edit::IEditHook
			{
			public:
				virtual string Filter(Controls::Edit * sender, const string & input) override
				{
					return input.Replace(L'0', L"ЫЫЫ");
				}
				virtual Array<uint8> * ColorHighlight(Controls::Edit * sender, const Array<uint32> & text) override
				{
					auto result = new Array<uint8>(text.Length());
					for (int i = 0; i < text.Length(); i++) {
						if (text[i] > 0xFFFF) result->Append(1);
						else result->Append(0);
					}
					return result;
				}
				virtual Array<UI::Color> * GetPalette(Controls::Edit * sender) override
				{
					auto result = new Array<UI::Color>(10);
					result->Append(Color(255, 0, 0));
					return result;
				}
			};
			class _thl2 : public Controls::MultiLineEdit::IMultiLineEditHook
			{
			public:
				virtual string Filter(Controls::MultiLineEdit * sender, const string & input, Point at) override
				{
					return input.Replace(L'0', L"ЫЫЫ");
				}
				virtual Array<uint8> * ColorHighlight(Controls::MultiLineEdit * sender, const Array<uint32> & text, int line) override
				{
					auto result = new Array<uint8>(text.Length());
					for (int i = 0; i < text.Length(); i++) {
						if (text[i] > 0xFFFF) result->Append(1);
						else if (text[i] >= L'0' && text[i] <= L'9') result->Append(2);
						else result->Append(0);
					}
					return result;
				}
				virtual Array<UI::Color> * GetPalette(Controls::MultiLineEdit * sender) override
				{
					auto result = new Array<UI::Color>(10);
					result->Append(Color(255, 0, 0));
					result->Append(Color(0, 0, 255));
					return result;
				}
			};
			auto Callback2 = new _cb2;
			auto Hook = new _thl;
			auto Hook2 = new _thl2;

			//::Template->Dialog[L"Test3"]->Properties->GetProperty(L"BackgroundColor").Set<UI::Color>(0);
			//::Template->Dialog[L"Test3"]->Properties->GetProperty(L"Background").Set<SafePointer<UI::Template::Shape>>(0);
			//::Template->Dialog[L"Test3"]->Properties->GetProperty(L"DefaultBackground").Set<bool>(false);

			auto templ = ::Template->Dialog[L"Test4"];
			{
				auto bookmark_ctl_props = new Template::Controls::BookmarkView;
				bookmark_ctl_props->ControlPosition = UI::Rectangle(
					Coordinate(0, 5.0, 0), Coordinate(0, 5.0, 0), Coordinate(0, -5.0, 1.0), Coordinate(0, -5.0, 1.0)
				);
				bookmark_ctl_props->Font = ::Template->Font[L"NormalFont"];
				bookmark_ctl_props->ID = 101;
				bookmark_ctl_props->Invisible = false;
				bookmark_ctl_props->TabHeight = int(Zoom * 28.0);
				bookmark_ctl_props->ViewBackground = ::Template->Application[L"WaffleFrame"];
				bookmark_ctl_props->ViewTabNormal = ::Template->Application[L"ButtonNormal"];
				bookmark_ctl_props->ViewTabHot = ::Template->Application[L"ButtonHot"];
				bookmark_ctl_props->ViewTabActive = ::Template->Application[L"ButtonPressed"];
				bookmark_ctl_props->ViewTabActiveFocused = ::Template->Application[L"ButtonFocused"];
				SafePointer<Template::ControlTemplate> bookmark_ctl = new Template::ControlTemplate(bookmark_ctl_props);
				templ->Children.Append(bookmark_ctl);
				auto bookmark1 = new Template::Controls::Bookmark;
				auto bookmark2 = new Template::Controls::Bookmark;
				auto bookmark3 = new Template::Controls::Bookmark;
				bookmark1->ID = 1; bookmark2->ID = 2; bookmark3->ID = 3;
				bookmark1->Text = L"kornevgen"; bookmark2->Text = L"pidor"; bookmark3->Text = L"Tab #3";
				bookmark1->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				bookmark2->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				bookmark3->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				SafePointer<Template::ControlTemplate> bookmark_o1 = new Template::ControlTemplate(bookmark1);
				SafePointer<Template::ControlTemplate> bookmark_o2 = new Template::ControlTemplate(bookmark2);
				SafePointer<Template::ControlTemplate> bookmark_o3 = new Template::ControlTemplate(bookmark3);
				bookmark_ctl->Children.Append(bookmark_o1); bookmark_ctl->Children.Append(bookmark_o2); bookmark_ctl->Children.Append(bookmark_o3);
				bookmark_o1->Children.Append(&::Template->Dialog[L"Test2"]->Children[0].Children[2].Children[0]);
				bookmark_o2->Children.Append(&::Template->Dialog[L"Test2"]->Children[0].Children[1].Children[0].Children[1].Children[0]);
				bookmark_o3->Children.Append(&::Template->Dialog[L"Test2"]->Children[0].Children[1].Children[0].Children[0].Children[0]);
			}
			auto w = Windows::CreateFramedDialog(::Template->Dialog[L"Test2"], 0, UI::Rectangle::Invalid(), station);
			auto w2 = Windows::CreateFramedDialog(::Template->Dialog[L"Test"], Callback, UI::Rectangle(0, 0, Coordinate(0, 0.0, 0.7), Coordinate(0, 0.0, 0.55)), station);
			auto w3 = Windows::CreateFramedDialog(::Template->Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid(), station);
			//auto w4 = Windows::CreateFramedDialog(::Template->Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid(), 0);
			//auto w5 = Windows::CreateFramedDialog(::Template->Dialog[L"Test2"], 0, UI::Rectangle::Invalid(), 0);
			auto w6 = Windows::CreateFramedDialog(::Template->Dialog[L"Test4"], 0, UI::Rectangle::Invalid(), 0);
			//w2->FindChild(7777)->As<Controls::ColorView>()->SetColor(0xDDFF8040);

			w2->FindChild(7777)->As<Controls::ColorView>()->SetColor(UI::GetSystemColor(UI::SystemColor::Theme));

			w2->SetText(L"window");

			w->AddDialogStandardAccelerators();
			w2->AddDialogStandardAccelerators();
			w3->AddDialogStandardAccelerators();
			//w5->AddDialogStandardAccelerators();
			w6->AddDialogStandardAccelerators();

			w->FindChild(101010)->As<Controls::Edit>()->CaretWidth = 4;
			w->FindChild(101010)->As<Controls::Edit>()->CaretColor = UI::Color(64, 64, 128);
			w->FindChild(101010)->As<Controls::Edit>()->SetHook(Hook);
			//w5->FindChild(101010)->As<Controls::Edit>()->SetHook(Hook);
			w3->FindChild(212121)->As<Controls::MultiLineEdit>()->SetHook(Hook2);
			//w4->FindChild(212121)->As<Controls::MultiLineEdit>()->SetHook(Hook2);

			w->Show(true);
			w2->Show(true);
			w3->Show(true);
			w6->Show(true);
			//w4->Show(true);
			//w5->Show(true);

			//WindowsSpecific::SetWindowTaskbarProgressDisplayMode(w4, WindowsSpecific::WindowTaskbarProgressDisplayMode::Normal);
			//WindowsSpecific::SetWindowTaskbarProgressValue(w4, 0.7);
			//WindowsSpecific::SetWindowTransparentcy(w4, 0.7);

			(*conout) << L"Done!" << IO::NewLineChar;
		}
	}

	NativeWindows::RunMainMessageLoop();
    
    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTS));
    wcex.hCursor        = 0;
    wcex.hbrBackground  = 0;
    wcex.lpszMenuName   = 0;
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Сохранить дескриптор экземпляра в глобальной переменной

   HWND hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, nullptr, nullptr, hInstance, nullptr);

   ::Window = hWnd;

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_COMMAND:
        {
            int wmId = LOWORD(wParam);
            // Разобрать выбор в меню:
            switch (wmId)
            {
            case IDM_EXIT:
                DestroyWindow(hWnd);
                break;
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
	case WM_ACTIVATE:
		SetTimer(hWnd, 1, 25, 0);
		break;
	case WM_TIMER:
		InvalidateRect(hWnd, 0, FALSE);
		if (station) return station->ProcessWindowEvents(message, wParam, lParam);
		return DefWindowProc(hWnd, message, wParam, lParam);
    case WM_PAINT:
        {
			RECT Rect;
			GetClientRect(hWnd, &Rect);
            HDC hdc = GetDC(hWnd);
			//FillRect(hdc, &Rect, (HBRUSH) GetStockObject(LTGRAY_BRUSH));

			//ValidateRect(hWnd, 0);
			if (Target) {
				Device->SetTimerValue(GetTimerValue());
				if (station) station->Animate();
				Target->SetDpi(96.0f, 96.0f);
				Target->BeginDraw();
				if (station) station->Render();
				Target->EndDraw();
				SwapChain->Present(1, 0);
			}
			ValidateRect(hWnd, 0);
            ReleaseDC(hWnd, hdc);
        }
        break;
	case WM_SIZE:
	{
		RECT Rect;
		GetClientRect(hWnd, &Rect);
		Direct3D::ResizeRenderBufferForD2DDevice(Target, SwapChain);
		if (station) station->ProcessWindowEvents(message, wParam, lParam);
	}
		break;
    case WM_DESTROY:
		NativeWindows::ExitMainLoop();
        break;
    default:
		if (station) return station->ProcessWindowEvents(message, wParam, lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}