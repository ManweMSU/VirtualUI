#pragma once

#include "../EngineBase.h"

#include <Windows.h>
#include <d3d11.h>

DECLARE_HANDLE(HLAYERS);

namespace Engine
{
	namespace Effect
	{
		enum CreateEngineEffectLayersFlags : uint {
			CreateEngineEffectTransparentBackground = 0x0001,
			CreateEngineEffectBlurBehind = 0x0002,
		};
		struct CreateEngineEffectLayersDesc
		{
			HWND window;
			uint layer_flags;
			ID3D11Device * device;
			uint width;
			uint height;
			double deviation;
		};

		namespace FunctionTypes
		{
			typedef BOOL (* func_EngineEffectInit) (void);
			typedef HLAYERS (* func_EngineEffectCreateLayers) (const CreateEngineEffectLayersDesc * desc);
			typedef BOOL (* func_EngineEffectResizeLayers) (HLAYERS layers, uint width, uint height);
			typedef VOID (* func_EngineEffectRetainLayers) (HLAYERS layers);
			typedef VOID (* func_EngineEffectReleaseLayers) (HLAYERS layers);
			typedef BOOL (* func_EngineEffectBeginDraw) (HLAYERS layers, ID3D11Texture2D ** surface, uint * orgx, uint * orgy);
			typedef BOOL (* func_EngineEffectEndDraw) (HLAYERS layers);
		}

		bool Init(void) noexcept;

		extern FunctionTypes::func_EngineEffectCreateLayers CreateLayers;
		extern FunctionTypes::func_EngineEffectResizeLayers ResizeLayers;
		extern FunctionTypes::func_EngineEffectRetainLayers RetainLayers;
		extern FunctionTypes::func_EngineEffectReleaseLayers ReleaseLayers;
		extern FunctionTypes::func_EngineEffectBeginDraw BeginDraw;
		extern FunctionTypes::func_EngineEffectEndDraw EndDraw;
	}
}