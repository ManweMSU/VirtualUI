#pragma once

#include "ControlService.h"

#include "../Interfaces/SystemWindows.h"

namespace Engine
{
	namespace UI
	{
		enum class ControlDepthOrder { SetFirst = 0, SetLast = 1, MoveUp = 2, MoveDown = 3 };
		enum class ControlEvent { Command = 0, AcceleratorCommand = 1, MenuCommand = 2, DoubleClick = 3, ContextClick = 4, ValueChange = 5, Deferred = 6 };
		enum class ControlRefreshPeriod { None = 0, CaretBlink = 1, Cinematic = 2 };

		class ControlSystem;
		class RootControl;
		class ParentControl;

		class Control : public IDispatchQueue
		{
			friend class ControlSystem;
			friend class RootControl;
			friend class ParentControl;
		private:
			ObjectArray<Control> __children;
			Control * __parent;
			ControlSystem * __system;

			void __set_system(ControlSystem * _system);
		protected:
			Box ControlBoundaries;
		public:
			Control(void);
			virtual ~Control(void) override;

			virtual void Render(Graphics::I2DDeviceContext * device, const Box & at);
			virtual void ResetCache(void);
			virtual void ArrangeChildren(void);
			virtual void Enable(bool enable);
			virtual bool IsEnabled(void);
			virtual void Show(bool visible);
			virtual bool IsVisible(void);
			virtual bool IsTabStop(void);
			virtual void SetID(int ID);
			virtual int GetID(void);
			virtual Control * FindChild(int ID);
			virtual void SetRectangle(const Rectangle & rect);
			virtual Rectangle GetRectangle(void);
			virtual void SetPosition(const Box & box);
			virtual Box GetPosition(void);
			virtual void SetText(const string & text);
			virtual string GetText(void);
			virtual void RaiseEvent(int ID, ControlEvent event, Control * sender);
			virtual void FocusChanged(bool got_focus);
			virtual void CaptureChanged(bool got_capture);
			virtual void LostExclusiveMode(void);
			virtual void LeftButtonDown(Point at);
			virtual void LeftButtonUp(Point at);
			virtual void LeftButtonDoubleClick(Point at);
			virtual void RightButtonDown(Point at);
			virtual void RightButtonUp(Point at);
			virtual void RightButtonDoubleClick(Point at);
			virtual void MouseMove(Point at);
			virtual void ScrollVertically(double delta);
			virtual void ScrollHorizontally(double delta);
			virtual bool KeyDown(int key_code);
			virtual void KeyUp(int key_code);
			virtual void CharDown(uint32 ucs_code);
			virtual void PopupMenuCancelled(void);
			virtual void Timer(void);
			virtual Control * HitTest(Point at);
			virtual void SetCursor(Point at);
			virtual bool IsWindowEventEnabled(Windows::WindowHandler handler);
			virtual void HandleWindowEvent(Windows::WindowHandler handler);
			virtual ControlRefreshPeriod GetFocusedRefreshPeriod(void);
			virtual string GetControlClass(void);

			Control * GetParent(void);
			ControlSystem * GetControlSystem(void);
			Windows::IWindow * GetWindow(void);

			void Beep(void);
			void SetOrder(ControlDepthOrder order);
			int ChildrenCount(void);
			Control * Child(int index);
			void SetFocus(void);
			Control * GetFocus(void);
			void SetCapture(void);
			Control * GetCapture(void);
			void ReleaseCapture(void);
			Graphics::I2DDeviceContext * GetRenderingDevice(void);
			Windows::ICursor * GetSystemCursor(Windows::SystemCursorClass cursor);
			void SelectCursor(Windows::SystemCursorClass cursor);

			void AddChild(Control * control);
			int GetChildIndex(Control * control);
			int GetIndexAtParent(void);
			void RemoveChild(Control * control);
			void RemoveChildAt(int index);
			void RemoveFromParent(void);
			void DeferredRemoveFromParent(void);
			
