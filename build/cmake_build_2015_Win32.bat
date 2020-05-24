@echo off

set PATH=%PATH%;../build/cmake/windows/bin
cd ..
mkdir 2015_Win32
cd 2015_Win32

@if "%1"=="talk" (
cmake .. -G"Visual Studio 14 2015" -DYOUME_MINI=1 -DWIN32_2015=1 -DCMAKE_GENERATOR_TOOLSET=v140_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
) else (
cmake .. -G"Visual Studio 14 2015" -DYOUME_MINI=0 -DWIN32_2015=1 -DCMAKE_GENERATOR_TOOLSET=v140_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
)