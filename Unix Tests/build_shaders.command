xcrun -sdk macosx metal -c TestShaders.metal -o TestShaders.air
xcrun -sdk macosx metallib TestShaders.air -o TestShaders.metallib