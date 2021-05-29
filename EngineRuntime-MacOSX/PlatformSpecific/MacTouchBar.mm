#include "MacTouchBar.h"

#ifdef ENGINE_MACOSX

@import Foundation;
@import AppKit;

#include "../PlatformDependent/CocoaInterop.h"
#include "../PlatformDependent/NativeStationBackdoors.h"
#include "../PlatformDependent/QuartzDevice.h"
#include "../UserInterface/ControlClasses.h"
#include "../Math/Color.h"

@interface EngineRuntimeTouchBarDelegate : NSObject<NSTouchBarDelegate>
{
@public
    Engine::MacOSXSpecific::TouchBar * bar;
}
- (NSTouchBarItem *) touchBar: (NSTouchBar *) touchBar makeItemForIdentifier: (NSTouchBarItemIdentifier) identifier;
- (void) handle_touch_bar_event: (id) sender;
- (void) handle_touch_bar_event_from_picker: (id) sender;
- (void) handle_touch_bar_event_from_slider: (id) sender;
@end

namespace Engine
{
    namespace MacOSXSpecific
    {
        NSImage * GetCocoaImage(UI::ITexture * texture, const string & ident)
        {
            if (ident.Length()) {
                NSString * name = Cocoa::CocoaString(ident);
                NSImage * image = [NSImage imageNamed: name];
                [image retain];
                [name release];
                return image;
            } else if (texture) {
                CGImageRef image = reinterpret_cast<CGImageRef>(Cocoa::GetCoreImageFromTexture(texture));
                CGSize size;
                size.width = double(texture->GetWidth()) / 2.0;
                size.height = double(texture->GetHeight()) / 2.0;
                return [[NSImage alloc] initWithCGImage: image size: size];
            } else return 0;
        }
        class TouchBarButtonImp : public TouchBarButton
        {
			double r, g, b, a;
            NSCustomTouchBarItem * item;
            int _ID;
            string Text;
            SafePointer<UI::ITexture> Image;
            string ImageID;
        public:
            TouchBarButtonImp(void) { item = 0; _ID = 0; Root = 0; r = g = b = a = 0.0; }
            virtual ~TouchBarButtonImp(void) override { [item release]; }

            virtual void WakeUp(void) override
            {
                @autoreleasepool {
                    NSString * ident = Cocoa::CocoaString(Identifier);
                    item = [[NSCustomTouchBarItem alloc] initWithIdentifier: ident];
                    [ident release];
                    NSButton * button = [NSButton buttonWithTitle: @"" target: (EngineRuntimeTouchBarDelegate *) Root->GetDelegate() action: @selector(handle_touch_bar_event:)];
                    [button setTag: _ID];
					if (a) [button setBezelColor: [NSColor colorWithDeviceRed: r green: g blue: b alpha: a]];
                    [item setView: button];
                    NSString * title = Cocoa::CocoaString(Text);
                    [[item view] setTitle: title];
                    [title release];
                    NSImage * image = GetCocoaImage(Image, ImageID);
                    [[item view] setImage: image];
                    [image release];
                }
            }
            virtual void Shutdown(void) override
            {
                [item release];
                item = 0;
            }
            virtual void * GetObject(void) override { return item; }
            virtual int GetID(void) override { return _ID; }
            virtual void SetID(int ID) override { _ID = ID; }
            virtual TouchBarItem * FindChild(int ID) override { if (_ID && _ID == ID) return this; else return 0; }
            virtual TouchBarItem * FindChildByIdentifier(const string & identifier) override { if (identifier == Identifier) return this; else return 0; }
            virtual int GetChildrenCount(void) override { return 0; }
            virtual TouchBarItem * GetChild(int index) override { return 0; }
            virtual string GetText(void) override { return Text; }
            virtual void SetText(const string & text) override
            {
                Text = text;
                if (item) {
                    NSString * title = Cocoa::CocoaString(Text);
                    [[item view] setTitle: title];
                    [title release];
                }
            }
            virtual string GetImageID(void) override { return ImageID; }
            virtual void SetImageID(const string & text) override
            {
                @autoreleasepool {
                    ImageID = text;
                    if (item) {
                        NSImage * image = GetCocoaImage(Image, ImageID);
                        [[item view] setImage: image];
                        [image release];
                    }
                }
            }
            virtual UI::ITexture * GetImage(void) override { return Image; }
            virtual void SetImage(UI::ITexture * image) override
            {
                @autoreleasepool {
                    Image.SetRetain(image);
                    if (item) {
                        NSImage * image = GetCocoaImage(Image, ImageID);
                        [[item view] setImage: image];
                        [image release];
                    }
                }
            }
			virtual UI::Color GetColor(void) override { return UI::Color(r, g, b, a); }
			virtual void SetColor(UI::Color color) override
			{
				r = double(color.r) / 255.0;
				g = double(color.g) / 255.0;
				b = double(color.b) / 255.0;
				a = double(color.a) / 255.0;
				@autoreleasepool {
					if (item) {
						[[item view] setBezelColor: [NSColor colorWithDeviceRed: r green: g blue: b alpha: a]];
					}
				}
			}
        };
        class TouchBarGroupImp : public TouchBarGroup
        {
            NSGroupTouchBarItem * item;
            ObjectArray<TouchBarItem> Items;
            int _ID;
			int MainID;
        public:
            TouchBarGroupImp(void) { item = 0; _ID = 0; Root = 0; MainID = 0; }
            virtual ~TouchBarGroupImp(void) override
            {
                [item release];
            }

