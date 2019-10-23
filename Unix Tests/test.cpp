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

UI::InterfaceTemplate interface;

MacOSXSpecific::TouchBar * tb;

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
        }
    }
    virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
    {
        // SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandardOutput());
        // Streaming::TextWriter Console(ConsoleOutStream);
        // if (event == Windows::FrameEvent::Close) Console << string(L"Close window") + IO::NewLineChar;
        // else if (event == Windows::FrameEvent::Move) Console << string(L"Move window") + IO::NewLineChar;
        if (event == Windows::FrameEvent::Close) {
            window->Destroy();
            Windows::ExitMessageLoop();
        }
    }
};

int Main(void)
{
    //SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandardOutput());
    //Streaming::TextWriter Console(ConsoleOutStream);
    IO::Console Console;
    SafePointer<Streaming::FileStream> ConsoleInStream = new Streaming::FileStream(IO::GetStandardInput());
    Streaming::TextReader Input(ConsoleInStream, Encoding::UTF8);

    Console.ClearScreen();
    Console.MoveCaret(5, 1);
    for (int i = 0; i < 8; i++) {
        Console.SetBackgroundColor(i);
        Console.Write(L' ');
    }
    Console.MoveCaret(5, 2);
    for (int i = 8; i < 16; i++) {
        Console.SetBackgroundColor(i);
        Console.Write(L' ');
    }
    Console.WriteLine(L"");
    Console.SetBackgroundColor(-1);
    Console.Write(L"pidor");
    //Console.ClearLine();
    Console.Write(L"kornevgen");

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

    UI::ITexture * tex;
    {
        Array<Math::Vector2> line;
        line << Math::Vector2(0.0, 0.0);
        line << Math::Vector2(200.0, 100.0);
        line << Math::Vector2(200.0, 200.0);
        line << Math::Vector2(20.0, 200.0);
        auto rd = UI::Windows::CreateNativeCompatibleTextureRenderingDevice(256, 256, Math::Color(1.0, 0.7, 0.7, 1.0));
        rd->BeginDraw();
        rd->FillPolygon(line.GetBuffer(), line.Length(), Math::Color(0.0, 0.0, 1.0, 1.0));
        rd->DrawPolygon(line.GetBuffer(), line.Length(), Math::Color(0.0, 0.0, 0.0, 0.5), 10.0);
        rd->EndDraw();
        tex = rd->GetRenderTargetAsTexture();
        SafePointer<Codec::Frame> frame = rd->GetRenderTargetAsFrame();
        Console << L"Set clipboard data : " << Clipboard::SetData(frame) << IO::NewLineChar;
        rd->Release();
    }
    // SafePointer<Codec::Frame> cframe;
    // if (Clipboard::GetData(cframe.InnerRef())) {
    //     tex = loader->LoadTexture(cframe);
    // }

    Console << string(tex) + IO::NewLineChar;
    Console << L"Text available : " << Clipboard::IsFormatAvailable(Clipboard::Format::Text) << IO::NewLineChar;
    Console << L"Image available: " << Clipboard::IsFormatAvailable(Clipboard::Format::Image) << IO::NewLineChar;

    MacOSXSpecific::SetWindowCreationAttribute(MacOSXSpecific::CreationAttribute::Transparent | MacOSXSpecific::CreationAttribute::TransparentTitle |
        MacOSXSpecific::CreationAttribute::EffectBackground | MacOSXSpecific::CreationAttribute::LightTheme);

    auto Callback2 = new _cb2;
    auto templ = interface.Dialog[L"Test3"];
    templ->Properties->GetProperty(L"Background").Get< SafePointer<UI::Template::Shape> >().SetReference(0);
    templ->Children.ElementAt(1)->Children.ElementAt(0)->Properties->GetProperty(L"Image").Get< SafePointer<UI::ITexture> >().SetRetain(tex);
    templ->Children.ElementAt(1)->Children.ElementAt(0)->Properties->GetProperty(L"ID").Set<int>(789);
    tex->Release();
    auto w4 = Windows::CreateFramedDialog(templ, Callback2, UI::Rectangle::Invalid(), 0);
    w4->FindChild(212121)->SetText(string(L"Heart ❤️ Heart 💙 Heart 🧡💛💚💜🖤"));
    MacOSXSpecific::SetWindowBackgroundColor(w4, UI::Color(255, 0, 255, 128));
    MacOSXSpecific::SetEffectBackgroundMaterial(w4, MacOSXSpecific::EffectBackgroundMaterial::Popover);
    if (w4) w4->Show(true);

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

    return 0;
}
