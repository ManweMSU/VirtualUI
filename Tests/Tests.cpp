// Tests.cpp: определяет точку входа для приложения.
//

#include "..\\VirtualUI\\ShapeBase.h"
#include "..\\VirtualUI\\Streaming.h"
#include "..\\VirtualUI\\PlatformDependent\\Direct2D.h"

#include "stdafx.h"
#include "Tests.h"

#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")

using namespace Engine;
using namespace Reflection;

#define MAX_LOADSTRING 100

// Глобальные переменные:
HINSTANCE hInst;                                // текущий экземпляр
WCHAR szTitle[MAX_LOADSTRING];                  // Текст строки заголовка
WCHAR szWindowClass[MAX_LOADSTRING];            // имя класса главного окна

// Отправить объявления функций, включенных в этот модуль кода:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

ID2D1DCRenderTarget * Target;
Engine::Direct2D::D2DRenderDevice * Device;
Engine::UI::FrameShape * Shape;
Engine::UI::ITexture * Texture;

class test : public Reflection::ReflectableObject
{
public:
	DECLARE_PROPERTY(int, value)
	DECLARE_PROPERTY(Coordinate, coord)
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

	CoInitializeEx(0, COINIT::COINIT_APARTMENTTHREADED);
	SetProcessDPIAware();

	/*auto spl = (L"abcabcabcab" + string(-1234)).Replace(L"bc", L"XYU").LowerCase().Split(L'-');
	SortArray(spl, true);
	for (int i = 0; i < spl.Length(); i++) MessageBox(0, spl[i], L"", 0);

	ObjectArray<string> safe;
	safe << new string(L"blablabla");
	safe.Append(new string(L"4epHblu Hurrep"));
	safe.Append(new string(L"kornevgen pidor"));
	safe.Append(new string(L"hui"));
	safe.Append(new string((void*) &safe));

	test t;
	auto p = t.GetProperty(L"value");
	int v = 666;
	p->Set(&v);
	MessageBox(0, string(t.value), 0, 0);

	SafePointer<string> r(new string(L"pidor"));
	MessageBox(0, *r, L"", 0);
	SafePointer<string> r2(new string(L"pidor"));
	MessageBox(0, string(r == r2), L"", 0);

	for (int i = 0; i < safe.Length(); i++) safe[i].Release();
	SortArray(safe);
	MessageBox(0, safe.ToString(), L"", 0);
	safe.Clear();
	MessageBox(0, safe.ToString(), L"", 0);*/

	auto v = string(L";123456").ToDouble(L";");

	AllocConsole();
	SetConsoleTitleW(L"ui tests");
	SafePointer<Engine::Streaming::FileStream> constream = new Engine::Streaming::FileStream(Engine::IO::GetStandartOutput());
	SafePointer<Engine::Streaming::TextWriter> conout = new Engine::Streaming::TextWriter(constream);
	{
		conout->WriteLine(string(L"xyu"));

		(*conout) << string(333.666555444333222111, L',') << IO::NewLineChar;
	}

	Engine::Direct2D::InitializeFactory();
	D2D1_RENDER_TARGET_PROPERTIES rtp;
	rtp.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
	rtp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
	rtp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
	rtp.dpiX = 0.0f;
	rtp.dpiY = 0.0f;
	rtp.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
	rtp.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
	if (Engine::Direct2D::D2DFactory->CreateDCRenderTarget(&rtp, &Target) != S_OK) MessageBox(0, L"XYU.", 0, MB_OK | MB_ICONSTOP);
	Device = new Engine::Direct2D::D2DRenderDevice(Target);

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

	auto General = new FrameShape(UI::Rectangle::Entire());
	{
		SafePointer<Streaming::FileStream> Input = new Streaming::FileStream(L"bl.gif", Streaming::AccessRead, Streaming::OpenExisting);
		Texture = Device->LoadTexture(Input);
		(*(conout.Inner())) << Texture->GetWidth() << L"x" << Texture->GetHeight() << IO::NewLineChar;
		auto ts = new TextureShape(UI::Rectangle(UI::Coordinate(0, 0.0, 0.5), UI::Coordinate(0, 0.0, 0.5), UI::Coordinate(0, 0.0, 0.75), UI::Coordinate(0, 0.0, 0.75)), Texture, UI::Rectangle::Entire(), TextureShape::TextureRenderMode::FillPattern);
		Texture->Release();
		General->Children.Append(ts);
		ts->Release();

		auto ls = new LineShape(UI::Rectangle(UI::Coordinate(0, 0.0, 0.0), UI::Coordinate(0, 0.0, 0.3), UI::Coordinate::Right(), UI::Coordinate(0, 0.0, 0.3)), Color(255, 0, 255), true);
		General->Children.Append(ls);
		ls->Release();

		float v = 0.0f;
		UI::IFont * Font = Device->LoadFont(L"Segoe UI", 40, 400, false, true, false);
		auto fs = new TextShape(UI::Rectangle(UI::Coordinate(0, 0.0, 0.5), UI::Coordinate(0, 0.0, 0.3), UI::Coordinate::Right(), UI::Coordinate(0, 0.0, 0.5)), L"kornevgen pidor \xD83C\xDF44\xD83C\xDF44\xD83C\xDF44 корневген пидор " + string(5.0 / v), Font, Color(128, 128, 255, 255),
			UI::TextShape::TextHorizontalAlign::Center, UI::TextShape::TextVerticalAlign::Center);
		General->Children.Append(fs);
		fs->Release();
		Font->Release();

		auto bs = new BarShape(UI::Rectangle(UI::Coordinate(0, 0.0, 0.5), UI::Coordinate(0, 0.0, 0.3), UI::Coordinate::Right(), UI::Coordinate(0, 0.0, 0.5)), Color(192, 255, 128));
		General->Children.Append(bs);
		bs->Release();
	}
	General->Children.Append(::Shape);
	::Shape->Release();
	::Shape = General;
	s2 = new BarShape(UI::Rectangle::Entire(), Color(192, 192, 192));
	::Shape->Children.Append(s2);
	s2->Release();

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

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_TESTS));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
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

			Target->BindDC(hdc, &Rect);
			Target->SetDpi(96.0f, 96.0f);
			Target->BeginDraw();
			Device->SetTimerValue(GetTimerValue());
			::Shape->Render(Device, Box(Rect.left, Rect.top, Rect.right, Rect.bottom));
			Target->EndDraw();

            ReleaseDC(hWnd, hdc);
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}