            void Populate(void)
            {
                @autoreleasepool {
                    if (item) {
                        Array<NSString *> items(0x10);
                        for (int i = 0; i < Items.Length(); i++) items << Cocoa::CocoaString(Items[i].Identifier);
                        auto bar = [item groupTouchBar];
                        NSArray<NSTouchBarItemIdentifier> * idents = [NSArray<NSTouchBarItemIdentifier> arrayWithObjects: items.GetBuffer() count: items.Length()];
                        [bar setDefaultItemIdentifiers: idents];
						auto main_item_ref = MainID ? FindChild(MainID) : 0;
						if (main_item_ref) {
							NSString * main_item = Cocoa::CocoaString(main_item_ref->Identifier);
							[bar setPrincipalItemIdentifier: main_item];
							[main_item release];
						}
                        for (int i = 0; i < items.Length(); i++) [items[i] release];
                    }
                }
            }
            virtual void WakeUp(void) override
            {
                @autoreleasepool {
                    NSString * ident = Cocoa::CocoaString(Identifier);
                    NSArray * a = [NSArray array];
                    item = [NSGroupTouchBarItem groupItemWithIdentifier: ident items: a];
                    [item retain];
                    [ident release];
                    [[item groupTouchBar] setDelegate: (id) Root->GetDelegate()];
                    Populate();
                }
            }
            virtual void Shutdown(void) override
            {
                [item release];
                item = 0;
            }
            virtual void * GetObject(void) override { return item; }
            virtual int GetID(void) override { return _ID; }
            virtual void SetID(int ID) override { _ID = ID; }
			virtual int GetMainItemID(void) override { return MainID; }
			virtual void SetMainItemID(int ID) override { MainID = ID; Populate(); }
            virtual TouchBarItem * FindChild(int ID) override
            {
                if (_ID && _ID == ID) return this;
                for (int i = 0; i < Items.Length(); i++) {
                    auto item = Items[i].FindChild(ID);
                    if (item) return item;
                }
                return 0;
            }
            virtual TouchBarItem * FindChildByIdentifier(const string & identifier) override
            {
                if (identifier == Identifier) return this;
                for (int i = 0; i < Items.Length(); i++) {
                    auto item = Items[i].FindChildByIdentifier(identifier);
                    if (item) return item;
                }
                return 0;
            }
            virtual int GetChildrenCount(void) override { return Items.Length(); }
            virtual TouchBarItem * GetChild(int index) override { return Items.ElementAt(index); }
            virtual void Clear(void) override
            {
                if (Root) for (int i = 0; i < Items.Length(); i++) Root->UnregisterChild(Items.ElementAt(i));
                Items.Clear();
                Populate();
            }
            virtual void AddChild(TouchBarItem * item) override
            {
                Items.Append(item);
                if (Root) Root->RegisterChild(item);
                Populate();
            }
        };
        class TouchBarPopoverImp : public TouchBarPopover
        {
            NSPopoverTouchBarItem * item;
            ObjectArray<TouchBarItem> Items;
            int _ID;
			int MainID;
            string Text;
            SafePointer<UI::ITexture> Image;
            string ImageID;
        public:
            TouchBarPopoverImp(void) { item = 0; _ID = 0; Root = 0; MainID = 0; }
            virtual ~TouchBarPopoverImp(void) override { [item release]; }

