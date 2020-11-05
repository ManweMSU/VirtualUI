#pragma once

#include "../UserInterface/Templates.h"
#include "../Miscellaneous/Dictionary.h"
#include "../UserInterface/ControlBase.h"

namespace Engine
{
    namespace MacOSXSpecific
    {
#ifdef ENGINE_MACOSX
        class TouchBar;
        class TouchBarItem : public Object
        {
        public:
            TouchBar * Root;
            string Identifier;
            virtual void WakeUp(void) = 0;
            virtual void Shutdown(void) = 0;
            virtual void * GetObject(void) = 0;
            virtual int GetID(void) = 0;
            virtual void SetID(int ID) = 0;
            virtual TouchBarItem * FindChild(int ID) = 0;
            virtual TouchBarItem * FindChildByIdentifier(const string & identifier) = 0;
            virtual int GetChildrenCount(void) = 0;
            virtual TouchBarItem * GetChild(int index) = 0;

            template <class W> W * As(void) { return static_cast<W *>(this); }
        };
        class TouchBarGroup : public TouchBarItem
        {
        public:
            virtual void Clear(void) = 0;
            virtual void AddChild(TouchBarItem * item) = 0;
			virtual int GetMainItemID(void) = 0;
			virtual void SetMainItemID(int ID) = 0;
        };
        class TouchBarPopover : public TouchBarGroup
        {
        public:
            virtual string GetText(void) = 0;
            virtual void SetText(const string & text) = 0;
            virtual string GetImageID(void) = 0;
            virtual void SetImageID(const string & text) = 0;
            virtual UI::ITexture * GetImage(void) = 0;
            virtual void SetImage(UI::ITexture * image) = 0;
			virtual int GetMainItemID(void) = 0;
			virtual void SetMainItemID(int ID) = 0;
        };
        class TouchBarButton : public TouchBarItem
        {
        public:
            virtual string GetText(void) = 0;
            virtual void SetText(const string & text) = 0;
            virtual string GetImageID(void) = 0;
            virtual void SetImageID(const string & text) = 0;
            virtual UI::ITexture * GetImage(void) = 0;
            virtual void SetImage(UI::ITexture * image) = 0;
			virtual UI::Color GetColor(void) = 0;
			virtual void SetColor(UI::Color color) = 0;
        };
        class TouchBarColorPicker : public TouchBarItem
        {
        public:
            virtual UI::Color GetColor(void) = 0;
            virtual void SetColor(UI::Color color) = 0;
            virtual bool CanBeTransparent(void) = 0;
            virtual void SetCanBeTransparent(bool flag) = 0;
            virtual string GetImageID(void) = 0;
            virtual void SetImageID(const string & text) = 0;
            virtual UI::ITexture * GetImage(void) = 0;
            virtual void SetImage(UI::ITexture * image) = 0;
        };
        class TouchBarSlider : public TouchBarItem
        {
        public:
            virtual double GetMinimalRange(void) = 0;
            virtual double GetMaximalRange(void) = 0;
            virtual double GetPosition(void) = 0;
            virtual int GetTicks(void) = 0;
            virtual void SetMinimalRange(double value) = 0;
            virtual void SetMaximalRange(double value) = 0;
            virtual void SetPosition(double value) = 0;
            virtual void SetTicks(int value) = 0;
            virtual string GetMinimalImageID(void) = 0;
            virtual void SetMinimalImageID(const string & text) = 0;
            virtual string GetMaximalImageID(void) = 0;
            virtual void SetMaximalImageID(const string & text) = 0;
            virtual UI::ITexture * GetMinimalImage(void) = 0;
            virtual void SetMinimalImage(UI::ITexture * image) = 0;
            virtual UI::ITexture * GetMaximalImage(void) = 0;
            virtual void SetMaximalImage(UI::ITexture * image) = 0;
        };
        class TouchBar : public Object
        {
            friend class TouchBarItem;
            void * Bar;
            void * Delegate;
            UI::Window * Host;
            ObjectArray<TouchBarItem> RootItems;
            Dictionary::Dictionary<string, TouchBarItem> Links;
			int MainItemID;

            void refresh_bar(void);
        public:
            void RegisterChild(TouchBarItem * item);
            void UnregisterChild(TouchBarItem * item);
        public:
            TouchBar(void);
            TouchBar(UI::Template::ControlTemplate * source);
            virtual ~TouchBar(void) override;

            int GetChildrenCount(void);
            TouchBarItem * GetChild(int index);
            void Clear(void);
            void AddChild(TouchBarItem * item);

			int GetMainItemID(void);
			void SetMainItemID(int ID);

            TouchBarItem * FindChild(int ID);
            TouchBarItem * FindChildByIdentifier(const string & identifier);
            UI::Window * GetHost(void);
            void * GetDelegate(void);

            static TouchBarGroup * CreateGroupItem(void);
            static TouchBarPopover * CreatePopoverItem(void);
            static TouchBarButton * CreateButtonItem(void);
            static TouchBarColorPicker * CreateColorPickerItem(void);
            static TouchBarSlider * CreateSliderItem(void);

            static void SetTouchBarForWindow(UI::Window * window, TouchBar * bar);
			static TouchBar * GetTouchBarFromWindow(UI::Window * window);
        };
        TouchBar * SetTouchBarFromTemplate(UI::Window * window, UI::Template::ControlTemplate * source);
#endif
    }
}