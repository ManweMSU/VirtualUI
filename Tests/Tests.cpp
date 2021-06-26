﻿#include <EngineRuntime.h>

#include "stdafx.h"
#include "Tests.h"

using namespace Engine;

#include <Windows.h>
#include <ObjIdl.h>

#undef CreateWindow
#undef GetCommandLine

SafePointer<Graphics::IBitmap> synth;

class WindowCallback : public Windows::IWindowCallback
{
	Box grad_box;
	Windows::I2DPresentationEngine * engine;
	SafePointer<Graphics::IFont> font;
	SafePointer<Graphics::IColorBrush> bar, grad;
	SafePointer<Graphics::IDevice> device;
	SafePointer<Graphics::IWindowLayer> layer;
	SafePointer<Graphics::ITexture> texture;
	SafePointer<Graphics::IBitmapBrush> texture_brush;
	SafePointer<UI::FrameShape> shape;
public:
	virtual void Created(Windows::IWindow * window) override
	{
		SafePointer<Graphics::I2DDeviceContextFactory> factory = Graphics::CreateDeviceContextFactory();
		font = factory->LoadFont(Graphics::SystemMonoSerifFont, 100, 400, false, false, false);
		engine = window->Set2DRenderingDevice();
		grad_box = Box(10, 10, 60, 60);
		shape = new UI::FrameShape(UI::Rectangle::Entire());
		SafePointer<UI::Shape> text = new UI::TextShape(UI::Rectangle::Entire(), L"TEXT-1 TEXT-2\nTEXT-3 TEXT-4 TEXT-5 TEXT-6", font, Color(0, 0, 255),
			UI::TextShape::TextRenderAlignCenter | UI::TextShape::TextRenderAlignVCenter | UI::TextShape::TextRenderMultiline);
		SafePointer<UI::Shape> fx = new UI::BlurEffectShape(UI::Rectangle::Entire(), 5.0);
		SafePointer<UI::Shape> bm = new UI::TextureShape(UI::Rectangle::Entire(), synth, UI::Rectangle::Entire(), UI::TextureShape::TextureRenderMode::AsIs);
		SafePointer<UI::Shape> clr = new UI::BarShape(UI::Rectangle(10, 150, 50, 190), Color(0, 128, 255));
		shape->Children.Append(clr);
		shape->Children.Append(text);
		shape->Children.Append(bm);
		shape->Children.Append(fx);
	}
	virtual void Destroyed(Windows::IWindow * window) override { Windows::GetWindowSystem()->ExitMainLoop(); }
	virtual void RenderWindow(Windows::IWindow * window) override
	{
		auto size = window->GetClientSize();
		Graphics::I2DDeviceContext * device;
		SafePointer<Graphics::ITexture> rt;
		Graphics::IDeviceContext * context;
		if (engine) {
			engine->BeginRenderingPass();
			device = engine->GetContext();
		}
		if (layer) {
			rt = layer->QuerySurface();
			context = this->device->GetDeviceContext();
			Graphics::RenderTargetViewDesc rtv;
			ZeroMemory(&rtv.ClearValue, sizeof(rtv.ClearValue));
			rtv.LoadAction = Graphics::TextureLoadAction::Clear;
			rtv.Texture = rt;
			context->BeginRenderingPass(1, &rtv, 0);
			context->EndCurrentPass();
			context->Begin2DRenderingPass(rt);
			device = context->Get2DContext();
			size.x = rt->GetWidth();
			size.y = rt->GetHeight();
		}
		if (!bar) {
			bar = device->CreateSolidColorBrush(Color(0, 0, 128, 64));
		}
		if (!texture_brush && texture) {
			texture_brush = device->CreateTextureBrush(texture, Graphics::TextureAlphaMode::Ignore);
		}
		if (!grad) {
			GradientPoint pt[3];
			pt[0].Value = Color(255, 0, 0, 255);
			pt[0].Position = 0.0;
			pt[1].Value = Color(255, 255, 0, 255);
			pt[1].Position = 0.5;
			pt[2].Value = Color(0, 255, 0, 255);
			pt[2].Position = 1.0;
			grad = device->CreateGradientBrush(Point(0, 0), Point(50, 50), pt, 3);
		}
		device->Render(bar, Box(10, 10, size.x - 10, size.y - 10));
		device->Render(grad, grad_box);
		shape->Render(device, Box(0, 0, size.x, size.y));
		if (texture_brush) {
			device->Render(texture_brush, Box(size.x - 50, 10, size.x - 10, 50));
		}
		if (engine) {
			engine->EndRenderingPass();
		}
		if (layer) {
			context->EndCurrentPass();
			rt.SetReference(0);
			layer->Present();
		}
	}
	virtual void WindowClose(Windows::IWindow * window) override
	{
		window->Destroy();
	}
	virtual bool KeyDown(Windows::IWindow * window, int key_code) override
	{
		if (key_code == KeyCodes::V) {
			SafePointer<Codec::Frame> frame;
			if (Clipboard::GetData(frame.InnerRef())) {
				window->SetBackbufferedRenderingDevice(frame, Windows::ImageRenderMode::FitKeepAspectRatio, Color(0x80, 0x00, 0xFF, 0x80));
			}
		} else if (key_code == KeyCodes::L) {
			engine = 0;
			bar.SetReference(0);
			shape->ClearCache();
			grad.SetReference(0);
			texture_brush.SetReference(0);
			SafePointer<Graphics::IDeviceFactory> factory = Graphics::CreateDeviceFactory();
			device = factory->CreateDefaultDevice();
			auto size = window->GetClientSize();
			Graphics::WindowLayerDesc desc;
			desc.Width = max(size.x, 1);
			desc.Height = max(size.y, 1);
			desc.Format = Graphics::PixelFormat::B8G8R8A8_unorm;
			desc.Usage = Graphics::ResourceUsageRenderTarget | Graphics::ResourceUsageShaderRead;
			layer = device->CreateWindowLayer(window, desc);
			SafePointer<Codec::Frame> frame = new Codec::Frame(40, 40, Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Straight);
			for (int y = 0; y < 40; y++) for (int x = 0; x < 40; x++) {
				int c = abs(x - 20) + abs(y - 20);
				if (c < 19) frame->SetPixel(x, y, Color(255, 0, 0, 128));
				else frame->SetPixel(x, y, Color(0, 0, 0, 0));
			}
			texture = Graphics::LoadTexture(device, frame, 1, Graphics::ResourceUsageShaderRead, Graphics::PixelFormat::B8G8R8A8_unorm,
				Graphics::ResourceMemoryPool::Immutable, Codec::AlphaMode::Premultiplied);
		} else if (key_code == KeyCodes::F) {
			if (layer) {
				if (layer->IsFullscreen()) layer->SwitchToWindow();
				else layer->SwitchToFullscreen();
			}
		} else if (key_code == KeyCodes::S) {
			if (layer) {
				auto size = window->GetClientSize();
				layer->ResizeSurface(max(size.x, 1), max(size.y, 1));
				window->InvalidateContents();
			}
		} else if (key_code == KeyCodes::Q) {
			grad_box.Left = grad_box.Top = (grad_box.Left + 40) % 200;
			grad_box.Right = grad_box.Bottom = grad_box.Left + 50;
			window->InvalidateContents();
		}
		return true;
	}
};
class WindowCallback2 : public UI::IEventCallback
{
public:
	virtual void Created(Windows::IWindow * window) override
	{

	}
	virtual void HandleControlEvent(Windows::IWindow * window, int ID, UI::ControlEvent event, UI::Control * sender) override
	{

	}
};

