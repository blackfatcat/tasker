@REM Creates the windows build files using VS26 build tools and generates all example projects

IF NOT EXIST "build-win" mkdir "build-win"
cd build-win
cmake -G "Visual Studio 18 2026" -A x64 -DTSKR_BUILD_EXAMPLES_ALL=1 ..\CMakeLists.txt

PAUSE