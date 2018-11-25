#include <EngineRuntime.h>

#include <PlatformDependent/SystemColors.h>

using namespace Engine;
using namespace Engine::UI;

UI::InterfaceTemplate interface;

ENGINE_REFLECTED_CLASS(lv_item, Reflection::Reflected)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text1)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text2)
	ENGINE_DEFINE_REFLECTED_PROPERTY(STRING, Text3)
ENGINE_END_REFLECTED_CLASS

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
        }
    }
    virtual void OnFrameEvent(UI::Window * window, Windows::FrameEvent event) override
    {
        // SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
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
    SafePointer<Streaming::FileStream> ConsoleOutStream = new Streaming::FileStream(IO::GetStandartOutput());
    Streaming::TextWriter Console(ConsoleOutStream);
    SafePointer<Streaming::FileStream> ConsoleInStream = new Streaming::FileStream(IO::GetStandartInput());
    Streaming::TextReader Input(ConsoleInStream, Encoding::UTF8);

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

    auto Callback2 = new _cb2;
    auto templ = interface.Dialog[L"Test3"];
    auto w4 = Windows::CreateFramedDialog(templ, Callback2, UI::Rectangle::Invalid(), 0);
    templ->Properties->GetProperty(L"ToolWindow").Set<bool>(true);
    SafePointer<UI::Template::BarShape> Shape = new UI::Template::BarShape;
    Shape->Position.Left = UI::Template::Coordinate(0);
    Shape->Position.Top = UI::Template::Coordinate(0);
    Shape->Position.Right = UI::Template::Coordinate(UI::Coordinate(0, 0.0, 1.0));
    Shape->Position.Bottom = UI::Template::Coordinate(UI::Coordinate(0, 0.0, 1.0));
    Shape->Gradient << UI::Template::GradientPoint(UI::Template::ColorTemplate(GetSystemColor(SystemColor::MenuHotBackground)), UI::Template::DoubleTemplate(0.0));
    Shape->GradientAngle = UI::Template::DoubleTemplate(0.0);
    w4->As<UI::Controls::OverlappedWindow>()->SetBackground(Shape);
    auto w666 = Windows::CreateFramedDialog(templ, 0, UI::Rectangle::Invalid(), w4->GetStation());
    w4->FindChild(212121)->SetText(req.ToString());
    if (w4) w4->Show(true);
    if (w666) w666->Show(true);
    
    Windows::RunMessageLoop();

    return 0;
}
