#include <EngineRuntime.h>

#include <Interfaces/SystemColors.h>
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
	uint nvert;
	SafePointer<Graphics::IBuffer> vertex_buffer;
	SafePointer<Graphics::IBuffer> index_buffer;
	SafePointer<Graphics::ITexture> depth_buffer;
	SafePointer<Graphics::IPipelineState> state;
public:
	struct vertex_struct { Math::Vector3f pos, norm, clr; };
	struct world_struct { Math::Matrix4x4f proj; Math::Vector3f light; float light_power; float ambient_power; };
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

		// Creating the device and layer
		IO::Console console;
		window->GetStation()->RequireRefreshRate(Window::RefreshPeriod::Cinematic);
		SafePointer<Graphics::IDeviceFactory> fact = Graphics::CreateDeviceFactory();
		device = fact->CreateDefaultDevice();
		console << string(device.Inner()) << L"\n";
		string tech;
		uint32 ver;
		device->GetImplementationInfo(tech, ver);
		console << tech << L"/" << ver << L"\n";
		console << device->GetDeviceName() << L" :: " << device->GetDeviceIdentifier() << L"\n";
		auto dbox = window->GetStation()->GetBox();
		Graphics::WindowLayerDesc desc;
		desc.Format = Graphics::PixelFormat::B8G8R8A8_unorm;
		desc.Width = dbox.Right - dbox.Left;
		desc.Height = dbox.Bottom - dbox.Top;
		desc.Usage = Graphics::ResourceUsageRenderTarget | Graphics::ResourceUsageShaderRead;
		layer = device->CreateWindowLayer(window, desc);
		console << string(layer.Inner()) << L"\n";
		// Testing texture wrong creation
		Graphics::TextureDesc tdesc;
		tdesc.Format = Graphics::PixelFormat::B8G8R8A8_unorm;
		tdesc.Type = Graphics::TextureType::Type2D;
		tdesc.Width = 0x8000;
		tdesc.Height = 0x8000;
		tdesc.MemoryPool = Graphics::ResourceMemoryPool::Default;
		tdesc.MipmapCount = 1;
		tdesc.Usage = Graphics::ResourceUsageShaderRead | Graphics::ResourceUsageRenderTarget;
		SafePointer<Graphics::ITexture> test_tex = device->CreateTexture(tdesc);
		console << string(test_tex.Inner()) << L"\n";
		// Loading shaders
		SafePointer<Streaming::Stream> slrs_stream = Assembly::QueryResource(L"SL");
		SafePointer<Graphics::IShaderLibrary> sl = device->LoadShaderLibrary(slrs_stream);
		console << string(sl.Inner()) << L"\n";
		SafePointer< Array<string> > snames = sl->GetShaderNames();
		for (auto & name : *snames) console << name << L"\n";
		SafePointer<Graphics::IShader> vs = sl->CreateShader(L"VertexFunction");
		console << string(vs.Inner()) << L"\n";
		SafePointer<Graphics::IShader> ps = sl->CreateShader(L"PixelFunction");
		console << string(ps.Inner()) << L"\n";
		slrs_stream.SetReference(0);
		sl.SetReference(0);
		// Creating resources
		Array<vertex_struct> vertex(0x100);
		Array<uint16> index(0x100);
		vertex << vertex_struct{ Math::Vector3f(0.0f, 0.0f, -1.0f), Math::Vector3f(0.0f, 0.0f, -1.0f), Math::Vector3f(1.0f, 1.0f, 1.0f) };
		vertex << vertex_struct{ Math::Vector3f(0.0f, 0.0f, +1.0f), Math::Vector3f(0.0f, 0.0f, +1.0f), Math::Vector3f(1.0f, 1.0f, 1.0f) };
		for (int i = 0; i < 100; i++) {
			float rad = float(i) * (2.0f * ENGINE_PI) / 100.0f;
			float c = cos(rad);
			float s = sin(rad);
			Math::Color clr = Math::ColorHSV(rad, 1.0f, 1.0f);
			vertex << vertex_struct{ Math::Vector3f(c, s, -1.0f), Math::Vector3f(0.0f, 0.0f, -1.0f), Math::Vector3f(clr.x, clr.y, clr.z) };
			vertex << vertex_struct{ Math::Vector3f(c, s, +1.0f), Math::Vector3f(0.0f, 0.0f, +1.0f), Math::Vector3f(clr.x, clr.y, clr.z) };
			if (i) {
				index << 0;
				index << vertex.Length() - 4;
				index << vertex.Length() - 2;
				index << 1;
				index << vertex.Length() - 3;
				index << vertex.Length() - 1;
			}
		}
		index << 0;
		index << 2;
		index << vertex.Length() - 2;
		index << 1;
		index << 3;
		index << vertex.Length() - 1;
		auto vstart = vertex.Length();
		for (int i = 0; i < 100; i++) {
			float rad = float(i) * (2.0f * ENGINE_PI) / 100.0f;
			float c = cos(rad);
			float s = sin(rad);
			Math::Color clr = Math::ColorHSV(rad, 1.0f, 1.0f);
			vertex << vertex_struct{ Math::Vector3f(c, s, -1.0f), Math::Vector3f(c, s, 0.0f), Math::Vector3f(clr.x, clr.y, clr.z) };
			vertex << vertex_struct{ Math::Vector3f(c, s, +1.0f), Math::Vector3f(c, s, 0.0f), Math::Vector3f(clr.x, clr.y, clr.z) };
			if (i) {
				index << vertex.Length() - 4;
				index << vertex.Length() - 3;
				index << vertex.Length() - 2;
				index << vertex.Length() - 3;
				index << vertex.Length() - 2;
				index << vertex.Length() - 1;
			}
		}
		index << vertex.Length() - 2;
		index << vertex.Length() - 1;
		index << vstart;
		index << vertex.Length() - 1;
		index << vstart;
		index << vstart + 1;
		nvert = index.Length();
	
		vertex_buffer = device->CreateBuffer(
			Graphics::CreateBufferDesc(vertex.Length() * sizeof(vertex_struct), Graphics::ResourceUsageShaderRead, sizeof(vertex_struct), Graphics::ResourceMemoryPool::Immutable),
			Graphics::CreateInitDesc(vertex.GetBuffer()));
		index_buffer = device->CreateBuffer(
			Graphics::CreateBufferDesc(index.Length() * sizeof(uint16), Graphics::ResourceUsageIndexBuffer, 0, Graphics::ResourceMemoryPool::Immutable),
			Graphics::CreateInitDesc(index.GetBuffer()));
		depth_buffer = device->CreateTexture(Graphics::CreateTextureDesc2D(Graphics::PixelFormat::D32_float, desc.Width, desc.Height, 1, Graphics::ResourceUsageDepthStencil));
		auto state_desc = Graphics::DefaultPipelineStateDesc(vs, ps, Graphics::PixelFormat::B8G8R8A8_unorm, true, Graphics::PixelFormat::D32_float);
		//state_desc.Rasterization.Fill = Graphics::FillMode::Wireframe;
		state = device->CreateRenderingPipelineState(state_desc);
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
				SafePointer<IObjectFactory> objfact = UI::CreateObjectFactory();
				Array<Math::Vector2> line;
				line << Math::Vector2(0.0, 0.0);
				line << Math::Vector2(200.0, 100.0);
				line << Math::Vector2(200.0, 200.0);
				line << Math::Vector2(20.0, 200.0);
				auto rd = objfact->CreateTextureRenderingDevice(256, 256, 0xFFC0C0FF);
				rd->BeginDraw();
				rd->FillPolygon(line.GetBuffer(), line.Length(), clr);
				rd->DrawPolygon(line.GetBuffer(), line.Length(), 0x80000000, 10.0);
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
			depth_buffer = device->CreateTexture(Graphics::CreateTextureDesc2D(Graphics::PixelFormat::D32_float, box.Right - box.Left, box.Bottom - box.Top, 1, Graphics::ResourceUsageDepthStencil));
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
			if (Keyboard::IsKeyPressed(KeyCodes::A)) console.WriteLine(L"'A' IS PRESSED");
			SafePointer<Graphics::ITexture> rt = layer->QuerySurface();
			auto context = device->GetDeviceContext();
			auto angle = (GetTimerValue() % 20000) * 2.0 * ENGINE_PI / 20000.0;
			auto a2 = (GetTimerValue() % 12000) * 2.0 * ENGINE_PI / 12000.0;
			auto a3 = (GetTimerValue() % 8000) * 2.0 * ENGINE_PI / 8000.0;
			auto cam = (Math::Vector3f(Math::cos(angle), Math::sin(angle), 0.0) + Math::Vector3f(0.0f, 0.0f, Math::sin(a2))) * 2.0f;
			auto view = Graphics::MakeLookAtTransform(cam, -cam, Math::Vector3f(0.0f, 0.0f, 1.0f));
			auto proj = Graphics::MakePerspectiveViewTransformFoV(ENGINE_PI / 2.0f, float(rt->GetWidth()) / float(rt->GetHeight()), 0.01f, 10.0f);
			world_struct world;
			world.proj = proj * view;
			world.light_power = 0.7f;
			world.ambient_power = 0.3f;
			world.light = Math::Vector3f(Math::cos(a3), 0.0f, Math::sin(a3));
			auto rtdesc = Graphics::CreateRenderTargetView(rt, Math::Vector4f(0.2f, 0.0f, 0.4f, 1.0f));
			auto dsdesc = Graphics::CreateDepthStencilView(depth_buffer, 1.0f);
			if (context->BeginRenderingPass(1, &rtdesc, &dsdesc)) {
				context->SetViewport(0.0f, 0.0f, rt->GetWidth(), rt->GetHeight(), 0.0f, 1.0f);
				context->SetRenderingPipelineState(state);
				context->SetVertexShaderResource(1, vertex_buffer);
				context->SetVertexShaderConstant(0, &world, sizeof(world));
				context->SetPixelShaderConstant(0, &world, sizeof(world));
				context->SetIndexBuffer(index_buffer, Graphics::IndexBufferFormat::UInt16);
				context->DrawIndexedPrimitives(nvert, 0, 0);
				context->EndCurrentPass();
			}
			if (context->Begin2DRenderingPass(rt)) {
				auto device_2d = context->Get2DRenderingDevice();
				device_2d->SetTimerValue(GetTimerValue());
				window->GetStation()->SetRenderingDevice(device_2d);
				window->GetStation()->Animate();
				window->GetStation()->Render();
				context->EndCurrentPass();
			}
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
	{
		SafePointer<Drawing::CanvasWindow> canvas = new Drawing::CanvasWindow(Drawing::Color(0.3, 0.3, 0.3), 256, 192, 1024, 768);
		Math::ColorHSV clr(0.0, 1.0, 1.0);
		while (true) {
			canvas->BeginDraw();
			canvas->DrawRectangleOutline(Drawing::Point(5.0, 5.0), Drawing::Point(128.0, 180.0), clr, 5.0);
			canvas->EndDraw();
			auto key = canvas->ReadKey();
			if (key == KeyCodes::Escape) break;
			else if (key == KeyCodes::Right) {
				clr.h += 1.0;
				clr.ClampChannels();
			}
		}
	}
	
	IO::Console Console;

	SafePointer< Array<IO::Search::Volume> > vols = IO::Search::GetVolumes();
	for (int i = 0; i < vols->Length(); i++) {
		Console << L"Volume \"" + vols->ElementAt(i).Label + L"\" at path " + vols->ElementAt(i).Path + IO::LineFeedSequence;
	}
	{
		SafePointer<Streaming::FileStream> self = new Streaming::FileStream(IO::GetExecutablePath(), Streaming::AccessRead, Streaming::OpenExisting);
		Console << IO::Unix::GetFileUserAccessRights(self->Handle()) << IO::ConsoleControl::LineFeed();
		Console << IO::Unix::GetFileGroupAccessRights(self->Handle()) << IO::ConsoleControl::LineFeed();
		Console << IO::Unix::GetFileOtherAccessRights(self->Handle()) << IO::ConsoleControl::LineFeed();
		IO::Unix::SetFileAccessRights(self->Handle(), IO::Unix::AccessRightReadOnly, IO::Unix::AccessRightReadOnly, IO::Unix::AccessRightReadOnly);
	}

	UI::Zoom = Windows::GetScreenScale();

	DynamicString req;
	req += Assembly::GetCurrentUserLocale() + IO::LineFeedSequence;
	// try {
	// 	SafePointer<Network::HttpSession> session = Network::OpenHttpSession(L"pidor");
	// 	if (!session) throw Exception();
	// 	SafePointer<Network::HttpConnection> connection = session->Connect(L"yandex.ru");
	// 	if (!connection) throw Exception();
	// 	SafePointer<Network::HttpRequest> request = connection->CreateRequest(L"/favicon.ico");
	// 	if (!request) throw Exception();
	// 	request->Send();
	// 	req += string(request->GetStatus()) + string(IO::NewLineChar);
	// 	SafePointer< Array<string> > hdrs = request->GetHeaders();
	// 	for (int i = 0; i < hdrs->Length(); i++) {
	// 		req += hdrs->ElementAt(i) + L": " + request->GetHeader(hdrs->ElementAt(i)) + IO::NewLineChar;
	// 	}
	// } catch (...) { req += L"kornevgen pidor"; }

	Streaming::Stream * source = Assembly::QueryResource(L"GUI");
	SafePointer<UI::IResourceLoader> loader = UI::CreateObjectFactory();
	UI::Loader::LoadUserInterfaceFromBinary(interface, source, loader, 0);
	source->Release();

	UI::ITexture * tex = 0;
	// try {
	// 	string host = L"i.pinimg.com";
	// 	string url = L"/originals/80/7a/a2/807aa2cb5485f9e3bbb353b124428d93.jpg";
	// 	SafePointer<HttpSession> session = OpenHttpSession();
	// 	if (!session) return 1;
	// 	SafePointer<HttpConnection> connection = session->SecureConnect(host);
	// 	if (!connection) return 1;
	// 	SafePointer<HttpRequest> request = connection->CreateRequest(url);
	// 	if (!connection) return 1;
	// 	request->Send();
	// 	Streaming::MemoryStream stream(0x10000);
	// 	request->GetResponceStream()->CopyToUntilEof(&stream);
	// 	stream.Seek(0, Streaming::Begin);
	// 	SafePointer<Codec::Image> image = Codec::DecodeImage(&stream);
	// 	UI::Windows::SetApplicationIcon(image);
	// 	SafePointer<UI::IResourceLoader> loader = UI::CreateObjectFactory();
	// 	tex = loader->LoadTexture(image->Frames.FirstElement());
	// } catch (Exception & e) { Console << e.ToString() << IO::NewLineChar; return 1; }

	Console << L"Screen scale: " << Windows::GetScreenScale() << IO::LineFeedSequence;

	auto Callback2 = new _cb2;
	auto templ = interface.Dialog[L"Test3"];
	templ->Properties->GetProperty(L"Background").Get< SafePointer<UI::Template::Shape> >().SetReference(0);
	templ->Children.ElementAt(1)->Children.ElementAt(0)->Properties->GetProperty(L"ID").Set<int>(789);
	if (tex) {
		templ->Children.ElementAt(1)->Children.ElementAt(0)->Properties->GetProperty(L"Image").Get< SafePointer<UI::ITexture> >().SetRetain(tex);
		tex->Release();
	}
	auto w4 = Windows::CreateFramedDialog(templ, Callback2, UI::Rectangle::Invalid(), 0, true);
	auto fnt = loader->LoadFont(L"Menlo", 60, 400, false, true, false);

	Console.WriteLine(FormatString(L"%0, %1, %2", fnt->GetHeight(), fnt->GetLineSpacing(), fnt->GetBaselineOffset()));
	DynamicString long_str;
	for (int i = 0; i <= 500; i++) long_str << L"Ыы";
	w4->FindChild(212121)->As<Controls::MultiLineEdit>()->Font = fnt;
	w4->FindChild(212121)->ResetCache();
	w4->FindChild(212121)->SetText(string(L"Heart ❤️ Heart 💙 Heart 🧡💛💚💜🖤\n║\n║\n \n║") + long_str.ToString());
	if (w4) w4->Show(true);

	SafePointer<UI::Windows::StatusBarIcon> status = UI::Windows::CreateStatusBarIcon();
	status->SetIconColorUsage(UI::Windows::StatusBarIconColorUsage::Monochromic);
	{
		SafePointer<IObjectFactory> objfact = UI::CreateObjectFactory();
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
		auto rd = objfact->CreateTextureRenderingDevice(40, 40, 0x00000000);
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
