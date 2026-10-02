cmake --build build-wasm --target clean

cmake --build build-wasm

IF NOT EXIST "bin" mkdir "bin"
IF NOT EXIST "bin/wasm" mkdir "bin/wasm"

emcc -g build-wasm/libtasker.a -o bin/wasm/tasker.js

pause