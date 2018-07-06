#include <EngineRuntime.h>
#include "ertrescodec.h"

#ifdef ENGINE_WINDOWS

void InitializeCodecs(void)
{
    Engine::Codec::CreateIconCodec();
    Engine::Storage::CreateVolumeCodec();
}
#endif
#ifdef ENGINE_MACOSX
#include <PlatformDependent/AppleCodec.h>
void InitializeCodecs(void)
{
    Engine::Codec::CreateIconCodec();
    Engine::Storage::CreateVolumeCodec();
    Engine::Cocoa::CreateAppleCodec();
}
#endif