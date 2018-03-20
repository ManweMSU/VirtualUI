// Tests.cpp: определяет точку входа для приложения.
//

#include <Miscellaneous/DynamicString.h>
#include <UserInterface/ShapeBase.h>
#include <UserInterface/Templates.h>
#include <UserInterface/ControlBase.h>
#include <Streaming.h>
#include <PlatformDependent/Direct2D.h>
#include <Miscellaneous/Dictionary.h>
#include <UserInterface/ControlClasses.h>
#include <UserInterface/BinaryLoader.h>
#include <UserInterface/StaticControls.h>
#include <UserInterface/ButtonControls.h>
#include <UserInterface/GroupControls.h>
#include <UserInterface/Menues.h>
#include <UserInterface/OverlappedWindows.h>
#include <Syntax/Tokenization.h>
#include <Syntax/Grammar.h>

#include "stdafx.h"
#include "Tests.h"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

#include <d3d11_1.h>
#pragma comment(lib, "d3d11.lib")

#include <PlatformDependent/WindowStation.h>

#undef CreateWindow
#undef GetCurrentDirectory

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

ID3D11Device * D3DDevice;
ID3D11DeviceContext * D3DDeviceContext;
IDXGIDevice1 * DXGIDevice;
ID2D1Device * D2DDevice;
ID2D1DeviceContext * Target = 0;
IDXGISwapChain1 * SwapChain = 0;

SafePointer<Engine::Streaming::TextWriter> conout;
SafePointer<Engine::UI::InterfaceTemplate> Template;

UI::IInversionEffectRenderingInfo * Inversion = 0;

SafePointer<Menues::Menu> menu;