            void Populate(void)
            {
                @autoreleasepool {
                    if (item) {
                        Array<NSString *> items(0x10);
                        for (int i = 0; i < Items.Length(); i++) items << Cocoa::CocoaString(Items[i].Identifier);
                        auto bar = [item popoverTouchBar];
                        NSArray<NSTouchBarItemIdentifier> * idents = [NSArray<NSTouchBarItemIdentifier> arrayWithObjects: items.GetBuffer() count: items.Length()];
                        [bar setDefaultItemIdentifiers: idents];
						auto main_item_ref = MainID ? FindChild(MainID) : 0;
						if (main_item_ref) {
							NSString * main_item = Cocoa::CocoaString(main_item_ref->Identifier);
							[bar setPrincipalItemIdentifier: main_item];
							[main_item release];
						}
                        for (int i = 0; i < items.Length(); i++) [items[i] release];
                    }
                }
            }
            virtual void WakeUp(void) override
            {
                @autoreleasepool {
                    NSString * ident = Cocoa::CocoaString(Identifier);
                    item = [[NSPopoverTouchBarItem alloc] initWithIdentifier: ident];
                    [ident release];
                    [[item popoverTouchBar] setDelegate: (id) Root->GetDelegate()];
                    Populate();
                    NSString * title = Cocoa::CocoaString(Text);
                    [item setCollapsedRepresentationLabel: title];
                    [title release];
                    NSImage * image = GetCocoaImage(Image, ImageID);
                    [item setCollapsedRepresentationImage: image];
                    [image release];
                }
            }
            virtual void Shutdown(void) override
            {
                [item release];
                item = 0;
            }
            virtual void * GetObject(void) override { return item; }
            virtual int GetID(void) override { return _ID; }
            virtual void SetID(int ID) override { _ID = ID; }
			virtual int GetMainItemID(void) override { return MainID; }
			virtual void SetMainItemID(int ID) override { MainID = ID; Populate(); }
            virtual TouchBarItem * FindChild(int ID) override
            {
                if (_ID && _ID == ID) return this;
                for (int i = 0; i < Items.Length(); i++) {
                    auto item = Items[i].FindChild(ID);
                    if (item) return item;
                }
                return 0;
            }
            virtual TouchBarItem * FindChildByIdentifier(const string & identifier) override
            {
                if (identifier == Identifier) return this;
                for (int i = 0; i < Items.Length(); i++) {
                    auto item = Items[i].FindChildByIdentifier(identifier);
                    if (item) return item;
                }
                return 0;
            }
            virtual int GetChildrenCount(void) override { return Items.Length(); }
            virtual TouchBarItem * GetChild(int index) override { return Items.ElementAt(index); }
            virtual void Clear(void) override
            {
                if (Root) for (int i = 0; i < Items.Length(); i++) Root->UnregisterChild(Items.ElementAt(i));
                Items.Clear();
                Populate();
            }
            virtual void AddChild(TouchBarItem * item) override
            {
                Items.Append(item);
                if (Root) Root->RegisterChild(item);
                Populate();
            }
            virtual string GetText(void) override { return Text; }
            virtual void SetText(const string & text) override
            {
                Text = text;
                if (item) {
                    NSString * title = Cocoa::CocoaString(Text);
                    [item setCollapsedRepresentationLabel: title];
                    [title release];
                }
            }
            virtual string GetImageID(void) override { return ImageID; }
            virtual void SetImageID(const string & text) override
            {
                @autoreleasepool {
                    ImageID = text;
                    if (item) {
                        NSImage * image = GetCocoaImage(Image, ImageID);
                        [item setCollapsedRepresentationImage: image];
                        [image release];
                    }
                }
            }
            virtual UI::ITexture * GetImage(void) override { return Image; }
            virtual void SetImage(UI::ITexture * image) override
            {
                @autoreleasepool {
                    Image.SetRetain(image);
                    if (item) {
                        NSImage * image = GetCocoaImage(Image, ImageID);
                        [item setCollapsedRepresentationImage: image];
                        [image release];
                    }
                }
            }
        };
        class TouchBarColorPickerImp : public TouchBarColorPicker
        {
            NSColorPickerTouchBarItem * item;
            int _ID;
            UI::Color Color;
            bool AllowTransparent;
            SafePointer<UI::ITexture> Image;
            string ImageID;
        public:
            TouchBarColorPickerImp(void) { item = 0; _ID = 0; Root = 0; }
            virtual ~TouchBarColorPickerImp(void) override { [item release]; }

