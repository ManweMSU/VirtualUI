DIR=$(cd "$(dirname "$0")"; pwd)
cd $DIR
rm -rf _build/EngineRuntime.framework
mkdir _build
mkdir _build/EngineRuntime.framework
mkdir _build/EngineRuntime.framework/Versions
mkdir _build/EngineRuntime.framework/Versions/A
mkdir _build/EngineRuntime.framework/Versions/A/Resources
mkdir _build/EngineRuntime.framework/Versions/A/Headers
cp Info.plist _build/EngineRuntime.framework/Versions/A/Resources/Info.plist
cp _build/EngineRuntime.dylib _build/EngineRuntime.framework/Versions/A/EngineRuntime
ln -s A _build/EngineRuntime.framework/Versions/Current
ln -s Versions/Current/EngineRuntime _build/EngineRuntime.framework/EngineRuntime
ln -s Versions/Current/Resources _build/EngineRuntime.framework/Resources
ln -s Versions/Current/Headers _build/EngineRuntime.framework/Headers
cp -rf ./*.h _build/EngineRuntime.framework/Versions/A/Headers
mkdir _build/EngineRuntime.framework/Versions/A/Headers/PlatformDependent
mkdir _build/EngineRuntime.framework/Versions/A/Headers/Miscellaneous
mkdir _build/EngineRuntime.framework/Versions/A/Headers/Syntax
mkdir _build/EngineRuntime.framework/Versions/A/Headers/Processes
cp -rf ./PlatformDependent/*.h _build/EngineRuntime.framework/Versions/A/Headers/PlatformDependent
cp -rf ./Miscellaneous/*.h _build/EngineRuntime.framework/Versions/A/Headers/Miscellaneous
cp -rf ./Syntax/*.h _build/EngineRuntime.framework/Versions/A/Headers/Syntax
cp -rf ./Processes/*.h _build/EngineRuntime.framework/Versions/A/Headers/Processes