#pragma once

#include "ShapeBase.h"
#include "../Math/MathBase.h"

namespace Engine
{
	namespace UI
	{
		namespace Animation
		{
			enum class AnimationClass { Smooth, Hard };
			enum class AnimationAction { None, ShowWindow, HideWindow, HideWindowKeepPosition };
			enum class SlideSide { Left, Top, Right, Bottom };
			template <class T, class V> class AnimationState
			{
			public:
				T * Target;
				V BeginState;
				V EndState;
				uint32 BeginTime;
				uint32 EndTime;
				uint32 Duration;
				AnimationClass BeginClass;
				AnimationClass EndClass;
				AnimationAction Action;
				uint32 Special;

				AnimationState(void) {}
				AnimationState(T * target, const V & begin, const V & end, uint32 current, uint32 duration,
					AnimationClass begin_class, AnimationClass end_class, AnimationAction action, uint32 special) :
					Target(target), BeginState(begin), EndState(end), BeginTime(current), EndTime(current + duration),
					Duration(duration), BeginClass(begin_class), EndClass(end_class), Action(action), Special(special) {}

				bool IsOver(uint32 time) const { return (time - BeginTime >= Duration); }
				V GetFrame(uint32 time) const
				{
					double t = double(time - BeginTime) / double(Duration);
					if (BeginClass == AnimationClass::Hard && EndClass == AnimationClass::Hard) {
						return Math::lerp(BeginState, EndState, t);
					} else if (BeginClass == AnimationClass::Smooth && EndClass == AnimationClass::Hard) {
						return Math::lerp(BeginState, EndState, t * t);
					} else if (BeginClass == AnimationClass::Hard && EndClass == AnimationClass::Smooth) {
						t = 1.0 - t;
						return Math::lerp(BeginState, EndState, 1.0 - t * t);
					} else {
						if (t < 0.5) {
							return Math::lerp(BeginState, EndState, 2.0 * t * t);
						} else {
							t = 1.0 - t;
							return Math::lerp(BeginState, EndState, 1.0 - 2.0 * t * t);
						}
					}
				}
			};
		}
	}
}