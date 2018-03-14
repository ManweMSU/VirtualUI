#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"

namespace Engine
{
	namespace UI
	{
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
		}
		namespace Controls
		{
			using namespace Windows;
			class OverlappedWindow;
			class ContentFrame;

			class OverlappedWindow : public ParentWindow
			{
			private:
				ContentFrame * _inner;
				IWindowEventCallback * _callback;
				bool _visible, _enabled;
				int _mode;
				Rectangle ControlPosition;
			public:
				OverlappedWindow(Window * Parent, WindowStation * Station);
				OverlappedWindow(Window * Parent, WindowStation * Station, Template::ControlTemplate * Template);
				~OverlappedWindow(void) override;

				virtual void Render(const Box & at);
				virtual void ResetCache(void);	
				virtual void Enable(bool enable);
				virtual bool IsEnabled(void);
				virtual void Show(bool visible);
				virtual bool IsVisible(void);
				virtual bool IsOverlapped(void);
				virtual void SetPosition(const Box & box);
				virtual void SetRectangle(const Rectangle & rect);
				virtual Rectangle GetRectangle(void);
				virtual void SetText(const string & text);
				virtual string GetText(void);
				virtual void RaiseEvent(int ID, Event event, Window * sender);
				virtual void CaptureChanged(bool got_capture);
				virtual void LeftButtonDown(Point at);
				virtual void LeftButtonUp(Point at);
				virtual void LeftButtonDoubleClick(Point at);
				virtual void MouseMove(Point at);
				virtual void PopupMenuCancelled(void);
				virtual void SetCursor(Point at);

				ContentFrame * GetContentFrame(void);
				IWindowEventCallback * GetCallback(void);
				void SetCallback(IWindowEventCallback * callback);
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

				virtual void Render(const Box & at);
				virtual void ResetCache(void);
				virtual void SetRectangle(const Rectangle & rect);
				virtual Rectangle GetRectangle(void);
			};

			namespace Constructor
			{
				void ConstructChildren(Window * on, Template::ControlTemplate * source);
			}
		}
		namespace Windows
		{
			Controls::OverlappedWindow * CreateFramelessDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, Rectangle Position, WindowStation * Station);
		}
	}
}