#include "Notifications.h"

#include "CocoaInterop.h"
#include "QuartzDevice.h"
#include "NativeStationBackdoors.h"
#import "Foundation/Foundation.h"
#import "AppKit/AppKit.h"

@interface EngineRuntimeStatusBarResponder : NSObject
{
@public
	Engine::UI::Windows::StatusBarIcon * icon;
}
- (void) status_bar_icon_event_handle: (id) argument;
@end

namespace Engine
{
	namespace UI
	{
		namespace Windows
		{
			class MacStatusBarIcon : public StatusBarIcon
			{
				IWindowEventCallback * _callback;
				SafePointer<Codec::Image> _icon;
				StatusBarIconColorUsage _icon_usage;
				string _tooltip;
				int _id;
				SafePointer<Menus::Menu> _menu;
				bool _present;
				SafePointer<IRenderingDevice> _menu_server_device;
				NSMenu * _cocoa_menu;
				NSImage * _image;
				NSStatusItem * _item;
				NSStatusBar * _bar;
				int _menu_result;
				EngineRuntimeStatusBarResponder * _event_responder;
				void _update_item(void)
				{
					[[_item button] setImage: _image];
					[_item setMenu: _cocoa_menu];
				}
				bool _is_light_theme(void)
				{
					NSAppearanceName name = [[[_item button] effectiveAppearance] name];
					if ([name compare: NSAppearanceNameDarkAqua] == NSOrderedSame) return false;
					else if ([name compare: NSAppearanceNameVibrantDark] == NSOrderedSame) return false;
					else if ([name compare: NSAppearanceNameAccessibilityHighContrastDarkAqua] == NSOrderedSame) return false;
					else if ([name compare: NSAppearanceNameAccessibilityHighContrastVibrantDark] == NSOrderedSame) return false;
					else return true;
				}
				void _recreate_icon(void)
				{
					double scale = [[NSScreen mainScreen] backingScaleFactor];
					int size = int([_bar thickness] * scale);
					Codec::Frame * frame = _icon ? _icon->GetFrameBestSizeFit(size, size) : 0;
					if (frame) {
					 	SafePointer<Codec::Frame> use_frame;
					 	if (_icon_usage == StatusBarIconColorUsage::Monochromic) {
							use_frame = frame->ConvertFormat(Codec::PixelFormat::R8G8B8A8, Codec::AlphaMode::Normal, Codec::ScanOrigin::TopDown);
							uint32 color = _is_light_theme() ? 0x00000000 : 0x00FFFFFF;
							for (int y = 0; y < use_frame->GetHeight(); y++) for (int x = 0; x < use_frame->GetWidth(); x++) {
								uint32 pixel = use_frame->GetPixel(x, y);
								use_frame->SetPixel(x, y, (pixel & 0xFF000000) | color);
							}
					 	} else use_frame.SetRetain(frame);
						_image = Cocoa::CocoaImage(use_frame, scale);
					} else _image = 0;
					if (_present) _update_item();
				}
				void _update_icon(void)
				{
					if (_image) [_image release];
					_image = 0;
					if (_present) _recreate_icon();
				}
			public:
				MacStatusBarIcon(void)
				{
					_callback = 0; _icon_usage = StatusBarIconColorUsage::Colourfull;
					_id = 0; _present = false;
					_bar = [NSStatusBar systemStatusBar];
					_item = 0;
					_image = 0;
					_menu_server_device = new Cocoa::QuartzRenderingDevice;
					_cocoa_menu = 0;
					_event_responder = [[EngineRuntimeStatusBarResponder alloc] init];
					_event_responder->icon = this;
				}
				virtual ~MacStatusBarIcon(void) override
				{
					[_event_responder release];
					if (_present) PresentIcon(false);
					if (_cocoa_menu) NativeWindows::DestroyCocoaMenu(_menu, _cocoa_menu);
					[_image release];
					[_item release];
				}
				virtual void SetCallback(IWindowEventCallback * callback) override { _callback = callback; }
				virtual IWindowEventCallback * GetCallback(void) override { return _callback; }
				virtual void SetIcon(Codec::Image * image) override { _icon.SetRetain(image); _update_icon(); }
				virtual Codec::Image * GetIcon(void) override { return _icon; }
				virtual void SetIconColorUsage(StatusBarIconColorUsage color_usage) override { _icon_usage = color_usage; _update_icon(); }
				virtual StatusBarIconColorUsage GetIconColorUsage(void) override { return _icon_usage; }
				virtual void SetTooltip(const string & text) override { _tooltip = text; }
				virtual string GetTooltip(void) override { return _tooltip; }
				virtual void SetEventID(int ID) override { _id = ID; }
				virtual int GetEventID(void) override { return _id; }
				int GetMenuLastEventID(void) { return _menu_result; }
				virtual void SetMenu(Menus::Menu * menu) override
				{
					if (_cocoa_menu) NativeWindows::DestroyCocoaMenu(_menu, _cocoa_menu);
					_cocoa_menu = 0;
					_menu.SetRetain(menu);
					if (_menu) _cocoa_menu = NativeWindows::CreateCocoaMenu(_menu, &_menu_result, _event_responder, @selector(status_bar_icon_event_handle:), _menu_server_device);
					_update_item();
				}
				virtual Menus::Menu * GetMenu(void) override { return _menu; }
				virtual void PresentIcon(bool present) override
				{
					@autoreleasepool {
						if (present == _present) return;
						if (!_icon) throw InvalidArgumentException();
						if (present) {
							_item = [_bar statusItemWithLength: NSSquareStatusItemLength];
							[_item retain];
							if (!_image) _recreate_icon();
							[[_item button] setImage: _image];
							[_item setMenu: _cocoa_menu];
							[[_item button] setTarget: _event_responder];
							[[_item button] setAction: @selector(status_bar_icon_event_handle:)];
						} else {
							[_bar removeStatusItem: _item];
							[_item release];
							_item = 0;
						}
					}
				}
				virtual bool IsVisible(void) override { return _present; }
			};
			StatusBarIcon * CreateStatusBarIcon(void) { return new MacStatusBarIcon(); }
			void PushUserNotification(const string & title, const string & text, Codec::Image * icon)
			{
				@autoreleasepool {
					NSString * ns_title = Cocoa::CocoaString(title);
					NSString * ns_text = Cocoa::CocoaString(text);
					[ns_title autorelease];
					[ns_text autorelease];
					NSUserNotificationCenter * center = [NSUserNotificationCenter defaultUserNotificationCenter];
					NSUserNotification * notification = [[NSUserNotification alloc] init];
					[notification autorelease];
					[notification setTitle: ns_title];
					[notification setInformativeText: ns_text];
					[center deliverNotification: notification];
				}
			}
		}
	}
}

@implementation EngineRuntimeStatusBarResponder
- (void) status_bar_icon_event_handle: (id) argument
{
	if (icon) {
		auto item = static_cast<Engine::UI::Windows::MacStatusBarIcon *>(icon);
		auto callback = item->GetCallback();
		if (callback) {
			if (item->GetEventID()) {
				callback->OnControlEvent(0, item->GetEventID(), Engine::UI::Window::Event::MenuCommand, 0);
			} else if (item->GetMenu()) {
				callback->OnControlEvent(0, item->GetMenuLastEventID(), Engine::UI::Window::Event::MenuCommand, 0);
			}
		}
	}
}
@end