			Box GetAbsolutePosition(void);
			bool IsGeneralizedParent(Control * control);
			bool IsAccessible(void);
			bool IsHovered(void);

			void MoveAnimated(const Rectangle & to, uint32 duration, Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action);
			void MoveAnimated(const Box & to, uint32 duration, Animation::AnimationClass begin_class, Animation::AnimationClass end_class, Animation::AnimationAction action);
			void HideAnimated(Animation::SlideSide side, uint32 duration, Animation::AnimationClass begin, Animation::AnimationClass end = Animation::AnimationClass::Hard);
			void ShowAnimated(Animation::SlideSide side, uint32 duration, Animation::AnimationClass end, Animation::AnimationClass begin = Animation::AnimationClass::Hard);
			
			Control * GetNextTabStopControl(void);
			Control * GetPreviousTabStopControl(void);
			void Invalidate(void);
			void SubmitEvent(int ID);
			virtual void SubmitTask(IDispatchTask * task) override;
			virtual void BeginSubmit(void) override;
			virtual void AppendTask(IDispatchTask * task) override;
			virtual void EndSubmit(void) override;

			template <class W> W * As(void) { return static_cast<W *>(this); }
		};
		class IEventCallback : public Windows::IWindowCallback
		{
		public:
			virtual void Timer(Windows::IWindow * window);
			virtual void PopupMenuCancelled(Windows::IWindow * window);
			virtual void HandleControlEvent(Windows::IWindow * window, int ID, ControlEvent event, Control * sender);
		};
		class IControlFactory : public Object
		{
		public:
			virtual Control * CreateControl(Template::ControlTemplate * base, IControlFactory * factory = 0) = 0;
		};
		class ControlSystem : public IDispatchQueue
		{
			friend class Control;
			friend class ControlSystemWindowCallback;
			friend class ControlSystemInitializer;
		public:
			class ControlSystemWindowCallback : public Windows::IWindowCallback
			{
			public:
				ControlSystem & system;
				ControlSystemWindowCallback(ControlSystem & _system);

				virtual void Created(Windows::IWindow * window) override;
				virtual void Destroyed(Windows::IWindow * window) override;
				virtual void Shown(Windows::IWindow * window, bool show) override;
				virtual void RenderWindow(Windows::IWindow * window) override;
				virtual void WindowClose(Windows::IWindow * window) override;
				virtual void WindowMaximize(Windows::IWindow * window) override;
				virtual void WindowMinimize(Windows::IWindow * window) override;
				virtual void WindowRestore(Windows::IWindow * window) override;
				virtual void WindowHelp(Windows::IWindow * window) override;
				virtual void WindowActivate(Windows::IWindow * window) override;
				virtual void WindowDeactivate(Windows::IWindow * window) override;
				virtual void WindowMove(Windows::IWindow * window) override;
				virtual void WindowSize(Windows::IWindow * window) override;
				virtual void FocusChanged(Windows::IWindow * window, bool got) override;
				virtual bool KeyDown(Windows::IWindow * window, int key_code) override;
				virtual void KeyUp(Windows::IWindow * window, int key_code) override;
				virtual void CharDown(Windows::IWindow * window, uint32 ucs_code) override;
				virtual void CaptureChanged(Windows::IWindow * window, bool got) override;
				virtual void SetCursor(Windows::IWindow * window, Point at) override;
				virtual void MouseMove(Windows::IWindow * window, Point at) override;
				virtual void LeftButtonDown(Windows::IWindow * window, Point at) override;
				virtual void LeftButtonUp(Windows::IWindow * window, Point at) override;
				virtual void LeftButtonDoubleClick(Windows::IWindow * window, Point at) override;
				virtual void RightButtonDown(Windows::IWindow * window, Point at) override;
				virtual void RightButtonUp(Windows::IWindow * window, Point at) override;
				virtual void RightButtonDoubleClick(Windows::IWindow * window, Point at) override;
				virtual void ScrollVertically(Windows::IWindow * window, double delta) override;
				virtual void ScrollHorizontally(Windows::IWindow * window, double delta) override;
				virtual void Timer(Windows::IWindow * window, int timer_id) override;
				virtual void ThemeChanged(Windows::IWindow * window) override;
				virtual bool IsWindowEventEnabled(Windows::IWindow * window, Windows::WindowHandler handler) override;
				virtual void HandleWindowEvent(Windows::IWindow * window, Windows::WindowHandler handler) override;
			};
			class ControlSystemInitializer : public Object
			{
			public:
				Windows::CreateWindowDesc WindowDesc;
				Windows::DeviceClass DeviceClass;
				Template::ControlTemplate * FrameTemplate;
				Rectangle OuterRectangle;
				IControlFactory * Factory;
				ControlSystem * System;
				Template::ControlReflectedBase * ExtendedData;
				Template::ControlReflectedBase * Data;