            void IntSetColor(UI::Color color) { Color = color; }
            virtual void WakeUp(void) override
            {
                @autoreleasepool {
                    NSString * ident = Cocoa::CocoaString(Identifier);
                    NSImage * image = GetCocoaImage(Image, ImageID);
                    item = [NSColorPickerTouchBarItem colorPickerWithIdentifier: ident buttonImage: image];
                    [item retain];
                    [image release];
                    [ident release];
                    NSArray<NSColorSpace *> * spaces = [NSArray<NSColorSpace *> arrayWithObject: [NSColorSpace deviceRGBColorSpace]];
                    [item setAllowedColorSpaces: spaces];
                    [item setShowsAlpha: AllowTransparent ? YES : NO];
                    [item setTarget: (id) Root->GetDelegate()];
                    [item setAction: @selector(handle_touch_bar_event_from_picker:)];
                    Math::Color mc(Color);
                    NSColor * color = [NSColor colorWithDeviceRed: mc.x green: mc.y blue: mc.z alpha: mc.w];
                    [item setColor: color];
                }
            }
            virtual void Shutdown(void) override
            {
                [item release];
                item = 0;
            }
            virtual void * GetObject(void) override { return item; }
            virtual int GetID(void) override { return _ID; }
            virtual void SetID(int ID) override { _ID = ID; }
            virtual TouchBarItem * FindChild(int ID) override { if (_ID && _ID == ID) return this; else return 0; }
            virtual TouchBarItem * FindChildByIdentifier(const string & identifier) override { if (identifier == Identifier) return this; else return 0; }
            virtual int GetChildrenCount(void) override { return 0; }
            virtual TouchBarItem * GetChild(int index) override { return 0; }
            virtual UI::Color GetColor(void) override { return Color; }
            virtual void SetColor(UI::Color color) override
            {
                @autoreleasepool {
                    Color = color;
                    if (item) {
                        Math::Color mc(Color);
                        NSColor * color = [NSColor colorWithDeviceRed: mc.x green: mc.y blue: mc.z alpha: mc.w];
                        [item setColor: color];
                    }
                }
            }
            virtual bool CanBeTransparent(void) override { return AllowTransparent; }
            virtual void SetCanBeTransparent(bool flag) override
            {
                AllowTransparent = flag;
                if (item) [item setShowsAlpha: AllowTransparent ? YES : NO];
            }
            virtual string GetImageID(void) override { return ImageID; }
            virtual void SetImageID(const string & text) override
            {
                ImageID = text;
                if (item) { Shutdown(); WakeUp(); }
            }
            virtual UI::ITexture * GetImage(void) override { return Image; }
            virtual void SetImage(UI::ITexture * image) override
            {
                Image.SetRetain(image);
                if (item) { Shutdown(); WakeUp(); }
            }
        };
        class TouchBarSliderImp : public TouchBarSlider
        {
            NSSliderTouchBarItem * item;
            int _ID;
            double RangeMinimal;
            double RangeMaximal;
            double Position;
            int Ticks;
            SafePointer<UI::ITexture> MinImage;
            SafePointer<UI::ITexture> MaxImage;
            string MinImageID;
            string MaxImageID;
        public:
            TouchBarSliderImp(void) { item = 0; _ID = 0; Root = 0; }
            virtual ~TouchBarSliderImp(void) override { [item release]; }

