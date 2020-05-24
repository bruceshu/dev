@echo off

set PATH=%PATH%;../build/cmake/windows/bin
cd ..
mkdir Win32
cd Win32

@if "%1"=="talk" (
cmake .. -G"Visual Studio 12 2013" -DYOUME_MINI=1 -DCMAKE_GENERATOR_TOOLSET=v120_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
) else (
cmake .. -G"Visual Studio 12 2013" -DYOUME_MINI=0 -DCMAKE_GENERATOR_TOOLSET=v120_xp -DCMAKE_CONFIGURATION_TYPES=Debug;Release
)

pause
@echo on