				ControlSystemInitializer(void);
				void PreInitialize(void);
				void PostInitialize(void);
				Windows::IWindow * CreateWindow(void);
				Windows::IWindow * CreateModalWindow(void);
			};
		private:
			ObjectArray<Animation::IAnimationController> _animations;
			SafePointer<Windows::I2DPresentationEngine> _engine;
			SafePointer<Graphics::I2DDeviceContext> _device;
			SafePointer<RootControl> _root;
			SafePointer<Control> _captured, _focused, _exclusive;
			SafePointer<VirtualPopupStyles> _virtual_styles;
			SafePointer<ControlSystemInitializer> _initializer;
			Windows::DeviceClass _preferred_device_class;
			int _caret_width;
			Point _virtual_size;
			bool _autosize, _beep;
			ControlSystemWindowCallback * _window_callback;
			IEventCallback * _event_callback;
			Windows::IWindow * _window;
			Volumes::ObjectDictionary<int, Control> _timers;
			Volumes::ObjectDictionary<Windows::SystemCursorClass, Windows::ICursor> _cursors;
			ControlRefreshPeriod _general_rate;
			ControlRefreshPeriod _evaluated_rate;

			ControlSystem(void);
			void ResetEffectiveRefreshRate(void);
			void ResetRenderingDevice(void);
		public:
			virtual ~ControlSystem(void) override;

			virtual void SubmitTask(IDispatchTask * task) override;
			virtual void BeginSubmit(void) override;
			virtual void AppendTask(IDispatchTask * task) override;
			virtual void EndSubmit(void) override;

			void SetCaretWidth(int width);
			int GetCaretWidth(void);
			void SetRefreshPeriod(ControlRefreshPeriod period);
			ControlRefreshPeriod GetRefreshPeriod(void);
			void SetRenderingDevice(Graphics::I2DDeviceContext * device);
			Graphics::I2DDeviceContext * GetRenderingDevice(void);
			void SetPrefferedDevice(Windows::DeviceClass device);
			Windows::DeviceClass GetPrefferedDevice(void);
			void SetVirtualClientSize(const Point & size);
			Point GetVirtualClientSize(void);
			void SetVirtualClientAutoresize(bool autoresize);
			bool IsVirtualClientAutoresize(void);
			void SetVirtualPopupStyles(VirtualPopupStyles * styles);
			VirtualPopupStyles * GetVirtualPopupStyles(void);
			void SetCallback(IEventCallback * callback);
			IEventCallback * GetCallback(void);

			Windows::ICursor * GetSystemCursor(Windows::SystemCursorClass cursor);
			void OverrideSystemCursor(Windows::SystemCursorClass cursor, Windows::ICursor * with);

			RootControl * GetRootControl(void);
			Control * HitTest(Point virtual_client);
			Control * AccessibleHitTest(Point virtual_client);

			Point GetVirtualCursorPosition(void);
			void SetVirtualCursorPosition(Point position);

			Point ConvertControlToClient(Control * control, Point point);
			Point ConvertClientToControl(Control * control, Point point);
			Point ConvertClientVirtualToScreen(Point point);
			Point ConvertClientScreenToVirtual(Point point);
			void EvaluateControlAt(Point virtual_client, Control ** control_ref, Point * local_ref);