            void IntSetPosition(double value) { Position = value; }
            virtual void WakeUp(void) override
            {
                @autoreleasepool {
                    NSString * ident = Cocoa::CocoaString(Identifier);
                    item = [[NSSliderTouchBarItem alloc] initWithIdentifier: ident];
                    [ident release];
                    [item setTarget: (id) Root->GetDelegate()];
                    [item setAction: @selector(handle_touch_bar_event_from_slider:)];
                    NSSlider * slider = [item slider];
                    [slider setMaxValue: RangeMaximal];
                    [slider setMinValue: RangeMinimal];
                    [slider setNumberOfTickMarks: Ticks];
                    [slider setAllowsTickMarkValuesOnly: YES];
                    [slider setDoubleValue: Position];
                    NSImage * minim = GetCocoaImage(MinImage, MinImageID);
                    if (minim) {
                        NSSliderAccessory * acc = [NSSliderAccessory accessoryWithImage: minim];
                        [item setMinimumValueAccessory: acc];
                        [minim release];
                    }
                    NSImage * maxim = GetCocoaImage(MaxImage, MaxImageID);
                    if (maxim) {
                        NSSliderAccessory * acc = [NSSliderAccessory accessoryWithImage: maxim];
                        [item setMaximumValueAccessory: acc];
                        [maxim release];
                    }
                }
            }
            virtual void Shutdown(void) override
            {
                [item release];
                item = 0;
            }
            virtual void * GetObject(void) override { return item; }
            virtual int GetID(void) override { return _ID; }
            virtual void SetID(int ID) override { _ID = ID; }
            virtual TouchBarItem * FindChild(int ID) override { if (_ID && _ID == ID) return this; else return 0; }
            virtual TouchBarItem * FindChildByIdentifier(const string & identifier) override { if (identifier == Identifier) return this; else return 0; }
            virtual int GetChildrenCount(void) override { return 0; }
            virtual TouchBarItem * GetChild(int index) override { return 0; }
            virtual double GetMinimalRange(void) override { return RangeMinimal; }
            virtual double GetMaximalRange(void) override { return RangeMaximal; }
            virtual double GetPosition(void) override { return Position; }
            virtual int GetTicks(void) override { return Ticks; }
            virtual void SetMinimalRange(double value) override
            {
                RangeMinimal = value;
                if (item) [[item slider] setMinValue: RangeMinimal];
            }
            virtual void SetMaximalRange(double value) override
            {
                RangeMaximal = value;
                if (item) [[item slider] setMaxValue: RangeMaximal];
            }
            virtual void SetPosition(double value) override
            {
                Position = value;
                if (item) [[item slider] setDoubleValue: Position];
            }
            virtual void SetTicks(int value) override
            {
                Ticks = value;
                if (item) [[item slider] setNumberOfTickMarks: Ticks];
            }
            virtual string GetMinimalImageID(void) override { return MinImageID; }
            virtual void SetMinimalImageID(const string & text) override
            {
                MinImageID = text;
                if (item) { Shutdown(); WakeUp(); }
            }
            virtual string GetMaximalImageID(void) override { return MaxImageID; }
            virtual void SetMaximalImageID(const string & text) override
            {
                MaxImageID = text;
                if (item) { Shutdown(); WakeUp(); }
            }
            virtual UI::ITexture * GetMinimalImage(void) override { return MinImage; }
            virtual void SetMinimalImage(UI::ITexture * image) override
            {
                MinImage.SetRetain(image);
                if (item) { Shutdown(); WakeUp(); }   
            }
            virtual UI::ITexture * GetMaximalImage(void) override { return MaxImage; }
            virtual void SetMaximalImage(UI::ITexture * image) override
            {
                MaxImage.SetRetain(image);
                if (item) { Shutdown(); WakeUp(); }
            }
        };

