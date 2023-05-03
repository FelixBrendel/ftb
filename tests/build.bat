@echo off

rmdir bin /s /q 2>NUL
mkdir bin 2>NUL

set EXE_RAW=test
set BINDIR_RAW=bin
set SRC=main.cpp

set EXE_WIN=%EXE_RAW%.exe
set EXE_LINUX=%EXE_RAW%
set BINDIR_WIN=.\%BINDIR_RAW%
set BINDIR_LINUX=./%BINDIR_RAW%

rem set FTB_NO_SIMD_TESTS=1

if defined FTB_NO_SIMD_TESTS (
   echo skipping tests including SIMD insructions
   set CLANG_DEFS="-DFTB_NO_SIMD_TESTS"
   set CL_DEFS="/DFTB_NO_SIMD_TESTS"
) else (
  set CLANG_DEFS=""
  set CL_DEFS=""
)

echo.
echo clang:
rem clang++ -g -std=c++17 alloctest.cpp -o %BINDIR_WIN%\clang_alloctest.exe || exit 1
rem clang++ -g -std=c++17 printer_colors.cpp -o %BINDIR_WIN%\clang_printer_colors.exe || exit 1
clang++ -g -fsanitize=undefined -std=c++17 %SRC% %CLANG_DEFS% -o %BINDIR_WIN%\clang_%EXE_WIN%  || exit 1
echo.
rem %BINDIR_WIN%\clang_alloctest.exe
rem %BINDIR_WIN%\clang_printer_colors
%BINDIR_WIN%\clang_%EXE_WIN%

rem echo.
rem echo g++:
rem g++ -std=c++17 %SRC% %CLANG_DEFS% -o %BINDIR_WIN%\g++_%EXE_WIN%
rem %BINDIR_WIN%\g++_%EXE_WIN%

rem echo.
rem echo cl:
rem cl %SRC% %CL_DEFS% /std:c++latest /nologo /Zi /Fd: %BINDIR_WIN%\cl_%EXE_WIN%.pdb /Fo: %BINDIR_WIN%\ /Fe: %BINDIR_WIN%\cl_%EXE_WIN% /wd4090 /DFTB_INTERNAL_DEBUG
rem echo running:
rem %BINDIR_WIN%\cl_%EXE_WIN%

rem echo.
rem echo bash_clang:
rem bash -c "clang++ --std=c++17 %SRC% %CLANG_DEFS% -o %BINDIR_LINUX%/bash_clang_%EXE_LINUX% && %BINDIR_LINUX%/bash_clang_%EXE_LINUX%"

rem echo.
rem echo bash_g++:
rem bash -c "g++ --std=c++17 %SRC% %CLANG_DEFS% -o %BINDIR_LINUX%/bash_g++_%EXE_LINUX% && %BINDIR_LINUX%/bash_g++_%EXE_LINUX%"
