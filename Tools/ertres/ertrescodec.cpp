#include <EngineRuntime.h>
#include "ertrescodec.h"

#ifdef ENGINE_WINDOWS
#include <PlatformDependent/Direct2D.h>
void InitializeCodecs(void)
{
    Engine::Direct2D::InitializeFactory();
    Engine::Codec::CreateIconCodec();
    Engine::Storage::CreateVolumeCodec();
    Engine::Direct2D::CreateWicCodec();
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