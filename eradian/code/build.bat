@echo off
mkdir ..\..\build
pushd ..\..\build
cl /FC /Zi ..\eradian\code\win32_eradian.cpp user32.lib gdi32.lib
popd