void PrintTree(IO::Console & cns, const Volumes::BinaryTree<int>::Element * element, int y, int x, int width)
{
	if (element) {
		auto repr = string(element->GetValue());
		auto l = repr.Length();
		cns.SetTextColor(element->IsBlack() ? 15 : 12);
		cns.MoveCaret(x + (width - l) / 2, y);
		cns.Write(repr);
		auto hw = width / 2;
		PrintTree(cns, element->GetLeft(), y + 1, x, hw);
		PrintTree(cns, element->GetRight(), y + 1, x + hw, width - hw);
	}
}
int CheckConsistency(IO::Console & cns, const Volumes::BinaryTree<int>::Element * element, const Volumes::BinaryTree<int>::Element * parent)
{
	auto value = element->GetValue();
	if (element->GetParent() != parent) { cns.WriteLine(FormatString(L"Check failed: wrong parent at %0", value)); throw Exception(); }
	int lh = 1;
	int rh = 1;
	if (element->GetLeft()) lh = CheckConsistency(cns, element->GetLeft(), element);
	if (element->GetRight()) rh = CheckConsistency(cns, element->GetRight(), element);
	if (parent) {
		if (!parent->IsBlack() && !element->IsBlack()) { cns.WriteLine(FormatString(L"Check failed: sequential red nodes at %0", value)); throw Exception(); }
	}
	if (lh != rh) { cns.WriteLine(FormatString(L"Check failed: wrong left and right black heights at %0", value)); throw Exception(); }
	if (element->IsBlack()) return lh + 1; else return lh;
}

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

	IO::Console cns;

	SafePointer<Windows::IScreen> screen = Windows::GetDefaultScreen();
	UI::CurrentScaleFactor = screen->GetDpiScale();
	UI::InterfaceTemplate ui;
	SafePointer<Streaming::Stream> uis = new Streaming::FileStream(L"C:/Users/Manwe/Documents/GitHub/VirtualUI/Debug/test.eui", Streaming::AccessRead, Streaming::OpenExisting);
	UI::Loader::LoadUserInterfaceFromBinary(ui, uis);
	uis.SetReference(0);

	auto fact = Graphics::CreateDeviceContextFactory();
	auto surface = fact->CreateBitmap(512, 256, Color(128, 0, 255, 128));
	auto device = fact->CreateBitmapContext();
	fact->Release();
	device->BeginRendering(surface);
	GradientPoint pt[] = { GradientPoint(Color(255, 0, 0), 0.0), GradientPoint(Color(255, 255, 0), 0.5), GradientPoint(Color(0, 255, 0), 1.0) };
	auto grad = device->CreateGradientBrush(Point(0, 0), Point(128, 128), pt, 3);
	device->Render(grad, Box(256, 128, 512, 256));
	grad->Release();
	device->EndRendering();
	device->Release();
	synth.SetRetain(surface);
	surface->Release();

	Codec::InitializeDefaultCodecs();
	
	WindowCallback callback;
	WindowCallback2 callback2;
	Windows::CreateWindowDesc desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Callback = &callback;
	desc.Flags = Windows::WindowFlagCloseButton | Windows::WindowFlagHasTitle |
		Windows::WindowFlagBlurBehind |
		Windows::WindowFlagTransparent |
		Windows::WindowFlagBlurFactor |
		Windows::WindowFlagMinimizeButton | Windows::WindowFlagMaximizeButton | Windows::WindowFlagSizeble;
	desc.MinimalConstraints = Point(300, 200);
	desc.Position = Box(100, 100, 800, 600);
	desc.Title = L"New Blur Behind Test";
	desc.BlurFactor = 150.0;
	auto window = Windows::GetWindowSystem()->CreateWindow(desc);
	auto window2 = UI::CreateWindow(ui.Dialog[L"Test2"], &callback2, UI::Rectangle::Entire());
	window->Show(true);
	window2->Show(true);
	cns.WriteLine(window->GetBackgroundFlags());
	Windows::GetWindowSystem()->RunMainLoop();
	return 0;
}