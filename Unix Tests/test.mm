#include <EngineRuntime.h>
#include <PlatformDependent/CocoaInterop.h>
#include <PlatformDependent/KeyCodes.h>

using namespace Engine;
using namespace Engine::UI;

#include <stdio.h>
#include <unistd.h>

@import Foundation;
@import AppKit;

UI::InterfaceTemplate interface;

class _cb2 : public Windows::IWindowEventCallback
{
public:
    virtual void OnInitialized(UI::Window * window) override {}
    virtual void OnControlEvent(UI::Window * window, int ID, Window::Event event, UI::Window * sender) override
    {
        if (ID == 1) {
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
        if (event == Windows::FrameEvent::Close) window->Destroy();
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

    NSUserDefaults * def = [[NSUserDefaults alloc] init];
    NSDictionary<NSString *,id> * dict = [def dictionaryRepresentation];
    NSArray<NSString *> * keys = [dict allKeys];
    int c = [keys count];
    Console << L"Dictionary length: " << c << IO::NewLineChar;
    for (int i = 0; i < c; i++) {
        Console << L"   - " << Cocoa::EngineString([keys objectAtIndex: i]) << IO::NewLineChar;
    }
    [keys release];
    [dict release];
    [def release];

    Console << L"Keyboard delay " + string(Keyboard::GetKeyboardDelay()) + L" ms" + IO::NewLineChar;
    Console << L"Keyboard speed " + string(Keyboard::GetKeyboardSpeed()) + L" ms" + IO::NewLineChar;

    auto sbox = Windows::GetScreenDimensions();
    Console << L"Screen " + string(sbox.Left) + L", " + string(sbox.Top) + L", " + string(sbox.Right) + L", " + string(sbox.Bottom) + IO::NewLineChar;
    Console << L"Scale  " + string(Windows::GetScreenScale()) + IO::NewLineChar;

    auto Callback2 = new _cb2;
    auto w4 = Windows::CreateFramedDialog(interface.Dialog[L"Test3"], Callback2, UI::Rectangle::Invalid(), 0);
    w4->FindChild(212121)->SetText(req.ToString());
    if (w4) w4->Show(true);
    
    Windows::RunMessageLoop();

    return 0;
}
