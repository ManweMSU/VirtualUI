// Tests.cpp: определяет точку входа для приложения.
//

#include <EngineRuntime.h>
#include <Interfaces/NativeStation.h>
#include <PlatformSpecific/WindowsTaskbar.h>
#include <PlatformSpecific/WindowsEffects.h>
#include <Interfaces/SystemColors.h>
#include <PlatformDependent/Console.h>
#include <Graphics/GraphicsHelper.h>
#include <PlatformDependent/ComInterop.h>
#include <Math/Color.h>
#include <Graphics/Drawing.h>

#include "stdafx.h"
#include "Tests.h"

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
using namespace Engine::IO::ConsoleControl;

Engine::UI::Window * main_window = 0;

class main_window_callback_class : public Engine::UI::Windows::IWindowEventCallback
{
	Controls::VirtualStation * station;
	SafePointer<Graphics::IDevice> device;
	SafePointer<Graphics::IWindowLayer> layer;
	SafePointer<Graphics::IBuffer> vertex_buffer;
	SafePointer<Graphics::IBuffer> index_buffer;
	SafePointer<Graphics::ITexture> depth_buffer;
	SafePointer<Graphics::IPipelineState> state;
	uint32 nvert;
public:
	struct vertex_struct { Math::Vector3f pos, norm, clr; };
	struct world_struct { Math::Matrix4x4f proj; Math::Vector3f light; float light_power; float ambient_power; };
	virtual void OnInitialized(Engine::UI::Window * window) override
	{
		auto overlapped = window->As<Controls::OverlappedWindow>();
		overlapped->GetAcceleratorTable() << Accelerators::AcceleratorCommand(668, KeyCodes::Return, false, false, true);
		station = window->FindChild(667)->As<Controls::VirtualStation>();
		station->GetInnerStation()->RequireRefreshRate(Window::RefreshPeriod::Cinematic);
		auto size = station->GetDesktopDimensions();
		SafePointer<Graphics::IDeviceFactory> factory = Graphics::CreateDeviceFactory();
		device = factory->CreateDefaultDevice();
		// Loading shaders
		layer = device->CreateWindowLayer(window, Graphics::CreateWindowLayerDesc(size.x, size.y));
		SafePointer<Streaming::Stream> slrs_stream = Assembly::QueryResource(L"SL");
		SafePointer<Graphics::IShaderLibrary> sl = device->LoadShaderLibrary(slrs_stream);
		SafePointer<Graphics::IShader> vs = sl->CreateShader(L"VertexFunction");
		SafePointer<Graphics::IShader> ps = sl->CreateShader(L"PixelFunction");
		slrs_stream.SetReference(0);
		sl.SetReference(0);
		// Creating buffers
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
		depth_buffer = device->CreateTexture(Graphics::CreateTextureDesc2D(Graphics::PixelFormat::D32_float, size.x, size.y, 1, Graphics::ResourceUsageDepthStencil));
		auto state_desc = Graphics::DefaultPipelineStateDesc(vs, ps, Graphics::PixelFormat::B8G8R8A8_unorm, true, Graphics::PixelFormat::D32_float);
		state_desc.Rasterization.Fill = Graphics::FillMode::Wireframe;
		state = device->CreateRenderingPipelineState(state_desc);
	}
	virtual void OnControlEvent(Engine::UI::Window * window, int ID, Engine::UI::Window::Event event, Engine::UI::Window * sender) override
	{
		if (ID == 668) {
			IO::Console console;
			if (layer->IsFullscreen()) {
				console.WriteLine(L"Going windowed.");
				if (!layer->SwitchToWindow()) console.WriteLine(L"Transition to window failed.");
			} else {
				console.WriteLine(L"Going to fullscreen.");
				if (!layer->SwitchToFullscreen()) console.WriteLine(L"Transition to fullscreen failed.");
			}
		}
	}
	virtual void OnFrameEvent(Engine::UI::Window * window, Engine::UI::Windows::FrameEvent event) override
	{
		if (event == Windows::FrameEvent::Close) {
			layer.SetReference(0);
			device.SetReference(0);
			window->Destroy();
		} else if (event == Windows::FrameEvent::Move) {
			auto box = station->GetPosition();
			auto size = Point(box.Right - box.Left, box.Bottom - box.Left);
			depth_buffer = device->CreateTexture(Graphics::CreateTextureDesc2D(Graphics::PixelFormat::D32_float, size.x, size.y, 1, Graphics::ResourceUsageDepthStencil));
			layer->ResizeSurface(size.x, size.y);
			station->SetDesktopDimensions(size);
		} else if (event == Windows::FrameEvent::Activate) {
			station->SetFocus();
		} else if (event == Windows::FrameEvent::Deactivate) {
			if (layer) layer->SwitchToWindow();
		} else if (event == Windows::FrameEvent::Draw) {
			SafePointer<Graphics::ITexture> rt = layer->QuerySurface();
			auto context = device->GetDeviceContext();
			auto angle = (GetTimerValue() % 20000) * 2.0 * ENGINE_PI / 20000.0;
			auto a2 = (GetTimerValue() % 12000) * 2.0 * ENGINE_PI / 12000.0;
			auto a3 = (GetTimerValue() % 8000) * 2.0 * ENGINE_PI / 8000.0;
			auto cam = (Math::Vector3f(cos(angle), sin(angle), 0.0) + Math::Vector3f(0.0f, 0.0f, sin(a2))) * 2.0f;
			auto view = Graphics::MakeLookAtTransform(cam, -cam, Math::Vector3f(0.0f, 0.0f, 1.0f));
			auto proj = Graphics::MakePerspectiveViewTransformFoV(ENGINE_PI / 2.0f, float(rt->GetWidth()) / float(rt->GetHeight()), 0.01f, 10.0f);
			world_struct world;
			world.proj = proj * view;
			world.light_power = 0.7f;
			world.ambient_power = 0.3f;
			world.light = Math::Vector3f(cos(a3), 0.0f, sin(a3));
			if (device->GetDeviceContext()->BeginRenderingPass(1, &Graphics::CreateRenderTargetView(rt, Math::Vector4f(0.2f, 0.0f, 0.4f, 1.0f)),
				&Graphics::CreateDepthStencilView(depth_buffer, 1.0f))) {
				device->GetDeviceContext()->SetViewport(0.0f, 0.0f, rt->GetWidth(), rt->GetHeight(), 0.0f, 1.0f);
				device->GetDeviceContext()->SetRenderingPipelineState(state);
				device->GetDeviceContext()->SetVertexShaderResource(0, vertex_buffer);
				device->GetDeviceContext()->SetVertexShaderConstant(0, &world, sizeof(world));
				device->GetDeviceContext()->SetPixelShaderConstant(0, &world, sizeof(world));
				device->GetDeviceContext()->SetIndexBuffer(index_buffer, Graphics::IndexBufferFormat::UInt16);
				device->GetDeviceContext()->DrawIndexedPrimitives(nvert, 0, 0);
				device->GetDeviceContext()->EndCurrentPass();
			}
			context->Begin2DRenderingPass(rt);
			auto device_2d = context->Get2DRenderingDevice();
			device_2d->SetTimerValue(GetTimerValue());
			station->GetInnerStation()->SetRenderingDevice(device_2d);
			station->GetInnerStation()->Animate();
			station->GetInnerStation()->Render();
			context->EndCurrentPass();
			layer->Present();
		}
	}
};
main_window_callback_class main_window_callback;

