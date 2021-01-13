#include <EngineRuntime.h>

#include <PlatformDependent/SystemColors.h>
#include <PlatformSpecific/MacWindowEffects.h>

ENGINE_REFLECTED_CLASS(lv_item, Engine::Reflection::Reflected)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text1)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text2)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text3)
ENGINE_END_REFLECTED_CLASS

using namespace Engine;
using namespace Engine::UI;
using namespace Engine::Network;

UI::InterfaceTemplate interface;

MacOSXSpecific::TouchBar * tb;

SafePointer<Graphics::IDevice> device;
SafePointer<Graphics::IWindowLayer> layer;

class _cb2 : public Windows::IWindowEventCallback
{
public:
	virtual void OnInitialized(UI::Window * window) override
	{
		window->As<Controls::OverlappedWindow>()->GetAcceleratorTable() << Accelerators::AcceleratorCommand(192939, KeyCodes::Return, false, false, true);
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
		if (!window) {
			IO::Console console;
			console.WriteLine(L"Status bar event ID = " + string(ID));
		}
		if (ID == 343434) {
			if (event == Window::Event::DoubleClick) {
				sender->As<Controls::ListView>()->CreateEmbeddedEditor(interface.Dialog[L"editor"],
					sender->As<Controls::ListView>()->GetLastCellID(),
					Rectangle::Entire())->FindChild(888888)->SetFocus();
			}
		} else if (ID == 353535) {
			if (event == Window::Event::DoubleClick) {
				sender->As<Controls::TreeView>()->CreateEmbeddedEditor(interface.Dialog[L"editor"], Rectangle::Entire())->FindChild(888888)->SetFocus();
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
		} else if (ID == 555) {
			UI::Color clr = static_cast<MacOSXSpecific::TouchBarColorPicker *>(tb->FindChild(555))->GetColor();
			UI::ITexture * tex;
			{
				Array<Math::Vector2> line;
				line << Math::Vector2(0.0, 0.0);
				line << Math::Vector2(200.0, 100.0);
				line << Math::Vector2(200.0, 200.0);
				line << Math::Vector2(20.0, 200.0);
				auto rd = UI::Windows::CreateNativeCompatibleTextureRenderingDevice(256, 256, Math::Color(1.0, 0.7, 0.7, 1.0));
				rd->BeginDraw();
				rd->FillPolygon(line.GetBuffer(), line.Length(), Math::Color(clr));
				rd->DrawPolygon(line.GetBuffer(), line.Length(), Math::Color(0.0, 0.0, 0.0, 0.5), 10.0);
				rd->EndDraw();
				tex = rd->GetRenderTargetAsTexture();
				rd->Release();
			}
			window->FindChild(789)->As<UI::Controls::Static>()->SetImage(tex);
			tex->Release();
			window->RequireRedraw();
		} else if (ID == 10101) {
			double value = static_cast<MacOSXSpecific::TouchBarSlider *>(tb->FindChild(10101))->GetPosition();
			Streaming::TextWriter(SafePointer<Streaming::Stream>(new Streaming::FileStream(IO::GetStandardOutput()))).WriteLine(L"Slider: " + string(value));
		} else if (ID == 192939 && layer) {
			if (layer->IsFullscreen()) {
				layer->SwitchToWindow();
			} else {
				layer->SwitchToFullscreen();
			}
		}
	}
	virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
	{
		IO::Console console;
		if (event == Windows::FrameEvent::Close) {
			window->Destroy();
			Windows::ExitMessageLoop();
			return;
		} else if (event == Windows::FrameEvent::Move) {
			console.SetTextColor(14);
			console.WriteLine(L"Move Window");
			auto box = window->GetStation()->GetBox();
			if (layer) layer->ResizeSurface(box.Right - box.Left, box.Bottom - box.Top);
		} else if (event == Windows::FrameEvent::Minimize) {
			console.SetTextColor(14);
			console.WriteLine(L"Minimize Window");
		} else if (event == Windows::FrameEvent::Maximize) {
			console.SetTextColor(14);
			console.WriteLine(L"Maximize Window");
		} else if (event == Windows::FrameEvent::Restore) {
			console.SetTextColor(14);
			console.WriteLine(L"Restore Window");
		} else if (event == Windows::FrameEvent::Activate) {
			console.SetTextColor(14);
			console.WriteLine(L"Activate Window");
		} else if (event == Windows::FrameEvent::Deactivate) {
			console.SetTextColor(14);
			console.WriteLine(L"Deactivate Window");
		} else if (event == Windows::FrameEvent::SessionEnding) {
			console.SetTextColor(14);
			console.WriteLine(L"Session is ending");
		} else if (event == Windows::FrameEvent::SessionEnd) {
			console.SetTextColor(14);
			console.WriteLine(L"End session");
		} else if (event == Windows::FrameEvent::Draw) {
			console.SetTextColor(14);
			console.WriteLine(L"Draw");
			SafePointer<Graphics::ITexture> rt = layer->QuerySurface();
			IO::Console cns;
			cns.WriteLine(string(rt.Inner()));
			auto context = device->GetDeviceContext();
			if (context->Begin2DRenderingPass(rt)) {
				auto device_2d = context->Get2DRenderingDevice();
				device_2d->SetTimerValue(GetTimerValue());
				window->GetStation()->SetRenderingDevice(device_2d);
				window->GetStation()->Animate();
				window->GetStation()->Render();
				if (!context->EndCurrentPass()) cns.WriteLine(L"2D rendering pass finalization failed");
			} else cns.WriteLine(L"2D rendering pass failed");
			layer->Present();
		}
		if (Windows::IsWindowActive(window)) {
			console.SetTextColor(10);
		} else {
			console.SetTextColor(12);
		}
		console.Write(L"A");
		if (Windows::IsWindowMaximized(window)) {
			console.SetTextColor(10);
		} else {
			console.SetTextColor(12);
		}
		console.Write(L"Z");
		if (Windows::IsWindowMinimized(window)) {
			console.SetTextColor(10);
		} else {
			console.SetTextColor(12);
		}
		console.Write(L"I");
		console.SetTextColor(-1);
		console.WriteLine(L"");
	}
};

int Main(void)
{
	IO::Console Console;

	SafePointer< Array<IO::Search::Volume> > vols = IO::Search::GetVolumes();
	for (int i = 0; i < vols->Length(); i++) {
		Console << L"Volume \"" + vols->ElementAt(i).Label + L"\" at path " + vols->ElementAt(i).Path + IO::NewLineChar;
	}

	UI::Zoom = Windows::GetScreenScale();

	DynamicString req;
	req += Assembly::GetCurrentUserLocale() + IO::NewLineChar;
	try {
		SafePointer<Network::HttpSession> session = Network::OpenHttpSession(L"pidor");
		if (!session) throw Exception();
		SafePointer<Network::HttpConnection> connection = session->Connect(L"yandex.ru");
		if (!connection) throw Exception();
		SafePointer<Network::HttpRequest> request = connection->CreateRequest(L"/favicon.ico");
		if (!request) throw Exception();
		request->Send();
		req += string(request->GetStatus()) + string(IO::NewLineChar);
		SafePointer< Array<string> > hdrs = request->GetHeaders();
		for (int i = 0; i < hdrs->Length(); i++) {
			req += hdrs->ElementAt(i) + L": " + request->GetHeader(hdrs->ElementAt(i)) + IO::NewLineChar;
		}
	} catch (...) { req += L"kornevgen pidor"; }

	Streaming::Stream * source = Assembly::QueryResource(L"GUI");
	SafePointer<UI::IResourceLoader> loader = Windows::CreateNativeCompatibleResourceLoader();
	UI::Loader::LoadUserInterfaceFromBinary(interface, source, loader, 0);
	source->Release();

	UI::ITexture * tex = 0;
	try {
		string host = L"i.pinimg.com";
		string url = L"/originals/80/7a/a2/807aa2cb5485f9e3bbb353b124428d93.jpg";
		SafePointer<HttpSession> session = OpenHttpSession();
		if (!session) return 1;
		SafePointer<HttpConnection> connection = session->SecureConnect(host);
		if (!connection) return 1;
		SafePointer<HttpRequest> request = connection->CreateRequest(url);
		if (!connection) return 1;
		request->Send();
		Streaming::MemoryStream stream(0x10000);
		request->GetResponceStream()->CopyToUntilEof(&stream);
		stream.Seek(0, Streaming::Begin);
		SafePointer<Codec::Image> image = Codec::DecodeImage(&stream);
		UI::Windows::SetApplicationIcon(image);
		SafePointer<UI::IResourceLoader> loader = UI::Windows::CreateNativeCompatibleResourceLoader();
		tex = loader->LoadTexture(image->Frames.FirstElement());
	} catch (Exception & e) { Console << e.ToString() << IO::NewLineChar; return 1; }

	Console << L"Screen scale: " << Windows::GetScreenScale() << IO::NewLineChar;

	auto Callback2 = new _cb2;
	auto templ = interface.Dialog[L"Test3"];
	templ->Properties->GetProperty(L"Background").Get< SafePointer<UI::Template::Shape> >().SetReference(0);
	templ->Children.ElementAt(1)->Children.ElementAt(0)->Properties->GetProperty(L"Image").Get< SafePointer<UI::ITexture> >().SetRetain(tex);
	templ->Children.ElementAt(1)->Children.ElementAt(0)->Properties->GetProperty(L"ID").Set<int>(789);
	tex->Release();
	auto w4 = Windows::CreateFramedDialog(templ, Callback2, UI::Rectangle::Invalid(), 0, true);
	w4->FindChild(212121)->SetText(string(L"Heart ❤️ Heart 💙 Heart 🧡💛💚💜🖤"));
	if (w4) w4->Show(true);

	{
		SafePointer<Graphics::IDeviceFactory> fact = Graphics::CreateDeviceFactory();
		device = fact->CreateDefaultDevice();
		Console << string(device.Inner()) << L"\n";
		string tech;
		uint32 ver;
		device->GetImplementationInfo(tech, ver);
		Console << tech << L"/" << ver << L"\n";
		Console << device->GetDeviceName() << L" :: " << device->GetDeviceIdentifier() << L"\n";
		auto box = w4->GetStation()->GetBox();
		Graphics::WindowLayerDesc desc;
		desc.Format = Graphics::PixelFormat::B8G8R8A8_unorm;
		desc.Width = box.Right - box.Left;
		desc.Height = box.Bottom - box.Top;
		desc.Usage = Graphics::ResourceUsageRenderTarget | Graphics::ResourceUsageShaderRead;
		layer = device->CreateWindowLayer(w4, desc);
		Console << string(layer.Inner()) << L"\n";
		w4->RequireRedraw();

		Graphics::TextureDesc tdesc;
		tdesc.Type = Graphics::TextureType::Type2D;
		tdesc.Format = Graphics::PixelFormat::B8G8R8A8_unorm;
		tdesc.Width = 256;
		tdesc.Height = 128;
		tdesc.MipmapCount = 1;
		tdesc.MemoryPool = Graphics::ResourceMemoryPool::Default;
		tdesc.Usage = Graphics::ResourceUsageShaderRead | Graphics::ResourceUsageCPUWrite;

		SafePointer<Codec::Frame> frame = new Codec::Frame(256, 128, -1, Codec::PixelFormat::B8G8R8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::TopDown);
		for (int y = 0; y < 128; y++) for (int x = 0; x < 256; x++) {
			Math::Color clr(x / 255.0, 0.5, y / 128.0);
			frame->SetPixel(x, y, UI::Color(clr));
		}
		Graphics::ResourceInitDesc idesc;
		idesc.Data = frame->GetData();
		idesc.DataPitch = frame->GetScanLineLength();
		SafePointer<Graphics::ITexture> tex = device->CreateTexture(tdesc, &idesc);
		Console << string(tex.Inner()) << L"\n";
		Console << string(tex->GetMipmapCount()) << L"\n";
		// TODO: TEST TO RENDER
	}

	SafePointer<UI::Windows::StatusBarIcon> status = UI::Windows::CreateStatusBarIcon();
	status->SetIconColorUsage(UI::Windows::StatusBarIconColorUsage::Monochromic);
	{
		SafePointer<Codec::Image> icon = new Codec::Image;
		Array<Math::Vector2> line1;
		line1 << Math::Vector2(6.0, 6.0);
		line1 << Math::Vector2(34.0, 6.0);
		line1 << Math::Vector2(34.0, 34.0);
		line1 << Math::Vector2(6.0, 34.0);
		Array<Math::Vector2> line2;
		line2 << Math::Vector2(20.0, 6.0);
		line2 << Math::Vector2(34.0, 34.0);
		line2 << Math::Vector2(6.0, 34.0);
		auto rd = UI::Windows::CreateNativeCompatibleTextureRenderingDevice(40, 40, Math::Color(0.0, 0.0, 0.0, 0.0));
		rd->BeginDraw();
		rd->FillPolygon(line1.GetBuffer(), line1.Length(), Math::Color(1.0, 0.0, 0.0, 0.5));
		rd->FillPolygon(line2.GetBuffer(), line2.Length(), Math::Color(0.0, 0.0, 1.0, 1.0));
		rd->EndDraw();
		SafePointer<Codec::Frame> frame = rd->GetRenderTargetAsFrame();
		icon->Frames.Append(frame);
		status->SetIcon(icon);
		SafePointer<Menus::Menu> my_menu = new Menus::Menu(interface.Dialog[L"EditContextMenu"]);
		for (int i = 0; i < interface.Dialog.Length(); i++) {
			Console.WriteLine(interface.Dialog.ElementByIndex(i).key);
		}
		status->SetCallback(Callback2);
		status->SetMenu(my_menu);
		//status->SetEventID(666);
	}
	status->SetTooltip(L"pidor");
	status->PresentIcon(true);

	MacOSXSpecific::TouchBar * bar = new MacOSXSpecific::TouchBar;
	MacOSXSpecific::TouchBarButton * btn = MacOSXSpecific::TouchBar::CreateButtonItem();
	MacOSXSpecific::TouchBarPopover * pover = MacOSXSpecific::TouchBar::CreatePopoverItem();
	pover->AddChild(btn);
	pover->SetID(102);
	pover->SetText(L"pidor");
	pover->SetMainItemID(101);
	btn->SetText(L"PIDOR");
	btn->SetColor(UI::Color(0, 190, 0));
	btn->SetID(101);
	bar->AddChild(pover);
	bar->SetMainItemID(102);
	MacOSXSpecific::TouchBar::SetTouchBarForWindow(w4, bar);
	
	Windows::RunMessageLoop();

	status->PresentIcon(false);

	return 0;
}
