#include "EffectPlugin.h"

namespace Engine
{
	namespace Effect
	{
		constexpr const widechar * main_name = L"ertwndfx.dll";
		#ifdef ENGINE_X64
		#ifdef ENGINE_ARM
		constexpr const widechar * alt_name = L"ertwndfx_arm64.dll";
		#else
		constexpr const widechar * alt_name = L"ertwndfx_x64.dll";
		#endif
		#else
		#ifdef ENGINE_ARM
		constexpr const widechar * alt_name = L"ertwndfx_arm.dll";
		#else
		constexpr const widechar * alt_name = L"ertwndfx_x86.dll";
		#endif
		#endif

		HMODULE library = 0;
		int status = 0;

		FunctionTypes::func_EngineEffectCreateLayers CreateLayers = 0;
		FunctionTypes::func_EngineEffectResizeLayers ResizeLayers = 0;
		FunctionTypes::func_EngineEffectRetainLayers RetainLayers = 0;
		FunctionTypes::func_EngineEffectReleaseLayers ReleaseLayers = 0;
		FunctionTypes::func_EngineEffectBeginDraw BeginDraw = 0;
		FunctionTypes::func_EngineEffectEndDraw EndDraw = 0;

		bool Init(void) noexcept
		{
			if (status) return library != 0;
			status = 1;
			library = LoadLibraryW(main_name);
			if (!library) library = LoadLibraryW(alt_name);
			if (!library) return false;
			FunctionTypes::func_EngineEffectInit init_routine = reinterpret_cast<FunctionTypes::func_EngineEffectInit>(GetProcAddress(library, "EngineEffectInit"));
			if (!init_routine || !init_routine()) {
				FreeLibrary(library);
				library = 0;
				return false;
			}
			CreateLayers = reinterpret_cast<FunctionTypes::func_EngineEffectCreateLayers>(GetProcAddress(library, "EngineEffectCreateLayers"));
			ResizeLayers = reinterpret_cast<FunctionTypes::func_EngineEffectResizeLayers>(GetProcAddress(library, "EngineEffectResizeLayers"));
			RetainLayers = reinterpret_cast<FunctionTypes::func_EngineEffectRetainLayers>(GetProcAddress(library, "EngineEffectRetainLayers"));
			ReleaseLayers = reinterpret_cast<FunctionTypes::func_EngineEffectReleaseLayers>(GetProcAddress(library, "EngineEffectReleaseLayers"));
			BeginDraw = reinterpret_cast<FunctionTypes::func_EngineEffectBeginDraw>(GetProcAddress(library, "EngineEffectBeginDraw"));
			EndDraw = reinterpret_cast<FunctionTypes::func_EngineEffectEndDraw>(GetProcAddress(library, "EngineEffectEndDraw"));
			if (!CreateLayers || !ResizeLayers || !RetainLayers || !ReleaseLayers || !BeginDraw || !EndDraw) {
				FreeLibrary(library);
				library = 0;
				return false;
			}
			return true;
		}
	}
}