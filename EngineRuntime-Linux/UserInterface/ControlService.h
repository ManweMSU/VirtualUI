#pragma once

#include "Templates.h"
#include "ControlClasses.h"
#include "../Math/MathBase.h"
#include "../Interfaces/SystemWindows.h"

namespace Engine
{
	namespace UI
	{
		namespace Animation
		{
			enum class AnimationClass { Smooth, Hard };
			enum class AnimationAction { None, ShowWindow, HideWindow, HideWindowKeepPosition };
			enum class SlideSide { Left, Top, Right, Bottom };

			class IAnimationController : public Object
			{
			public:
				uint32 InitialStateTime;
				uint32 FinalStateTime;
				uint32 Duration;

				virtual void OnAnimationStart(void) = 0;
				virtual void OnAnimationTick(uint32 time) = 0;
				virtual void OnAnimationEnd(void) = 0;
			};
			template <class V> class AnimationController : public IAnimationController
			{
			public:
				V InitialState;
				V FinalState;
				AnimationClass InitialAnimationClass;
				AnimationClass FinalAnimationClass;

				virtual void OnAnimationFrame(const V & state) = 0;
				virtual void OnAnimationTick(uint32 time) override final
				{
					double t = double(time - InitialStateTime) / double(Duration);
					if (InitialAnimationClass == AnimationClass::Hard && FinalAnimationClass == AnimationClass::Hard) {
						OnAnimationFrame(Math::lerp(InitialState, FinalState, t));
					} else if (InitialAnimationClass == AnimationClass::Smooth && FinalAnimationClass == AnimationClass::Hard) {
						OnAnimationFrame(Math::lerp(InitialState, FinalState, t * t));
					} else if (InitialAnimationClass == AnimationClass::Hard && FinalAnimationClass == AnimationClass::Smooth) {
						t = 1.0 - t;
						return OnAnimationFrame(Math::lerp(InitialState, FinalState, 1.0 - t * t));
					} else {
						if (t < 0.5) {
							return OnAnimationFrame(Math::lerp(InitialState, FinalState, 2.0 * t * t));
						} else {
							t = 1.0 - t;
							return OnAnimationFrame(Math::lerp(InitialState, FinalState, 1.0 - 2.0 * t * t));
						}
					}
				}
			};
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

		class VirtualPopupStyles : public Object
		{
		public:
			VirtualPopupStyles(void);
			virtual ~VirtualPopupStyles(void) override;

			SafePointer<Shape> PopupShadow, MenuFrame, SubmenuArrow, MenuSeparator;
			SafePointer<IFont> MenuItemReferenceFont;
			SafePointer<Template::Shape> MenuItemNormal, MenuItemGrayed, MenuItemHot;
			SafePointer<Template::Shape> MenuItemNormalChecked, MenuItemGrayedChecked, MenuItemHotChecked;
			int MenuBorderWidth, MenuElementDefaultSize, MenuSeparatorDefaultSize;
		};
	}
}