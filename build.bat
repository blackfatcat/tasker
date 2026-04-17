IF NOT EXIST "build-win" mkdir "build-win"
cd build-win
cmake -G "Visual Studio 18 2026" -A x64 ..\CMakeLists.txt

PAUSE