#pragma once

#include "ControlBase.h"
#include "ControlClasses.h"
#include "Canvas.h"

namespace Engine
{
	namespace UI
	{
		namespace Controls
		{
			class OverlappedWindow;
			class ContentFrame;
		}
		namespace Accelerators
		{
			enum class AcceleratorSystemCommand { WindowClose, WindowInvokeHelp, WindowNextControl, WindowPreviousControl };
			class AcceleratorCommand
			{
			public:
				uint KeyCode;
				bool Shift;
				bool Control;
				bool Alternative;
				int CommandID;
				bool SystemCommand;

				AcceleratorCommand(void);
				AcceleratorCommand(int invoke_command, uint on_key, bool control = true, bool shift = false, bool alternative = false);
				AcceleratorCommand(AcceleratorSystemCommand command, uint on_key, bool control = true, bool shift = false, bool alternative = false);
			};
		}
		namespace Windows
		{
			enum class FrameEvent { Close, Move, Maximize, Minimize, Help, PopupMenuCancelled, Timer };
			class IWindowEventCallback
			{
			public:
				virtual void OnInitialized(Window * window) = 0;
				virtual void OnControlEvent(Window * window, int ID, Window::Event event, Window * sender) = 0;
				virtual void OnFrameEvent(Window * window, FrameEvent event) = 0;
			};
			Controls::OverlappedWindow * CreateFramelessDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
			Controls::OverlappedWindow * CreateFramedDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
			Controls::OverlappedWindow * CreatePopupDialog(Template::ControlTemplate * Template, IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);

			void InitializeCodecCollection(void);
			IResourceLoader * CreateNativeCompatibleResourceLoader(void);
			Drawing::ITextureRenderingDevice * CreateNativeCompatibleTextureRenderingDevice(int width, int height, const Math::Color & color);
			Box GetScreenDimensions(void);
			double GetScreenScale(void);
			void RunMessageLoop(void);
			void ExitMessageLoop(void);
		}
		namespace Controls
		{
			class OverlappedWindow : public ParentWindow
			{
				friend Controls::OverlappedWindow * ::Engine::UI::Windows::CreateFramelessDialog(Template::ControlTemplate * Template, Windows::IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
				friend Controls::OverlappedWindow * ::Engine::UI::Windows::CreateFramedDialog(Template::ControlTemplate * Template, Windows::IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
				friend Controls::OverlappedWindow * ::Engine::UI::Windows::CreatePopupDialog(Template::ControlTemplate * Template, Windows::IWindowEventCallback * Callback, const Rectangle & Position, WindowStation * Station);
				friend class ContentFrame;
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
				Array<Accelerators::AcceleratorCommand> _accels;
				SafePointer<Shape> _active;
				SafePointer<Shape> _inactive;
				bool _visible, _enabled;
				int _mode, _border, _caption, _state, _minwidth, _minheight, _btnwidth;
				bool _initialized = false;
				Rectangle ControlPosition;
				Window * _close;
				Window * _maximize;
				Window * _minimize;
				Window * _help;
				size _size;
				bool _overlaps = true;
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
				virtual bool IsNeverActive(void) override;
				virtual void SetPosition(const Box & box) override;
				virtual Box GetPosition(void) override;
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
				virtual bool TranslateAccelerators(int key_code) override;
				virtual void PopupMenuCancelled(void) override;
				virtual void SetCursor(Point at) override;
				virtual void Timer(void) override;

				ContentFrame * GetContentFrame(void);
				Windows::IWindowEventCallback * GetCallback(void);
				void SetCallback(Windows::IWindowEventCallback * callback);
				int GetBorderWidth(void);
				int GetCaptionWidth(void);
				int GetButtonsWidth(void);
				int GetPart(Point cursor);
				void RaiseFrameEvent(Windows::FrameEvent event);
				void SetBackground(Template::Shape * shape);

				Array<Accelerators::AcceleratorCommand> & GetAcceleratorTable(void);
				const Array<Accelerators::AcceleratorCommand> & GetAcceleratorTable(void) const;
				void AddDialogStandardAccelerators(void);
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
				virtual void ArrangeChildren(void) override;
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