			void LaunchAnimation(Animation::IAnimationController * controller);
			void MoveWindowAnimated(const Box & to, uint32 duration, Animation::AnimationClass begin_class, Animation::AnimationClass end_class);
			void HideWindowAnimated(uint32 duration);
			void ShowWindowAnimated(uint32 duration);

			void SetFocus(Control * control);
			Control * GetFocus(void);
			void SetCapture(Control * control);
			Control * GetCapture(void);
			void ReleaseCapture(void);
			void SetExclusiveControl(Control * control);
			Control * GetExclusiveControl(void);
			void SetTimer(Control * control, uint32 period);
			void Invalidate(void);
			void Render(Graphics::I2DDeviceContext * device);

			void EnableBeep(bool enable);
			bool IsBeepEnabled(void);
			void Beep(void);

			Windows::IWindow * GetWindow(void);
		};

		class ParentControl : public Control
		{
		public:
			ParentControl(void);
			virtual Control * FindChild(int ID) override;
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
			virtual void ArrangeChildren(void) override;
			virtual void SetPosition(const Box & box) override;
			virtual Control * HitTest(Point at) override;
			virtual string GetControlClass(void) override;
		};
		class RootControl : public ParentControl
		{
			Array<Accelerators::AcceleratorCommand> _accelerators;
			SafePointer<Shape> _background;
			SafePointer<Template::Shape> _background_template;
		public:
			RootControl(void);
			virtual void Render(Graphics::I2DDeviceContext * device, const Box & at) override;
			virtual void ResetCache(void) override;
			virtual void RaiseEvent(int ID, ControlEvent event, Control * sender) override;
			virtual void PopupMenuCancelled(void) override;
			virtual void Timer(void) override;
			virtual string GetControlClass(void) override;

			void SetBackground(Template::Shape * shape);
			Template::Shape * GetBackground(void);
			Array<Accelerators::AcceleratorCommand> & GetAcceleratorTable(void);
			const Array<Accelerators::AcceleratorCommand> & GetAcceleratorTable(void) const;
			void AddDialogStandardAccelerators(void);
			bool TranslateAccelerators(int key_code);
		};

		Windows::IWindow * CreateWindow(const Windows::CreateWindowDesc & desc, Windows::DeviceClass device = Windows::DeviceClass::DontCare);
		Windows::IWindow * CreateWindow(Template::ControlTemplate * base, IEventCallback * callback, const Rectangle & outer, Windows::IWindow * parent = 0, IControlFactory * factory = 0, Windows::DeviceClass device = Windows::DeviceClass::DontCare);
		Windows::IWindow * CreateModalWindow(const Windows::CreateWindowDesc & desc, Windows::DeviceClass device = Windows::DeviceClass::DontCare);
		Windows::IWindow * CreateModalWindow(Template::ControlTemplate * base, IEventCallback * callback, const Rectangle & outer, Windows::IWindow * parent = 0, IControlFactory * factory = 0, Windows::DeviceClass device = Windows::DeviceClass::DontCare);

		Control * CreateStandardControl(Template::ControlTemplate * base, IControlFactory * factory = 0);
		void CreateChildrenControls(Control * control_on, Template::ControlTemplate * base, IControlFactory * factory = 0);

		Windows::IMenu * CreateMenu(Template::ControlTemplate * base);

		void RunMenu(Windows::IMenu * menu, Control * for_control, Point at);

		Box GetPopupMargins(Control * for_control);
		Control * CreatePopup(Control * for_control, const Box & at, Windows::DeviceClass device = Windows::DeviceClass::DontCare);
		void ShowPopup(Control * control, bool show);
		void DestroyPopup(Control * control);

		ControlSystem * GetControlSystem(Windows::IWindow * window);
		Control * FindControl(Windows::IWindow * window, int id);
		RootControl * GetRootControl(Windows::IWindow * window);

		void SelectCursor(Windows::IWindow * window, Windows::SystemCursorClass cursor);
	}
}