SafePointer<Engine::UI::InterfaceTemplate> Template;
UI::IInversionEffectRenderingInfo * Inversion = 0;
SafePointer<Menus::Menu> menu;

ENGINE_REFLECTED_CLASS(lv_item, Reflection::Reflected)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text1)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text2)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text3)
ENGINE_END_REFLECTED_CLASS

class RenderTargetShape : public UI::Shape
{
public:
	SafePointer<Graphics::ITexture> texture;
	mutable SafePointer<ITextureRenderingInfo> info;
	virtual void Render(IRenderingDevice * Device, const Box & Outer) const noexcept override
	{
		if (!info) info = Device->CreateTextureRenderingInfo(texture);
		Device->RenderTexture(info, Box(Position, Outer));
	}
	virtual void ClearCache(void) noexcept override
	{
		info.SetReference(0);
	}
	virtual Shape * Clone(void) const override
	{
		SafePointer<RenderTargetShape> copy = new RenderTargetShape;
		copy->Position = Position;
		copy->texture = texture;
		copy->Retain();
		return copy;
	}
};
class RenderTargetShapeTemplate : public UI::Template::Shape
{
public:
	SafePointer<Graphics::ITexture> texture;
	RenderTargetShapeTemplate(void) {}
	virtual bool IsDefined(void) const override { return true; }
	virtual Engine::UI::Shape * Initialize(IArgumentProvider * provider) const override
	{
		SafePointer<RenderTargetShape> copy = new RenderTargetShape;
		copy->Position = Position.Initialize(provider);
		copy->texture = texture;
		copy->Retain();
		return copy;
	}
};

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

	Console cns;

	IO::SetCurrentDirectory(IO::Path::GetDirectory(IO::GetExecutablePath()));
	Codec::InitializeDefaultCodecs();

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

	SafePointer<IResourceLoader> resource_loader = Engine::UI::CreateObjectFactory();

	if (!main_window) {
		UI::Zoom = UI::Windows::GetScreenScale();
		auto desktop = Engine::UI::Windows::GetScreenDimensions();
		auto station_desc = new UI::Template::Controls::VirtualStation;
		station_desc->ControlPosition = UI::Rectangle::Entire();
		station_desc->Disabled = false;
		station_desc->Invisible = false;
		station_desc->Autosize = false;
		station_desc->Render = false;
		station_desc->Width = 2600;
		station_desc->Height = 1300;
		station_desc->ID = 667;
		SafePointer<Engine::UI::Template::ControlTemplate> mwt =
			Engine::UI::Controls::CreateOverlappedWindowTemplate(L"Test Graphics Window", Engine::UI::Rectangle(0, 0, 0.7 * desktop.Right, 0.7 * desktop.Bottom),
				Engine::UI::Controls::OverlappedWindowCaptioned | Engine::UI::Controls::OverlappedWindowCloseButton |
				Engine::UI::Controls::OverlappedWindowSizeble | Engine::UI::Controls::OverlappedWindowMinimizeButton, 400, 200);
		SafePointer<Engine::UI::Template::ControlTemplate> vst = new Engine::UI::Template::ControlTemplate(station_desc);
		mwt->Children.Append(vst);
		main_window = Engine::UI::Windows::CreateFramedDialog(mwt, &main_window_callback, Engine::UI::Rectangle::Entire());// , 0, true);
		main_window->Show(true);
	}

	auto fact = Graphics::CreateDeviceFactory();
	auto dl = fact->GetAvailableDevices();
	for (auto & d : dl->Elements()) cns.WriteLine(FormatString(L"%0: %1", d.value, d.key));
	fact->Release();
	dl->Release();

	{
		{
			::Template.SetReference(new Engine::UI::InterfaceTemplate());
			{
				SafePointer<Streaming::Stream> Source = Assembly::QueryResource(L"GUI");
				Engine::UI::Loader::LoadUserInterfaceFromBinary(*::Template, Source, resource_loader, 0);
			}
			auto station = main_window->FindChild(667)->As<Controls::VirtualStation>()->GetInnerStation();

			station->GetVisualStyles().MenuArrow.SetReference(::Template->Application[L"a"]);
			SafePointer<Template::FrameShape> MenuBack = new Template::FrameShape;
			SafePointer<Template::BarShape> MenuShadow = new Template::BarShape;
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
				MenuShadow->Position.Left = UI::Template::Coordinate(0, -5, 0);
				MenuShadow->Position.Top = UI::Template::Coordinate(0, -5, 0);
				MenuShadow->Position.Right = UI::Template::Coordinate(0, 5, 1);
				MenuShadow->Position.Bottom = UI::Template::Coordinate(0, 5, 1);
				MenuShadow->Gradient << UI::Template::GradientPoint(UI::Color(0, 0, 0, 100), 0.0);
			}
			station->GetVisualStyles().MenuBackground.SetRetain(MenuBack);
			station->GetVisualStyles().MenuShadow.SetRetain(MenuShadow);
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
				Active->Children.Append(MenuShadow);
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

			menu = new Menus::Menu(::Template->Dialog[L"Menu"]);

			class _cb : public Windows::IWindowEventCallback
			{
			public:
				virtual void OnInitialized(UI::Window * window) override
				{
					IO::Console cns;
					cns << L"Callback: Initialized, window = " << string(static_cast<handle>(window)) << IO::LineFeedSequence;
				}
				virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
				{
					IO::Console cns;
					cns << L"Callback: Event with ID = " << ID << L", window = " << string(static_cast<handle>(window)) << L", sender = " << string(static_cast<handle>(sender)) << IO::LineFeedSequence;
					if (!window) return;
					if (event == Window::Event::Command || event == Window::Event::MenuCommand) {
						if (ID == 876) {
							menu->RunPopup(sender, window->GetStation()->GetCursorPos());
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
					IO::Console cns;
					cns << L"Callback: ";
					if (event == Windows::FrameEvent::Move) cns << L"Move";
					else if (event == Windows::FrameEvent::Close) cns << L"Close";
					else if (event == Windows::FrameEvent::Minimize) cns << L"Minimize";
					else if (event == Windows::FrameEvent::Maximize) cns << L"Maximize";
					else if (event == Windows::FrameEvent::Help) cns << L"Help";
					else if (event == Windows::FrameEvent::PopupMenuCancelled) cns << L"Popup menu cancelled";
					cns << L", window = " << string(static_cast<handle>(window)) << IO::LineFeedSequence;
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
					} else if (ID == 201) {
						window->FindChild(6602)->Show(true);
					} else if (ID == 202) {
						window->FindChild(6602)->Show(false);
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
					IO::Console console;
					if (event == Windows::FrameEvent::Close) {
						window->Destroy();
						return;
					} else if (event == Windows::FrameEvent::Move) {
						console.SetTextColor(14);
						console.WriteLine(L"Move Window");
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
					}
					if (NativeWindows::IsWindowActive(window->GetStation())) {
						console.SetTextColor(10);
					} else {
						console.SetTextColor(12);
					}
					console.Write(L"A");
					if (NativeWindows::IsWindowMaximized(window->GetStation())) {
						console.SetTextColor(10);
					} else {
						console.SetTextColor(12);
					}
					console.Write(L"Z");
					if (NativeWindows::IsWindowMinimized(window->GetStation())) {
						console.SetTextColor(10);
					} else {
						console.SetTextColor(12);
					}
					console.Write(L"I");
					console.SetTextColor(-1);
					console.WriteLine(L"");
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
			class _cb3 : public Windows::IWindowEventCallback
			{
			public:
				virtual void OnInitialized(UI::Window * window) override
				{
					auto wnd = window->As<Controls::OverlappedWindow>();
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10001, KeyCodes::B);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10002, KeyCodes::I);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10003, KeyCodes::U);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10004, KeyCodes::S);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10005, KeyCodes::D1);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10006, KeyCodes::D2);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10007, KeyCodes::D3);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10008, KeyCodes::D4);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10009, KeyCodes::D5);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10010, KeyCodes::L);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10011, KeyCodes::M);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10012, KeyCodes::D6);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10013, KeyCodes::D7);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10014, KeyCodes::R);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10015, KeyCodes::E);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10016, KeyCodes::T);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10017, KeyCodes::Left, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10018, KeyCodes::Right, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10019, KeyCodes::Up, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10020, KeyCodes::Down, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10021, KeyCodes::Left, true, false, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10022, KeyCodes::Right, true, false, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10023, KeyCodes::Up, true, false, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10024, KeyCodes::Down, true, false, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10025, KeyCodes::W);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10026, KeyCodes::W, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10027, KeyCodes::W, true, false, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10028, KeyCodes::O);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10029, KeyCodes::D8);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10030, KeyCodes::D9);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10031, KeyCodes::D0);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10032, KeyCodes::O, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10033, KeyCodes::H);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10034, KeyCodes::H, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10035, KeyCodes::G);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10036, KeyCodes::G, true, true);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10037, KeyCodes::J);
					wnd->GetAcceleratorTable() << Accelerators::AcceleratorCommand(10038, KeyCodes::J, true, true);
				}
				virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
				{
					if (ID == 898911 && event == UI::Window::Event::ValueChange) {
						auto re = sender->As<Controls::RichEdit>();
						if (re) {
							//MessageBoxW(0, re->GetAttributedText(), L"", 0);
						}
					} else if (ID == 10001) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto stl = re->GetSelectedTextStyle();
						if (stl & Controls::RichEdit::StyleBold) stl = Controls::RichEdit::StyleNone; else stl = Controls::RichEdit::StyleBold;
						re->SetSelectedTextStyle(stl, Controls::RichEdit::StyleBold);
					} else if (ID == 10002) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto stl = re->GetSelectedTextStyle();
						if (stl & Controls::RichEdit::StyleItalic) stl = Controls::RichEdit::StyleNone; else stl = Controls::RichEdit::StyleItalic;
						re->SetSelectedTextStyle(stl, Controls::RichEdit::StyleItalic);
					} else if (ID == 10003) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto stl = re->GetSelectedTextStyle();
						if (stl & Controls::RichEdit::StyleUnderline) stl = Controls::RichEdit::StyleNone; else stl = Controls::RichEdit::StyleUnderline;
						re->SetSelectedTextStyle(stl, Controls::RichEdit::StyleUnderline);
					} else if (ID == 10004) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto stl = re->GetSelectedTextStyle();
						if (stl & Controls::RichEdit::StyleStrikeout) stl = Controls::RichEdit::StyleNone; else stl = Controls::RichEdit::StyleStrikeout;
						re->SetSelectedTextStyle(stl, Controls::RichEdit::StyleStrikeout);
					} else if (ID == 10005) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTextAlignment(Controls::RichEdit::TextAlignment::Left);
					} else if (ID == 10006) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTextAlignment(Controls::RichEdit::TextAlignment::Center);
					} else if (ID == 10007) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTextAlignment(Controls::RichEdit::TextAlignment::Right);
					} else if (ID == 10008) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTextAlignment(Controls::RichEdit::TextAlignment::Stretch);
					} else if (ID == 10009) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						if (re->GetSelectedTextFontFamily() == L"Segoe UI") re->SetSelectedTextFontFamily(L"Times New Roman");
						else re->SetSelectedTextFontFamily(L"Segoe UI");
					} else if (ID == 10010) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int h = re->GetSelectedTextHeight();
						if (h < 100) h += 10;
						re->SetSelectedTextHeight(h);
					} else if (ID == 10011) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int h = re->GetSelectedTextHeight();
						if (h > 19) h -= 10;
						re->SetSelectedTextHeight(h);
					} else if (ID == 10012) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTextColor(0xFF444444);
					} else if (ID == 10013) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTextColor(0xFFFF0044);
					} else if (ID == 10014) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->TransformSelectionIntoLink(re->SerializeSelection());
					} else if (ID == 10015) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->DetransformSelectedLink();
					} else if (ID == 10016) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->InsertTable(Point(1, 1));
					} else if (ID == 10017) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->InsertSelectedTableColumn(cell.x);
					} else if (ID == 10018) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->InsertSelectedTableColumn(cell.x + 1);
					} else if (ID == 10019) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->InsertSelectedTableRow(cell.y);
					} else if (ID == 10020) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->InsertSelectedTableRow(cell.y + 1);
					} else if (ID == 10021) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->RemoveSelectedTableColumn(cell.x);
					} else if (ID == 10022) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->RemoveSelectedTableColumn(cell.x);
					} else if (ID == 10023) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->RemoveSelectedTableRow(cell.y);
					} else if (ID == 10024) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto cell = re->GetSelectedTableCell();
						if (cell.x >= 0) re->RemoveSelectedTableRow(cell.y);
					} else if (ID == 10025) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableColumnWidth(re->GetSelectedTableCell().x);
						re->SetSelectedTableColumnWidth(w + 50);
					} else if (ID == 10026) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableColumnWidth(re->GetSelectedTableCell().x);
						re->SetSelectedTableColumnWidth(max(w - 50, 50));
					} else if (ID == 10027) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->AdjustSelectedTableColumnWidth();
					} else if (ID == 10028) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto c = re->GetSelectedTableCellBackground(re->GetSelectedTableCell());
						if (c.a) re->SetSelectedTableCellBackground(0);
						else re->SetSelectedTableCellBackground(Color(100, 200, 255, 128));
					} else if (ID == 10029) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTableCellVerticalAlignment(Controls::RichEdit::TextVerticalAlignment::Top);
					} else if (ID == 10030) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTableCellVerticalAlignment(Controls::RichEdit::TextVerticalAlignment::Center);
					} else if (ID == 10031) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						re->SetSelectedTableCellVerticalAlignment(Controls::RichEdit::TextVerticalAlignment::Bottom);
					} else if (ID == 10032) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						auto c = re->GetSelectedTableBorderColor();
						if (c == re->DefaultTextColor) re->SetSelectedTableBorderColor(Color(255, 128, 0, 255));
						else re->SetSelectedTableBorderColor(re->DefaultTextColor);
					} else if (ID == 10033) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableBorderWidth();
						re->SetSelectedTableBorderWidth(w + 1);
					} else if (ID == 10034) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableBorderWidth();
						re->SetSelectedTableBorderWidth(max(w - 1, 0));
					} else if (ID == 10035) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableVerticalBorderWidth();
						re->SetSelectedTableVerticalBorderWidth(w + 1);
					} else if (ID == 10036) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableVerticalBorderWidth();
						re->SetSelectedTableVerticalBorderWidth(max(w - 1, 0));
					} else if (ID == 10037) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableHorizontalBorderWidth();
						re->SetSelectedTableHorizontalBorderWidth(w + 1);
					} else if (ID == 10038) {
						auto re = window->FindChild(898911)->As<Controls::RichEdit>();
						int w = re->GetSelectedTableHorizontalBorderWidth();
						re->SetSelectedTableHorizontalBorderWidth(max(w - 1, 0));
					}
				}
				virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
				{
					if (event == Windows::FrameEvent::Close) window->Destroy();
				}
			};
			auto Callback3 = new _cb3;

			auto & pp = *static_cast<UI::Template::Controls::DialogFrame *>(::Template->Dialog[L"Test3"]->Properties);
			pp.Background.SetReference(0);
			pp.BackgroundColor = 0;
			pp.DefaultBackground = true;

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
				auto bookmark4 = new Template::Controls::Bookmark;
				bookmark1->ID = 1; bookmark2->ID = 2; bookmark3->ID = 3; bookmark4->ID = 4;
				bookmark1->Text = L"Rich Edit";  bookmark2->Text = L"kornevgen"; bookmark3->Text = L"pidor"; bookmark4->Text = L"Tab #3";
				bookmark1->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				bookmark2->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				bookmark3->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				bookmark4->ControlPosition.Right = Coordinate(0, 100.0, 0.0);
				SafePointer<Template::ControlTemplate> bookmark_o1 = new Template::ControlTemplate(bookmark1);
				SafePointer<Template::ControlTemplate> bookmark_o2 = new Template::ControlTemplate(bookmark2);
				SafePointer<Template::ControlTemplate> bookmark_o3 = new Template::ControlTemplate(bookmark3);
				SafePointer<Template::ControlTemplate> bookmark_o4 = new Template::ControlTemplate(bookmark4);
				bookmark_ctl->Children.Append(bookmark_o1); bookmark_ctl->Children.Append(bookmark_o2);
				bookmark_ctl->Children.Append(bookmark_o3); bookmark_ctl->Children.Append(bookmark_o4);
				auto richedit_props = new Template::Controls::RichEdit;
				richedit_props->ControlPosition = UI::Rectangle(UI::Coordinate(0, 5, 0), UI::Coordinate(0, 5, 0),
					UI::Coordinate(0, -5, 1), UI::Coordinate(0, -5, 1));
				Reflection::PropertyZeroInitializer initializer;
				richedit_props->EnumerateProperties(initializer);
				richedit_props->ViewScrollBarDownButtonNormal.SetRetain(::Template->Application[L"VerticalScrollDownNormal"]);
				richedit_props->ViewScrollBarDownButtonHot.SetRetain(::Template->Application[L"VerticalScrollDownHot"]);
				richedit_props->ViewScrollBarDownButtonPressed.SetRetain(::Template->Application[L"VerticalScrollDownPressed"]);
				richedit_props->ViewScrollBarScrollerNormal.SetRetain(::Template->Application[L"VerticalScrollScrollerNormal"]);
				richedit_props->ViewScrollBarScrollerHot.SetRetain(::Template->Application[L"VerticalScrollScrollerHot"]);
				richedit_props->ViewScrollBarScrollerPressed.SetRetain(::Template->Application[L"VerticalScrollScrollerPressed"]);
				richedit_props->ViewScrollBarUpButtonNormal.SetRetain(::Template->Application[L"VerticalScrollUpNormal"]);
				richedit_props->ViewScrollBarUpButtonHot.SetRetain(::Template->Application[L"VerticalScrollUpHot"]);
				richedit_props->ViewScrollBarUpButtonPressed.SetRetain(::Template->Application[L"VerticalScrollUpPressed"]);
				richedit_props->BackgroundColor = UI::Color(255, 255, 230);
				richedit_props->DefaultFontFace = L"Times New Roman";
				richedit_props->DefaultFontHeight = 20;
				richedit_props->DefaultTextColor = UI::Color(0, 0, 0);
				richedit_props->HyperlinkColor = UI::Color(0, 0, 255);
				richedit_props->HyperlinkHotColor = UI::Color(128, 128, 255);
				richedit_props->ID = 898911;
				richedit_props->ScrollSize = 32;
				richedit_props->SelectionColor = UI::Color(255, 0, 0, 128);
				richedit_props->Border = 10;
				richedit_props->ContextMenu.SetRetain(::Template->Dialog[L"EditContextMenu"]);
				richedit_props->Text = L"\33X\33nSegoe UI\33e\33n*\33e\33f00\33cFF444444\33h001E\33a4"
					L"\tkornevgen \33bpi\33idor\33e\33e \33cFFFF0000kornev\33cFFFF0088gen\33e \33ipidor\33e\33e kornevgen \33upi\33edor kor\33snev\33egen pidor\nkornevgen \33h0030pidor\33e \33f01kornevgen\33e pidor kornevgen pidor \33lRESOURCE\33kornevgen pidor\33e kornevgen pidor kornevgen pidor"
					L"\33e" L"\n\33a2TITLE\n\33e\33a3Right Text\n\33e"
					L"\33t00020002040201FF88440000800100"
					L"000000001pidor\n123\33e"
					L"440000FF2kor\33lPIDOR\33nevgen\33e\33e"
					L"4400FF003kornevgen\33e"
					L"888888881pidor\n456\33e"
					L"\33e"
					L"\33e\33e\33e";
				SafePointer<Template::ControlTemplate> richedit = new Template::ControlTemplate(richedit_props);
				bookmark_o1->Children.Append(richedit);
				bookmark_o2->Children.Append(&::Template->Dialog[L"Test2"]->Children[0].Children[2].Children[0]);
				bookmark_o3->Children.Append(&::Template->Dialog[L"Test2"]->Children[0].Children[1].Children[0].Children[1].Children[0]);
				bookmark_o4->Children.Append(&::Template->Dialog[L"Test2"]->Children[0].Children[1].Children[0].Children[0].Children[0]);
			}
			auto w = Windows::CreateFramedDialog(::Template->Dialog[L"Test2"], 0, UI::Rectangle::Invalid(), station);
			auto w2 = Windows::CreateFramedDialog(::Template->Dialog[L"Test"], Callback, UI::Rectangle(0, 0, Coordinate(0, 0.0, 0.7), Coordinate(0, 0.0, 0.55)), station);
			auto w3 = Windows::CreateFramedDialog(::Template->Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid(), station);
			auto w4 = Windows::CreateFramedDialog(::Template->Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid());
			auto w6 = Windows::CreateFramedDialog(::Template->Dialog[L"Test4"], Callback3, UI::Rectangle::Invalid());

			w2->FindChild(7777)->As<Controls::ColorView>()->SetColor(UI::GetSystemColor(UI::SystemColor::Theme));

			w2->SetText(L"window");

			w->AddDialogStandardAccelerators();
			w2->AddDialogStandardAccelerators();
			w3->AddDialogStandardAccelerators();
			w6->AddDialogStandardAccelerators();
			w4->GetStation()->GetVisualStyles() = station->GetVisualStyles();
			w4->GetStation()->GetVisualStyles().ForcedVirtualMenu = true;

			w->FindChild(101010)->As<Controls::Edit>()->CaretWidth = 4;
			w->FindChild(101010)->As<Controls::Edit>()->CaretColor = UI::Color(64, 64, 128);
			w->FindChild(101010)->As<Controls::Edit>()->SetHook(Hook);
			w3->FindChild(212121)->As<Controls::MultiLineEdit>()->SetHook(Hook2);
			w4->FindChild(212121)->As<Controls::MultiLineEdit>()->SetHook(Hook2);

			w->Show(true);
			w2->Show(true);
			w3->Show(true);
			w6->Show(true);
			w4->Show(true);

			cns << L"Done!" << IO::LineFeedSequence;
		}
	}

	NativeWindows::RunMainMessageLoop();
    
    return 0;
}