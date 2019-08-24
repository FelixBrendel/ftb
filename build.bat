@echo off
pushd %~dp0

set exeName=main.exe
set binDir=bin

mkdir %bindir%
pushd %bindir%

cl^
   ../test.cpp^
   /Fe%exeName% /MP /openmp /W3 /std:c++latest^
   /nologo /EHsc /Z7^
   /link /incremental /debug:fastlink

if %errorlevel% == 0 (
   echo.
   if not exist ..\%binDir% mkdir ..\%binDir%
   move %exeName% ..\%binDir%\ > NUL
  echo ---------- Output start ----------
   %exeName%
   echo ---------- Output   end ----------
   popd
) else (
  echo.
  echo Fucki'n 'ell
)

popd