        void TouchBar::refresh_bar(void)
        {
            @autoreleasepool {
                Array<NSString *> items(0x10);
                for (int i = 0; i < RootItems.Length(); i++) items << Cocoa::CocoaString(RootItems[i].Identifier);
                auto bar = ((NSTouchBar *) Bar);
                NSArray<NSTouchBarItemIdentifier> * idents = [NSArray<NSTouchBarItemIdentifier> arrayWithObjects: items.GetBuffer() count: items.Length()];
                [bar setDefaultItemIdentifiers: idents];
				auto main_item_ref = MainItemID ? FindChild(MainItemID) : 0;
				if (main_item_ref) {
					NSString * main_item = Cocoa::CocoaString(main_item_ref->Identifier);
					[bar setPrincipalItemIdentifier: main_item];
					[main_item release];
				}
                for (int i = 0; i < items.Length(); i++) [items[i] release];
            }
        }
        void TouchBar::RegisterChild(TouchBarItem * item)
        {
            int i = 1;
            while (true) {
                auto obj = Links[string(i)];
                if (!obj) break;
                i++;
            }
            item->Identifier = string(i);
            item->Root = this;
            Links.Append(item->Identifier, item);
            for (int j = 0; j < item->GetChildrenCount(); j++) RegisterChild(item->GetChild(j));
            item->WakeUp();
        }
        void TouchBar::UnregisterChild(TouchBarItem * item)
        {
            item->Shutdown();
            for (int j = 0; j < item->GetChildrenCount(); j++) UnregisterChild(item->GetChild(j));
            Links.RemoveByKey(item->Identifier);
            item->Identifier = L"";
            item->Root = 0;
        }
        TouchBarItem * MakeItem(UI::Template::ControlTemplate * source)
        {
            if (source->Properties->GetTemplateClass() == L"TouchBarButton") {
                auto props = reinterpret_cast<UI::Template::Controls::TouchBarButton *>(source->Properties);
                SafePointer<TouchBarButton> item = TouchBar::CreateButtonItem();
                item->SetID(props->ID);
                item->SetText(props->Text);
                item->SetImage(props->Image);
                item->SetImageID(props->ImageID);
				item->SetColor(props->Color);
                item->Retain();
                return item;
            } else if (source->Properties->GetTemplateClass() == L"TouchBarGroup") {
                auto props = reinterpret_cast<UI::Template::Controls::TouchBarGroup *>(source->Properties);
                SafePointer<TouchBarGroup> item = TouchBar::CreateGroupItem();
				item->SetMainItemID(props->MainItemID);
                item->SetID(props->ID);
                for (int i = 0; i < source->Children.Length(); i++) {
                    SafePointer<TouchBarItem> sub = MakeItem(source->Children.ElementAt(i));
                    item->AddChild(sub);
                }
                item->Retain();
                return item;
            } else if (source->Properties->GetTemplateClass() == L"TouchBarPopover") {
                auto props = reinterpret_cast<UI::Template::Controls::TouchBarPopover *>(source->Properties);
                SafePointer<TouchBarPopover> item = TouchBar::CreatePopoverItem();
                item->SetID(props->ID);
				item->SetMainItemID(props->MainItemID);
                item->SetImage(props->Image);
                item->SetImageID(props->ImageID);
                for (int i = 0; i < source->Children.Length(); i++) {
                    SafePointer<TouchBarItem> sub = MakeItem(source->Children.ElementAt(i));
                    item->AddChild(sub);
                }
                item->Retain();
                return item;
            } else if (source->Properties->GetTemplateClass() == L"TouchBarColorPicker") {
                auto props = reinterpret_cast<UI::Template::Controls::TouchBarColorPicker *>(source->Properties);
                SafePointer<TouchBarColorPicker> item = TouchBar::CreateColorPickerItem();
                item->SetID(props->ID);
                item->SetColor(props->Color);
                item->SetCanBeTransparent(props->AllowTransparent);
                item->SetImage(props->Image);
                item->SetImageID(props->ImageID);
                item->Retain();
                return item;
            } else if (source->Properties->GetTemplateClass() == L"TouchBarSlider") {
                auto props = reinterpret_cast<UI::Template::Controls::TouchBarSlider *>(source->Properties);
                SafePointer<TouchBarSlider> item = TouchBar::CreateSliderItem();
                item->SetID(props->ID);
                item->SetMinimalRange(props->RangeMinimal);
                item->SetMaximalRange(props->RangeMaximal);
                item->SetPosition(props->Position);
                item->SetTicks(props->Ticks);
                item->SetMinimalImage(props->MinImage);
                item->SetMinimalImageID(props->MinImageID);
                item->SetMaximalImage(props->MaxImage);
                item->SetMaximalImageID(props->MaxImageID);
                item->Retain();
                return item;
            } else throw InvalidArgumentException();
        }
        TouchBar::TouchBar(void) : RootItems(0x10), Links(0x20), Host(0), MainItemID(0)
        {
            Bar = [[NSTouchBar alloc] init];
            Delegate = [[EngineRuntimeTouchBarDelegate alloc] init];
            ((EngineRuntimeTouchBarDelegate * ) Delegate)->bar = this;
            [((NSTouchBar *) Bar) setDelegate: ((EngineRuntimeTouchBarDelegate * ) Delegate)];
        }
        TouchBar::TouchBar(UI::Template::ControlTemplate * source) : TouchBar()
        {
            if (source->Properties->GetTemplateClass() != L"TouchBar") throw InvalidArgumentException();
			auto props = reinterpret_cast<UI::Template::Controls::TouchBar *>(source->Properties);
			SetMainItemID(props->MainItemID);
            for (int i = 0; i < source->Children.Length(); i++) {
                SafePointer<TouchBarItem> item = MakeItem(source->Children.ElementAt(i));
                AddChild(item);
            }
        }
        TouchBar::~TouchBar(void)
        {
            [((NSTouchBar *) Bar) release];
            [((EngineRuntimeTouchBarDelegate * ) Delegate) release];
            while (RootItems.Length()) {
                UnregisterChild(RootItems.ElementAt(0));
                RootItems.RemoveFirst();
            }
        }

