#include "BinaryLoader.h"

#include "InterfaceFormat.h"
#include "BinaryLoaderLegacy.h"
#include "../Interfaces/Assembly.h"
#include "../Interfaces/SystemGraphics.h"

#ifdef ENGINE_WINDOWS
#define CURRENT_SYSTEM	L"Windows"
#endif
#ifdef ENGINE_MACOSX
#define CURRENT_SYSTEM	L"MacOSX"
#endif
#ifdef ENGINE_LINUX
#define CURRENT_SYSTEM	L"Linux"
#endif

namespace Engine
{
	namespace UI
	{
		namespace Loader
		{
			void LoadUserInterfaceFromBinary(InterfaceTemplate & Template, Streaming::Stream * Source, Graphics::I2DDeviceContextFactory * ResourceLoader, IResourceResolver * ResourceResolver)
			{
				Codec::InitializeDefaultCodecs();
				try {
					SafePointer<Format::InterfaceTemplateImage> image = new Format::InterfaceTemplateImage(Source, Assembly::CurrentLocale, CURRENT_SYSTEM, CurrentScaleFactor);
					image->Compile(Template, ResourceLoader, ResourceResolver);
				}
				catch (InvalidFormatException &) {
					Source->Seek(0, Streaming::Begin);
					SafePointer<Graphics::I2DDeviceContextFactory> loader;
					if (ResourceLoader) loader.SetRetain(ResourceLoader); else loader = Graphics::CreateDeviceContextFactory();
					LoadUserInterfaceFromBinaryLegacy(Template, Source, loader, ResourceResolver);
				}
			}
			void LoadUserInterfaceWithStyleSet(InterfaceTemplate & Template, InterfaceTemplate & Styles, Streaming::Stream * Source, Graphics::I2DDeviceContextFactory * ResourceLoader, IResourceResolver * ResourceResolver)
			{
				Codec::InitializeDefaultCodecs();
				SafePointer<Format::InterfaceTemplateImage> image = new Format::InterfaceTemplateImage(Source, Assembly::CurrentLocale, CURRENT_SYSTEM, CurrentScaleFactor);
				image->Compile(Template, Styles, ResourceLoader, ResourceResolver);
			}
		}
	}
}