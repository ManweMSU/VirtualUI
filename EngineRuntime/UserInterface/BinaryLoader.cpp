#include "BinaryLoader.h"

#include "InterfaceFormat.h"
#include "BinaryLoaderLegacy.h"
#include "../PlatformDependent/Assembly.h"

#ifdef ENGINE_WINDOWS
#define CURRENT_SYSTEM	L"Windows"
#endif
#ifdef ENGINE_MACOSX
#define CURRENT_SYSTEM	L"MacOSX"
#endif

namespace Engine
{
	namespace UI
	{
		namespace Loader
		{
			void LoadUserInterfaceFromBinary(InterfaceTemplate & Template, Streaming::Stream * Source, IResourceLoader * ResourceLoader, IResourceResolver * ResourceResolver)
			{
				try {
					SafePointer<Format::InterfaceTemplateImage> image = new Format::InterfaceTemplateImage(Source, Assembly::CurrentLocale, CURRENT_SYSTEM, Zoom);
					image->Compile(Template, ResourceLoader, ResourceResolver);
				}
				catch (InvalidFormatException &) {
					Source->Seek(0, Streaming::Begin);
					LoadUserInterfaceFromBinaryLegacy(Template, Source, ResourceLoader, ResourceResolver);
				}
			}
		}
	}
}