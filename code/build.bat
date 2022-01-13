@echo off

set CompilerSwitches= -Od -nologo -EHa- -Oi -WX -W4 -wd4127 -wd4100 -wd4189 -wd4201 -wd4505 -wd4996 -Gm- -GR- -FC -Z7 -FeCalc
set CompilerLinkerFlags= -incremental:no -opt:ref  

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

call "c:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64 1>NUL 2>NUL

cl %CompilerSwitches% ..\code\calc_nates.cpp /link %CompilerLinkerFlags% 

popd