		int TouchBar::GetMainItemID(void) { return MainItemID; }
		void TouchBar::SetMainItemID(int ID) { MainItemID = ID; refresh_bar(); }

        int TouchBar::GetChildrenCount(void) { return RootItems.Length(); }
        TouchBarItem * TouchBar::GetChild(int index) { return RootItems.ElementAt(index); }
        void TouchBar::Clear(void)
        {
            while (RootItems.Length()) {
                UnregisterChild(RootItems.ElementAt(0));
                RootItems.RemoveFirst();
            }
            refresh_bar();
        }
        void TouchBar::AddChild(TouchBarItem * item)
        {
            RootItems.Append(item);
            RegisterChild(item);
            refresh_bar();
        }

        TouchBarItem * TouchBar::FindChild(int ID)
        {
            for (int i = 0; i < RootItems.Length(); i++) {
                auto item = RootItems[i].FindChild(ID);
                if (item) return item;
            }
            return 0;
        }
        TouchBarItem * TouchBar::FindChildByIdentifier(const string & identifier)
        {
            for (int i = 0; i < RootItems.Length(); i++) {
                auto item = RootItems[i].FindChildByIdentifier(identifier);
                if (item) return item;
            }
            return 0;
        }
        UI::Window * TouchBar::GetHost(void) { return Host; }
        void * TouchBar::GetDelegate(void) { return Delegate; }

