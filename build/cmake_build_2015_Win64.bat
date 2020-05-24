@echo off

set PATH=%PATH%;../build/cmake/windows/bin
cd ..
mkdir 2015_Win64
cd 2015_Win64

@if "%1"=="talk" (
cmake -DWIN64=1 .. -G"Visual Studio 14 2015 Win64" -DYOUME_MINI=1  -DCMAKE_GENERATOR_TOOLSET=v140_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
) else (
cmake -DWIN64=1 .. -G"Visual Studio 14 2015 Win64" -DYOUME_MINI=0  -DCMAKE_GENERATOR_TOOLSET=v140_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
)
