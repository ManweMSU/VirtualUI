#pragma once

#include "../ShapeBase.h"

#include <d2d1.h>

namespace Engine
{
	namespace Direct2D
	{
		using namespace ::Engine::UI;

		extern ID2D1Factory * Factory;

		void InitializeFactory(void);

		class D2DRenderDevice : public IRenderingDevice
		{
			ID2D1RenderTarget * Target;
			Array<ID2D1Layer *> Layers;
		public:
			D2DRenderDevice(ID2D1RenderTarget * target);
			~D2DRenderDevice(void) override;

			ID2D1RenderTarget * GetRenderTarget(void) const;

			virtual IBarRenderingInfo * CreateBarRenderingInfo(const Array<GradientPoint>& gradient, double angle) override;
			virtual IBlurEffectRenderingInfo * CreateBlurEffectRenderingInfo(double power) override;

			virtual void RenderBar(IBarRenderingInfo * Info, const Box & At) override;
			virtual void ApplyBlur(IBlurEffectRenderingInfo * Info, const Box & At) override;

			virtual void PushClip(const Box & Rect) override;
			virtual void PopClip(void) override;
			virtual void BeginLayer(const Box & Rect, double Opacity) override;
			virtual void EndLayer(void) override;
		};
	}
}