        TouchBarGroup * TouchBar::CreateGroupItem(void) { return new TouchBarGroupImp; }
        TouchBarPopover * TouchBar::CreatePopoverItem(void) { return new TouchBarPopoverImp; }
        TouchBarButton * TouchBar::CreateButtonItem(void) { return new TouchBarButtonImp; }
        TouchBarColorPicker * TouchBar::CreateColorPickerItem(void) { return new TouchBarColorPickerImp; }
        TouchBarSlider * TouchBar::CreateSliderItem(void) { return new TouchBarSliderImp; }

        void TouchBar::SetTouchBarForWindow(UI::Window * window, TouchBar * bar)
        {
            if (bar) {
                if (bar->Host) throw InvalidArgumentException();
                bar->Host = window;
                [NativeWindows::GetWindowObject(window->GetStation()) setTouchBar: ((NSTouchBar *) bar->Bar)];
            }
            NativeWindows::SetTouchBarObject(window->GetStation(), bar);
        }
		TouchBar * TouchBar::GetTouchBarFromWindow(UI::Window * window)
		{
			return static_cast<TouchBar *>(NativeWindows::GetTouchBarObject(window->GetStation()));
		}
        TouchBar * SetTouchBarFromTemplate(UI::Window * window, UI::Template::ControlTemplate * source)
        {
            SafePointer<TouchBar> bar = new TouchBar(source);
            TouchBar::SetTouchBarForWindow(window, bar);
            bar->Retain();
            return bar;
        }
    }
}

@implementation EngineRuntimeTouchBarDelegate
- (NSTouchBarItem *) touchBar: (NSTouchBar *) touchBar makeItemForIdentifier: (NSTouchBarItemIdentifier) identifier;
{
    auto ident = Engine::Cocoa::EngineString(identifier);
    auto * child = bar->FindChildByIdentifier(ident);
    if (child) {
        NSTouchBarItem * item = (NSTouchBarItem *) child->GetObject();
        [item retain];
        return item;
    }
    return 0;
}
- (void) handle_touch_bar_event: (id) sender
{
    int ID = [sender tag];
    if (ID) bar->GetHost()->RaiseEvent(ID, Engine::UI::Window::Event::MenuCommand, 0);
}
- (void) handle_touch_bar_event_from_picker: (id) sender
{
    auto item = bar->FindChildByIdentifier(Engine::Cocoa::EngineString([sender identifier]));
    if (item) {
        int ID = item->GetID();
        NSColor * color = [sender color];
        Engine::Math::Color mc;
        [color getRed: &mc.x green: &mc.y blue: &mc.z alpha: &mc.w];
        static_cast<Engine::MacOSXSpecific::TouchBarColorPickerImp *>(item)->IntSetColor(mc);
        if (ID) bar->GetHost()->RaiseEvent(ID, Engine::UI::Window::Event::MenuCommand, 0);
    }
}
- (void) handle_touch_bar_event_from_slider: (id) sender
{
    auto item = bar->FindChildByIdentifier(Engine::Cocoa::EngineString([sender identifier]));
    if (item) {
        int ID = item->GetID();
        double value = [[sender slider] doubleValue];
        static_cast<Engine::MacOSXSpecific::TouchBarSliderImp *>(item)->IntSetPosition(value);
        if (ID) bar->GetHost()->RaiseEvent(ID, Engine::UI::Window::Event::MenuCommand, 0);
    }
}
@end

#endif