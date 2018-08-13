#include <EngineRuntime.h>

using namespace Engine;
using namespace Engine::UI;

UI::InterfaceTemplate interface;

class _cb2 : public Windows::IWindowEventCallback
{
public:
    virtual void OnInitialized(UI::Window * window) override
    {
        auto list = window->FindChild(343434)->As<Controls::ListBox>();
        list->AddItem(L"List Box Item 1");
        list->AddItem(L"List Box Item 2");
        list->AddItem(L"List Box Item 3");
        list->AddItem(L"List Box Item 4");
        list->AddItem(L"List Box Item 5");
        list->AddItem(L"List Box Item 6");
        list->AddItem(L"List Box Item 7");
        list->AddItem(L"List Box Item 8");
        list->AddItem(L"List Box Item 9");
        list->AddItem(L"List Box Item 10");
        list->MultiChoose = true;
        auto list2 = window->FindChild(353535)->As<Controls::ListBox>();
        list2->AddItem(L"1");
        list2->AddItem(L"2");
        list2->AddItem(L"3");
        list2->AddItem(L"4");
        list2->AddItem(L"5");
        list2->AddItem(L"6");
        list2->AddItem(L"7");
        list2->AddItem(L"8");
        list2->AddItem(L"9");
        list2->AddItem(L"10");
        list2->MultiChoose = true;
    }
    virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
    {
        if (ID == 343434 || ID == 353535) {
            if (event == Window::Event::DoubleClick) {
                sender->As<Controls::ListBox>()->CreateEmbeddedEditor(interface.Dialog[L"editor"], Rectangle::Entire())->FindChild(888888)->SetFocus();
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
    Console << string((void*) source) << IO::NewLineChar;
    SafePointer<UI::IResourceLoader> loader = Windows::CreateNativeCompatibleResourceLoader();
    UI::Loader::LoadUserInterfaceFromBinary(interface, source, loader, 0);
    source->Release();

    auto Callback2 = new _cb2;
    auto w4 = Windows::CreateFramedDialog(interface.Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid(), 0);
    w4->FindChild(212121)->SetText(req.ToString());
    if (w4) w4->Show(true);
    
    Windows::RunMessageLoop();

    return 0;
}
