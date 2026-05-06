:: detect emscripten SDK
:: validate emcmake exists
:: validate minGW package and env variables (emsdk activate)

@echo off

if NOT "%PATH%"=="%PATH:Emscripten=%" (
    @echo on
    echo Emscripten SDK found
) else (
    @echo on
    echo Please install and activate Emscripten SDK
    pause
    exit
)

@echo off
if NOT "%PATH%"=="%PATH:node=%" (
    @echo on
    echo node found
) else (
    @echo on
    echo Please install and activate the node package inside the emscripten SDK
    pause
    exit
) 

@echo off
if NOT "%PATH%"=="%PATH:mingw=%" (
    @echo on
    echo MingGW found
) else (
    @echo on
    echo Please install and activate the mingw package inside the emscripten SDK
    pause
    exit
) 

emcmake cmake . -B build-wasm

PAUSE