ENGINE_PACKED_STRUCTURE struct TestPacked
{
	uint8 foo;
	uint32 bar;
	uint16 foobar;
};
ENGINE_END_PACKED_STRUCTURE

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
	spelling.ContinuousCharCombos << L"#";
	spelling.ContinuousCharCombos << L"=";
	spelling.ContinuousCharCombos << L"+";
	spelling.ContinuousCharCombos << L"-";
	spelling.ContinuousCharCombos << L"*";
	spelling.ContinuousCharCombos << L"/";
	spelling.ContinuousCharCombos << L"%";
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	SetProcessDPIAware();

	AllocConsole();
	SetConsoleTitleW(L"ui tests");
	SafePointer<Engine::Streaming::FileStream> constream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
	conout.SetReference(new Engine::Streaming::TextWriter(constream));

	(*conout) << IO::GetCurrentDirectory() << IO::NewLineChar;
	(*conout) << L"Full path      : " << IO::GetExecutablePath() << IO::NewLineChar;
	(*conout) << L"Directory      : " << IO::Path::GetDirectory(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"File name      : " << IO::Path::GetFileName(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"Clear file name: " << IO::Path::GetFileNameWithoutExtension(IO::GetExecutablePath()) << IO::NewLineChar;
	(*conout) << L"Extension      : " << IO::Path::GetExtension(IO::GetExecutablePath()) << IO::NewLineChar;

	{
		Syntax::Spelling blang;
		CreateBlangSpelling(blang);

		SafePointer<Streaming::FileStream> Input = new Streaming::FileStream(L"test.blbl", Streaming::AccessRead, Streaming::OpenExisting);
		Array<uint8> Data;
		Data.SetLength(Input->Length() - 2);
		Input->Seek(2, Streaming::Begin);
		Input->Read(Data, Input->Length() - 2);
		SafePointer< Array<Syntax::Token> > Tokens;
		string text = string(Data, Input->Length() / 2 - 1, Encoding::UTF16);
		try {
			Tokens.SetReference(Syntax::ParseText(text, blang));
		}
		catch (Syntax::ParserSpellingException & e) {
			(*conout) << e.Comments << IO::NewLineChar;
			(*conout) << text.Fragment(e.Position, -1) << IO::NewLineChar;
		}

		if (Tokens) {
			(*conout) << L"==============================" << IO::NewLineChar;
			for (int i = 0; i < Tokens->Length(); i++) {
				(*conout) << Tokens->ElementAt(i).Content << IO::NewLineChar;
			}
			(*conout) << L"==============================" << IO::NewLineChar;
		}
		Syntax::Grammar grammar;
		SafePointer<Syntax::Grammar::GrammarRule> expression = new Syntax::Grammar::GrammarRule;
		SafePointer<Syntax::Grammar::GrammarRule> composition = new Syntax::Grammar::GrammarRule;
		SafePointer<Syntax::Grammar::GrammarRule> add_operation = new Syntax::Grammar::GrammarRule;
		SafePointer<Syntax::Grammar::GrammarRule> mul_operation = new Syntax::Grammar::GrammarRule;
		SafePointer<Syntax::Grammar::GrammarRule> operand = new Syntax::Grammar::GrammarRule;
		expression->Label = L"expression";
		composition->Label = L"composition";
		add_operation->Label = L"add_operation";
		mul_operation->Label = L"mul_operation";
		operand->Label = L"operand";

		expression->Class = Syntax::Grammar::GrammarRule::RuleClass::Sequence;
		Syntax::Grammar::GrammarRule temp, temp2, temp3;
		temp.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp.Reference = L"composition";
		expression->Rules.Append(temp);
		temp.Class = Syntax::Grammar::GrammarRule::RuleClass::Sequence;
		temp.MinRepeat = 0;
		temp.MaxRepeat = -1;
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Token;
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp2.Reference = L"add_operation";
		temp.Rules.Append(temp2);
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp2.Reference = L"composition";
		temp.Rules.Append(temp2);
		expression->Rules.Append(temp);

		composition->Class = Syntax::Grammar::GrammarRule::RuleClass::Sequence;
		temp.Rules.Clear();
		temp.MaxRepeat = temp.MinRepeat = 1;
		temp.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp.Reference = L"operand";
		composition->Rules.Append(temp);
		temp.Class = Syntax::Grammar::GrammarRule::RuleClass::Sequence;
		temp.MinRepeat = 0;
		temp.MaxRepeat = -1;
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Token;
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp2.Reference = L"mul_operation";
		temp.Rules.Append(temp2);
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp2.Reference = L"operand";
		temp.Rules.Append(temp2);
		composition->Rules.Append(temp);

		add_operation->Class = Syntax::Grammar::GrammarRule::RuleClass::Variant;
		temp3.Class = Syntax::Grammar::GrammarRule::RuleClass::Token;
		temp3.TokenClass = Syntax::Token::CharacterToken(L"+");
		add_operation->Rules.Append(temp3);
		temp3.TokenClass = Syntax::Token::CharacterToken(L"-");
		add_operation->Rules.Append(temp3);

		mul_operation->Class = Syntax::Grammar::GrammarRule::RuleClass::Variant;
		temp3.TokenClass = Syntax::Token::CharacterToken(L"*");
		mul_operation->Rules.Append(temp3);
		temp3.TokenClass = Syntax::Token::CharacterToken(L"/");
		mul_operation->Rules.Append(temp3);
		temp3.TokenClass = Syntax::Token::CharacterToken(L"%");
		mul_operation->Rules.Append(temp3);

		operand->Class = Syntax::Grammar::GrammarRule::RuleClass::Variant;
		temp.Rules.Clear();
		temp.MaxRepeat = temp.MinRepeat = 1;
		temp.Class = Syntax::Grammar::GrammarRule::RuleClass::Sequence;
		temp3.TokenClass = Syntax::Token::CharacterToken(L"(");
		temp.Rules.Append(temp3);
		temp2.Class = Syntax::Grammar::GrammarRule::RuleClass::Reference;
		temp2.Reference = L"expression";
		temp.Rules.Append(temp2);
		temp3.TokenClass = Syntax::Token::CharacterToken(L")");
		temp.Rules.Append(temp3);
		operand->Rules.Append(temp);
		temp3.TokenClass = Syntax::Token::IdentifierToken();
		operand->Rules.Append(temp3);
		temp3.TokenClass = Syntax::Token::ConstantToken(Syntax::TokenConstantClass::Numeric);
		operand->Rules.Append(temp3);

		grammar.Rules.Append(expression->Label, expression);
		grammar.Rules.Append(composition->Label, composition);
		grammar.Rules.Append(add_operation->Label, add_operation);
		grammar.Rules.Append(mul_operation->Label, mul_operation);
		grammar.Rules.Append(operand->Label, operand);

		expression->BuildBeginnings(grammar);

		(*conout) << L"==============================" << IO::NewLineChar;
	}

	Engine::Direct2D::InitializeFactory();

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

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_TESTS));

    MSG msg;

	// Starting D3D
	{
		D3D_FEATURE_LEVEL lvl[] = {
			D3D_FEATURE_LEVEL_11_1,
			D3D_FEATURE_LEVEL_11_0,
			D3D_FEATURE_LEVEL_10_1,
			D3D_FEATURE_LEVEL_10_0,
			D3D_FEATURE_LEVEL_9_3,
			D3D_FEATURE_LEVEL_9_2,
			D3D_FEATURE_LEVEL_9_1
		};
		D3D_FEATURE_LEVEL selected;

		D3D11CreateDevice(0, D3D_DRIVER_TYPE_HARDWARE, 0, D3D11_CREATE_DEVICE_BGRA_SUPPORT, lvl, 7, D3D11_SDK_VERSION, &D3DDevice, &selected, &D3DDeviceContext);
		D3DDevice->QueryInterface(__uuidof(IDXGIDevice1), (void**) &DXGIDevice);
		Direct2D::D2DFactory->CreateDevice(DXGIDevice, &D2DDevice);
		D2DDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &Target);

		DXGI_SWAP_CHAIN_DESC1 scd;
		RtlZeroMemory(&scd, sizeof(scd));
		scd.Width = 0;
		scd.Height = 0;
		scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		scd.Stereo = false;
		scd.SampleDesc.Count = 1;
		scd.SampleDesc.Quality = 0;
		scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scd.BufferCount = 2;
		scd.Scaling = DXGI_SCALING_NONE;
		scd.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
		scd.Flags = 0;
		scd.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
		scd.Scaling = DXGI_SCALING_STRETCH;

		SafePointer<IDXGIAdapter> Adapter;
		DXGIDevice->GetAdapter(Adapter.InnerRef());
		SafePointer<IDXGIFactory2> Factory;
		Adapter->GetParent(IID_PPV_ARGS(Factory.InnerRef()));
		HRESULT r = Factory->CreateSwapChainForHwnd(D3DDevice, ::Window, &scd, 0, 0, &SwapChain);
		DXGIDevice->SetMaximumFrameLatency(1);

		D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0.0f, 0.0f);
		SafePointer<IDXGISurface> Surface;
		SwapChain->GetBuffer(0, IID_PPV_ARGS(Surface.InnerRef()));
		SafePointer<ID2D1Bitmap1> Bitmap;
		Target->CreateBitmapFromDxgiSurface(Surface, props, Bitmap.InnerRef());
		Target->SetTarget(Bitmap);
	}
	Device = new Engine::Direct2D::D2DRenderDevice(Target);
	Inversion = Device->CreateInversionEffectRenderingInfo();
	{
		Array<UI::GradientPoint> ps;
		ps << UI::GradientPoint(UI::Color(0, 255, 0), 0.0);
		ps << UI::GradientPoint(UI::Color(0, 0, 255), 1.0);
		::Shape = new FrameShape(UI::Rectangle(100, 100, Coordinate::Right(), 600), FrameShape::FrameRenderMode::Layering, 0.5);
		auto s2 = new BarShape(UI::Rectangle(100, 100, Coordinate(-100, 0.0, 1.0), 300), ps, 3.141592 / 6.0);
		::Shape->Children.Append(s2);
		s2->Release();
		s2 = new BarShape(UI::Rectangle(0, -50, 200, 200), UI::Color(255, 0, 255));
		::Shape->Children.Append(s2);
		s2->Release();

		{
			UI::Zoom = 2.0;
			::Template.SetReference(new Engine::UI::InterfaceTemplate());
			{
				SafePointer<Streaming::Stream> Source = new Streaming::FileStream(L"Test.eui", Streaming::AccessRead, Streaming::OpenExisting);
				struct _loader : public IResourceLoader
				{
					virtual ITexture * LoadTexture(Streaming::Stream * Source) override
					{
						return Device->LoadTexture(Source);
					}
					virtual ITexture * LoadTexture(const string & Name) override
					{
						SafePointer<Streaming::Stream> Source = new Streaming::FileStream(Name, Streaming::AccessRead, Streaming::OpenExisting);
						return Device->LoadTexture(Source);
					}
					virtual UI::IFont * LoadFont(const string & FaceName, int Height, int Weight, bool IsItalic, bool IsUnderline, bool IsStrikeout) override
					{
						return Device->LoadFont(FaceName, Height, Weight, IsItalic, IsUnderline, IsStrikeout);
					}
					virtual void ReloadTexture(ITexture * Texture, Streaming::Stream * Source) override
					{
						Texture->Reload(Device, Source);
					}
					virtual void ReloadTexture(ITexture * Texture, const string & Name) override
					{
						SafePointer<Streaming::Stream> Source = new Streaming::FileStream(Name, Streaming::AccessRead, Streaming::OpenExisting);
						Texture->Reload(Device, Source);
					}
					virtual void ReloadFont(UI::IFont * Font) override
					{
						Font->Reload(Device);
					}
				} loader;
				Engine::UI::Loader::LoadUserInterfaceFromBinary(*::Template, Source, &loader, 0);
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
				Back->Texture = ::Template->Texture[L"Wallpaper"];
				SafePointer<Template::BarShape> Fill = new Template::BarShape;
				Fill->Gradient << GradientPoint(0xFF303050);
				back->Children.Append(Back);
				back->Children.Append(Fill);
			}
			Main->Background.SetRetain(back);

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
					Template::Coordinate(Template::IntegerTemplate::Undefined(L"NegBorder"), 0.0, 1.0),
					Template::Coordinate(Template::IntegerTemplate::Undefined(L"Caption"), 0.0, 0.0)
				);
				Title->Font = ::Template->Font[L"NormalFont"];
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

			auto w = Windows::CreateFramedDialog(::Template->Dialog[L"Test"], Callback, UI::Rectangle::Invalid(), station);
			auto w2 = Windows::CreateFramedDialog(::Template->Dialog[L"Test"], Callback, UI::Rectangle(0, 0, Coordinate(0, 0.0, 0.5), Coordinate(0, 0.0, 0.4)), station);
			static_cast<Controls::ColorView *>(w2->FindChild(7777))->SetColor(0xDDFF8040);
			w2->SetText(L"window");

			(*conout) << L"Done!" << IO::NewLineChar;
		}
	}

    // Цикл основного сообщения:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
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
    case WM_PAINT:
        {
			RECT Rect;
			GetClientRect(hWnd, &Rect);
            HDC hdc = GetDC(hWnd);
			//FillRect(hdc, &Rect, (HBRUSH) GetStockObject(LTGRAY_BRUSH));

			//ValidateRect(hWnd, 0);
			if (Target) {
				Target->SetDpi(96.0f, 96.0f);
				Target->BeginDraw();
				Device->SetTimerValue(GetTimerValue());
				if (station) station->Render();
				Target->EndDraw();
				SwapChain->Present(1, 0);
			}

            ReleaseDC(hWnd, hdc);
        }
        break;
	case WM_SIZE:
	{
		RECT Rect;
		GetClientRect(hWnd, &Rect);
		if (SwapChain) {
			Target->SetTarget(0);
			HRESULT r = SwapChain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW, D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_IGNORE), 0.0f, 0.0f);
			SafePointer<IDXGISurface> Surface;
			SwapChain->GetBuffer(0, IID_PPV_ARGS(Surface.InnerRef()));
			SafePointer<ID2D1Bitmap1> Bitmap;
			Target->CreateBitmapFromDxgiSurface(Surface, props, Bitmap.InnerRef());
			Target->SetTarget(Bitmap);
		}
		if (station) station->ProcessWindowEvents(message, wParam, lParam);
	}
		break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
		if (station) return station->ProcessWindowEvents(message, wParam, lParam);
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}