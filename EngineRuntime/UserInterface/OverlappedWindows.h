#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class OverlappedWindow;
			class ContentFrame;
		}
		namespace Windows
		{
			enum class FrameEvent { Close, Move, Maximize, Minimize, Help, PopupMenuCancelled };
			class IWindowEventCallback
			{
			public:
				virtual void OnInitialized(Window * window) = 0;
				virtual void OnControlEvent(Window * window, int ID, Window::Event event, Window * sender) = 0;
				virtual void OnFrameEvent(Window * window, FrameEvent event) = 0;
			};
			Controls::OverlappedWindow * CreateFramelessDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
			Controls::OverlappedWindow * CreateFramedDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
		}
		namespace Controls
		{
			class OverlappedWindow : public ParentWindow
			{
				friend Controls::OverlappedWindow * ::Engine::UI::Windows::CreateFramedDialog(Template::ControlTemplate * Template, Windows::IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
			private:
				struct size
				{
					int part = 0;
					int sl = 0;
					int st = 0;
					int sr = 0;
					int sb = 0;
					Point lp;
				};
				ContentFrame * _inner;
				Windows::IWindowEventCallback * _callback;
				SafePointer<Shape> _active;
				SafePointer<Shape> _inactive;
				bool _visible, _enabled;
				int _mode, _border, _caption, _state, _minwidth, _minheight, _btnwidth;
				Rectangle ControlPosition;
				Window * _close;
				Window * _maximize;
				Window * _minimize;
				Window * _help;
				size _size;
			public:
				OverlappedWindow(Window * Parent, WindowStation * Station);
				OverlappedWindow(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~OverlappedWindow(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void Enable(bool enable) override;
				virtual bool IsEnabled(void) override;
				virtual void Show(bool visible) override;
				virtual bool IsVisible(void) override;
				virtual bool IsOverlapped(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
				virtual void SetText(const string & text) override;
				virtual string GetText(void) override;
				virtual void RaiseEvent(int ID, Event event, Window * sender) override;
				virtual void CaptureChanged(bool got_capture) override;
				virtual void LeftButtonDown(Point at) override;
				virtual void LeftButtonUp(Point at) override;
				virtual void LeftButtonDoubleClick(Point at) override;
				virtual void MouseMove(Point at) override;
				virtual void PopupMenuCancelled(void) override;
				virtual void SetCursor(Point at) override;

				ContentFrame * GetContentFrame(void);
				Windows::IWindowEventCallback * GetCallback(void);
				void SetCallback(Windows::IWindowEventCallback * callback);
				int GetBorderWidth(void);
				int GetCaptionWidth(void);
				int GetButtonsWidth(void);
				int GetPart(Point cursor);
			};
			class ContentFrame : public ParentWindow, private Template::Controls::DialogFrame
			{
				friend class OverlappedWindow;
			private:
				SafePointer<Shape> _background;
			public:
				ContentFrame(Window * Parent, WindowStation * Station);
				ContentFrame(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~ContentFrame(void) override;

				virtual void Render(const Box & at) override;
				virtual void ResetCache(void) override;
				virtual void SetRectangle(const Rectangle & rect) override;
				virtual Rectangle GetRectangle(void) override;
			};

			namespace Constructor
			{
				void ConstructChildren(Window * on, Template::ControlTemplate * source);
			}
		}
	}
}