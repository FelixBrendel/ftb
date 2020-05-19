@echo off

set EXE_RAW=test
set BINDIR_RAW=bin
set SRC=test.cpp

set EXE_WIN=%EXE_RAW%.exe
set EXE_LINUX=%EXE_RAW%
set BINDIR_WIN=.\%BINDIR_RAW%
set BINDIR_LINUX=./%BINDIR_RAW%

echo.
echo clang:
clang %SRC% -o %BINDIR_WIN%\clang_%EXE_WIN%
%BINDIR_WIN%\clang_%EXE_WIN%

echo.
echo g++:
g++ %SRC% -o %BINDIR_WIN%\g++_%EXE_WIN%
%BINDIR_WIN%\g++_%EXE_WIN%

echo.
echo cl:
cl %SRC% /nologo /Zi /Fd: %BINDIR_WIN%\cl_%EXE_WIN%.pdb /Fo: %BINDIR_WIN%\ /Fe: %BINDIR_WIN%\cl_%EXE_WIN% /wd4090
%BINDIR_WIN%\cl_%EXE_WIN%

echo.
echo bash_clang:
bash -c "clang -std=c++11 %SRC% -o %BINDIR_LINUX%/bash_clang_%EXE_LINUX% && %BINDIR_LINUX%/bash_clang_%EXE_LINUX%"

echo.
echo bash_g++:
bash -c "g++ -std=c++11 %SRC% -o %BINDIR_LINUX%/bash_g++_%EXE_LINUX% && %BINDIR_LINUX%/bash_g++_%EXE_LINUX%"
