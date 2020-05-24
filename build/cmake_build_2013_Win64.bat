@echo off

set PATH=%PATH%;../build/cmake/windows/bin
cd ..
mkdir 2013_Win64
cd 2013_Win64

@if "%1"=="talk" (
cmake -DWIN64=1 .. -G"Visual Studio 12 2013 Win64" -DYOUME_MINI=1  -DCMAKE_GENERATOR_TOOLSET=v120_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
) else (
cmake -DWIN64=1 .. -G"Visual Studio 12 2013 Win64" -DYOUME_MINI=0  -DCMAKE_GENERATOR_TOOLSET=v120_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
)

