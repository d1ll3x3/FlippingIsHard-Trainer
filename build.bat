@echo off
echo =========================================
echo Setting up Visual Studio C++ Environment...
echo =========================================
call "C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

echo =========================================
echo Compiling DLL Injector (injector.cpp)...
echo =========================================
cl.exe /EHsc /O2 /Fe:injector.exe injector.cpp user32.lib

echo =========================================
echo Compiling Trainer DLL (trainer.cpp)...
echo =========================================
cl.exe /LD /EHsc /O2 /Fe:trainer.dll trainer.cpp user32.lib gdi32.lib

echo =========================================
echo Build complete!
echo =========================================
